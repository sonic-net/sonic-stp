/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stp_inc.h"
#include <stdlib.h>

/* FUNCTION
 *		stputil_bridge_to_string()
 *
 * SYNOPSIS
 *		converts bridge id into string format (XXXXXXXXXXXXXXXX) for
 *		display purposes.
 */
void stputil_bridge_to_string(BRIDGE_IDENTIFIER *bridge_id, UINT8 *buffer, UINT16 size)
{
	union
	{
		UINT8 b[6];
		MAC_ADDRESS s;
	} mac_address;
    uint16_t tmp;

	HOST_TO_NET_MAC(&(mac_address.s), &(bridge_id->address));
    tmp = (bridge_id->priority << 12) | (bridge_id->system_id);

	snprintf (buffer, size, "%04x%02x%02x%02x%02x%02x%02x",
		tmp,
		mac_address.b[0],
		mac_address.b[1],
		mac_address.b[2],
		mac_address.b[3],
		mac_address.b[4],
		mac_address.b[5]);
}

/* FUNCTION
 *	stputil_get_default_path_cost()
 *
 * SYNOPSIS
 *	returns the path cost associated with port number based on its
 *	speed characteristics. returns 0 if error. if extend is true, returns
 *	new standard values otherwise returns old standard values. this takes
 *	care of auto-negotiated ports.
 */
UINT32 stputil_get_default_path_cost(PORT_ID port_number, bool extend)
{
    STP_PORT_SPEED port_speed;
    UINT32 path_cost;
    port_speed = stp_intf_get_speed(port_number);

    path_cost = stputil_get_path_cost(port_speed, extend);

    if (!path_cost)
        STP_LOG_ERR("zero path cost %d for intf:%d", path_cost, port_number);

    return path_cost;
}

UINT32 stputil_get_path_cost(STP_PORT_SPEED port_speed, bool extend)
{
	switch (port_speed)
	{
		case STP_SPEED_10M:
			return ((extend) ? STP_PORT_PATH_COST_10M : STP_LEGACY_PORT_PATH_COST_10M);

		case STP_SPEED_100M:
			return ((extend) ? STP_PORT_PATH_COST_100M : STP_LEGACY_PORT_PATH_COST_100M);

		case STP_SPEED_1G:
			return ((extend) ? STP_PORT_PATH_COST_1G : STP_LEGACY_PORT_PATH_COST_1G);

		case STP_SPEED_10G:
			return ((extend) ? STP_PORT_PATH_COST_10G : STP_LEGACY_PORT_PATH_COST_10G);

		case STP_SPEED_25G:
			return ((extend) ? STP_PORT_PATH_COST_25G : STP_LEGACY_PORT_PATH_COST_25G);
			
		case STP_SPEED_40G:
			return ((extend) ? STP_PORT_PATH_COST_40G : STP_LEGACY_PORT_PATH_COST_40G);
			
		case STP_SPEED_100G:
			return ((extend) ? STP_PORT_PATH_COST_100G: STP_LEGACY_PORT_PATH_COST_100G);
		
		case STP_SPEED_400G:
			return ((extend) ? STP_PORT_PATH_COST_400G: STP_LEGACY_PORT_PATH_COST_400G);

		case STP_SPEED_800G:
			return ((extend) ? STP_PORT_PATH_COST_800G: STP_LEGACY_PORT_PATH_COST_800G);

        default:
            break;
	}

	STP_LOG_ERR("unknown port speed %d", port_speed);
	return 0; /* error */
}

/* FUNCTION
 *	stputil_set_vlan_topo_change()
 *
 * SYNOPSIS
 *	called from stptimer_update() to signal to the vlan that a topology
 *	change is in place.
 */
void stputil_set_vlan_topo_change(STP_CLASS *stp_class)
{
	/* The stp fast aging bit keeps track if the fast aging is enabled/disabled. 
	Returns if there is no topology change and fast aging is not enabled or if 
	there is a topology change and fast aging is already enabled. */
	if (stp_class->bridge_info.topology_change == stp_class->fast_aging)
		return;

    stpsync_update_fastage_state(stp_class->vlan_id, stp_class->bridge_info.topology_change);
	stp_class->fast_aging = stp_class->bridge_info.topology_change;
}




/* FUNCTION
 *		stputil_set_kernel_bridge_port_state()
 *
 * SYNOPSIS
 *      Linux VLAN aware brige model doesnt have a mechanism to update the STP port state.
 *		As a workaround we update the VLAN port membership for linux kernel bridge for filtering the traffic 
 *		if port is blocking (not forwarding) then VLAN is removed from the port in kernel
 *		when it moves to forwarding VLAN is added back to the port
 */
bool stputil_set_kernel_bridge_port_state(STP_CLASS * stp_class, STP_PORT_CLASS * stp_port_class)
{
    char cmd_buff[100];
    int ret;
    char * if_name = GET_STP_PORT_IFNAME(stp_port_class);
    char * tagged;

    if(is_member(stp_class->untag_mask, stp_port_class->port_id.number))
        tagged = "untagged";
    else
        tagged = "tagged";

    if(stp_port_class->state == FORWARDING && stp_port_class->kernel_state != STP_KERNEL_STATE_FORWARD) 
    {
        stp_port_class->kernel_state = STP_KERNEL_STATE_FORWARD;
        snprintf(cmd_buff, 100, "/sbin/bridge vlan add vid %u dev %s %s", stp_class->vlan_id, if_name, tagged);
    }
    else if(stp_port_class->state != FORWARDING && stp_port_class->kernel_state != STP_KERNEL_STATE_BLOCKING)
    {
        stp_port_class->kernel_state = STP_KERNEL_STATE_BLOCKING;
        snprintf(cmd_buff, 100, "/sbin/bridge vlan del vid %u dev %s %s", stp_class->vlan_id, if_name, tagged);
    }
    else
    {
        return true;//no-op
    }

    ret = system(cmd_buff);
    if(ret == -1)
    {
        STP_LOG_ERR("Error: cmd - %s strerr - %s", cmd_buff, strerror(errno));
        return false;
    }

    return true;
}

/* FUNCTION
 *		stputil_set_port_state()
 *
 * SYNOPSIS
 *		sets the port state for specific port class.
 */
bool stputil_set_port_state(STP_CLASS * stp_class, STP_PORT_CLASS * stp_port_class)
{
    stputil_set_kernel_bridge_port_state(stp_class, stp_port_class);
    stpsync_update_port_state(GET_STP_PORT_IFNAME(stp_port_class), GET_STP_INDEX(stp_class), stp_port_class->state);
    return true;
}

bool stputil_is_protocol_enabled(L2_PROTO_MODE proto_mode)
{
    if (stp_global.enable == true && stp_global.proto_mode == proto_mode)
        return true;

    return false;
}

/* FUNCTION
 *	stputil_get_class_from_vlan()
 *
 * SYNOPSIS
 *	utility function that searches the instances and returns the stp class
 *	associated with the input vlan id. returns NULL if error
 *	or unsuccessful.
 */
STP_CLASS *stputil_get_class_from_vlan(VLAN_ID vlan_id)
{
	STP_INDEX stp_index;
	STP_CLASS *stp_class;

	for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
	{
		stp_class = GET_STP_CLASS(stp_index);
		if (stp_class->state == STP_CLASS_FREE)
			continue;

		if (stp_class->vlan_id == vlan_id)
			return stp_class;
	}

	return NULL;
}

bool stputil_is_port_untag(VLAN_ID vlan_id, PORT_ID port_id)
{
    STP_CLASS *stp_class = 0;

    stp_class = stputil_get_class_from_vlan(vlan_id);
    if (stp_class)
        return is_member(stp_class->untag_mask, port_id);

    return false;
}

/* FUNCTION
 *	stputil_get_index_from_vlan()
 *
 * SYNOPSIS
 *	utility function that searches the instances and returns the stp index
 *	associated with the input vlan id. returns STP_INDEX_INVALID if error
 *	or unsuccessful.
 */
bool stputil_get_index_from_vlan(VLAN_ID vlan_id, STP_INDEX *stp_index)
{
	STP_CLASS *stp_class;
    STP_INDEX i = 0;

	for (i = 0; i < g_stp_instances; i++)
	{
		stp_class = GET_STP_CLASS(i);
		if (stp_class->state == STP_CLASS_FREE)
			continue;

		if (stp_class->vlan_id == vlan_id)
        {
			*stp_index = i;
            return true;
        }
	}
    return false;
}

/* STP HELPER ROUTINES ------------------------------------------------------ */

/* FUNCTION
 *		stputil_compare_mac()
 *
 * SYNOPSIS
 *		compares mac addresses - returns GREATER_THAN, LESS_THAN or EQUAL_TO
 */
enum SORT_RETURN stputil_compare_mac(MAC_ADDRESS *mac1, MAC_ADDRESS *mac2)
{

    if (mac1->_ulong > mac2->_ulong)
        return (GREATER_THAN);

    if(mac1->_ulong == mac2->_ulong)
    {
        if(mac1->_ushort > mac2->_ushort)
            return (GREATER_THAN);
        if(mac1->_ushort == mac2->_ushort)
            return (EQUAL_TO);
    }

    return (LESS_THAN);
}

/* FUNCTION
 *		stputil_compare_bridge_id()
 *
 * SYNOPSIS
 *		compares the bridge identifiers. returns GREATER_THAN, LESS_THAN or
 *		EQUAL_TO.
 */
enum SORT_RETURN stputil_compare_bridge_id(BRIDGE_IDENTIFIER *id1, BRIDGE_IDENTIFIER *id2)
{
	UINT16 priority1, priority2;

	priority1 = stputil_get_bridge_priority(id1);
	priority2 = stputil_get_bridge_priority(id2);

	if (priority1 > priority2)
		return (GREATER_THAN);

	if (priority1 < priority2)
		return (LESS_THAN);

	return stputil_compare_mac(&id1->address, &id2->address);
}

/* FUNCTION
 *		stputil_compare_port_id()
 *
 * SYNOPSIS
 *		compares port identifiers. returns GREATER_THAN, LESS_THAN or EQUAL_TO
 */
enum SORT_RETURN stputil_compare_port_id(PORT_IDENTIFIER *port_id1, PORT_IDENTIFIER *port_id2)
{
	UINT16 port1 = *((UINT16 *) port_id1);
	UINT16 port2 = *((UINT16 *) port_id2);

	if (port1 > port2)
		return (GREATER_THAN);

	if (port1 < port2)
		return (LESS_THAN);

	return (EQUAL_TO);
}

/* extended and legacy mode functions --------------------------------------- */

UINT16 stputil_get_bridge_priority(BRIDGE_IDENTIFIER *id)
{
    if (g_stpd_extend_mode)
        return (id->priority << 12);
    else
        return ((id->priority << 12) | (id->system_id));
}

void stputil_set_bridge_priority(BRIDGE_IDENTIFIER *id, UINT16 priority, VLAN_ID vlan_id)
{
	if (g_stpd_extend_mode)
    {
		id->priority = priority >> 12;
		id->system_id = vlan_id & 0xFFF;
    }
	else
	{
		id->priority = priority >> 12;
		id->system_id = priority & 0xFFF;
	}
}

// sets the global mask enabling/disabling ports.
void stputil_set_global_enable_mask(PORT_ID port_id, uint8_t add)
{
    if (add)
        set_mask_bit(g_stp_enable_mask, port_id);
    else
        clear_mask_bit(g_stp_enable_mask, port_id);
}

/* validation functions ----------------------------------------------------- */
/* FUNCTION
 *		stputil_validate_bpdu()
 *
 * SYNOPSIS
 *		validates that the received packet is a bpdu
 */
bool stputil_validate_bpdu(STP_CONFIG_BPDU *bpdu)
{
	if (bpdu->llc_header.destination_address_DSAP != LSAP_BRIDGE_SPANNING_TREE_PROTOCOL ||
		bpdu->llc_header.source_address_SSAP != LSAP_BRIDGE_SPANNING_TREE_PROTOCOL ||
		bpdu->llc_header.llc_frame_type != UNNUMBERED_INFORMATION)
	{
		return false;
	}

	if (bpdu->type != CONFIG_BPDU_TYPE &&
		bpdu->type != TCN_BPDU_TYPE)
	{
		return false;
	}

	if (bpdu->type != TCN_BPDU_TYPE)
	{
		if (ntohs(bpdu->hello_time) < STP_MIN_HELLO_TIME << 8)
		{
			// reset to default if received bpdu is incorrect.
			bpdu->hello_time = htons(STP_DFLT_HELLO_TIME << 8);
		}
	}

	return true;
}

/* FUNCTION
 *		stputil_validate_pvst_bpdu()
 *
 * SYNOPSIS
 *		validates that the received packet is a pvst bpdu
 */
bool stputil_validate_pvst_bpdu(PVST_CONFIG_BPDU *bpdu)
{
	if (bpdu->snap_header.destination_address_DSAP != LSAP_SNAP_LLC ||
		bpdu->snap_header.source_address_SSAP != LSAP_SNAP_LLC ||
		bpdu->snap_header.llc_frame_type != (enum LLC_FRAME_TYPE) UNNUMBERED_INFORMATION ||
		bpdu->snap_header.protocol_id_filler[0] != 0x00 ||
		bpdu->snap_header.protocol_id_filler[1] != 0x00 ||
		bpdu->snap_header.protocol_id_filler[2] != 0x0c ||
		ntohs(bpdu->snap_header.protocol_id) != SNAP_CISCO_PVST_ID ||
		ntohs(bpdu->protocol_id) != 0)
	{
		return false;
	}

	if (bpdu->type != CONFIG_BPDU_TYPE &&
		bpdu->type != TCN_BPDU_TYPE)
	{
		return false;
	}

	if (bpdu->type != TCN_BPDU_TYPE)
	{
		bpdu->vlan_id = ntohs(bpdu->vlan_id);
		bpdu->tag_length = ntohs(bpdu->tag_length);

		if ((bpdu->tag_length != 2) ||
			(bpdu->vlan_id < MIN_VLAN_ID || bpdu->vlan_id > MAX_VLAN_ID))
		{
			if (STP_DEBUG_BPDU_RX(bpdu->vlan_id, bpdu->port_id.number))
			{
				STP_PKTLOG("Discarding PVST BPDU with invalid VLAN:%u Port:%d", 
                        bpdu->vlan_id, bpdu->port_id.number);
			}
			return false;
		}

		if (ntohs(bpdu->hello_time) < (STP_MIN_HELLO_TIME << 8))
		{
			// reset to default if received bpdu is incorrect.
			bpdu->hello_time = htons(STP_DFLT_HELLO_TIME << 8);
		}
	}

	return true;
}

/*****************************************************************************/
/* stputil_root_protect_timer_expired: timer callback routine when the root  */
/* protect timer expires. logs message to the syslog and makes port          */
/* forwarding if required.                                                   */
/*****************************************************************************/
static bool stputil_root_protect_timer_expired(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port;

	if (stp_intf_is_port_up(port_number))
	{
		STP_SYSLOG("STP: Root Guard interface %s, VLAN %u consistent (Timeout) ", 
		        stp_intf_get_port_name(port_number), stp_class->vlan_id);
	    stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
        SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);
	}

	make_forwarding(stp_class, port_number);	

	return true;
}

/*****************************************************************************/
/* stputil_root_protect_violation: called when there is a root protect       */
/* violation i.e. a superior bpdu was received on the protected port. this   */
/* will result in setting the port blocking, triggering an snmp trap as well */
/* as a syslog message indicating that the port is inconsistent.             */
/*****************************************************************************/
static bool stputil_root_protect_violation(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port;

	stp_port = GET_STP_PORT_CLASS(stp_class, port_number);

	make_blocking(stp_class, port_number);
	if (!is_timer_active(&stp_port->root_protect_timer))
	{
		// log message
		STP_SYSLOG("STP: Root Guard interface %s, VLAN %u inconsistent (Received superior BPDU) ", 
			stp_intf_get_port_name(port_number), stp_class->vlan_id);
        SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);
	}

	// start/reset timer
	start_timer(&stp_port->root_protect_timer, 0);
	return true;
}

/*****************************************************************************/
/* stputil_root_protect_validate: routine validates the bpdu to see that it  */
/* is conforming to the root protect configuration.                          */
/*****************************************************************************/
static bool stputil_root_protect_validate(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu)
{

	if (bpdu->type != TCN_BPDU_TYPE &&
		supercedes_port_info(stp_class, port_number, bpdu))
	{
		// received superior bpdu on root-protect port
		stputil_root_protect_violation(stp_class, port_number);
		STP_LOG_INFO("STP_RAS_ROOT_PROTECT_VIOLATION I:%lu P:%lu V:%lu",GET_STP_INDEX(stp_class),port_number, stp_class->vlan_id);
		return false;
	}

	return true;
}

/* STP FAST SPAN AND FAST UPLINK ROUTINES ----------------------------------- */

/* FUNCTION
 *		stputil_is_fastuplink_ok()
 *
 * SYNOPSIS
 *		checks if it is OK to set the input-port forwarding. this routine
 *		checks that none of the other ports in the uplink-mask will be
 *		forwarding
 */
bool stputil_is_fastuplink_ok(STP_CLASS *stp_class, PORT_ID input_port)
{
	PORT_ID port_number;
	STP_PORT_CLASS *stp_port;

	if (!is_member(g_fastuplink_mask, input_port))
		return false;

	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		if ((is_member(g_fastuplink_mask, port_number)) &&
			(port_number != input_port))
		{
			stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
			if ((stp_port->state != BLOCKING) &&
				(stp_port->state != DISABLED))
			{
				return false;
			}
		}

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}

	return true;
}

/* STP PACKET TX AND RX ROUTINES -------------------------------------------- */

/* FUNCTION
 *		stputil_encode_bpdu()
 *
 * SYNOPSIS
 *		converts bpdu from host to network order.
 */
void stputil_encode_bpdu(STP_CONFIG_BPDU *bpdu)
{
	bpdu->protocol_id = htons(bpdu->protocol_id);
	if (bpdu->type == CONFIG_BPDU_TYPE ||
		bpdu->type == RSTP_BPDU_TYPE)
	{
		HOST_TO_NET_MAC(&bpdu->root_id.address, &bpdu->root_id.address);
		*((UINT16 *)&bpdu->root_id)     = htons(*((UINT16 *)&bpdu->root_id));
		bpdu->root_path_cost            = htonl(bpdu->root_path_cost);
		HOST_TO_NET_MAC(&bpdu->bridge_id.address, &bpdu->bridge_id.address);
		*((UINT16 *)&bpdu->bridge_id)   = htons(*((UINT16 *)&bpdu->bridge_id));
		*((UINT16 *) &bpdu->port_id)    = htons(*((UINT16 *) &bpdu->port_id));

		bpdu->message_age               = htons(bpdu->message_age);
		bpdu->max_age                   = htons(bpdu->max_age);
		bpdu->hello_time                = htons(bpdu->hello_time);
		bpdu->forward_delay             = htons(bpdu->forward_delay);
	}
}

/* FUNCTION
 *		stputil_decode_bpdu()
 *
 * SYNOPSIS
 *		converts bpdu from network to host order.
 */
void stputil_decode_bpdu(STP_CONFIG_BPDU *bpdu)
{
	bpdu->protocol_id = ntohs(bpdu->protocol_id);
	if (bpdu->type == CONFIG_BPDU_TYPE ||
		bpdu->type == RSTP_BPDU_TYPE)
	{
		NET_TO_HOST_MAC(&bpdu->root_id.address, &bpdu->root_id.address);
		*((UINT16 *)&bpdu->root_id)     = ntohs(*((UINT16 *)&bpdu->root_id));
		bpdu->root_path_cost            = ntohl(bpdu->root_path_cost);
		NET_TO_HOST_MAC(&bpdu->bridge_id.address, &bpdu->bridge_id.address);
		*((UINT16 *)&bpdu->bridge_id)   = ntohs(*((UINT16 *)&bpdu->bridge_id));
		*((UINT16 *) &bpdu->port_id)    = ntohs(*((UINT16 *) &bpdu->port_id));

		bpdu->message_age               = ntohs(bpdu->message_age) >> 8;
		bpdu->max_age                   = ntohs(bpdu->max_age) >> 8;
		bpdu->hello_time                = ntohs(bpdu->hello_time) >> 8;
		bpdu->forward_delay             = ntohs(bpdu->forward_delay) >> 8;
	}
}

VLAN_ID stputil_get_untag_vlan(PORT_ID port_id)
{
	STP_INDEX index;
	STP_CLASS * stp_class;

	for (index = 0; index < g_stp_instances; index++)
	{
		stp_class = GET_STP_CLASS(index);
		if(is_member(stp_class->untag_mask, port_id))
		    return stp_class->vlan_id;
    }
    return VLAN_ID_INVALID;
}

/* FUNCTION
 *		stputil_send_bpdu()
 *
 * SYNOPSIS
 *		transmits an stp bpdu.
 */
void stputil_send_bpdu(STP_CLASS* stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type)
{
	UINT8 *bpdu;
	UINT16 bpdu_size;
    VLAN_ID vlan_id;
	STP_PORT_CLASS *stp_port_class;
    MAC_ADDRESS port_mac = {0, 0};

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_intf_get_mac(port_number, &port_mac);

	if (type == CONFIG_BPDU_TYPE)
	{
		//stputil_encode_bpdu(&g_stp_config_bpdu);
		COPY_MAC(&g_stp_config_bpdu.mac_header.source_address, &port_mac);

		bpdu = (UINT8*) &g_stp_config_bpdu;
		bpdu_size = sizeof(STP_CONFIG_BPDU);
		(stp_port_class->tx_config_bpdu)++;
	}
	else
	{
	    if (STP_DEBUG_BPDU_TX(stp_class->vlan_id, port_number))
        {
		    STP_PKTLOG("Sending %s BPDU on Vlan:%d Port:%d", (type == CONFIG_BPDU_TYPE ? "Config" : "TCN"), 
                stp_class->vlan_id, port_number);
	    }

		COPY_MAC(&g_stp_tcn_bpdu.mac_header.source_address, &port_mac);

		bpdu = (UINT8*) &g_stp_tcn_bpdu;
		bpdu_size = sizeof(STP_TCN_BPDU);
		(stp_port_class->tx_tcn_bpdu)++;
	}

	vlan_id = stputil_get_untag_vlan(port_number);
    // to send pkt untagged
    if(vlan_id == VLAN_ID_INVALID)
    {
        // port is strictly tagged. can't send out untagged.
        return;
    }

	if (-1 == stp_pkt_tx_handler(port_number, vlan_id, (void*) bpdu, bpdu_size, false))
    {
        //Handle send err
        STP_LOG_ERR("Send STP-BPDU Failed");
    }
}

/* FUNCTION
 *		stputil_send_pvst_bpdu()
 *
 * SYNOPSIS
 *		transmits a pvst-bpdu. if the bpdu is associated with vlan 1, also
 *		triggers the transmission of an ieee bpdu.
 */
void stputil_send_pvst_bpdu(STP_CLASS* stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type)
{
	UINT8 *bpdu, *bpdu_tmp, *pvst_tmp;
	UINT16 bpdu_size, pvst_inc, bpdu_inc;
    VLAN_ID	vlan_id;
    MAC_ADDRESS port_mac = {0, 0};
    STP_PORT_CLASS *stp_port_class;
    bool untagged;

    stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    stp_intf_get_mac(port_number, &port_mac);
	if (type == CONFIG_BPDU_TYPE)
	{
		stputil_encode_bpdu(&g_stp_config_bpdu);

		COPY_MAC(&g_stp_pvst_config_bpdu.mac_header.source_address, &port_mac);

		g_stp_pvst_config_bpdu.vlan_id = htons(GET_VLAN_ID_TAG(stp_class->vlan_id));

		bpdu_inc = sizeof(MAC_HEADER) + sizeof(LLC_HEADER);
		pvst_inc = sizeof(MAC_HEADER) + sizeof(SNAP_HEADER);

		pvst_tmp = (UINT8 *)(&g_stp_pvst_config_bpdu) + pvst_inc;
		bpdu_tmp = (UINT8 *)(&g_stp_config_bpdu) + bpdu_inc;
		memcpy(pvst_tmp, bpdu_tmp, STP_SIZEOF_CONFIG_BPDU);

		bpdu = (UINT8*) &g_stp_pvst_config_bpdu;
		bpdu_size = sizeof(PVST_CONFIG_BPDU);
		stp_port_class->tx_config_bpdu++;
	}
	else
	{
	    if (STP_DEBUG_BPDU_TX(stp_class->vlan_id, port_number))
        {
            STP_PKTLOG("Sending PVST TCN BPDU on Vlan:%d Port:%d", stp_class->vlan_id, port_number);
	    }

		COPY_MAC(&g_stp_pvst_tcn_bpdu.mac_header.source_address, &port_mac);

		bpdu = (UINT8*) &g_stp_pvst_tcn_bpdu;
		bpdu_size = sizeof(PVST_TCN_BPDU);
		stp_port_class->tx_tcn_bpdu++;
	}

	vlan_id = stp_class->vlan_id;

    untagged = stputil_is_port_untag(vlan_id, port_number);

	if (-1 == stp_pkt_tx_handler(port_number, vlan_id, (void*) bpdu, bpdu_size, !untagged))
    {
        //Handle send err
        STP_LOG_ERR("Send PVST-BPDU Failed Vlan %u Port %u", vlan_id, port_number);
    }

	// PVST+ compatibility
	// send an untagged IEEE BPDU when sending a PVST BPDU for VLAN 1
	if (stp_class->vlan_id == 1)
	{
		stputil_send_bpdu(stp_class, port_number, type);
	}
}


/* FUNCTION
 *		stputil_process_bpdu()
 *
 * SYNOPSIS
 *		calls appropriate handlers to handle tcn and config bpdus. assume
 *		that the bpdu type has been validated by the time this function
 *		was called.
 */
void stputil_process_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer)
{
	STP_CONFIG_BPDU *bpdu = (STP_CONFIG_BPDU *) buffer;
	STP_CLASS *stp_class = GET_STP_CLASS(stp_index);
	UINT32 last_bpdu_rx_time = 0;
	UINT32 current_time = 0;

	// disable fast span on this port
	if(STP_IS_FASTSPAN_ENABLED(port_number))
    {
	    stputil_update_mask(g_fastspan_mask, port_number, false);
        stpsync_update_port_fast(stp_intf_get_port_name(port_number), false);
    }

	if (STP_IS_ROOT_PROTECT_CONFIGURED(port_number) &&
		stputil_root_protect_validate(stp_class, port_number, bpdu) == false)
	{
		// root protect code was triggered, return without any more processing.
		stp_class->rx_drop_bpdu++;
		return;
	}

	last_bpdu_rx_time = stp_class->last_bpdu_rx_time;
	current_time = sys_get_seconds();
	stp_class->last_bpdu_rx_time = current_time;			

	if (current_time < last_bpdu_rx_time)
	{
		last_bpdu_rx_time = last_bpdu_rx_time - current_time - 1; 
		current_time = (UINT32) -1;
	}

	if (((current_time - last_bpdu_rx_time) > (stp_class->bridge_info.hello_time + 1))
					       && (last_bpdu_rx_time != 0))
	{
		//RAS logging - Rx Delay Event
        if (debugGlobal.stp.enabled)
        {
            if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
            {
		        STP_LOG_INFO("Inst:%u Port:%u Vlan:%u Ev:%d Cur:%d Last:%d",
                    stp_index, port_number, stp_class->vlan_id, STP_RAS_MP_RX_DELAY_EVENT,
                    current_time, last_bpdu_rx_time);
            }
        }
        else
        {
            STP_LOG_INFO("Inst:%u Port:%u Vlan:%u Ev:%d Cur:%d Last:%d",
                stp_index, port_number, stp_class->vlan_id, STP_RAS_MP_RX_DELAY_EVENT,
                current_time, last_bpdu_rx_time);
        }
	}

	
	if (bpdu->type == TCN_BPDU_TYPE)
    {
        received_tcn_bpdu(stp_class, port_number, (STP_TCN_BPDU*) bpdu);
#if 0
        if(root_bridge(stp_class))
            vlanutils_send_layer2_protocol_tcn_event (stp_class->vlan_id, L2_STP, true);
#endif
    }
	else
		received_config_bpdu(stp_class, port_number, bpdu); // both RSTP and CONFIG bpdu
}

/* FUNCTION
 *		stputil_update_mask()
 *
 * SYNOPSIS
 *		trunk aware function to add or remove the port from a specified mask.
 */
void stputil_update_mask(PORT_MASK *mask, PORT_ID port_number, bool add)
{
    if (add)
        set_mask_bit(mask, port_number);
    else
        clear_mask_bit(mask, port_number);
    return;
}

void stptimer_sync_port_class(STP_CLASS *stp_class, STP_PORT_CLASS * stp_port)
{
    STP_VLAN_PORT_TABLE stp_vlan_intf = {0};
    char * ifname;
    UINT32 timer_value = 0;

    if(!stp_port->modified_fields)
        return;

    ifname = stp_intf_get_port_name(stp_port->port_id.number);
    if(ifname == NULL)
        return;

    strcpy(stp_vlan_intf.if_name, ifname);

    stp_vlan_intf.vlan_id = stp_class->vlan_id;

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_ID_BIT))
    {
        stp_vlan_intf.port_id = stp_port->port_id.number;
    }
    else
    {
        stp_vlan_intf.port_id = 0xFFFF;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT))
    {
        stp_vlan_intf.port_priority = stp_port->port_id.priority;
    }
    else
    {
        stp_vlan_intf.port_priority = -1;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT))
    {
        stputil_bridge_to_string(&stp_port->designated_root, stp_vlan_intf.designated_root, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT))
    {
        stp_vlan_intf.designated_cost = stp_port->designated_cost;
    }
    else
    {
        stp_vlan_intf.designated_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT))
    {
        stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_intf.designated_bridge, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_STATE_BIT))
    {
        get_timer_value(&stp_port->root_protect_timer, &timer_value);
        if(timer_value != 0 && stp_port->state == BLOCKING)
            strcpy(stp_vlan_intf.port_state, "ROOT-INC");
        else
            strcpy(stp_vlan_intf.port_state, l2_port_state_to_string(stp_port->state, stp_port->port_id.number));

        if (stp_port->state == DISABLED)
        {
            stp_vlan_intf.designated_cost = 0;
            strncpy(stp_vlan_intf.designated_bridge, "0000000000000000", STP_SYNC_BRIDGE_ID_LEN);
            strncpy(stp_vlan_intf.designated_root, "0000000000000000", STP_SYNC_BRIDGE_ID_LEN);
        }
    }
    else
    {
        stp_vlan_intf.port_state[0] = '\0';
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PATH_COST_BIT))
    {
        stp_vlan_intf.path_cost = stp_port->path_cost;
    }
    else
    {
        stp_vlan_intf.path_cost = 0xFFFFFFFF;
    }


    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT))
    {
        stp_vlan_intf.designated_port = (stp_port->designated_port.priority << 12 | stp_port->designated_port.number);
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_FWD_TRANSITIONS_BIT))
    {
        stp_vlan_intf.forward_transitions = stp_port->forward_transitions;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT))
    {
        stp_vlan_intf.tx_config_bpdu = stp_port->tx_config_bpdu;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT))
    {
        stp_vlan_intf.rx_config_bpdu = stp_port->rx_config_bpdu;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_SENT_BIT))
    {
        stp_vlan_intf.tx_tcn_bpdu = stp_port->tx_tcn_bpdu;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_RECVD_BIT))
    {
        stp_vlan_intf.rx_tcn_bpdu = stp_port->rx_tcn_bpdu;
    }
    
    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT))
    {
        get_timer_value(&stp_port->root_protect_timer, &timer_value);
        if(timer_value != 0)
            stp_vlan_intf.root_protect_timer = (stp_global.root_protect_timeout - STP_TICKS_TO_SECONDS(timer_value));
        else
            stp_vlan_intf.root_protect_timer = 0;
    }
    else
    {
        stp_vlan_intf.root_protect_timer = -1;
    }

    if(IS_BIT_SET(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT))
    {
        stp_vlan_intf.clear_stats = 1;
    }

    stp_port->modified_fields = 0;
    stpsync_update_port_class(&stp_vlan_intf);
}

void stptimer_sync_stp_class(STP_CLASS *stp_class)
{
    STP_PORT_CLASS *stp_port;
    STP_VLAN_TABLE stp_vlan_table = {0};
    char * ifname;

    if(!stp_class->modified_fields && !stp_class->bridge_info.modified_fields)
        return;

    stp_vlan_table.vlan_id = stp_class->vlan_id;

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT))
    {
        stputil_bridge_to_string(&stp_class->bridge_info.root_id, stp_vlan_table.root_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
        if (root_bridge(stp_class))
        {
            strcpy(stp_vlan_table.desig_bridge_id, stp_vlan_table.root_bridge_id);
        }
        else
        {
            stp_port = GET_STP_PORT_CLASS(stp_class, stp_class->bridge_info.root_port);
            if(stp_port)
            {
                stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_table.desig_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }
        }
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT))
    {
        stp_vlan_table.root_path_cost = stp_class->bridge_info.root_path_cost;
    }
    else
    {
        stp_vlan_table.root_path_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT))
    {
        if (root_bridge(stp_class))
        {
            strcpy(stp_vlan_table.root_port, "Root");
            strcpy(stp_vlan_table.desig_bridge_id, stp_vlan_table.root_bridge_id);
        }
        else
        {
            ifname = stp_intf_get_port_name(stp_class->bridge_info.root_port);
            if(ifname)
                snprintf(stp_vlan_table.root_port, IFNAMSIZ, "%s", ifname);
            
            stp_port = GET_STP_PORT_CLASS(stp_class, stp_class->bridge_info.root_port);
            if(stp_port)
            {
                stputil_bridge_to_string(&stp_port->designated_bridge, stp_vlan_table.desig_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }
        }
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT))
    {
        stp_vlan_table.root_max_age = stp_class->bridge_info.max_age;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT))
    {
        stp_vlan_table.root_hello_time = stp_class->bridge_info.hello_time;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT))
    {
        stp_vlan_table.root_forward_delay = stp_class->bridge_info.forward_delay;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HOLD_TIME_BIT))
    {
        stp_vlan_table.hold_time = stp_class->bridge_info.hold_time;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT))
    {
        stp_vlan_table.max_age = stp_class->bridge_info.bridge_max_age;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT))
    {
        stp_vlan_table.hello_time = stp_class->bridge_info.bridge_hello_time;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT))
    {
        stp_vlan_table.forward_delay = stp_class->bridge_info.bridge_forward_delay;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT))
    {
        stputil_bridge_to_string(&stp_class->bridge_info.bridge_id, stp_vlan_table.bridge_id, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_COUNT_BIT))
    {
        stp_vlan_table.topology_change_count = stp_class->bridge_info.topology_change_count;
    }

    if(IS_BIT_SET(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT))
    {
        stp_vlan_table.topology_change_time = (stp_class->bridge_info.topology_change_tick ? 
                (sys_get_seconds() - stp_class->bridge_info.topology_change_tick) : 0 );
    }

    stp_vlan_table.stp_instance = GET_STP_INDEX(stp_class);

    stp_class->modified_fields = 0;
    stp_class->bridge_info.modified_fields = 0;

    stpsync_update_stp_class(&stp_vlan_table);
}


void stptimer_sync_db(STP_CLASS *stp_class)
{
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port_class;

    stptimer_sync_stp_class(stp_class);

    if(!stp_class->control_mask)
        return;

    for(port_number = port_mask_get_first_port(stp_class->control_mask); 
            port_number != BAD_PORT_ID; 
                port_number = port_mask_get_next_port(stp_class->control_mask, port_number))
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
        stptimer_sync_port_class(stp_class, stp_port_class);
    }
}

void stputil_sync_port_counters(STP_CLASS *stp_class, STP_PORT_CLASS * stp_port)
{
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_SENT_BIT);
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_TC_RECVD_BIT);

	if (is_timer_active(&stp_port->root_protect_timer))
	    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_ROOT_PROTECT_BIT);
        
    stptimer_sync_port_class(stp_class, stp_port);
}

void stptimer_sync_bpdu_counters(STP_CLASS * stp_class)
{
    PORT_ID port_number;
    STP_PORT_CLASS *stp_port_class;
    
    /* Sync topology change tick count */
    if(stp_class->bridge_info.topology_change_tick)
    {
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT);
        stptimer_sync_stp_class(stp_class);
    }

    for(port_number = port_mask_get_first_port(stp_class->control_mask); 
            port_number != BAD_PORT_ID; 
                port_number = port_mask_get_next_port(stp_class->control_mask, port_number))
    {
        stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
        stputil_sync_port_counters(stp_class, stp_port_class);
    }
}

/* STP TIMER ROUTINES ------------------------------------------------------- */

/* FUNCTION
 *		stptimer_tick()
 *
 * SYNOPSIS
 *		This function is invoked every 100 ms. The STP instances are
 *		divided into 5 groups with one group getting serviced per
 *		stp_timer_tick() call. Each STP instance will therefore be
 *		serviced every 500 ms. This is done by keeping track of each
 *		100 ms tick (using g_stp_tick_id).
 *
 *		The following table applies -
 *
 *		g_stp_tick_id   schedule STP instances
 *		      0                0,5,10 ...
 *		      1                1,6,11 ...
 *		      2                2,7,12 ...
 *		      3                3,8,13 ...
 *		      4                4,9,14 ...
 *
 */
void stptimer_tick()
{
	STP_CLASS *stp_class;
	UINT16 i, start_instance;

	// handle stp timer
	if (g_stp_active_instances)
	{
		for (i = g_stp_tick_id; i < g_stp_instances; i+=5)
		{
			stp_class = GET_STP_CLASS(i);

			if (stp_class->state == STP_CLASS_ACTIVE)
				stptimer_update(stp_class);

			if (stp_class->state == STP_CLASS_ACTIVE || stp_class->state == STP_CLASS_CONFIG)
				stptimer_sync_db(stp_class);
        }

        if(g_stp_bpdu_sync_tick_id % 10 == 0)
        {
            start_instance = g_stp_bpdu_sync_tick_id/10;
            for(i = start_instance; i < g_stp_instances; i+=10)
            {
                stp_class = GET_STP_CLASS(i);

                if (stp_class->state == STP_CLASS_ACTIVE)
                    stptimer_sync_bpdu_counters(stp_class);
            }
        }
	}

	g_stp_bpdu_sync_tick_id++;
	if(g_stp_bpdu_sync_tick_id >= 100)
    {
        g_stp_bpdu_sync_tick_id = 0;
    }

	g_stp_tick_id++;
	if (g_stp_tick_id >= 5)
	{
		g_stp_tick_id = 0;
	}
}

/* FUNCTION
 *		stptimer_update()
 *
 * SYNOPSIS
 *		this is called every 500ms for every stp class. it updates the values
 *		of the timers. if any timer has expired, it also executes the timer
 *		expiry routine. this is also currently the point when a topology
 *		change is indicated to the parent vlan.
 */
void stptimer_update(STP_CLASS *stp_class)
{
	UINT32 forward_delay;
	PORT_ID port_number;
	STP_PORT_CLASS *stp_port_class;

	if (stptimer_expired(&stp_class->hello_timer, stp_class->bridge_info.hello_time))
	{
		hello_timer_expiry(stp_class);
	}

	if (stptimer_expired(&stp_class->topology_change_timer, stp_class->bridge_info.topology_change_time))
	{
		topology_change_timer_expiry(stp_class);
	}

	if (stptimer_expired(&stp_class->tcn_timer, stp_class->bridge_info.hello_time))
	{
		tcn_timer_expiry(stp_class);
	}

	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

		if (STP_IS_FASTSPAN_ENABLED(port_number))
		{
			forward_delay = STP_FASTSPAN_FORWARD_DELAY;
		}
        else if(stputil_is_fastuplink_ok(stp_class, port_number))
        {
            /* With uplink fast transition to forwarding should happen in 1 sec */
            if(stp_port_class->state == LISTENING)
                forward_delay = STP_FASTUPLINK_FORWARD_DELAY;
            else
                forward_delay = 0;
        }
		else
		{
			forward_delay = stp_class->bridge_info.forward_delay;
		}

        if (stptimer_expired(&stp_port_class->forward_delay_timer, forward_delay))
        {
            forwarding_delay_timer_expiry(stp_class, port_number);
        }

        if (stptimer_expired(&stp_port_class->message_age_timer, stp_class->bridge_info.max_age))
        {
            message_age_timer_expiry(stp_class, port_number);

            if (debugGlobal.stp.enabled)
            {
                if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
                {
                    STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number, 
                        stp_class->vlan_id, STP_RAS_MES_AGE_TIMER_EXPIRY);
                }
            }
            else
            {
                STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d", GET_STP_INDEX(stp_class), port_number, 
                    stp_class->vlan_id, STP_RAS_MES_AGE_TIMER_EXPIRY);
            }

            /* Sync to APP DB */
            SET_ALL_BITS(stp_class->bridge_info.modified_fields); 
            SET_ALL_BITS(stp_class->modified_fields);
        }
		
		if (stptimer_expired(&stp_port_class->hold_timer,
			stp_class->bridge_info.hold_time))
		{
			hold_timer_expiry(stp_class, port_number);
		}

		if (stptimer_expired(&stp_port_class->root_protect_timer, stp_global.root_protect_timeout) ||
			(stp_port_class->root_protect_timer.active && !STP_IS_ROOT_PROTECT_CONFIGURED(port_number)))
		{
			stp_port_class->root_protect_timer.active = false;
			stputil_root_protect_timer_expired(stp_class, port_number);	

            if (debugGlobal.stp.enabled)
            {
                if (STP_DEBUG_VP(stp_class->vlan_id, port_number))
                {
                    STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d",GET_STP_INDEX(stp_class),port_number, 
                            stp_class->vlan_id,STP_RAS_ROOT_PROTECT_TIMER_EXPIRY);
                }
            }
            else
            {
                STP_LOG_INFO("I:%lu P:%u V:%u Ev:%d",GET_STP_INDEX(stp_class),port_number, 
                    stp_class->vlan_id,STP_RAS_ROOT_PROTECT_TIMER_EXPIRY);
            }
		}

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}

	// initiate fast-aging on vlan if the stp instance is in topology change
	stputil_set_vlan_topo_change(stp_class);
}

/* FUNCTION
 *		stptimer_start()
 *
 * SYNOPSIS
 *		activates timer and sets the initial value to the input start value.
 */
void stptimer_start(TIMER *timer, UINT32 start_value_in_seconds)
{
    start_timer(timer, STP_SECONDS_TO_TICKS(start_value_in_seconds));
}

/* FUNCTION
 *		stptimer_stop()
 *
 * SYNOPSIS
 *		de-activates timer
 */
void stptimer_stop(TIMER *timer)
{
    stop_timer(timer);
}

/* FUNCTION
 *		stptimer_expired()
 *
 * SYNOPSIS
 *		increments the active timer. if the active timer exceeds the timer
 *		limit, de-activates the timer and signal the caller that the timer has
 *		expired (returns true). if the timer is inactive or has not expired
 *		returns false
 */
bool stptimer_expired(TIMER *timer, UINT32 timer_limit_in_seconds)
{
    return timer_expired(timer, STP_SECONDS_TO_TICKS(timer_limit_in_seconds));
}

/* FUNCTION
 *		stptimer_start()
 *
 * SYNOPSIS
 *		Checks if the timer is Active
 */
bool stptimer_is_active(TIMER *timer)
{
    return (is_timer_active(timer));
}

int mask_to_string(BITMAP_T *bmp, uint8_t *str, uint32_t maxlen)
{
    if (!bmp || !bmp->arr || (maxlen < 5) || !str)
    {
        STP_LOG_ERR("Invalid inputs");
        return -1;
    }

    *str = '\0';

    if(is_mask_clear(bmp))
    {
        STP_LOG_DEBUG("BMP is Clear");
        return -1;
    }

    int32_t bmp_id = BMP_INVALID_ID;
    uint32_t len = 0;
    do {
        if (len >= maxlen)
        {
            return -1;
        }

        bmp_id = bmp_get_next_set_bit(bmp, bmp_id);
        if (bmp_id != -1)
            len += snprintf((char *)str+len, maxlen-len, "%d ", bmp_id);
    }while(bmp_id != -1);

    return len;
}

int mask_to_string2(BITMAP_T *bmp, uint8_t *str, uint32_t maxlen)
{
    if (!bmp || !bmp->arr || (maxlen < 5) || !str)
    {
        STP_LOG_ERR("Invalid inputs");
        return -1;
    }

    *str = '\0';

    if(is_mask_clear(bmp))
    {
        STP_LOG_DEBUG("BMP is Clear");
        return -1;
    }

    int32_t bmp_id = BMP_INVALID_ID;
    uint32_t len = 0;
    do {
        if (len >= maxlen)
        {
            return -1;
        }

        bmp_id = bmp_get_next_set_bit(bmp, bmp_id);
        if (bmp_id != -1)
            len += snprintf((char *)str+len, maxlen-len, "%d,", bmp_id);
    }while(bmp_id != -1);

    return len;
}

void sys_assert(int status)
{
    assert(status);
}

