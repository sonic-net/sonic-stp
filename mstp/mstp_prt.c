/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_prt_active_port_action: actions associated with active port roles    */
/*****************************************************************************/
static void mstp_prt_active_port_action(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);
	MSTP_COMMON_PORT *cport;
	MSTP_MSTI_PORT *msti_port = NULL;
	MSTP_INDEX temp_mstp_index;

	if (mstp_bridge == NULL ||
		mstp_port == NULL)
		return;
	
	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
		return;

	switch (cport->prtState)
	{
		case MSTP_PRT_PROPOSED:
			mstp_setSyncTree(mstp_index);
			cport->proposed = false;
			/* ALTERNATE_PROPOSED case */
			if (cport->role == MSTP_ROLE_ALTERNATE) //UCT
				cport->prtState = MSTP_PRT_BLOCKED_PORT;
			break;

		case MSTP_PRT_PROPOSING:
			cport->proposing = true;
			if (cist)
				mstp_port->newInfoCist = true;
			else
				mstp_port->newInfoMsti = true;
			break;

		case MSTP_PRT_AGREES:
			cport->proposed = false;
			cport->agree = true;
			if (cport->role != MSTP_ROLE_ALTERNATE)
				cport->sync = false;
			if (cport->role != MSTP_ROLE_MASTER)
			{
				if (cist)
					mstp_port->newInfoCist = true;
				else
					mstp_port->newInfoMsti = true;
			}
			/* ALTERNATE_AGREED */
			if (cport->role == MSTP_ROLE_ALTERNATE) //UCT
				cport->prtState = MSTP_PRT_BLOCKED_PORT;
			break;

		case MSTP_PRT_SYNCED:
			if (cport->role != MSTP_ROLE_ROOT)
				cport->rrWhile = 0;
			cport->synced = true;
			cport->sync = false;
			break;

		case MSTP_PRT_REROOT:
			mstp_setReRootTree(mstp_index);
			break;

		case MSTP_PRT_FORWARD:
			cport->forward = true;
			cport->fdWhile = 0;
			if (cport->role == MSTP_ROLE_MASTER || cport->role == MSTP_ROLE_DESIGNATED)
				cport->agreed = mstp_port->sendRSTP;
			break;

		case MSTP_PRT_LEARN:
			cport->learn = true;
			cport->fdWhile = mstp_bridge->cist.rootTimes.fwdDelay;
			break;

		case MSTP_PRT_DISCARD:
			cport->learn = false;
			cport->forward = false;
			cport->fdWhile = mstp_bridge->cist.rootTimes.fwdDelay;
			if(cport->role == MSTP_ROLE_ROOT)
			{
				if(cport->disputed)
					cport->rbWhile = 3 * mstp_bridge->cist.rootTimes.helloTime;
			}
			cport->disputed = false;
			break;

		case MSTP_PRT_REROOTED:
			cport->reRoot = false;
			break;

		case MSTP_PRT_ROOT:
			cport->rrWhile = mstp_bridge->cist.rootTimes.fwdDelay;
			/* Due to case 1b in mstp_rcvInfoCist() where we accept inferior info as superior info
			   for a faster replacement of stale info and thereby faster convergence, a port where we get 
			   PIM_SUPERIOR_DESIGNATED state need not become Root port, some other port becomes Root port
			   for which agree might have been already set. Due to this agreement is sent quickly to the
			   peer port which sets agreed flag for CIST (and also for MSTIs if its a boundary port) making
			   them move to rapid forwarding causing transient loops. Hence reset agree flag here for CIST
			   and MSTIs if its a boundary port such that states such as ROOT_PROPOSED/MASTER_PROPOSED are
			   entered and hence SYNC mechanism is carried out correctly for both CIST and MSTIs */
			if (cist && 
				!mstp_port->rcvdInternal &&
				!l2_proto_mask_is_clear(&mstp_port->instance_mask))
			{
				cport->agree = false;
				for (temp_mstp_index = MSTP_INDEX_MIN; temp_mstp_index <= MSTP_INDEX_MAX; temp_mstp_index++)
				{
					msti_port = MSTP_GET_MSTI_PORT(mstp_port, temp_mstp_index);

					if (msti_port == NULL)
					  continue;

					msti_port->co.agree = cport->agree;
				}
			}
			break;
	}
	
	if (cport->prtState == MSTP_PRT_BLOCKED_PORT)
	{
			cport->fdWhile = (cport->role != MSTP_ROLE_DISABLED?mstp_bridge->cist.rootTimes.fwdDelay:mstp_bridge->cist.rootTimes.maxAge);
			cport->synced = true;
			cport->rrWhile = 0;
			cport->sync =
			cport->reRoot = false;
	}
}

/*****************************************************************************/
/* mstp_prt_action: actions associated with specific states in prt           */
/*****************************************************************************/
static void mstp_prt_action(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);
	MSTP_COMMON_PORT *cport;
	MSTP_COMMON_BRIDGE *cbridge;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}

	switch (cport->prtState)
	{
		case MSTP_PRT_INIT_PORT:
			cport->selectedRole = MSTP_ROLE_DISABLED;
			cport->synced = false;
			cport->sync = true;
			cport->reRoot = true;
			cport->rrWhile = mstp_bridge->cist.rootTimes.fwdDelay;
			cport->fdWhile = mstp_bridge->cist.rootTimes.maxAge;
			cport->rbWhile = 0;

			cport->prtState = MSTP_PRT_BLOCK_PORT;
			
		// Fall through - UCT
		case MSTP_PRT_BLOCK_PORT:
			if(cport->role != cport->selectedRole)
			{
				cport->role = cport->selectedRole;
				SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_PORT_ROLE_BIT);
			}
			cport->learn =
			cport->forward = false;
			break;

		case MSTP_PRT_BACKUP_PORT:
			cport->rbWhile = mstp_bridge->cist.rootTimes.helloTime << 1;
			// fall thru

		case MSTP_PRT_BLOCKED_PORT:
			cport->fdWhile = (cport->role != MSTP_ROLE_DISABLED?mstp_bridge->cist.rootTimes.fwdDelay:mstp_bridge->cist.rootTimes.maxAge);
			cport->synced = true;
			cport->rrWhile = 0;
			cport->sync = false;
			cport->reRoot = false;
			break;

		case MSTP_PRT_PROPOSED:
		case MSTP_PRT_PROPOSING:
		case MSTP_PRT_AGREES:
		case MSTP_PRT_SYNCED:
		case MSTP_PRT_REROOT:
		case MSTP_PRT_FORWARD:
		case MSTP_PRT_LEARN:
		case MSTP_PRT_DISCARD:
		case MSTP_PRT_REROOTED:
		case MSTP_PRT_ROOT:
			mstp_prt_active_port_action(mstp_index, port_number);
			if (cport->role != MSTP_ROLE_ALTERNATE)
				cport->prtState = MSTP_PRT_ACTIVE_PORT;
			// fall thru

		case MSTP_PRT_ACTIVE_PORT:
			if (cist && cport->selectedRole == MSTP_ROLE_DESIGNATED)
			{
				cport->proposing |= (!mstplib_port_is_admin_edge_port(port_number) && mstp_port->operPt2PtMac);
			}
			if(cport->role != cport->selectedRole)
			{
				cport->role = cport->selectedRole;
				SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_PORT_ROLE_BIT);
			}
			break;

		default:
			break;
	}
}

/*****************************************************************************/
/* mstp_prt: executes the port role transitions state machine                */
/*****************************************************************************/
static bool mstp_prt(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool flag = false;
	MSTP_COMMON_PORT *cport;
	MSTP_PRT_STATE old_state;
	
	if (mstp_bridge == NULL ||
		mstp_port == NULL)
		return false;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return false;
	}

	// all transitions except UCT need to be qualified by (selected && !updtInfo)
	if (cport->selected && !cport->updtInfo)
	{
		old_state = cport->prtState;
		switch (cport->prtState)
		{
			case MSTP_PRT_BLOCK_PORT:
				if (!cport->learning &&
					!cport->forwarding)
				{
					flag = true;
					cport->prtState = MSTP_PRT_BLOCKED_PORT;
				}
				break;

			case MSTP_PRT_BLOCKED_PORT:
				if ((cport->role == MSTP_ROLE_BACKUP) &&
					(cport->rbWhile != (mstp_port->cist.portTimes.helloTime << 1)))
				{
					flag = true;
					cport->prtState = MSTP_PRT_BACKUP_PORT;
				}
				else
				if (((cport->role == MSTP_ROLE_DISABLED) && (cport->fdWhile != mstp_bridge->cist.rootTimes.maxAge)) ||
					((cport->role == MSTP_ROLE_ALTERNATE) && (cport->fdWhile != mstp_bridge->cist.rootTimes.fwdDelay)) ||
					cport->sync ||
					cport->reRoot ||
					!cport->synced)
				{
					flag = true;
				}
				else /* 	See ALTERNATE_PROPOSED and ALTERNATE_AGREED in 802.1Q-2005 Fig. 13-19 Page 186 */
				if(cport->role == MSTP_ROLE_ALTERNATE)
				{
					if (cport->proposed && 
						!cport->agree)
					{
						flag = true;
						cport->prtState = MSTP_PRT_PROPOSED;
					}
					else
					{
						bool allSynced = mstp_allSynced(mstp_index, port_number);
						if ((allSynced && !cport->agree) || 
							(cport->proposed && cport->agree))
						{
							flag = true;
							cport->prtState = MSTP_PRT_AGREES;
						}
					}
				}
				break;

			case MSTP_PRT_ACTIVE_PORT:
				if (cport->proposed &&
					!cport->agree)
				{
					flag = true;
					cport->prtState = MSTP_PRT_PROPOSED;
				}
				else
				if ((cport->role == MSTP_ROLE_DESIGNATED) &&
					!cport->forward &&
					!cport->agreed &&
					!cport->proposing &&
					!mstp_port->operEdge)
				{
					flag = true;
					cport->prtState = MSTP_PRT_PROPOSING;
				}
				else
				if (((cport->role == MSTP_ROLE_DESIGNATED || cport->role == MSTP_ROLE_MASTER) && 
					((!cport->learning && !cport->forwarding && !cport->synced) || (mstp_port->operEdge && !cport->synced))) ||
					(cport->agreed && !cport->synced) ||
					(cport->sync && cport->synced))
				{
					flag = true;
					cport->prtState = MSTP_PRT_SYNCED;
				}
				else
				if ((cport->role == MSTP_ROLE_ROOT) &&
					(cport->rrWhile != mstp_bridge->cist.rootTimes.fwdDelay))
				{
					flag = true;
					cport->prtState = MSTP_PRT_ROOT;
				}
				else
				if ((cport->role == MSTP_ROLE_ROOT) &&
					!cport->forward &&
					(cport->rbWhile == 0) &&
					!cport->reRoot)
				{
					flag = true;
					cport->prtState = MSTP_PRT_REROOT;
				}
				else
				if (cport->reRoot &&
					(((cport->role == MSTP_ROLE_ROOT) && cport->forward) ||
					 ((cport->role == MSTP_ROLE_DESIGNATED || cport->role == MSTP_ROLE_MASTER) && cport->rrWhile == 0)))
				{
					flag = true;
					cport->prtState = MSTP_PRT_REROOTED;
				}
				else
				if ((cport->role != MSTP_ROLE_ROOT) &&
					(cport->learn || cport->forward) &&
					(!mstp_port->operEdge) &&
					((cport->sync && !cport->synced) ||
					 (cport->reRoot && (cport->rrWhile != 0)) || cport->disputed))
				{
					flag = true;
					cport->prtState = MSTP_PRT_DISCARD;
				}
				else if ((cport->role == MSTP_ROLE_ROOT) &&
					cport->disputed)
				{
				    flag = true;
				    cport->prtState = MSTP_PRT_DISCARD;
				}
				else
				{
					bool allSynced = mstp_allSynced(mstp_index, port_number);
					bool reRooted = ((cport->role == MSTP_ROLE_ROOT) ? 
										mstp_reRooted(mstp_index, port_number) : 
										false);
					
					if (((cport->role == MSTP_ROLE_DESIGNATED) && (cport->proposed || !cport->agree) && (allSynced)) ||
					 ((cport->role != MSTP_ROLE_DESIGNATED) && ((allSynced && !cport->agree) || (cport->proposed && cport->agree))))
					{
						flag = true;
						cport->prtState = MSTP_PRT_AGREES;
					}
					else
					if ((cport->role == MSTP_ROLE_MASTER && !cport->learn && ((cport->fdWhile == 0) || allSynced)) ||
						((cport->role == MSTP_ROLE_ROOT) && !cport->learn && 
						((cport->fdWhile == 0) ||(reRooted && (cport->rbWhile == 0)) && mstp_bridge->forceVersion >= RSTP_VERSION_ID)) || 
						((cport->role == MSTP_ROLE_DESIGNATED) && !cport->learn && 
						(cport->fdWhile == 0 || cport->agreed || mstp_port->operEdge) && (cport->rrWhile == 0 || !cport->reRoot) && !cport->sync))
					{
						flag = true;
						cport->prtState = MSTP_PRT_LEARN;
					}
					else
					if (cport->learn &&
						!cport->forward &&
						(((cport->role == MSTP_ROLE_ROOT) && 
						 (cport->fdWhile == 0 || (reRooted && cport->rbWhile == 0 && mstp_bridge->forceVersion >= RSTP_VERSION_ID))) ||
						((cport->role == MSTP_ROLE_DESIGNATED) && 
						  (cport->fdWhile == 0 || cport->agreed || mstp_port->operEdge) && (cport->rrWhile == 0 || !cport->reRoot) && !cport->sync) ||
						((cport->role == MSTP_ROLE_MASTER) && (cport->fdWhile == 0 || allSynced)))) 
					{
						flag = true;
						cport->prtState = MSTP_PRT_FORWARD;
					}
				}
				break;

			default:
				// print error
				break;
		}

		if (flag)
		{
			if(old_state != cport->prtState)
			{
				STP_LOG_INFO("[MST %d] Port %d PRT %s->%s",mstputil_get_mstid(mstp_index), port_number,
                    MSTP_PRT_STATE_STRING(old_state), MSTP_PRT_STATE_STRING(cport->prtState));
            }
            mstp_prt_action(mstp_index, port_number);
		}
	}

	return flag;
}

/*****************************************************************************/
/* mstp_prt_init: initializes the port role transitions state machine        */
/*****************************************************************************/
void mstp_prt_init(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTP_COMMON_BRIDGE *cbridge;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}

	cport->prtState = MSTP_PRT_INIT_PORT;
	mstp_prt_action(mstp_index, port_number);
}

/*****************************************************************************/
/* mstp_prt_signal2: signals from prt to other state machines                 */
/*****************************************************************************/
static void mstp_prt_signal2(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(calling_port);
	MSTP_COMMON_PORT *cport;
	PORT_ID port_number;

	if (mstp_port == NULL)
		return;
	
	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}
	if (cport->proposing || !cport->synced)
	{
		mstp_pim_gate(mstp_index, calling_port, NULL);
	}

	/* learn, forward - PST
	 * only when learning and forwarding differ from the modified values
	 */
	if ((cport->learn != cport->learning) ||
		(cport->forward != cport->forwarding))
	{
		mstp_pst_gate(mstp_index, calling_port);
	}

	/* role - TCM
	 * all roles have applicable transitions
	 */
	mstp_tcm_gate(mstp_index, calling_port);

	/* role, synced, proposing, agree, newInfoXst - PTX
	 * role, proposing, agree are used to fill the bpdu flags.
	 */
	/*
	 * call port transmit state machine in case new info needs to be sent
	 * in order to ensure that transmission of bpdus does not happen multiple
	 * times, call the ptx gate only when trigger events happen (bpdu is
	 * received, configuration events, and timer).
	 */
	if (mstp_port->newInfoCist ||
		mstp_port->newInfoMsti)
	{
		mstp_ptx_gate(calling_port);
	}
}

/*****************************************************************************/
/* mstp_prt_gate2: invokes the port role transitions state machine            */
/*****************************************************************************/
void mstp_prt_gate2(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	bool flag = false, allSyncd;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstp_port);
	UINT mstp_prt_gate2_counter = 0;
	MSTP_COMMON_BRIDGE *cbridge;

	if (cport == NULL)
	{
		return;
	}
	// all transitions except UCT are qualified by selected && !updtInfo
	if (cport->selected && !cport->updtInfo && cport->selectedRole != cport->role)
	{
		if (cport->selectedRole == MSTP_ROLE_DISABLED ||
			cport->selectedRole == MSTP_ROLE_ALTERNATE ||
			cport->selectedRole == MSTP_ROLE_BACKUP)
		{
			cport->prtState = MSTP_PRT_BLOCK_PORT;
			mstp_prt_action(mstp_index, port_number);
		}
		else
		if (cport->selectedRole == MSTP_ROLE_ROOT ||
			cport->selectedRole == MSTP_ROLE_DESIGNATED ||
			cport->selectedRole == MSTP_ROLE_MASTER)
		{
			cport->prtState = MSTP_PRT_ACTIVE_PORT;
			mstp_prt_action(mstp_index, port_number);
		}

		flag = true;

		/* fall through and check for other transitions */
	}

	while (mstp_prt(mstp_index, port_number))
	{
		flag = true;
	}	
	/* only check for global transitions if PRT had to execute */
	if (flag)
	{
		mstp_prt_signal2(mstp_index, port_number);
	}
}

/*****************************************************************************/
/* mstp_prt_signal: signals from prt to other state machines                 */
/*****************************************************************************/
static void mstp_prt_signal(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(calling_port);
	MSTP_COMMON_PORT *cport;
	PORT_ID port_number;

	/* reRoot, sync - PRT
	 */
	if (mstp_bridge == NULL ||
		mstp_port == NULL)
		return;

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		if (port_number != calling_port)
		{
			cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));

			if (cport && (cport->state == DISABLED))
				goto mstp_prt_signal_next_port;
			
			if (cport &&
				(cport->reRoot ||
				cport->sync ||
				((!cport->reRoot && !cport->sync) && 
				(cport->role == MSTP_ROLE_ROOT || cport->role == MSTP_ROLE_DESIGNATED))))
			{
				mstp_prt_gate2(mstp_index, port_number);
			}
		}
mstp_prt_signal_next_port:
		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
		return;	

	if (cport->proposing ||	!cport->synced)
	{
		mstp_pim_gate(mstp_index, calling_port, NULL);
	}

	/* learn, forward - PST
	 * only when learning and forwarding differ from the modified values
	 */
	if ((cport->learn != cport->learning) ||
		(cport->forward != cport->forwarding))
	{
		mstp_pst_gate(mstp_index, calling_port);
	}

	/* role - TCM
	 * all roles have applicable transitions
	 */
	mstp_tcm_gate(mstp_index, calling_port);

	/* role, synced, proposing, agree, newInfoXst - PTX
	 * role, proposing, agree are used to fill the bpdu flags.
	 */
	/*
	 * call port transmit state machine in case new info needs to be sent
	 * in order to ensure that transmission of bpdus does not happen multiple
	 * times, call the ptx gate only when trigger events happen (bpdu is
	 * received, configuration events, and timer).
	 */

	if (mstp_port->newInfoCist ||
		mstp_port->newInfoMsti)
	{
		mstp_ptx_gate(calling_port);
	}
}

/*****************************************************************************/
/* mstp_prt_gate: invokes the port role transitions state machine            */
/*****************************************************************************/
void mstp_prt_gate(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	bool flag = false, allSyncd;	
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstp_port);
	UINT mstp_prt_gate_counter = 0;
	MSTP_COMMON_BRIDGE *cbridge;

	if (mstp_bridge == NULL || 
		cport == NULL)
		return;

	// all transitions except UCT are qualified by selected && !updtInfo
	if (cport->selected && !cport->updtInfo && cport->selectedRole != cport->role)
	{
		if (cport->selectedRole == MSTP_ROLE_DISABLED ||
			cport->selectedRole == MSTP_ROLE_ALTERNATE ||
			cport->selectedRole == MSTP_ROLE_BACKUP)
		{
			cport->prtState = MSTP_PRT_BLOCK_PORT;
			mstp_prt_action(mstp_index, port_number);
		}
		else
		if (cport->selectedRole == MSTP_ROLE_ROOT ||
			cport->selectedRole == MSTP_ROLE_DESIGNATED ||
			cport->selectedRole == MSTP_ROLE_MASTER)
		{
			if (cport->selectedRole == MSTP_ROLE_ROOT)
			{
				cport->rrWhile = mstp_bridge->cist.rootTimes.fwdDelay;				
			}
			cport->prtState = MSTP_PRT_ACTIVE_PORT;
			mstp_prt_action(mstp_index, port_number);
		}

		flag = true;

		/* fall through and check for other transitions */
	}

	while (mstp_prt(mstp_index, port_number))
	{
		flag = true;
	}

	/* only check for global transitions if PRT had to execute */
	if (flag)
	{
		mstp_prt_signal(mstp_index, port_number);
	}
}
