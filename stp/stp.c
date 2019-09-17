/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

DEBUG_GLOBAL debugGlobal;

/* 8.6.1 */
void transmit_config(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class, *stp_root_port_class;
	UINT32 val = 0;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	if (stp_port_class->hold_timer.active)
	{
		stp_port_class->config_pending = true;
		return;
	}

    g_stp_config_bpdu.root_id = stp_class->bridge_info.root_id;
    g_stp_config_bpdu.root_path_cost = stp_class->bridge_info.root_path_cost;
    g_stp_config_bpdu.bridge_id = stp_class->bridge_info.bridge_id;
    g_stp_config_bpdu.port_id = stp_port_class->port_id;

	if (root_bridge(stp_class))
	{
		g_stp_config_bpdu.message_age = 0;
	}
	else
	{
		stp_root_port_class = GET_STP_PORT_CLASS(stp_class, stp_class->bridge_info.root_port);

		get_timer_value(&stp_root_port_class->message_age_timer, &val);
		g_stp_config_bpdu.message_age = (UINT16) ((STP_TICKS_TO_SECONDS(val) + STP_MESSAGE_AGE_INCREMENT) << 8);
	}

	g_stp_config_bpdu.max_age = (UINT16) stp_class->bridge_info.max_age << 8;
	g_stp_config_bpdu.hello_time = (UINT16) stp_class->bridge_info.hello_time << 8;
	g_stp_config_bpdu.forward_delay = (UINT16) stp_class->bridge_info.forward_delay << 8;
	g_stp_config_bpdu.flags.topology_change_acknowledgement = stp_port_class->topology_change_acknowledge;
	g_stp_config_bpdu.flags.topology_change = stp_class->bridge_info.topology_change;

	if (g_stp_config_bpdu.message_age < g_stp_config_bpdu.max_age)
	{
		stp_port_class->topology_change_acknowledge = false;
		stp_port_class->config_pending = false;
		send_config_bpdu(stp_class, port_number);
		stptimer_start(&stp_port_class->hold_timer, 0);
	}
}

/* 8.6.2.2 */
bool supercedes_port_info(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu)
{
	enum SORT_RETURN result;
	STP_PORT_CLASS *stp_port_class;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	stp_port_class->self_loop = false;

	result = stputil_compare_bridge_id(&bpdu->root_id, &stp_port_class->designated_root);
	if (result == LESS_THAN)
    {
		return (true); 
    }
	if (result == GREATER_THAN)
		return (false); 

	if (bpdu->root_path_cost < stp_port_class->designated_cost)
    {
		return (true);
    }
	if (bpdu->root_path_cost > stp_port_class->designated_cost)
		return (false); 

	result = stputil_compare_bridge_id(&bpdu->bridge_id, &stp_port_class->designated_bridge);
	if (result == LESS_THAN)
    {
		return (true);
    }
	if (result == GREATER_THAN)
		return (false);

	if (stputil_compare_bridge_id(&bpdu->bridge_id, &stp_class->bridge_info.bridge_id) != EQUAL_TO)
		return (true); 

	result = stputil_compare_port_id(&bpdu->port_id, &stp_port_class->designated_port);
	if (result == LESS_THAN)
    {
		return (true);
    }
	if (result == GREATER_THAN)
		return (false);

	// check if sending port is member of this stp instance (this is 
	// necessary to isolate the case when another stp instance's bpdu is 
	// received on this port).
	if (!is_member(stp_class->enable_mask, bpdu->port_id.number))
	{
		return true;
	}

	// received on a backup port or on a designated port on token-ring cabling
	stp_port_class->self_loop = true;

	return (true);
}

/* 8.6.2 */
void record_config_information(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);;
	
    if(stputil_compare_bridge_id(&stp_port_class->designated_root, &bpdu->root_id) != 0)
    {
	    stp_port_class->designated_root = bpdu->root_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT);
    }

	if(stp_port_class->designated_cost != bpdu->root_path_cost)
    {
	    stp_port_class->designated_cost = bpdu->root_path_cost;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT);
    }

    if(stputil_compare_bridge_id(&stp_port_class->designated_bridge, &bpdu->bridge_id) != 0)
    {
	    stp_port_class->designated_bridge = bpdu->bridge_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT);
    }

	if(stputil_compare_port_id(&stp_port_class->designated_port, &bpdu->port_id))
    {
	    stp_port_class->designated_port = bpdu->port_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT);
    }

	stptimer_start(&stp_port_class->message_age_timer, bpdu->message_age);
}

/* 8.6.3 */
void record_config_timeout_values(STP_CLASS *stp_class, STP_CONFIG_BPDU *bpdu)
{
	if(stp_class->bridge_info.max_age != (UINT8) bpdu->max_age)
    {
	    stp_class->bridge_info.max_age = (UINT8) bpdu->max_age;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
    }

	if(stp_class->bridge_info.hello_time != (UINT8) bpdu->hello_time)
    {
	    stp_class->bridge_info.hello_time = (UINT8) bpdu->hello_time;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
    }

	if(stp_class->bridge_info.forward_delay != (UINT8) bpdu->forward_delay)
    {
	    stp_class->bridge_info.forward_delay = (UINT8) bpdu->forward_delay;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
    }

	stp_class->bridge_info.topology_change = bpdu->flags.topology_change;
}

/* 8.6.4 */ 
void config_bpdu_generation (STP_CLASS *stp_class)
{
	PORT_ID port_number;

	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		if (designated_port(stp_class, port_number))
		{
			transmit_config(stp_class, port_number);
		}

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}
}

/* 8.6.5 */
void reply(STP_CLASS *stp_class, PORT_ID port_number)
{
	transmit_config(stp_class, port_number);
}

/* 8.6.6 */
void transmit_tcn(STP_CLASS *stp_class)
{
	PORT_ID port_number = stp_class->bridge_info.root_port;
	if (port_number != STP_INVALID_PORT)
	{
		send_tcn_bpdu(stp_class, port_number);
	}
}

/* 8.6.7 */
void configuration_update(STP_CLASS *stp_class)
{
	root_selection(stp_class);
	designated_port_selection(stp_class);
}

/* 8.6.8 */
void root_selection(STP_CLASS *stp_class)
{
	enum SORT_RETURN result;
	PORT_ID port_number, root_port;
	STP_PORT_CLASS *stp_port_class, *root_port_class;
	
	root_port = STP_INVALID_PORT;

	for (
		port_number = port_mask_get_first_port(stp_class->enable_mask);
		port_number != BAD_PORT_ID; 
		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number)
		)
	{
        if (STP_DEBUG_EVENT(stp_class->vlan_id, port_number))
            STP_LOG_DEBUG("vlan %d port %d", stp_class->vlan_id, port_number);

		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

		// do not service ports that are on token-ring cabling or backup ports
		if (stp_port_class->self_loop) 
			continue;
		

		if ((!designated_port(stp_class, port_number)) && 
			(stputil_compare_bridge_id(&stp_port_class->designated_root,
						&stp_class->bridge_info.bridge_id) == LESS_THAN)) 
		{
			if (root_port == STP_INVALID_PORT)
			{
				root_port = port_number;
				continue;
			}

			root_port_class = GET_STP_PORT_CLASS(stp_class, root_port);

			result = stputil_compare_bridge_id(&stp_port_class->designated_root,
							&root_port_class->designated_root);
			if (result == LESS_THAN)
			{
				root_port = port_number;
				continue;
			}
			if (result == GREATER_THAN)
				continue;

            if (stp_port_class->path_cost + stp_port_class->designated_cost <
                root_port_class->path_cost + root_port_class->designated_cost)
            {
                root_port = port_number;
                continue;
            }

            if (stp_port_class->path_cost + stp_port_class->designated_cost >
                root_port_class->path_cost + root_port_class->designated_cost)
                continue;

			result = stputil_compare_bridge_id (&stp_port_class->designated_bridge,
						&root_port_class->designated_bridge);
			if (result == LESS_THAN)
			{
				root_port = port_number;
				continue;
			}
			if (result == GREATER_THAN)
				continue;

			result = stputil_compare_port_id (&stp_port_class->designated_port,
							&root_port_class->designated_port);
			if (result == LESS_THAN)
			{
				root_port = port_number;
				continue;
			}
			if (result == GREATER_THAN)
				continue;

			if (stputil_compare_port_id(&stp_port_class->port_id,
				&root_port_class->port_id) == LESS_THAN)
			{
				root_port = port_number;
			}
		}
	} // for

	if (root_port == STP_INVALID_PORT)
	{
		stp_class->bridge_info.root_id = stp_class->bridge_info.bridge_id;
		stp_class->bridge_info.root_path_cost = 0;
		stpmgr_set_bridge_params(stp_class);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT);
	}
	else
	{
		root_port_class = GET_STP_PORT_CLASS(stp_class, root_port);
	    if(stputil_compare_bridge_id(&stp_class->bridge_info.root_id, &root_port_class->designated_root) != 0)
        {
    		stp_class->bridge_info.root_id = root_port_class->designated_root;
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
        }

		if(stp_class->bridge_info.root_path_cost != 
				root_port_class->designated_cost + root_port_class->path_cost)
        {
		    stp_class->bridge_info.root_path_cost = 
			    	root_port_class->designated_cost + root_port_class->path_cost;
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT);
        }

		if (stp_class->bridge_info.root_port != root_port)
		{
            STP_LOG_INFO("STP_RAS_ROOT_ROLE I:%lu P:%lu V:%u",GET_STP_INDEX(stp_class), root_port, stp_class->vlan_id);
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT);
		}
	}

	stp_class->bridge_info.root_port = root_port;
}

/* 8.6.9 */
void designated_port_selection(STP_CLASS *stp_class)
{
	PORT_ID port_number;
	STP_PORT_CLASS *stp_port_class;
	enum SORT_RETURN result;

	for (
		port_number = port_mask_get_first_port(stp_class->enable_mask);
		port_number != BAD_PORT_ID; 
		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number)
		)
	{
        if (STP_DEBUG_EVENT(stp_class->vlan_id, port_number))
            STP_LOG_DEBUG("vlan %d port %d", stp_class->vlan_id, port_number);

		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

		// case 1
		if (designated_port(stp_class, port_number))
		{
			become_designated_port(stp_class, port_number);
			continue;
		}

		// case 2
		if (stputil_compare_bridge_id(&stp_port_class->designated_root,
				&stp_class->bridge_info.root_id) != EQUAL_TO) 
		{
			become_designated_port(stp_class, port_number);
			continue;
		}

		// case 3
		if (stp_class->bridge_info.root_path_cost < stp_port_class->designated_cost)
		{
			become_designated_port(stp_class, port_number);
			continue;
		}

		if (stp_class->bridge_info.root_path_cost > stp_port_class->designated_cost) 
			continue;

		result = stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id,
			 &stp_port_class->designated_bridge);

		// case 4
		if (result == LESS_THAN)
		{
			become_designated_port(stp_class, port_number);
			continue;
		}

		if (result == GREATER_THAN)
			continue;

		// case 5
		if ((stputil_compare_port_id(&stp_port_class->port_id,
			 &stp_port_class->designated_port) != GREATER_THAN))
		{
			become_designated_port(stp_class, port_number);
            STP_LOG_INFO("STP_RAS_DESIGNATED_ROLE I:%lu P:%lu V:%lu",GET_STP_INDEX(stp_class),port_number, stp_class->vlan_id);
		}
	}
}

/* 8.6.10 */
void become_designated_port(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	if (!stp_port_class)
		return;

    if(stputil_compare_bridge_id(&stp_class->bridge_info.root_id, &stp_port_class->designated_root) != 0)
    {
	    stp_port_class->designated_root = stp_class->bridge_info.root_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT);
    }

	if(stp_port_class->designated_cost != stp_class->bridge_info.root_path_cost)
    {
	    stp_port_class->designated_cost = stp_class->bridge_info.root_path_cost;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT);
    }

    if(stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id, &stp_port_class->designated_bridge) != 0)
    {
	    stp_port_class->designated_bridge = stp_class->bridge_info.bridge_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT);
    }

	if(stputil_compare_port_id(&stp_port_class->designated_port, &stp_port_class->port_id) != 0)
    {
	    stp_port_class->designated_port = stp_port_class->port_id;
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT);
    }
}

/* 8.6.11 */
void port_state_selection(STP_CLASS *stp_class)
{
	STP_PORT_CLASS *stp_port_class;
	PORT_ID port_number;
	UINT8	prev_state = 0;
	
	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
        if (STP_DEBUG_EVENT(stp_class->vlan_id, port_number))
            STP_LOG_DEBUG("vlan %d port %d", stp_class->vlan_id, port_number);

		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

		if (port_number == stp_class->bridge_info.root_port)
		{
			stp_port_class->config_pending = false;
			stp_port_class->topology_change_acknowledge = false;
			make_forwarding(stp_class, port_number);
		}
		else if (stp_port_class->self_loop) 
		{
			stp_port_class->config_pending = false;
			stp_port_class->topology_change_acknowledge = false;
			make_blocking(stp_class, port_number);
		} 
		else if (designated_port(stp_class, port_number))
		{
			stptimer_stop(&stp_port_class->message_age_timer);
			make_forwarding(stp_class, port_number);
		}
		else
		{
			stp_port_class->config_pending = false;
			stp_port_class->topology_change_acknowledge = false;			
			prev_state = stp_port_class->state;
            make_blocking(stp_class, port_number);
		}

		prev_state = 0;

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}

}

/* 8.6.12 */
void make_forwarding(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	if ((stp_port_class->state == BLOCKING) && (!is_timer_active(&stp_port_class->root_protect_timer)))
	{
	
		stp_port_class->state = LISTENING;

        stputil_set_port_state(stp_class, stp_port_class);
		stptimer_start(&stp_port_class->forward_delay_timer, 0);

		stplog_port_state_change(stp_class, port_number, STP_MAKE_FORWARDING);
		if (STP_DEBUG_EVENT(stp_class->vlan_id, port_number))
		{
			STP_LOG_DEBUG("LISTENING Vlan:%d Port:%d", stp_class->vlan_id, port_number);
		}
		SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_PORT_STATE_BIT);
	}
}

/* 8.6.13 */
void make_blocking(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class;
	
	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	switch (stp_port_class->state)
	{
		case BLOCKING:
		case DISABLED:
			return; // do nothing

		case FORWARDING:
		case LEARNING:
			if ((stp_port_class->change_detection_enabled) &&
				!STP_IS_FASTSPAN_ENABLED(port_number))
			{
				topology_change_detection(stp_class);
				stplog_topo_change(stp_class, port_number, STP_MAKE_BLOCKING);
				if (stp_port_class->state == FORWARDING) {
                    //TODO:Find Alternate for SNMP_TRAP
                    //send_stp_topo_change_trap();
				}
			}
			// fall thru

		case LISTENING:
			stp_port_class->state = BLOCKING;
			stputil_set_port_state(stp_class, stp_port_class);
			SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_PORT_STATE_BIT);
			stptimer_stop(&stp_port_class->forward_delay_timer);			
			break;

		default:
			STP_LOG_ERR("make_blocking() - unknown stp state for port %d - state %d", 
					port_number, stp_port_class->state);
			return;
	}

	stplog_port_state_change(stp_class, port_number, STP_MAKE_BLOCKING);	
    STP_LOG_INFO("STP_RAS_BLOCKING I:%lu P:%lu V:%lu",GET_STP_INDEX(stp_class),port_number, stp_class->vlan_id);
} 

/* 8.6.14 */
void topology_change_detection(STP_CLASS *stp_class)
{
	if (root_bridge(stp_class))
	{
		stp_class->bridge_info.topology_change = true;
		stp_class->bridge_info.topology_change_time = 
			stp_class->bridge_info.forward_delay + stp_class->bridge_info.max_age ;
		stptimer_start(&stp_class->topology_change_timer, 0);
	}
	else 
	if (!stp_class->bridge_info.topology_change_detected)
	{
		transmit_tcn(stp_class);
		stptimer_start(&stp_class->tcn_timer, 0);
	}

	stp_class->bridge_info.topology_change_detected = true;
	stp_class->bridge_info.topology_change_tick = sys_get_seconds();
	(stp_class->bridge_info.topology_change_count)++;
    SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_COUNT_BIT);
}

/* 8.6.15 */
void topology_change_acknowledged(STP_CLASS *stp_class)
{
	stp_class->bridge_info.topology_change_detected = false;
	/* If there was already a topology change active, then reset it, so that 
	 * when we receive TC from root we will trigger the flush again
	 * */
    STP_LOG_INFO("TCAcked reset TC vlan %u", stp_class->vlan_id);
	stp_class->bridge_info.topology_change = false;
	stptimer_stop(&stp_class->tcn_timer);
}

/* 8.6.16 */
void acknowledge_topology_change(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	stp_port_class->topology_change_acknowledge = true;
	transmit_config(stp_class, port_number);
}

/* 8.7.1 */
void received_config_bpdu(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu)
{
	bool root;
	STP_PORT_CLASS *stp_port_class, *ccep_stp_port_class;
	bool result;
	PORT_ID root_port = stp_class->bridge_info.root_port;
	
	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	if (stp_port_class->state == DISABLED)
		return;

	root = root_bridge(stp_class);
	result = supercedes_port_info(stp_class, port_number, bpdu);

	if (result)
	{
		
		record_config_information(stp_class, port_number, bpdu);
		configuration_update(stp_class);

		port_state_selection(stp_class);

		if (!root_bridge(stp_class) && root)
		{
			stptimer_stop(&stp_class->hello_timer);

			if (stp_class->bridge_info.topology_change_detected)
			{
				stptimer_stop(&stp_class->topology_change_timer);
				transmit_tcn(stp_class);
				stptimer_start(&stp_class->tcn_timer, 0);
			}

			stplog_root_change(stp_class, STP_BPDU_RECEIVED);
		}

		if (port_number == stp_class->bridge_info.root_port)
		{
			record_config_timeout_values(stp_class, bpdu);
			config_bpdu_generation(stp_class);

			if (bpdu->flags.topology_change_acknowledgement)
			{
				if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
				{
					STP_PKTLOG("TCN ACK Received Vlan:%d Port:%d", stp_class->vlan_id, port_number);
				}
				topology_change_acknowledged(stp_class);

			}
		}
	}
	else 
	{
		if (designated_port(stp_class, port_number))
		{
			if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
			{
				STP_PKTLOG("Inferior BPDU Received Inst:%lu Port:%u Vlan:%u",
                        GET_STP_INDEX(stp_class), port_number, stp_class->vlan_id);
			}
            reply(stp_class, port_number);
		}
		
	}
}

/* 8.7.2 */
void received_tcn_bpdu(STP_CLASS *stp_class, PORT_ID port_number, STP_TCN_BPDU *bpdu)
{
	bool	root;
	STP_PORT_CLASS	*stp_port_class;
	
	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	if (stp_port_class->state == DISABLED)
		return;
	
    if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
        STP_PKTLOG("Process TCN I:%lu P:%u V:%u",GET_STP_INDEX(stp_class),port_number, stp_class->vlan_id);

	root = root_bridge(stp_class);

	if (designated_port(stp_class, port_number))
	{
		topology_change_detection(stp_class);
		acknowledge_topology_change(stp_class, port_number);
	}
}

/* 8.7.3 */
void hello_timer_expiry(STP_CLASS *stp_class)
{
	UINT32 last_expiry_time = 0;
	UINT32 current_time = 0;

	last_expiry_time = stp_class->last_expiry_time;
	current_time = sys_get_seconds();
	stp_class->last_expiry_time = current_time;

	if (current_time < last_expiry_time)
	{
		last_expiry_time = last_expiry_time - current_time - 1; 
		current_time = (UINT32) -1;
	}

	if (((current_time - last_expiry_time) > (stp_class->bridge_info.hello_time + 1))
				 	   && (last_expiry_time != 0))
	{
		//RAS logging - Timer Delay Event
        if (debugGlobal.stp.enabled)
        {
            if (STP_DEBUG_VP(stp_class->vlan_id, (uint16_t)BAD_PORT_ID))
            {
                STP_LOG_INFO("Inst:%lu Vlan:%u Ev:%d Cur:%d Last:%d",
                    GET_STP_INDEX(stp_class), stp_class->vlan_id, STP_RAS_TIMER_DELAY_EVENT, current_time, last_expiry_time);
            }
        }
        else
        {
            STP_LOG_INFO("Inst:%lu Vlan:%u Ev:%d Cur:%d Last:%d",
                GET_STP_INDEX(stp_class), stp_class->vlan_id, STP_RAS_TIMER_DELAY_EVENT, current_time, last_expiry_time);
        }
	}

	config_bpdu_generation(stp_class);
	stptimer_start(&stp_class->hello_timer, 0);
}

/* 8.7.4 */
void message_age_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number)
{
	bool root;
	STP_PORT_CLASS *stp_port_class, *stp_ccep_port_class;
	PORT_ID ccep_port_id;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	stp_port_class->self_loop = false;

	root = root_bridge(stp_class);
	become_designated_port(stp_class, port_number);
	configuration_update(stp_class);
	port_state_selection(stp_class);

	if (root_bridge(stp_class) && !root)
	{
		stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
		stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
		stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;

		topology_change_detection(stp_class);
		stptimer_stop(&stp_class->tcn_timer);
		config_bpdu_generation(stp_class);
		stptimer_start(&stp_class->hello_timer, 0);

		stplog_topo_change(stp_class, port_number, STP_MESSAGE_AGE_EXPIRY);
		stplog_new_root(stp_class, STP_MESSAGE_AGE_EXPIRY);
	}
}

/* 8.7.5 */
void forwarding_delay_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	switch (stp_port_class->state)
	{
		case LISTENING:
			stp_port_class->state = LEARNING;
			stptimer_start(&stp_port_class->forward_delay_timer, 0);
			break;

		case LEARNING:
			stp_port_class->state = FORWARDING;
			(stp_port_class->forward_transitions)++;

			// request for sending tcn when root port goes forwarded.
			if ((port_number == stp_class->bridge_info.root_port) ||
				(designated_for_some_port(stp_class)))
			{
				if (stp_port_class->change_detection_enabled &&
					!STP_IS_FASTSPAN_ENABLED(port_number))
				{
					topology_change_detection(stp_class);
					stplog_topo_change(stp_class, port_number, STP_FWD_DLY_EXPIRY);
				}
			}

			break;

		default:
			// print error
			return;
	}

    SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_PORT_STATE_BIT);
	stputil_set_port_state(stp_class, stp_port_class);
	stplog_port_state_change(stp_class, port_number, STP_FWD_DLY_EXPIRY);
	if (STP_DEBUG_EVENT(stp_class->vlan_id, port_number))
	{
		STP_LOG_DEBUG("%s Vlan:%d Port:%d", l2_port_state_to_string(stp_port_class->state, port_number), stp_class->vlan_id, port_number);
	}
}

/* 8.7.6 */
void tcn_timer_expiry(STP_CLASS *stp_class)
{
	transmit_tcn(stp_class);
	stptimer_start(&stp_class->tcn_timer, 0);
}

/* 8.7.7 */
void topology_change_timer_expiry(STP_CLASS * stp_class)
{
	stp_class->bridge_info.topology_change_detected = false;
	stp_class->bridge_info.topology_change = false;
}

/* 8.7.8 */
void hold_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	if (stp_port_class->config_pending)
	{
		transmit_config(stp_class, port_number);
	}
}

bool root_bridge(STP_CLASS *stp_class)
{
	BRIDGE_IDENTIFIER *bridge_id = &stp_class->bridge_info.bridge_id;
	BRIDGE_IDENTIFIER *root_id = &stp_class->bridge_info.root_id;

	return (stputil_compare_bridge_id(root_id, bridge_id) == EQUAL_TO);
}

bool designated_port(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	STP_PORT_CLASS *stp_icl_port_class;
	
	if (stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id, 
			&stp_port_class->designated_bridge) != EQUAL_TO)
	{
		return (false);
	}

	if (stputil_compare_port_id(&stp_port_class->designated_port, 
			&stp_port_class->port_id) != EQUAL_TO)
	{
		return (false);
	}
	
	return (true);
}

bool designated_for_some_port(STP_CLASS *stp_class)
{
	PORT_ID port_number;
	STP_PORT_CLASS *stp_port_class;

	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
		if (stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id, 
			&stp_port_class->designated_bridge) == EQUAL_TO)
		{
			return (true);
		}

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}

	return (false);
}

void send_config_bpdu(STP_CLASS* stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	

	if (!g_sstp_enabled)
		stputil_send_pvst_bpdu(stp_class, port_number, CONFIG_BPDU_TYPE);
	else
		stputil_send_bpdu(stp_class, port_number, CONFIG_BPDU_TYPE);
}

void send_tcn_bpdu(STP_CLASS* stp_class, PORT_ID port_number)
{
	
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	

	if (stptimer_is_active(&stp_port_class->root_protect_timer))
		return;
	
	if (!g_sstp_enabled)
		stputil_send_pvst_bpdu(stp_class, port_number, TCN_BPDU_TYPE);
	else
		stputil_send_bpdu(stp_class, port_number, TCN_BPDU_TYPE);
}
