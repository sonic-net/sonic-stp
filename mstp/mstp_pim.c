/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_pim_signal: signaling between pim and other state machines           */
/*****************************************************************************/
static void mstp_pim_signal(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();	
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));

	if (mstp_port == NULL || cport == NULL)
		return;

	// rcvdTcn, rcvdTcAck - TCM
	if (mstp_port->rcvdTcAck || 
		mstp_port->rcvdTcn ||
		cport->rcvdTc)
	{
		mstp_tcm_gate(mstp_index, port_number);
	}

	/*
	 * reselect - PRS
	 * selected, portPriority, portTimes, infoIs, infoInternal do not trigger
	 * transitions in the PRS state machine.
	 */
	if (mstputil_computeReselect(mstp_index) ||!cport->selected)
	{
		mstp_prs_gate(mstp_index);
	}

	/*
	 * proposed, synced, agree, agreed - PRT
	 * synced can cause a transition only when sync is set
	 * agreed and !agreed cause transitions only when role is designated
	 */
	if (cport->proposing ||
		cport->proposed ||
		!cport->synced ||
		!cport->agree ||
		cport->disputed ||
		(cport->agreed && ((cport->selectedRole == MSTP_ROLE_DESIGNATED) || (cport->selectedRole == MSTP_ROLE_ROOT))))
	{
		mstp_prt_gate(mstp_index, port_number);
	}
	
	/*
	 * newInfoXst - PTX
	 * designatedPriority, designatedTimes do not trigger transitions in the
	 * PTX state machine. call port transmit state machine in case new info 
	 * needs to be sent in order to ensure that transmission of bpdus does 
	 * not happen multiple times, call the ptx gate only when trigger events 
	 * happen.
	 */	
	if (mstp_port->newInfoCist ||
		mstp_port->newInfoMsti)
	{
		mstp_ptx_gate(port_number);
	}
}

/*****************************************************************************/
/* mstp_pim_set_reselect: sets reselectTree based on instance (if required   */
/* triggers reselect on all mstis on boundary ports)                         */
/*****************************************************************************/
static void mstp_pim_set_reselect(MSTP_INDEX mstp_index, MSTP_PORT *mstp_port)
{
	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		mstp_setReselectTree(mstp_index);
		return;
	}

	// cist
	mstp_setReselectTree(mstp_index);
	if (mstp_port->rcvdInternal)
	{
		// internal port - nothing to do
		return;
	}

	// boundary port - signal to the mstis as well
	mstp_index = l2_proto_get_first_instance(&mstp_port->instance_mask);
	while (mstp_index != L2_PROTO_INDEX_INVALID)
	{
		mstp_setReselectTree(mstp_index);
		mstp_index = l2_proto_get_next_instance(&mstp_port->instance_mask, mstp_index);
	}
}

/*****************************************************************************/
/* mstp_pim_action: pim state machine actions based on state                 */
/*****************************************************************************/
static void mstp_pim_action(MSTP_INDEX mstp_index, PORT_ID port_number, void *bpdu)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTP_CIST_PORT *cist_port = NULL;
	MSTP_MSTI_PORT *msti_port = NULL, *temp_msti_port = NULL;
	MSTP_INDEX temp_mstp_index;
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);
	bool (*betterorsameInfoXst)(MSTP_INDEX, PORT_ID, MSTP_INFOIS);
	void (*recordProposalXst)(MSTP_INDEX, PORT_ID, void *);
	void (*recordAgreementXst)(MSTP_INDEX, PORT_ID, void *);
	void (*updtRcvdInfoWhileXst)(MSTP_INDEX, PORT_ID);
	MSTP_RCVD_INFO (*rcvInfoXst)(MSTP_INDEX, PORT_ID, void *);
	void (*recordMasteredXst)(MSTP_INDEX, PORT_ID, void *);
	void (*recordDisputeXst)(MSTP_INDEX, PORT_ID, void *);

	if (mstp_port == NULL)
		return;
	
	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}

	// set up function pointers
	if (cist)
	{
		cist_port = MSTP_GET_CIST_PORT(mstp_port);
		betterorsameInfoXst = mstp_betterorsameInfoCist;
		recordProposalXst = mstp_recordProposalCist;
		recordAgreementXst = mstp_recordAgreementCist;
		updtRcvdInfoWhileXst = mstp_updtRcvdInfoWhileCist;
		rcvInfoXst = mstp_rcvInfoCist;
		recordMasteredXst = mstp_recordMasteredCist;
        recordDisputeXst =  mstp_recordDisputeCist;
	}
	else
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (!msti_port)
			return;

		betterorsameInfoXst = mstp_betterorsameInfoMsti;
		recordProposalXst = mstp_recordProposalMsti;
		recordAgreementXst = mstp_recordAgreementMsti;
		updtRcvdInfoWhileXst = mstp_updtRcvdInfoWhileMsti;
		rcvInfoXst = mstp_rcvInfoMsti;
		recordMasteredXst = mstp_recordMasteredMsti;
        recordDisputeXst =  mstp_recordDisputeMsti;
	}

	switch (cport->pimState)
	{
		case MSTP_PIM_DISABLED:
			cport->rcvdMsg =
			cport->proposing =
			cport->proposed =
			cport->agree =
			cport->agreed = false;
			cport->rcvdInfoWhile = 0;
			cport->infoIs = MSTP_INFOIS_DISABLED;
			mstp_pim_set_reselect(mstp_index, mstp_port); 
			cport->selected = false; 
			break;

		case MSTP_PIM_AGED:
			cport->infoIs = MSTP_INFOIS_AGED;
			mstp_pim_set_reselect(mstp_index, mstp_port);
			cport->selected = false;
			break;

		case MSTP_PIM_UPDATE:
			cport->proposed =
			cport->proposing = false;
			cport->agreed = cport->agreed &&
				(*betterorsameInfoXst)(mstp_index, port_number, MSTP_INFOIS_MINE);
			cport->synced = cport->synced && cport->agreed;
			/* 
			 * If the agreed and synced flags were already set for MSTIs, 
			 * they should be reset too if these flags gets reset for CIST in case of boundary ports.
			 */
            if(!mstp_port->rcvdInternal)
            {
                if(cist)
                {
                    if(!l2_proto_mask_is_clear(&mstp_port->instance_mask))
                    {
                        for (temp_mstp_index = MSTP_INDEX_MIN; temp_mstp_index <= MSTP_INDEX_MAX; temp_mstp_index++)
                        {
                            temp_msti_port = MSTP_GET_MSTI_PORT(mstp_port, temp_mstp_index);

                            if (temp_msti_port == NULL)
                                continue;

                            temp_msti_port->co.sync = cport->changedMaster;
                            temp_msti_port->co.agreed = cport->agreed;
                            temp_msti_port->co.synced = cport->synced;
                            temp_msti_port->co.proposed = cport->proposed;
                            STP_LOG_INFO("[MST %d] Port %d sync %d syncd %d agreed %d agree %d learn %d fwd %d learning %d fwding %d ",
                                    mstputil_get_mstid(temp_mstp_index), port_number, temp_msti_port->co.sync,
                                    temp_msti_port->co.synced, temp_msti_port->co.agreed, temp_msti_port->co.agree, temp_msti_port->co.learn,
                                    temp_msti_port->co.forward, temp_msti_port->co.learning, temp_msti_port->co.forwarding);
                            temp_msti_port->co.rrWhile = 0;
                            temp_msti_port->co.reRoot = 0;
                            if(temp_msti_port->co.sync && !temp_msti_port->co.synced)
                            {
                                temp_msti_port->co.learning = false;
                                temp_msti_port->co.forwarding = false;
                            }
                        }
                    }
                }
                else
                {
                    STP_LOG_INFO("[MST %d] Port %d sync %d syncd %d agreed %d agree %d learn %d fwd %d learning %d fwding %d ",
                            mstputil_get_mstid(mstp_index), port_number, cport->sync, cport->synced,
                            cport->agreed,cport->agree,cport->learn, cport->forward, cport->learning, cport->forwarding);
                    cist_port = MSTP_GET_CIST_PORT(mstp_port);
                    if(cist_port)
                    {
                        //cport->sync = cport->changedMaster;
                        cport->agreed = cist_port->co.agreed;
                        cport->synced = cist_port->co.synced;
                        cport->proposed = cist_port->co.proposed;
                        cport->rrWhile = 0;
                        cport->reRoot = 0;
                        if(cport->sync && !cport->synced)
                        {
                            cport->learning = false;
                            cport->forwarding = false;
                        }
                    }
                }
            }

            STP_LOG_INFO("[MST %d] Port %d sync %d syncd %d agreed %d agree %d learn %d fwd %d learning %d fwding %d proposed %d ",
                    mstputil_get_mstid(mstp_index), port_number, cport->sync, cport->synced,
                    cport->agreed,cport->agree,cport->learn, cport->forward, cport->learning, cport->forwarding, cport->proposed);
			cport->changedMaster =
			cport->updtInfo = false;
			cport->infoIs = MSTP_INFOIS_MINE;
			if (cist)
			{
				cist_port->portPriority = cist_port->designatedPriority;
				cist_port->portTimes = cist_port->designatedTimes;
				mstp_port->newInfoCist = true;
			}
			else
			{
				msti_port->portPriority = msti_port->designatedPriority;
				msti_port->portTimes = msti_port->designatedTimes;
				mstp_port->newInfoMsti = true;
			}
			cport->pimState = MSTP_PIM_CURRENT; // UCT
			break;

		case MSTP_PIM_SUPERIOR_DESIGNATED:
			mstp_port->infoInternal = mstp_port->rcvdInternal;

			cport->agreed = cport->proposing = false;
			(*recordProposalXst)(mstp_index, port_number, bpdu);
			mstp_setTcFlags(mstp_index, port_number, bpdu);
			cport->agree = cport->agree && 
							(*betterorsameInfoXst)(mstp_index, port_number, MSTP_INFOIS_RECEIVED);
			/* For boundary ports,if the agree flag were already set for MSTIs, it should be reset 
			 * too if this flag gets reset for CIST. Otherwise state such as MASTER_PROPOSED will 
			 * never get executed and so SYNC mechanism fails for MSTIs causing transient loops */
			if (cist && 
				!mstp_port->rcvdInternal &&
				!l2_proto_mask_is_clear(&mstp_port->instance_mask))
			{
				 for (temp_mstp_index = MSTP_INDEX_MIN; temp_mstp_index <= MSTP_INDEX_MAX; temp_mstp_index++)
				 {
					temp_msti_port = MSTP_GET_MSTI_PORT(mstp_port, temp_mstp_index);
					
					if (temp_msti_port == NULL)
					  continue;
			  
					temp_msti_port->co.agree = cport->agree;
				 }
			}
			(*recordAgreementXst)(mstp_index, port_number, bpdu);
			cport->synced = cport->synced && cport->agreed;
			if (cist)
			{
				cist_port->portPriority = cist_port->msgPriority;
				cist_port->portTimes = cist_port->msgTimes;
			}
			else
			{
				msti_port->portPriority = msti_port->msgPriority;
				msti_port->portTimes = msti_port->msgTimes;
			}
			(*updtRcvdInfoWhileXst)(mstp_index, port_number);
			cport->infoIs = MSTP_INFOIS_RECEIVED;
			mstp_pim_set_reselect(mstp_index, mstp_port);
			cport->selected = false;
			cport->rcvdMsg = false;
			cport->pimState = MSTP_PIM_CURRENT; // UCT
			STP_LOG_INFO("[MST %d] Port %d Received superior bpdu", mstputil_get_mstid(mstp_index), port_number);		
			break;

		case MSTP_PIM_REPEATED_DESIGNATED:
			mstp_port->infoInternal = mstp_port->rcvdInternal;
			(*recordProposalXst)(mstp_index, port_number, bpdu);
			mstp_setTcFlags(mstp_index, port_number, bpdu);
			(*recordAgreementXst)(mstp_index, port_number, bpdu);
			(*updtRcvdInfoWhileXst)(mstp_index, port_number);
			cport->rcvdMsg = false;
			cport->pimState = MSTP_PIM_CURRENT; // UCT
			break;

		case MSTP_PIM_ROOT:
			(*recordAgreementXst)(mstp_index, port_number, bpdu);
			mstp_setTcFlags(mstp_index, port_number, bpdu);
			cport->rcvdMsg = false;
			cport->pimState = MSTP_PIM_CURRENT; // UCT
			break;

		case MSTP_PIM_OTHER:
			cport->rcvdMsg = false;
			cport->pimState = MSTP_PIM_CURRENT; // UCT
			break;

		case MSTP_PIM_RECEIVE:
			cport->rcvdInfo = (*rcvInfoXst)(mstp_index, port_number, bpdu);
			(*recordMasteredXst)(mstp_index, port_number, bpdu);
			break;

        case MSTP_PIM_INFERIOR_DESIGNATED:
            (*recordDisputeXst)(mstp_index, port_number, bpdu);
			cport->rcvdMsg = false;
			cport->pimState = MSTP_PIM_CURRENT; // UCT
            break;

		default:
			break;
	}
}

/*****************************************************************************/
/* mstp_pim: checks for conditions that could trigger transitions            */
/*****************************************************************************/
static bool mstp_pim(MSTP_INDEX mstp_index, PORT_ID port_number, void *bpdu)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool flag = false, print_log = false;
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);
	MSTP_COMMON_PORT *cport;
	bool (*rcvdXstInfo)(MSTP_INDEX, PORT_ID);
	bool (*updtXstInfo)(MSTP_INDEX, PORT_ID);
	MSTP_PIM_STATE old_state;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return false;
	}

	if (cist)
	{
		rcvdXstInfo = mstp_rcvdCistInfo;
		updtXstInfo = mstp_updtCistInfo;
	}
	else
	{
		rcvdXstInfo = mstp_rcvdMstiInfo;
		updtXstInfo = mstp_updtMstiInfo;
	}

	old_state = cport->pimState;
	switch (cport->pimState)
	{
		case MSTP_PIM_DISABLED:
			if (cport->rcvdMsg)
			{
				flag = true;
			}
			else
			if (mstp_port->portEnabled)
			{
				cport->pimState = MSTP_PIM_AGED;
				flag = true;
                print_log = true;
			}
			break;

		case MSTP_PIM_AGED:
			if (cport->selected &&
				cport->updtInfo)
			{
				cport->pimState = MSTP_PIM_UPDATE;
				flag = true;
                print_log = true;
			}
			break;

		case MSTP_PIM_CURRENT:
			if (cport->selected &&
				cport->updtInfo)
			{
				cport->pimState = MSTP_PIM_UPDATE;
				flag = true;
                print_log = true;
			}
            else
			if (cport->infoIs == MSTP_INFOIS_RECEIVED &&
				cport->rcvdInfoWhile == 0 &&
				!cport->updtInfo &&
				!(*rcvdXstInfo)(mstp_index, port_number))
			{
				cport->pimState = MSTP_PIM_AGED;
                flag = true;
                print_log = true;
                STP_LOG_INFO("[MST %d] Port %d PIM %s->%s",mstputil_get_mstid(mstp_index), port_number,
                        MSTP_PIM_STATE_STRING(old_state), MSTP_PIM_STATE_STRING(cport->pimState));
			}
			else
			// check for valid bpdu (possible since PRX->PIM(CIST)->PRS->PIM(MSTI)
			// could be called before bpdu gets fully processed
			if (bpdu != NULL &&
				(*rcvdXstInfo)(mstp_index, port_number) &&
				!(*updtXstInfo)(mstp_index, port_number))
			{
				cport->pimState = MSTP_PIM_RECEIVE;
				flag = true;
			}
            break;

		case MSTP_PIM_RECEIVE:
			if (cport->rcvdInfo == MSTP_SUPERIOR_DESIGNATED_INFO)
			{
				cport->pimState = MSTP_PIM_SUPERIOR_DESIGNATED;
				flag = true;
			}
			else
			if (cport->rcvdInfo == MSTP_REPEATED_DESIGNATED_INFO)
			{
				cport->pimState = MSTP_PIM_REPEATED_DESIGNATED;
				flag = true;
			}
			else
			if (cport->rcvdInfo == MSTP_ROOT_INFO)
			{
				cport->pimState = MSTP_PIM_ROOT;
				flag = true;
			}
            else 
            if (cport->rcvdInfo == MSTP_INFERIOR_DESIGNATED_INFO)
            {
                cport->pimState = MSTP_PIM_INFERIOR_DESIGNATED;
                flag = true;
            }   
            else
			if (cport->rcvdInfo == MSTP_OTHER_INFO)
			{
				cport->pimState = MSTP_PIM_OTHER;
				flag = true;
			}
			break;

		default:
			break;
	}

	if (flag)
	{
		if (MSTP_DEBUG_EVENT(mstp_index, port_number) && (old_state != cport->pimState))
		{
            STP_LOG_INFO("[MST %d] Port %d PIM %s->%s",mstputil_get_mstid(mstp_index), port_number,		
                    MSTP_PIM_STATE_STRING(old_state), MSTP_PIM_STATE_STRING(cport->pimState));
        }
        else if (print_log)
        {
            STP_LOG_INFO("[MST %d] Port %d PIM %s->%s",mstputil_get_mstid(mstp_index), port_number,
                    MSTP_PIM_STATE_STRING(old_state), MSTP_PIM_STATE_STRING(cport->pimState));
        }

		mstp_pim_action(mstp_index, port_number, bpdu);
    }

    
	return flag;
}

/*****************************************************************************/
/* mstp_pim_init: initializes port information state machine for instance    */
/*****************************************************************************/
void mstp_pim_init(MSTP_INDEX mstp_index, PORT_ID port_number)
{	
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTP_COMMON_BRIDGE *cbridge;
	
	if (mstp_port == NULL)
		return;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}

    cport->pimState = MSTP_PIM_DISABLED;
    mstp_pim_action(mstp_index, port_number, NULL);
}

/*****************************************************************************/
/* mstp_pim_gate: triggers port information state machine for instance       */
/*****************************************************************************/
void mstp_pim_gate(MSTP_INDEX mstp_index, PORT_ID port_number, void *bpdu)
{
	bool flag = false;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
    MSTP_COMMON_BRIDGE *cbridge;
	
	if (mstp_port == NULL)
		return;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}


    if (!mstp_port->portEnabled)
	{
		if (cport->infoIs != MSTP_INFOIS_DISABLED)
		{
			mstp_pim_init(mstp_index, port_number);
		}
		return;
	}

	while (mstp_pim(mstp_index, port_number, bpdu))
	{
		flag = true;
	}

    if (flag)
    {
		mstp_pim_signal(mstp_index, port_number);
	}
}