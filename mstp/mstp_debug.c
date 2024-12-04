/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

#define MSTP_DUMP          STP_DUMP

/*****************************************************************************/
/* global variables                                                          */
/*****************************************************************************/
extern MSTP_VLAN_PORT_DB   g_mstp_vlan_port_db[MAX_VLAN_ID];
extern MSTP_PORT_VLAN_DB  **g_mstp_port_vlan_db;

UINT8 mstp_err_string[] = "BROKEN";

UINT8 mstp_pim_state_string[][20] =
{
	"UNKNOWN",
	"DISABLED",
	"AGED",
	"UPDATE",
	"SUPERIOR_DESIGNATED",
	"REPEATED_DESIGNATED",
	"INFERIOR_DESIGNATED",
	"ROOT",
	"OTHER",
	"CURRENT",
	"RECEIVE"
};

UINT8 mstp_prs_state_string[][20] =
{
	"UNKNOWN",
	"INIT_BRIDGE",
	"ROLE_SELECTION"
};

UINT8 mstp_prt_state_string[][20] =
{
	"UNKNOWN",
	"INIT_PORT",
	"BLOCK_PORT",
	"BACKUP_PORT",
	"BLOCKED_PORT",
	"PROPOSED",
	"PROPOSING",
	"AGREES",
	"SYNCED",
	"REROOT",
	"FORWARD",
	"LEARN",
	"DISCARD",
	"REROOTED",
	"ROOT",
	"ACTIVE_PORT"
};

UINT8 mstp_pst_state_string[][20] =
{
	"UNKNOWN",
	"DISCARDING",
	"LEARNING",
	"FORWARDING",
};

UINT8 mstp_tcm_state_string[][20] =
{
	"UNKNOWN",
	"INACTIVE",
	"LEARNING",
	"DETECTED",
	"NOTIFIED_TCN",
	"NOTIFIED_TC",
	"PROPAGATING",
	"ACKNOWLEDGED",
	"ACTIVE"
};

UINT8 mstp_ppm_state_string[][20] =
{
	"UNKNOWN",
	"CHECKING_RSTP",
	"SELECTING_STP",
	"SENSING"
};

UINT8 mstp_ptx_state_string[][20] =
{
	"UNKNOWN",
	"TRANSMIT_INIT",
	"IDLE",
	"TRANSMIT_PERIODIC",
	"TRANSMIT_CONFIG",
	"TRANSMIT_TCN",
	"TRANSMIT_RSTP"
};

UINT8 mstp_prx_state_string[][20] =
{
	"UNKNOWN",
	"DISCARD",
	"RECEIVE"
};

UINT8 mstp_rcvdinfo_string[][20] =
{
	"UNKNOWN",
	"SUPERIOR_DESIGNATED",
	"REPEATED_DESIGNATED",
	"ROOT",
	"OTHER"
};

UINT8 mstp_role_string[][20] =
{
	"MASTER",
	"ALTERNATE",
	"ROOT",
	"DESIGNATED",
	"BACKUP",
	"DISABLED"
};

UINT8 mstp_infoIs_string[][20] =
{
	"UNKNOWN",
	"RECEIVED",
	"MINE",
	"AGED",
	"DISABLED"
};

/*****************************************************************************/
/* mstpdebug_display_bpdu: prints out the given mstp bpdu in verbose or      */
/* terse format based on input parameters.                                   */
/*****************************************************************************/
void mstpdebug_display_bpdu(MSTP_BPDU *bpdu, bool rx)
{
	UINT8	root_id_string[20];
	UINT8	reg_root_id_string[20];
	UINT8	flag_string[100], *role_string;
    UINT8 	config_id_string[300];
    UINT16  message_age;
    UINT16  max_age;
    UINT16  hello_time;
    UINT16  forward_delay;
    char 	buffer[36];

    if(rx)
    {
        message_age = bpdu->message_age;
        max_age = bpdu->max_age;
        hello_time = bpdu->hello_time;
        forward_delay = bpdu->forward_delay;
    }
    else
    {
        message_age = bpdu->message_age >> 8;
        max_age = bpdu->max_age >> 8;
        hello_time = bpdu->hello_time >> 8;
        forward_delay = bpdu->forward_delay >> 8;
    }

	if (bpdu->type == TCN_BPDU_TYPE)
	{
		if (debugGlobal.mstp.verbose)
		{
            STP_PKTLOG("protocol-id: %d protocol-version: %d type: %d",
                    bpdu->protocol_id,bpdu->protocol_version_id,bpdu->type);
		}
		else
		{
            STP_PKTLOG("%d %d %d",
                    bpdu->protocol_id,bpdu->protocol_version_id,bpdu->type);
		}
    }
	else
	{
		memset(flag_string, 0, sizeof(flag_string));
        memset(config_id_string, 0, sizeof(config_id_string));

        snprintf(buffer, 36, "0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                bpdu->mst_config_id.config_digest[0],
                bpdu->mst_config_id.config_digest[1],
                bpdu->mst_config_id.config_digest[2],
                bpdu->mst_config_id.config_digest[3],
                bpdu->mst_config_id.config_digest[4],
                bpdu->mst_config_id.config_digest[5],
                bpdu->mst_config_id.config_digest[6],
                bpdu->mst_config_id.config_digest[7],
                bpdu->mst_config_id.config_digest[8],
                bpdu->mst_config_id.config_digest[9],
                bpdu->mst_config_id.config_digest[10],
                bpdu->mst_config_id.config_digest[11],
                bpdu->mst_config_id.config_digest[12],
                bpdu->mst_config_id.config_digest[13],
                bpdu->mst_config_id.config_digest[14],
                bpdu->mst_config_id.config_digest[15]);
        
        snprintf(config_id_string,sizeof(config_id_string),"%d:%s:%d:%s",
                bpdu->mst_config_id.format_selector,bpdu->mst_config_id.name,
                bpdu->mst_config_id.revision_number,buffer);

		mstputil_bridge_to_string(&bpdu->cist_root, root_id_string, sizeof(root_id_string));
		mstputil_bridge_to_string(&bpdu->cist_regional_root, reg_root_id_string, sizeof(reg_root_id_string));

    
		if (debugGlobal.mstp.verbose)
        {
            switch (bpdu->cist_flags.role)
            {
                case MSTP_ROLE_DESIGNATED: role_string = " designated"; break;
                case MSTP_ROLE_ROOT: role_string = " root"; break;
                case MSTP_ROLE_ALTERNATE: role_string = " alternate"; break;
                default: role_string = ""; break;
            }

            snprintf
                (
                 flag_string,
                 sizeof(flag_string),
                 "%s%s%s%s%s%s%s",
                 bpdu->cist_flags.topology_change_acknowledgement ? " tcAck" : "",
                 bpdu->cist_flags.agreement ? " agree" : "",
                 bpdu->cist_flags.forwarding ? " forward" : "",
                 bpdu->cist_flags.learning ? " learn" : "",
                 role_string,
                 bpdu->cist_flags.proposal ? " propose" : "",
                 bpdu->cist_flags.topology_change ? " tc" : ""
                );
            STP_PKTLOG("Region(%s) protocol-id: %d protocol-version: %d type: %d flags: %s " 
                    "root-id: %s path-cost: %d regional-root-id: %s port-id: %u message-age: %d max-age: %d hello-time: %d forward-delay: %d",
                    config_id_string,bpdu->protocol_id,bpdu->protocol_version_id,bpdu->type,flag_string,root_id_string,
                    bpdu->cist_ext_path_cost,reg_root_id_string,*((UINT16 *) &(bpdu->cist_port)),message_age,max_age,
                    hello_time,forward_delay);
        }
		else
		{
			snprintf(flag_string, sizeof(flag_string), "%02x", *((UINT8*)&(bpdu->cist_flags)));

            STP_PKTLOG("Region(%s)%d %d %d %d %s %d %s %d %d %d %d %d",
                    config_id_string,bpdu->protocol_id,bpdu->protocol_version_id,bpdu->type,flag_string,root_id_string,
                    bpdu->cist_ext_path_cost,reg_root_id_string,*((UINT16 *) &(bpdu->cist_port)),message_age,max_age,
                    hello_time,forward_delay);
		}
	}
}

/*****************************************************************************/
/* mstpdebug_display_msti_bpdu: displays the msti config message in verbose  */
/* or terse format based on input                                            */
/*****************************************************************************/
void mstpdebug_display_msti_bpdu(MSTI_CONFIG_MESSAGE *bpdu)
{
	UINT8 root_string[20], flag_string[100], *print_format;
	UINT8 *role_string;

	mstputil_bridge_to_string(&bpdu->msti_regional_root, root_string, sizeof(root_string));

	if (debugGlobal.mstp.verbose)
	{
		role_string = " unknown";
		switch (bpdu->msti_flags.role)
		{
			case MSTP_ROLE_MASTER: role_string = " master"; break;
			case MSTP_ROLE_ALTERNATE: role_string = " alternate"; break;
			case MSTP_ROLE_ROOT: role_string = " root"; break;
			case MSTP_ROLE_DESIGNATED: role_string = " designated"; break;
		}

        snprintf
		(
			flag_string,
			sizeof(flag_string),
			"%s%s%s%s%s%s%s",
			bpdu->msti_flags.master ? " master" : "",
			bpdu->msti_flags.agreement ? " agree" : "",
			bpdu->msti_flags.forwarding ? " forward" : "",
			bpdu->msti_flags.learning ? " learn" : "",
			role_string,
            bpdu->msti_flags.proposal ? " propose" : "",
            bpdu->msti_flags.topology_change ? " tc" : ""
        );
        STP_PKTLOG("flags: %s regional_root: %s int_path_cost: %d bridge_priority: %d port_priority: %d remaining_hops: %d",
                flag_string,root_string,bpdu->msti_int_path_cost,MSTP_DECODE_PRIORITY(bpdu->msti_bridge_priority),
                MSTP_DECODE_PORT_PRIORITY(bpdu->msti_port_priority),bpdu->msti_remaining_hops);

	}
	else
	{
		snprintf(flag_string, sizeof(flag_string), "%d", *((UINT8*)&(bpdu->msti_flags)));
		STP_PKTLOG("%s %s %d %d %d %d",
                flag_string,root_string,bpdu->msti_int_path_cost,MSTP_DECODE_PRIORITY(bpdu->msti_bridge_priority),
                MSTP_DECODE_PORT_PRIORITY(bpdu->msti_port_priority),bpdu->msti_remaining_hops);
	}

}

/*****************************************************************************/
/* mstpdebug_print_config_digest: utility routine to output config digest    */
/*****************************************************************************/
void mstpdebug_print_config_digest(UINT8 *config_digest)
{
	UINT8 i;
	MSTP_DUMP("0x");
	for (i = 0; i < 16; i++)
	{
		MSTP_DUMP("%-2x", config_digest[i]);
	}
	MSTP_DUMP("\n");
}

/*****************************************************************************/
/* mstpdebug_print_common_bridge: utility routine to output common bridge    */
/*****************************************************************************/
static void mstpdebug_print_common_bridge(MSTP_COMMON_BRIDGE *cbridge)
{
	UINT8 buffer[500];
	UINT8 s1[500], s2[500], s3[500];

	if (!vlanmask_is_clear(&cbridge->vlanmask))
	{
		vlanmask_to_string((BITMAP_T *)&cbridge->vlanmask, buffer, sizeof(buffer));
		STP_DUMP("vlanmask %s\n", buffer);
	}

	if (!is_mask_clear(cbridge->portmask))
	{
		mask_to_string(cbridge->portmask, buffer, sizeof(buffer));
		STP_DUMP("portmask %s\n", buffer);
	}
	mstputil_bridge_to_string(&cbridge->bridgeIdentifier, buffer, sizeof(buffer));
	STP_DUMP("bridgeIdentifier %s\n", buffer);

	STP_DUMP("active %u, reselect %u, prsState %s, rootPortId ",
		cbridge->active, cbridge->reselect,
		MSTP_PRS_STATE_STRING(cbridge->prsState));
	if (cbridge->rootPortId == MSTP_INVALID_PORT)
		STP_DUMP("None\n");
	else
		STP_DUMP("%d\n", cbridge->rootPortId);

}
/*****************************************************************************/
/* mstpdebug_print_common_port: utility routine to output common port        */
/*****************************************************************************/
static void mstpdebug_print_common_port(MSTP_COMMON_PORT *cport)
{
	STP_DUMP("fdWhile %u, rrWhile %u, rbWhile %u, tcWhile %u, rcvdInfoWhile %u\n\n",
		cport->fdWhile,
		cport->rrWhile,
		cport->rbWhile,
		cport->tcWhile,
		cport->rcvdInfoWhile);

	STP_DUMP("agree %u, agreed %u, changedMaster %u, forward %u, forwarding %u, infoIs %s\n"
			"learn %u, learning %u, proposed %u, proposing %u, rcvdInfo %s\n\n",
			cport->agree,
			cport->agreed,
			cport->changedMaster,
			cport->forward,
			cport->forwarding,
			MSTP_INFOIS_STRING(cport->infoIs),
			cport->learn,
			cport->learning,
			cport->proposed,
			cport->proposing,
			MSTP_RCVDINFO_STRING(cport->rcvdInfo));

	STP_DUMP("rcvdMsg %u, rcvdTc %u, reRoot %u, role %s\n"
			"selected %u, sync %u, synced %u, tcProp %u, updtInfo %u\n"
			"selectedRole %s, portId %u-%d, intPortPathCost %u, auto %u\n"
            "disputed %d \n\n",
			cport->rcvdMsg,
			cport->rcvdTc,
			cport->reRoot,
			MSTP_PORT_ROLE_STRING(cport->role),
			cport->selected,
			cport->sync,
			cport->synced,
			cport->tcProp,
			cport->updtInfo,
			MSTP_PORT_ROLE_STRING(cport->selectedRole),
			cport->portId.priority,
			cport->portId.number,
			cport->intPortPathCost,
			cport->autoConfigPathCost,
            cport->disputed);


	STP_DUMP("PIM %s, PRT %s, PST %s, TCM %s\n\n",
			MSTP_PIM_STATE_STRING(cport->pimState),
			MSTP_PRT_STATE_STRING(cport->prtState),
			MSTP_PST_STATE_STRING(cport->pstState),
			MSTP_TCM_STATE_STRING(cport->tcmState));
}

/*****************************************************************************/
/* mstpdebug_print_cist_vector: utility routine to output the cist priority  */
/* vector                                                                    */
/*****************************************************************************/
static void mstpdebug_print_cist_vector(MSTP_CIST_VECTOR *vec)
{
	UINT8 buffer[500];

	mstputil_bridge_to_string(&vec->root, buffer, sizeof(buffer));
	STP_DUMP("  root %s, ext_cost %u,\n", buffer, vec->extPathCost);

	mstputil_bridge_to_string(&vec->regionalRoot, buffer, sizeof(buffer));
	STP_DUMP("  regional_root %s, int_cost %u,\n", buffer, vec->intPathCost);

	mstputil_bridge_to_string(&vec->designatedId, buffer, sizeof(buffer));
	STP_DUMP("  designated_id %s, port ", buffer);
	mstputil_port_to_string(&vec->designatedPort, buffer, sizeof(buffer));
	STP_DUMP("%s\n", buffer);
}

/*****************************************************************************/
/* mstpdebug_print_cist_times: utility routine to output the cist times      */
/* structure                                                                 */
/*****************************************************************************/
static void mstpdebug_print_cist_times(MSTP_CIST_TIMES *times)
{
	STP_DUMP("maxage %u, fwd %u, hello %u, msgage %u, hops %u\n",
		times->maxAge,
		times->fwdDelay,
		times->helloTime,
		times->messageAge,
		times->remainingHops);
}

/*****************************************************************************/
/* mstpdebug_print_msti_vector: utility function to output the msti priority */
/* vector structure                                                          */
/*****************************************************************************/
static void mstpdebug_print_msti_vector(MSTP_MSTI_VECTOR *vec)
{
	UINT8 buffer[500];

	mstputil_bridge_to_string(&vec->regionalRoot, buffer, sizeof(buffer));
	STP_DUMP("  regionalRoot %s, intPathCost %u\n", buffer, vec->intPathCost);

	mstputil_bridge_to_string(&vec->designatedId, buffer, sizeof(buffer));
	STP_DUMP("  designatedId %s, port ", buffer);
	mstputil_port_to_string(&vec->designatedPort, buffer, sizeof(buffer));
	STP_DUMP("%s\n", buffer);
}

/*****************************************************************************/
/* mstpdebug_print_msti_times: utility routine to output the msti times      */
/* structure                                                                 */
/*****************************************************************************/
static void mstpdebug_print_msti_times(MSTP_MSTI_TIMES *times)
{
	STP_DUMP("remainingHops %u\n", times->remainingHops);
}

/*****************************************************************************/
/* mstpdebug_print_port_state: utility routine to output the port states for */
/* the input mstp_index and port mask                                        */
/*****************************************************************************/
static void mstpdebug_print_port_state(MSTP_INDEX mstp_index, PORT_MASK *mask)
{
	PORT_ID port_number;
	MSTP_PORT *mstp_port;
	MSTP_COMMON_PORT *cport;

	if (is_mask_clear(mask))
		return;

	STP_DUMP("port_state ");
	port_number = port_mask_get_first_port(mask);
	while (port_number != BAD_PORT_ID)
	{
		mstp_port = mstpdata_get_port(port_number);
		cport = mstputil_get_common_port(mstp_index, mstp_port);
		if (cport)
		{
			STP_DUMP("%d-%s ", port_number, MSTP_PST_STATE_STRING(cport->pstState));
		}
		port_number = port_mask_get_next_port(mask, port_number);
    }
}

/*****************************************************************************/
/* mstpdm_mstp_bridge: dumps the mstp bridge structure                       */
/*****************************************************************************/
void mstpdm_mstp_bridge()
{
	MSTP_MSTID mstid;
	MSTP_INDEX mstp_index;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    UINT8 buffer[100], buffer2[100], buffer3[100], buffer4[100], buffer5[100];

    MSTP_GLOBAL *mstp_global = &g_mstp_global;
    VLAN_ID vlan_id;
   
	if(mstp_bridge == NULL)
	{
		STP_DUMP("mstp_bridge INVALID");
		return;
	}

	STP_DUMP("\nMSTP GLOBAL STRUCTURES INFO");
	STP_DUMP("\n===========================\n");

	STP_DUMP("sizeof MSTP_GLOBAL %lu bytes, MSTP_BPDU %lu bytes\n"
			"sizeof MSTP_BRIDGE %lu bytes, MSTP_PORT %lu bytes\n"
			"sizeof MSTP_CIST_BRIDGE %lu bytes, MSTP_CIST_PORT %lu bytes\n"
			"sizeof MSTP_MSTI_BRIDGE %lu bytes, MSTP_MSTI_PORT %lu bytes\n",
			sizeof(MSTP_GLOBAL), sizeof(MSTP_BPDU),
			sizeof(MSTP_BRIDGE), sizeof(MSTP_PORT),
			sizeof(MSTP_CIST_BRIDGE), sizeof(MSTP_CIST_PORT),
			sizeof(MSTP_MSTI_BRIDGE), sizeof(MSTP_MSTI_PORT));

	STP_DUMP("current memory usage (appx) %lu bytes\n\n",
		(sizeof(MSTP_GLOBAL) +
		sizeof(MSTP_BRIDGE) +
		(sizeof(MSTP_PORT) * bmp_count_set_bits(mstp_bridge->control_mask)) +
		(mstplib_get_num_active_instances() * sizeof(MSTP_MSTI_BRIDGE)) +
		(mstplib_get_num_active_instances() * bmp_count_set_bits(mstp_bridge->control_mask) * sizeof(MSTP_MSTI_PORT))));
	

	if (!is_mask_clear(mstp_bridge->control_mask))
	{
		mask_to_string(mstp_bridge->control_mask, buffer, sizeof(buffer));
		STP_DUMP("control_mask %s\n", buffer);
	}
	if (!is_mask_clear(mstp_bridge->enable_mask))
	{
		mask_to_string(mstp_bridge->enable_mask, buffer, sizeof(buffer));
		STP_DUMP("enable_mask %s\n", buffer);
	}
	if (!is_mask_clear(mstp_bridge->admin_disable_mask))
	{
		mask_to_string(mstp_bridge->admin_disable_mask, buffer, sizeof(buffer));
		STP_DUMP("admin_disable_mask %s\n", buffer);
	}
	if (!is_mask_clear(mstp_bridge->admin_pt2pt_mask))
	{
		mask_to_string(mstp_bridge->admin_pt2pt_mask, buffer, sizeof(buffer));
		STP_DUMP("admin_pt2pt_mask %s\n", buffer);
	}
	if (!is_mask_clear(mstp_bridge->admin_edge_mask))
	{
		mask_to_string(mstp_bridge->admin_edge_mask, buffer, sizeof(buffer));
		STP_DUMP("admin_edge_mask %s\n", buffer);
    }
    
    if (!is_mask_clear(g_stp_protect_mask))
    {
        mask_to_string(g_stp_protect_mask, buffer, sizeof(buffer));
        STP_DUMP("g_stp_protect_mask= %s\n", buffer);
    }
    if (!is_mask_clear(g_stp_protect_do_disable_mask))
    {
        mask_to_string(g_stp_protect_do_disable_mask, buffer, sizeof(buffer));
        STP_DUMP("g_stp_protect_do_disable_mask= %s\n", buffer);
    }
    if (!is_mask_clear(g_stp_protect_disabled_mask))
    {
        mask_to_string(g_stp_protect_disabled_mask, buffer, sizeof(buffer));
        STP_DUMP("g_stp_protect_disabled_mask= %s\n", buffer);
    }
    if (!is_mask_clear(g_stp_root_protect_mask))
    {
        mask_to_string(g_stp_root_protect_mask, buffer, sizeof(buffer));
        STP_DUMP("g_stp_root_protect_mask= %s\n", buffer);
    }

    STP_DUMP("INDEX TABLE\n");
	for (mstid = MSTP_MSTID_MIN; mstid <= MSTP_MSTID_MAX; mstid++)
	{
		mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
		if (mstp_index != MSTP_INDEX_INVALID)
		{
			STP_DUMP("%u->%u, ", mstid, mstp_index);
		}
	}
	STP_DUMP("\n\n");
    STP_DUMP("VLAN - MST mapping(Vlan not displayed is part of MST 0)\n");
    for (vlan_id = MIN_VLAN_ID; vlan_id < MAX_VLAN_ID; vlan_id++)
    {
        mstid = MSTP_GET_MSTID(mstp_bridge, vlan_id);
        if(mstid != MSTP_MSTID_CIST)
            STP_DUMP(" %u->%u\n", vlan_id, mstid);
    }
    STP_DUMP("\n\n");

	STP_DUMP("max_instance %u, free_instances %u, active %u\n"
			"disable_auto_edge_port %u, forceVersion %u\n"
			"fwdDelay %u, helloTime %u, maxHops %u, txHoldCount %u\n",
			mstp_global->max_instances,
			mstp_global->free_instances,
			mstp_bridge->active,
			mstp_bridge->disable_auto_edge_port,
			mstp_bridge->forceVersion,
			mstp_bridge->fwdDelay,
			mstp_bridge->helloTime,
			mstp_bridge->maxHops,
			mstp_bridge->txHoldCount);

	STP_DUMP("mstConfigId:\n\tformat_selector %u revision %u name %s\n",
		mstp_bridge->mstConfigId.format_selector,
		mstp_bridge->mstConfigId.revision_number,
		mstp_bridge->mstConfigId.name);

    STP_DUMP ("\tconfig_digest ");
    mstpdebug_print_config_digest(mstp_bridge->mstConfigId.config_digest);
}

/*****************************************************************************/
/* mstpdm_mstp_port: dumps the mstp port structure                           */
/*****************************************************************************/
void mstpdm_mstp_port(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	UINT8 buffer[500];

	if (mstp_port == NULL)
	{
		STP_DUMP("Port %d is not a member of MSTP", port_number);
		return;
	}
	
	STP_DUMP("\nSTP PORT %s (%d)\n", stp_intf_get_port_name(port_number), port_number);

	STP_DUMP("infoInternal %u, mcheck %u, newInfoCist %u, newInfoMsti %u\n"
			"portEnabled %u, operEdge %u, operPt2PtMac %u, rcvdInternal %u\n"
			"rcvdBpdu %u, rcvdRSTP %u, rcvdSTP %u, rcvdTcAck %u, rcvdTcn %u\n"
			"sendRSTP %u, tcAck %u, txCount %u\n"
			"mdelayWhile %u, helloWhen %u restrictedRole %u\n"
            "bpdu_guard_active %d \n\n",
			mstp_port->infoInternal,
			mstp_port->mcheck,
			mstp_port->newInfoCist,
			mstp_port->newInfoMsti,
			mstp_port->portEnabled,
			mstp_port->operEdge,
			mstp_port->operPt2PtMac,
			mstp_port->rcvdInternal,
			mstp_port->rcvdBpdu,
			mstp_port->rcvdRSTP,
			mstp_port->rcvdSTP,
			mstp_port->rcvdTcAck,
			mstp_port->rcvdTcn,
			mstp_port->sendRSTP,
			mstp_port->tcAck,
			mstp_port->txCount,
			mstp_port->mdelayWhile,
			mstp_port->helloWhen,
            mstp_port->restrictedRole,
            mstp_port->bpdu_guard_active);

	STP_DUMP("ppmState %s, prxState %s, ptxState %s\n",
		MSTP_PPM_STATE_STRING(mstp_port->ppmState),
		MSTP_PRX_STATE_STRING(mstp_port->prxState),
        MSTP_PTX_STATE_STRING(mstp_port->ptxState));

	STP_DUMP("rx: mstp %u, rstp %u, config %u, tcn %u\n"
			"tx: mstp %u, rstp %u, config %u, tcn %u\n",
			mstp_port->stats.rx.mstp_bpdu,
			mstp_port->stats.rx.rstp_bpdu,
			mstp_port->stats.rx.config_bpdu,
			mstp_port->stats.rx.tcn_bpdu,
			mstp_port->stats.tx.mstp_bpdu,
			mstp_port->stats.tx.rstp_bpdu,
			mstp_port->stats.tx.config_bpdu,
			mstp_port->stats.tx.tcn_bpdu);

	if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
	{
		l2_proto_mask_to_string(&mstp_port->instance_mask, buffer, sizeof(buffer));
		STP_DUMP("instance_index_mask %s\n", buffer);
	}
    STP_DUMP("\n");
}

/*****************************************************************************/
/* mstpdm_cist: dumps the common spanning tree instance bridge structure     */
/*****************************************************************************/
void mstpdm_cist()
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_CIST_BRIDGE *cist_bridge;
    PORT_ID port_number;
    MSTP_COMMON_BRIDGE *cbridge;

	if(mstp_bridge == NULL)
	{
		STP_DUMP("mstp_bridge INVALID");
		return;
	}

	cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

    if(!cist_bridge)
         return;

	STP_DUMP("\nCIST CLASS (MST ID %d)",cist_bridge->co.mstid);
	STP_DUMP("\n=====================\n");
	
	mstpdebug_print_common_bridge(&cist_bridge->co);

	STP_DUMP("bridgePriority\n"); mstpdebug_print_cist_vector(&(cist_bridge->bridgePriority));
	STP_DUMP("rootPriority\n"); mstpdebug_print_cist_vector(&(cist_bridge->rootPriority));
	STP_DUMP("bridgeTimes "); mstpdebug_print_cist_times(&(cist_bridge->bridgeTimes));
	STP_DUMP("rootTimes "); mstpdebug_print_cist_times(&(cist_bridge->rootTimes));
	STP_DUMP("\n");

    port_number = port_mask_get_first_port(mstp_bridge->control_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstpdm_cist_port(port_number);
        port_number = port_mask_get_next_port(mstp_bridge->control_mask, port_number);
    }

	mstpdebug_print_port_state(MSTP_INDEX_CIST, mstp_bridge->enable_mask);
	STP_DUMP("\n");
}

/*****************************************************************************/
/* mstpdm_cist_port: dumps the common spanning tree instance port structure  */
/*****************************************************************************/
void mstpdm_cist_port(PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge;
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;

	if (port_number > MAX_PORT)
	{
		STP_DUMP("error - invalid port\n");
		return;
	}

	mstp_bridge = mstpdata_get_bridge();

	if(mstp_bridge == NULL)
	{
		STP_DUMP("mstp_bridge INVALID");
		return;
	}

	if (!IS_MEMBER(mstp_bridge->control_mask, port_number))
	{
		STP_DUMP("error - input port is not running mstp\n");
		return;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_DUMP("error - mstp_port null for port %d\n", port_number);
		return;
	}
	
	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	STP_DUMP("\nCIST PORT %s (%d)\n", stp_intf_get_port_name(port_number), port_number);
	mstpdebug_print_common_port(&cist_port->co);

	STP_DUMP("designatedPriority "); mstpdebug_print_cist_vector(&cist_port->designatedPriority);
	STP_DUMP("msgPriority "); mstpdebug_print_cist_vector(&cist_port->msgPriority);
	STP_DUMP("portPriority "); mstpdebug_print_cist_vector(&cist_port->portPriority);

	STP_DUMP("\ndesignatedTimes "); mstpdebug_print_cist_times(&cist_port->designatedTimes);
	STP_DUMP("msgTimes "); mstpdebug_print_cist_times(&cist_port->msgTimes);
	STP_DUMP("portTimes "); mstpdebug_print_cist_times(&cist_port->portTimes);

	STP_DUMP("\n");
}

/*****************************************************************************/
/* mstpdm_msti: dumps the instance bridge structure for the input mstid      */
/*****************************************************************************/
void mstpdm_msti(MSTP_MSTID mstid)
{
	MSTP_INDEX mstp_index;
	MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    PORT_ID port_number;
    PORT_MASK_LOCAL l_mask;
    PORT_MASK *mask = portmask_local_init(&l_mask);

	if (!MSTP_IS_VALID_MSTID(mstid))
	{
		STP_DUMP("error - mstid %u is invalid\n", mstid);
		return;
	}

	mstp_bridge = mstpdata_get_bridge();

	if(mstp_bridge == NULL)
	{
		STP_DUMP("mstp_bridge INVALID");
		return;
	}

	mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
	if (mstp_index == MSTP_INDEX_INVALID)
	{
		STP_DUMP("error - mstid %u has not been configured\n", mstid);
		return;
	}

	msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
	if (msti_bridge == NULL)
	{
		STP_DUMP("error - msti_bridge null for mstid %u (index %u)\n",
			mstid, mstp_index);
		return;
	}
	

    STP_DUMP("\nMSTI CLASS (MST ID %d)",msti_bridge->co.mstid);
    STP_DUMP("\n=====================\n");

	mstpdebug_print_common_bridge(&msti_bridge->co);

	STP_DUMP("bridgePriority\n"); mstpdebug_print_msti_vector(&(msti_bridge->bridgePriority));
	STP_DUMP("rootPriority\n"); mstpdebug_print_msti_vector(&(msti_bridge->rootPriority));
	STP_DUMP("bridgeTimes "); mstpdebug_print_msti_times(&(msti_bridge->bridgeTimes));
	STP_DUMP("rootTimes "); mstpdebug_print_msti_times(&(msti_bridge->rootTimes));

    port_number = port_mask_get_first_port(msti_bridge->co.portmask);
    while (port_number != BAD_PORT_ID)
    {
        mstpdm_msti_port(mstid, port_number);
        port_number = port_mask_get_next_port(msti_bridge->co.portmask, port_number);
    }

    and_masks(mask, msti_bridge->co.portmask, mstp_bridge->enable_mask);
    mstpdebug_print_port_state(mstp_index, mask);
	STP_DUMP("\n");
}

/*****************************************************************************/
/* mstpdm_msti_port: dumps the instance port structure                       */
/*****************************************************************************/
void mstpdm_msti_port(MSTP_MSTID mstid, PORT_ID port_number)
{
	MSTP_INDEX mstp_index;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	
	if (!MSTP_IS_VALID_MSTID(mstid))
	{
		STP_DUMP("error - mstid %u is invalid\n", mstid);
		return;
	}

	mstp_bridge = mstpdata_get_bridge();

	if(mstp_bridge == NULL)
	{
		STP_DUMP("mstp_bridge INVALID ");
		return;
	}
	
	mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);

	if (mstp_index == MSTP_INDEX_INVALID)
	{
		STP_DUMP("error - mstid %u has not been configured\n", mstid);
		return;
	}

	if (!IS_MEMBER(mstp_bridge->control_mask, port_number))
	{
		STP_DUMP("error - input port is not running mstp\n");
		return;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_DUMP("error - mstp_port null for port %d\n", port_number);
		return;
	}

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
	{
		STP_DUMP("error - msti port null for port %d, mstid %u, index %u\n",
			port_number, mstid, mstp_index);
		return;
	}

	STP_DUMP("MSTI PORT CLASS %s (%d)\n", stp_intf_get_port_name(port_number), port_number);
	mstpdebug_print_common_port(&msti_port->co);
	STP_DUMP("mstiMaster %u, mstiMastered %u\n", msti_port->master, msti_port->mastered);

	STP_DUMP("designatedPriority "); mstpdebug_print_msti_vector(&msti_port->designatedPriority);
	STP_DUMP("msgPriority "); mstpdebug_print_msti_vector(&msti_port->msgPriority);
	STP_DUMP("portPriority "); mstpdebug_print_msti_vector(&msti_port->portPriority);

	STP_DUMP("\ndesignatedTimes "); mstpdebug_print_msti_times(&msti_port->designatedTimes);
	STP_DUMP("msgTimes "); mstpdebug_print_msti_times(&msti_port->msgTimes);
	STP_DUMP("portTimes "); mstpdebug_print_msti_times(&msti_port->portTimes);

	STP_DUMP("\n");
}

/*****************************************************************************/
/* mstp_debug_global_enable_port: enable/disable debugs on all ports         */
/*****************************************************************************/
void mstp_debug_global_enable_port(uint32_t port_id, uint8_t flag)
{
    if (flag)
    {
        if (port_id == BAD_PORT_ID)
        {
            debugGlobal.mstp.all_ports = 1;
            bmp_reset_all(debugGlobal.mstp.port_mask);
        }
        else
        {
            debugGlobal.mstp.all_ports = 0;
            bmp_set(debugGlobal.mstp.port_mask, port_id);
        }
    }
    else
    {
        debugGlobal.mstp.all_ports = 0;
        if (port_id == BAD_PORT_ID)
            bmp_reset_all(debugGlobal.mstp.port_mask);
        else
            bmp_reset(debugGlobal.mstp.port_mask, port_id);
    }
}

/*****************************************************************************/
/* mstp_debug_global_enable_mst: enable/disable debugs on msti        		 */
/*****************************************************************************/
void mstp_debug_global_enable_mst(uint16_t mst_id, uint8_t flag)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge= mstpdata_get_bridge();

    if(mstp_bridge == NULL)
    {
        return;
    }

    if (flag)
    {
        if (mst_id)
        {
            mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mst_id);
            if (mstp_index != MSTP_INDEX_INVALID)
            {
                debugGlobal.mstp.all_instance = 0;
                bmp_set(debugGlobal.mstp.instance_mask, mstp_index);
            }
        }
        else
        {
            debugGlobal.mstp.all_instance = 1;
            bmp_reset_all(debugGlobal.mstp.instance_mask);
        }
    }
    else
    {
        debugGlobal.mstp.all_instance = 0;
        if (mst_id)
        { 
            mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mst_id);
            if (mstp_index != MSTP_INDEX_INVALID)
            {
                bmp_reset(debugGlobal.mstp.instance_mask, mstp_index);
            }
        }
        else
            bmp_reset_all(debugGlobal.mstp.instance_mask);
    }
}
/*****************************************************************************/
/* mstp_debug_show: displays the debug settings  	       		 			 */
/*****************************************************************************/
void mstp_debug_show()
{
    char buffer[500];
    bool flag;

    flag = (debugGlobal.mstp.event || debugGlobal.mstp.bpdu_rx || debugGlobal.mstp.bpdu_tx);

    STP_DUMP("MSTP Debug Parameters\n"
            "--------------------\n"
            "MSTP debugging is %s [Mode: %s]\n"
            "%s%s%s%s\n",
            debugGlobal.mstp.enabled ? "ON" : "OFF",
            debugGlobal.mstp.verbose ? "Verbose" : "Brief",
            debugGlobal.mstp.event ? "NonBpduEvents " : "",
            debugGlobal.mstp.bpdu_rx ? "BpduRxEvents " : "",
            debugGlobal.mstp.bpdu_tx ? "BpduTxEvents " : "",
            !flag ? "No events being tracked" : "are being tracked");

    STP_DUMP("Ports: ");
    if (debugGlobal.mstp.all_ports)
    {
        STP_DUMP("All\n");
    }
    else
    {
        mask_to_string(debugGlobal.mstp.port_mask, buffer, sizeof(buffer));
        STP_DUMP("%s\n", buffer);
    }

    STP_DUMP("MST INSTANCEs: ");
    if (debugGlobal.mstp.all_instance)
    {
        STP_DUMP("All\n");
    }
    else
    {
        mask_to_string(debugGlobal.mstp.instance_mask, buffer, sizeof(buffer));
        STP_DUMP("%s\n", buffer);
    }

    STP_DUMP("\n");
}

void mstpdbg_process_ctl_msg(void *msg)
{
    STP_CTL_MSG *pmsg = (STP_CTL_MSG *)msg;
    MSTP_MSTID mstid;
    PORT_ID port_number;
    MSTP_BRIDGE *mstp_bridge;
	UINT8 buffer[500];
    VLAN_ID vlan_id;
    MSTP_PORT *mstp_port;
    MSTP_INDEX mstp_index;

    if (!pmsg)
    {
        STP_LOG_ERR("pmsg null");
        return;
    }

    STP_LOG_INFO("cmd: %d", pmsg->cmd_type);

    switch(pmsg->cmd_type)
    {
        case STP_CTL_DUMP_GLOBAL:
        {
            mstpdm_mstp_bridge();
            break;
        }
        case STP_CTL_DUMP_ALL:
        {
            mstpdm_mstp_bridge();

            mstp_bridge = mstpdata_get_bridge();

            if(mstp_bridge)
            {
                STP_DUMP("\n");
                for(port_number = port_mask_get_first_port(mstp_bridge->control_mask);
                        port_number != BAD_PORT_ID;
                        port_number = port_mask_get_next_port(mstp_bridge->control_mask, port_number))
                {
                    mstpdm_mstp_port(port_number);
                }
            }

            mstid = mstplib_get_first_mstid();
            for(mstid = mstplib_get_first_mstid(); mstid != MSTP_MSTID_INVALID; mstid = mstplib_get_next_mstid(mstid))
            {
               if (MSTP_IS_CIST_MSTID(mstid))
               {
                   mstpdm_cist();
               }
               else if (MSTP_IS_VALID_MSTID(mstid))
               {
                   mstpdm_msti(mstid);
               }
            }
           
            STP_DUMP("Vlan DB \n=======\n");
            for (vlan_id = MIN_VLAN_ID; vlan_id < MAX_VLAN_ID; vlan_id++)
            {
                if(!is_mask_clear(g_mstp_vlan_port_db[vlan_id].port_mask))
                {
                    mask_to_string((BITMAP_T *)g_mstp_vlan_port_db[vlan_id].port_mask, buffer, sizeof(buffer));
                    STP_DUMP("Vlan %d - %s\n", vlan_id, buffer);
                }
            }

            STP_DUMP("Port DB \n=======\n");
            for(port_number = 0; port_number  < g_max_stp_port ; port_number++)
            {
                if(!is_mask_clear(g_mstp_port_vlan_db[port_number]->vlan_mask))
                {
                    vlanmask_to_string((BITMAP_T *)g_mstp_port_vlan_db[port_number]->vlan_mask, buffer, sizeof(buffer));
                    STP_DUMP("%s - Vlan  %s\n", stp_intf_get_port_name(port_number), buffer);
                    if(g_mstp_port_vlan_db[port_number]->untag_vlan != VLAN_ID_INVALID)
                        STP_DUMP("Untag Vlan - %d\n", g_mstp_port_vlan_db[port_number]->untag_vlan);
                }
            }

            /* Dump intf DB */
            STP_DUMP("\nNL_DB:\n");
            stpdbg_dump_nl_db();

            /* Dump Lib event stats */
            STP_DUMP("\nLSTATS:\n");
            stpdbg_dump_stp_stats();
            break;
        }
        case STP_CTL_DUMP_MST:
        {
            if (MSTP_IS_CIST_MSTID(pmsg->mst_id))
            {
                mstpdm_cist();
            }
            else if (MSTP_IS_VALID_MSTID(pmsg->mst_id))
            {
                mstpdm_msti(pmsg->mst_id);
            }
            break;
        }
        case STP_CTL_DUMP_MST_PORT:
        {
            port_number = stp_intf_get_port_id_by_name(pmsg->intf_name);

            if (port_number != BAD_PORT_ID)
            {
                if (MSTP_IS_CIST_MSTID(pmsg->mst_id))
                {
                    mstpdm_cist_port(port_number);
                }
                else if (MSTP_IS_VALID_MSTID(pmsg->mst_id))
                {
                    mstpdm_msti_port(pmsg->mst_id, port_number);
                }
            }
            break;
        }
        case STP_CTL_DUMP_INTF:
        {
            port_number = stp_intf_get_port_id_by_name(pmsg->intf_name);
            if (port_number != BAD_PORT_ID)
            {
                mstpdm_mstp_port(port_number);
            }
            break;
        }
        case STP_CTL_SET_DBG:
        {
            if (pmsg->dbg.flags & STPCTL_DBG_SET_ENABLED)
            {
                debugGlobal.mstp.enabled = pmsg->dbg.enabled;

                if (!debugGlobal.mstp.enabled)
                {
                    /* Reset */
                    debugGlobal.mstp.event = 0;
                    debugGlobal.mstp.bpdu_rx =
                    debugGlobal.mstp.bpdu_tx =
                    debugGlobal.mstp.all_ports =
                    debugGlobal.mstp.all_instance = 1;

                    bmp_reset_all(debugGlobal.mstp.instance_mask);
                    bmp_reset_all(debugGlobal.mstp.port_mask);

                    STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
                }
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SET_VERBOSE)
            {
                debugGlobal.mstp.verbose = pmsg->dbg.verbose;
                if (debugGlobal.mstp.verbose)
                {
                    STP_LOG_SET_LEVEL(STP_LOG_LEVEL_DEBUG);
                }
                else
                {
                    STP_LOG_SET_LEVEL(STP_LOG_LEVEL_INFO);
                }
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_RX ||
                     pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_TX)
            {
                if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_RX)
                {
                    debugGlobal.mstp.bpdu_rx = pmsg->dbg.bpdu_rx;
                }
                if (pmsg->dbg.flags & STPCTL_DBG_SET_BPDU_TX)
                {   
                    debugGlobal.mstp.bpdu_tx = pmsg->dbg.bpdu_tx;
                }
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SET_EVENT)
            {
                debugGlobal.mstp.event = pmsg->dbg.event;
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SET_PORT)
            {
                port_number = stp_intf_get_port_id_by_name(pmsg->intf_name);
                mstp_debug_global_enable_port(port_number, pmsg->dbg.port);
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SET_VLAN)
            {
                mstp_debug_global_enable_mst(pmsg->mst_id, pmsg->dbg.mst);
            }
            else if (pmsg->dbg.flags & STPCTL_DBG_SHOW)
            {
                mstp_debug_show();
            }
            break;
        }
        case STP_CTL_DUMP_NL_DB:
        {
            stpdbg_dump_nl_db();
            break;
        }

        case STP_CTL_DUMP_NL_DB_INTF:
        {
            stpdbg_dump_nl_db_intf(pmsg->intf_name);
            break;
        }

        case STP_CTL_SET_LOG_LVL:
        {
            STP_LOG_SET_LEVEL(pmsg->level);
            MSTP_DUMP("log level set to %d\n", pmsg->level);
            break;
        }

        case STP_CTL_CLEAR_ALL:
        {
            mstpmgr_clear_statistics_all();
            MSTP_DUMP("All stats cleared\n");
            break;
        }

        case STP_CTL_CLEAR_INTF:
        {
            port_number = stp_intf_get_port_id_by_name(pmsg->intf_name);
            if (port_number != BAD_PORT_ID)
            {
                mstpmgr_clear_port_statistics(port_number);
                MSTP_DUMP("Stats cleared for %s\n", pmsg->intf_name);
            }
            break;
        }
    }
}
