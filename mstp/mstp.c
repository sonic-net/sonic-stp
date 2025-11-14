/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"


MSTP_ROLE txRole[] =
{
	MSTP_ROLE_MASTER,
	MSTP_ROLE_ALTERNATE,
	MSTP_ROLE_ROOT,
	MSTP_ROLE_DESIGNATED,
	MSTP_ROLE_ALTERNATE,
	MSTP_ROLE_MASTER
};


/*****************************************************************************/
/* 13.26 state machine conditions                                         	 */
/*****************************************************************************/

// 13.26.1 - 802.1Q 2011
bool mstp_allSynced(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_COMMON_PORT *cport, *temp_cport;
	UINT mstp_allsynced_counter = 0;

	mstp_bridge = mstpdata_get_bridge();
	
	cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(calling_port));
	
	if (cport == NULL)
		return false;

	for (port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
		 port_number != BAD_PORT_ID;
		 port_number =  port_mask_get_next_port(mstp_bridge->enable_mask, port_number))
	{
		temp_cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));

		if (temp_cport == NULL)
			continue;
		
		/* 13.26.1a - 802.1Q 2011*/
		if (!temp_cport->selected || 
		 	temp_cport->role != temp_cport->selectedRole ||
		 	temp_cport->updtInfo)
		{
			return false;
		}

		/* 13.26.1b - 802.1Q 2011*/
		switch (cport->role)
		{
			case MSTP_ROLE_ROOT:
			case MSTP_ROLE_ALTERNATE:
				if (temp_cport->role != MSTP_ROLE_ROOT && !temp_cport->synced)
					return false;
				break;
			case MSTP_ROLE_DESIGNATED:
			case MSTP_ROLE_MASTER:
				if ((cport != temp_cport) && !temp_cport->synced)
					return false;
				break;
			default:
				return false;
		}
	}
	return true;
}

// 13.26.4
bool mstp_cistRootPort(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;

	mstp_port = mstpdata_get_port(port_number);
	return (mstp_port && mstp_port->cist.co.role == MSTP_ROLE_ROOT);
}

// 13.26.5
bool mstp_cistDesignatedPort(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;

	mstp_port = mstpdata_get_port(port_number);
	return (mstp_port && mstp_port->cist.co.role == MSTP_ROLE_DESIGNATED);
}

// 13.26.12 - true if any instance's port role is root port
bool mstp_mstiRootPort(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX mstp_index;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	if (l2_proto_mask_is_clear(&mstp_port->instance_mask))
		return false;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port && msti_port->co.role == MSTP_ROLE_ROOT)
			return true;
	}

	return false;
}

// 13.26.12 - true if any instance's port role is designated
bool mstp_mstiDesignatedPort(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX mstp_index;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	if (l2_proto_mask_is_clear(&mstp_port->instance_mask))
		return false;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port && msti_port->co.role == MSTP_ROLE_DESIGNATED)
			return true;
	}

	return false;
}

// 13.26.13 - true if any instance's port role is Master
bool mstp_mstiMasterPort(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX mstp_index;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	if (l2_proto_mask_is_clear(&mstp_port->instance_mask))
		return false;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port && msti_port->co.role == MSTP_ROLE_MASTER)
			return true;
	}

	return false;
}

// 13.26.15 - true if rcvdMsg is true for any msti or cist.
bool mstp_rcvdAnyMsg(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_INDEX mstp_index;
	MSTP_COMMON_PORT *cport;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	// check cist
	cport = mstputil_get_common_port(MSTP_INDEX_CIST, mstp_port);
	if (cport && cport->rcvdMsg)
		return true;

	// check msti
	if (l2_proto_mask_is_clear(&mstp_port->instance_mask))
		return false;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		cport = mstputil_get_common_port(mstp_index, mstp_port);
		if (cport && cport->rcvdMsg)
			return true;
	}

	return false;
}

// 13.26.16
bool mstp_rcvdCistInfo(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port;

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist msg", mstputil_get_mstid(mstp_index));
		return false;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port && mstp_port->cist.co.rcvdMsg)
	{
		return true;
	}

	return false;
}

// 13.26.17
bool mstp_rcvdMstiInfo(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_COMMON_PORT *cport;

	cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
	if (cport &&
		cport->rcvdMsg &&
		!mstp_rcvdCistInfo(MSTP_INDEX_CIST, port_number))
	{
		return true;
	}

	return false;
}

// 13.26.18
bool mstp_reRooted(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_COMMON_PORT *cport;

	mstp_bridge = mstpdata_get_bridge();

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		if (port_number != calling_port)
		{
			cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
			if (cport && cport->rrWhile != 0)
				return false;
		}

		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}

	return true;
}

// 13.26.21
bool mstp_updtCistInfo(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist msg", mstputil_get_mstid(mstp_index));
		return false;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	cist_port = MSTP_GET_CIST_PORT(mstp_port);
	return cist_port->co.updtInfo;
}

// 13.26.22
bool mstp_updtMstiInfo(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_CIST_PORT *cist_port;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);

	if (msti_port)
	{
		cist_port = MSTP_GET_CIST_PORT(mstp_port);

		if (msti_port->co.updtInfo ||
			cist_port->co.updtInfo )
		{
			return true;
		}
	}

	return false;
}

/*****************************************************************************/
/* 13.27 state machine procedures                                            */
/*****************************************************************************/

// 13.27.1
bool mstp_betterorsameInfoCist(MSTP_INDEX mstp_index, PORT_ID port_number, MSTP_INFOIS newInfoIs)
{
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;
	MSTP_CIST_VECTOR *cistVector = NULL;
	SORT_RETURN val;

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist msg", mstputil_get_mstid(mstp_index));
		return false;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	if (cist_port->co.infoIs == newInfoIs)
	{
		if (newInfoIs == MSTP_INFOIS_MINE)
			cistVector = &(cist_port->designatedPriority);
		else
		if (newInfoIs == MSTP_INFOIS_RECEIVED)
			cistVector = &(cist_port->msgPriority);

		if (cistVector)
		{
			val = mstputil_compare_cist_vectors(cistVector, &(cist_port->portPriority));
			if (val != GREATER_THAN)
			{
				return true; // better or same info
			}
		}
	}

	return false;
}

// 13.27.1
bool mstp_betterorsameInfoMsti(MSTP_INDEX mstp_index, PORT_ID port_number, MSTP_INFOIS newInfoIs)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_MSTI_VECTOR *mstiVector = NULL;
	SORT_RETURN val;

	if (MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Cist msg", mstputil_get_mstid(mstp_index));
		return false;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (!msti_port)
	{
		return false;
	}

	if (msti_port->co.infoIs == newInfoIs)
	{
		if (newInfoIs == MSTP_INFOIS_MINE)
			mstiVector = &(msti_port->designatedPriority);
		else
		if (newInfoIs == MSTP_INFOIS_RECEIVED)
			mstiVector = &(msti_port->msgPriority);

		if (mstiVector)
		{
			val = mstputil_compare_msti_vectors(mstiVector, &(msti_port->portPriority));
			if (val != GREATER_THAN)
			{
				return true; // better or same info
			}
		}
	}

	return false;
}

// 13.27.2
void mstp_clearAllRcvdMsgs(PORT_ID port_number)
{
	MSTP_INDEX mstp_index;
	MSTP_PORT *mstp_port;
	MSTP_COMMON_PORT *cport;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return;

	// cist
	cport = mstputil_get_common_port(MSTP_INDEX_CIST, mstp_port);
	if (cport)
	{
		cport->rcvdMsg = false;
	}

	// msti
	if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
	{
		for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
		{
			cport = mstputil_get_common_port(mstp_index, mstp_port);
			if (cport)
			{
				cport->rcvdMsg = false;
			}
		}
	}
}

// 13.27.3
void mstp_clearReselectTree(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE *cbridge;
	
	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge)
	{
		cbridge->reselect = false;
	}
}

// 13.27.4
void mstp_disableForwarding(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	mstputil_set_port_state(mstp_index, port_number, BLOCKING);

}

// 13.27.5
void mstp_disableLearning(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	// no specific change required here.
}

// 13.27.6
void mstp_enableForwarding(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	mstputil_set_port_state(mstp_index, port_number, FORWARDING);

}

// 13.27.7
void mstp_enableLearning(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    mstputil_set_port_state(mstp_index, port_number, LEARNING);
}
	
bool mstp_flush(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    PORT_MASK *portmask;

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        portmask = mstplib_instance_get_portmask(mstp_index);
        if(!is_member(portmask, port_number))
        {
            return false;
        }
    }

    mstputil_flush(mstp_index, port_number);

    return true;
}

// 13.27.8
bool mstp_fromSameRegion(MSTP_PORT *mstp_port, MSTP_BPDU *bpdu)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	
	if (mstp_bridge->forceVersion >= MSTP_VERSION_ID && 
		mstp_port->rcvdRSTP &&
		bpdu->protocol_version_id == MSTP_VERSION_ID &&
		MSTP_CONFIG_ID_IS_EQUAL(mstp_bridge->mstConfigId, bpdu->mst_config_id))
	{
		return true;
	}

	return false;
}

// 13.27.10
void mstp_newTcWhile(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge;
	MSTP_CIST_PORT *cist_port;
	MSTP_CIST_BRIDGE *cist_bridge;
	MSTP_COMMON_PORT *cport;
	MSTP_PORT *mstp_port;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport && cport->tcWhile == 0)
	{
		if (mstp_port->sendRSTP)
		{
			cist_port = MSTP_GET_CIST_PORT(mstp_port);
			cport->tcWhile = cist_port->portTimes.helloTime+1;
			
			if (MSTP_IS_CIST_INDEX(mstp_index))
				mstp_port->newInfoCist = true;
			else
				mstp_port->newInfoMsti = true;
		}
		else
		{
			mstp_bridge = mstpdata_get_bridge();
			cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
			cport->tcWhile = cist_bridge->rootTimes.fwdDelay + cist_bridge->rootTimes.maxAge;
		}
	}
}

// 13.27.12
MSTP_RCVD_INFO mstp_rcvInfoCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	SORT_RETURN result;
	SORT_RETURN macCmp = -1;
	bool flag;
	MSTP_BPDU *bpdu = (MSTP_BPDU *) bufptr;
	MSTP_CIST_PORT *cist_port;
	MSTP_COMMON_BRIDGE *cbridge = mstputil_get_common_bridge(mstp_index);
    MSTP_MSTID mstid = mstputil_get_mstid(mstp_index);

	if (bpdu == NULL)
	{
		STP_LOG_DEBUG("[MST %d] Port %d bdpu null", mstid ,port_number); 
		return MSTP_OTHER_INFO;
	}

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("[MST %d] Port %d mstp_port null", mstid, port_number);
		return MSTP_OTHER_INFO;
	}

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist info",mstid);
		return MSTP_OTHER_INFO;
	}

	cist_port = mstputil_get_cist_port(port_number);
	if (cist_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d cist_port null", mstid, port_number);
		return MSTP_OTHER_INFO;
	}

	/* case 4 */
	if (bpdu->type == TCN_BPDU_TYPE)
	{
        if(cist_port->co.rcvdInfo != MSTP_OTHER_INFO)
            STP_LOG_INFO("[MST %d] Port %d Case OTHER_MSG ", mstid, port_number);

		return MSTP_OTHER_INFO;
	}

	result = mstputil_compare_cist_vectors(&cist_port->msgPriority, &cist_port->portPriority);

	if (cbridge != NULL)
	{
		macCmp = stputil_compare_mac(&cist_port->msgPriority.root.address, &cbridge->bridgeIdentifier.address);
	}

	/* case 1 and case 2 */
	if ((bpdu->type == CONFIG_BPDU_TYPE) ||
		(bpdu->type == RSTP_BPDU_TYPE && bpdu->cist_flags.role == MSTP_ROLE_DESIGNATED))
	{
		/* case 1 a */
		if (result == LESS_THAN)
		{
			/* This check prevents a DUT, which is initially forced to be ROOT by reducing its priority and 
			  * then when the priority is reverted back, from believing that there is another root when it 
			  * receives its own Bridge ID from another DUT, with its previous reduced priority (stale info)
              * and thus prevents stale info to be circulating too long.
              */
            if((macCmp == EQUAL_TO) && (mstputil_compare_cist_bridge_id(&cist_port->msgPriority.root, &cbridge->bridgeIdentifier) != EQUAL_TO))
            {
                if(cist_port->co.rcvdInfo != MSTP_OTHER_INFO)
                    STP_LOG_INFO("[MST %d] Port %d Case 1 OTHER_MSG ", mstid, port_number);
                return MSTP_OTHER_INFO;
            }
            else
            {
                /* Case 1a */
                if(cist_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                    STP_LOG_INFO("[MST %d] Port %d Case 1a SUPERIOR_DESIGNATED_INFO", mstid, port_number);
                return MSTP_SUPERIOR_DESIGNATED_INFO;
            }
        }
        flag = mstputil_are_cist_times_equal(&(cist_port->msgTimes), &(cist_port->portTimes));

        if(result == EQUAL_TO && flag == false)
        {
            if(cist_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 1b SUPERIOR_DESIGNATED_INFO", mstid, port_number);
            return MSTP_SUPERIOR_DESIGNATED_INFO;
        }

		/* case 1c */
		// 802.1Q-Rev changes. this will prevent
		// stale information from circulating too long
        
        if((stputil_compare_mac(&(cist_port->msgPriority.designatedId.address),
				&(cist_port->portPriority.designatedId.address)) == EQUAL_TO) &&
			(cist_port->msgPriority.designatedPort.number ==
				cist_port->portPriority.designatedPort.number) && (result == GREATER_THAN))
        {
            if(cist_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 1c SUPERIOR_DESIGNATED_INFO", mstid, port_number);
            return MSTP_SUPERIOR_DESIGNATED_INFO;
		}

		/* case 2 */
		if (flag && (result == EQUAL_TO) && (cist_port->co.infoIs == MSTP_INFOIS_RECEIVED))
		{
            if(cist_port->co.rcvdInfo != MSTP_REPEATED_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 2 REPEATED_DESIGNATED_INFO",  mstid, port_number);
            return MSTP_REPEATED_DESIGNATED_INFO;
		}

        if(result == GREATER_THAN)
        {
            if(cist_port->co.rcvdInfo != MSTP_INFERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 3 MSTP_INFERIOR_DESIGNATED", mstid, port_number);
            return MSTP_INFERIOR_DESIGNATED_INFO;
        }
	}

	/* case 4 */
	if ((bpdu->type == RSTP_BPDU_TYPE) && 
		((bpdu->cist_flags.role == MSTP_ROLE_ROOT) ||
		 (bpdu->cist_flags.role == MSTP_ROLE_ALTERNATE) ||
		 (bpdu->cist_flags.role == MSTP_ROLE_BACKUP)) &&
		(result != LESS_THAN))
    {
        if(cist_port->co.rcvdInfo != MSTP_ROOT_INFO)
            STP_LOG_INFO("[MST %d] Port %d Case 4 ROOT_INFO",  mstid, port_number);	
		return MSTP_ROOT_INFO;
	}
	
	/* case 5
	 * The received bpdu contains inferior information.
	 */
    if(cist_port->co.rcvdInfo != MSTP_OTHER_INFO)
        STP_LOG_INFO("[MST %d] Port %d Case 5 OTHER_MSG", mstid, port_number);	
    return MSTP_OTHER_INFO;
}

// 13.27.12
MSTP_RCVD_INFO mstp_rcvInfoMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_MSTI_PORT *msti_port;
	MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;
	MSTP_COMMON_BRIDGE *cbridge = mstputil_get_common_bridge(mstp_index);
	bool flag;
	SORT_RETURN result;
	SORT_RETURN macCmp = -1;
    MSTP_MSTID mstid =  mstputil_get_mstid(mstp_index);

	if (msg == NULL)
	{
		STP_LOG_DEBUG("[MST %d] Port %d Msti config msg null", mstid, port_number);
		return MSTP_OTHER_INFO;
	}

	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return MSTP_OTHER_INFO;
	}

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return MSTP_OTHER_INFO;
	}

	// compare vectors
	result = mstputil_compare_msti_vectors(&msti_port->msgPriority, &msti_port->portPriority);

	if (cbridge != NULL)
	{
		macCmp = stputil_compare_mac(&msti_port->msgPriority.regionalRoot.address, &cbridge->bridgeIdentifier.address);
	}
	
	/* case 1 and case 2 */
	if (msg->msti_flags.role == MSTP_ROLE_DESIGNATED)
	{
		/* case 1a */
		if (result == LESS_THAN)
		{
			/* This check prevents a DUT, which is initially forced to be ROOT by reducing its priority and 
			  * then when the priority is reverted back, from believing that there is another root when it 
			  * receives its own Bridge ID from another DUT, with its previous reduced priority (stale info)
			  * and thus prevents stale info to be circulating too long.
              */
            if((macCmp == EQUAL_TO) && (mstputil_compare_bridge_id(&msti_port->msgPriority.regionalRoot, &cbridge->bridgeIdentifier) != EQUAL_TO))
            {
                if(msti_port->co.rcvdInfo != MSTP_OTHER_INFO)
                    STP_LOG_INFO("[MST %d] Port %d Case 1 OTHER_MSG ", mstid, port_number);

                return MSTP_OTHER_INFO;
            }
            else
            {
                if(msti_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                    STP_LOG_INFO("[MST %d] Port %d Case 1a SUPERIOR_DESIGNATED_INFO", mstid, port_number);

                return MSTP_SUPERIOR_DESIGNATED_INFO;
            }

		}
		// compare times
		flag = (msti_port->msgTimes.remainingHops == msti_port->portTimes.remainingHops);

		/* case 1c */
        if((stputil_compare_mac(&(msti_port->msgPriority.designatedId.address),
				& (msti_port->portPriority.designatedId.address)) == EQUAL_TO) &&
			(msti_port->msgPriority.designatedPort.number ==
				msti_port->portPriority.designatedPort.number) &&
			((result == GREATER_THAN)))
		{
            if(msti_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 1c SUPERIOR_DESIGNATED_INFO", mstid, port_number);
            return MSTP_SUPERIOR_DESIGNATED_INFO;
        }

		/* case 1b */
        if((result == EQUAL_TO) && !flag)
        {
            if(msti_port->co.rcvdInfo != MSTP_SUPERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 1b SUPERIOR_DESIGNATED_INFO", mstid, port_number);
            return MSTP_SUPERIOR_DESIGNATED_INFO;
        }

		/* case 2 */
		if (flag && (result == EQUAL_TO) && (msti_port->co.infoIs == MSTP_INFOIS_RECEIVED))
		{
            if(msti_port->co.rcvdInfo != MSTP_REPEATED_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 2 REPEATED_DESIGNATED_INFO",  mstid, port_number);
			return MSTP_REPEATED_DESIGNATED_INFO;
		}

		/* case 3 */
        if(result == GREATER_THAN)
        {
            if(msti_port->co.rcvdInfo != MSTP_INFERIOR_DESIGNATED_INFO)
                STP_LOG_INFO("[MST %d] Port %d Case 3 MSTP_INFERIOR_DESIGNATED", mstid, port_number);
            return MSTP_INFERIOR_DESIGNATED_INFO;
        }
	}

	/* case 4 */
	if (((msg->msti_flags.role == MSTP_ROLE_ROOT) ||
		 (msg->msti_flags.role == MSTP_ROLE_ALTERNATE) ||
         (msg->msti_flags.role == MSTP_ROLE_BACKUP)) &&
            (result != LESS_THAN))
    {
        if(msti_port->co.rcvdInfo != MSTP_ROOT_INFO)
            STP_LOG_INFO("[MST %d] Port %d Case 4 ROOT_INFO", mstid, port_number);

        return MSTP_ROOT_INFO;
    }

    /* case 5
     * The received bpdu contains inferior information.
     */
    if(msti_port->co.rcvdInfo != MSTP_OTHER_INFO)
        STP_LOG_INFO("[MST %d] Port %d Case 5 OTHER_MSG", mstid, port_number);	

	return MSTP_OTHER_INFO;
}

// 13.27.14
void mstp_recordAgreementCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_BPDU *bpdu = (MSTP_BPDU *) bufptr;
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;
	MSTP_MSTI_PORT *msti_port;
	SORT_RETURN result;
    MSTP_MSTID mstid = mstputil_get_mstid(mstp_index);

	if (mstp_bridge == NULL)
		return;
	
	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
        STP_LOG_ERR("[MST %d] Non-cist info", mstid);
		return;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	cist_port = MSTP_GET_CIST_PORT(mstp_port);
	cist_port->co.agreed = false;

	if (mstp_bridge->forceVersion >= RSTP_VERSION_ID &&
		bpdu && bpdu->cist_flags.agreement &&
		mstp_port->operPt2PtMac)
	{
		{
			cist_port->co.agreed = true;
			cist_port->co.proposing = false;
		}
	}

	// mark agreed flags for all mstis if received from different mst region
	if (!mstp_port->rcvdInternal &&
		!l2_proto_mask_is_clear(&mstp_port->instance_mask))
	{
		for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
			if (msti_port == NULL)
				continue;

			msti_port->co.agreed = cist_port->co.agreed;
			msti_port->co.proposing = cist_port->co.proposing;
		}
	}
}

// 13.27.14
void mstp_recordAgreementMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_CIST_PORT *cist_port;
	SORT_RETURN result;
    MSTP_MSTID mstid = mstputil_get_mstid(mstp_index);

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return;

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
		return;

	if (!mstp_port->rcvdInternal)
		return;
	
	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	msti_port->co.agreed = false;

	if (msg && msg->msti_flags.agreement &&
		mstp_port->operPt2PtMac)
	{
		if ((mstputil_compare_cist_bridge_id(&cist_port->msgPriority.root, &cist_port->portPriority.root) == EQUAL_TO) &&
			(cist_port->msgPriority.extPathCost == cist_port->portPriority.extPathCost) &&
			(mstputil_compare_cist_bridge_id(&cist_port->msgPriority.regionalRoot, &cist_port->portPriority.regionalRoot) == EQUAL_TO))
		{
			{
				msti_port->co.agreed = true;
				msti_port->co.proposing = false;
			}
		}
	}
}

// 13.27.16
void mstp_recordMasteredCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX index;
    MSTP_MSTID mstid = mstputil_get_mstid(mstp_index);

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
        STP_LOG_ERR("[MST %d] Non-cist info", mstid);
		return;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	if (!mstp_port->rcvdInternal &&
		!l2_proto_mask_is_clear(&mstp_port->instance_mask))
	{
		for (index = MSTP_INDEX_MIN; index <= MSTP_INDEX_MAX; index++)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, index);
			if (msti_port == NULL)
				continue;

			msti_port->mastered = false;
		}
	}
}

// 13.27.16
void mstp_recordMasteredMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port;
	MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;
	MSTP_MSTI_PORT *msti_port;
    MSTP_MSTID mstid = mstputil_get_mstid(mstp_index);

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	if (mstp_port->operPt2PtMac && msg && msg->msti_flags.master)
		msti_port->mastered = true;
	else
		msti_port->mastered = false;
}

// 13.27.18
void mstp_recordProposalCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX index;
	MSTP_BPDU *bpdu = (MSTP_BPDU *) bufptr;
    MSTP_MSTID mstid =  mstputil_get_mstid(mstp_index);

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist info", mstid); 
		return;
	}

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	if(bpdu && bpdu->cist_flags.role == MSTP_ROLE_DESIGNATED && bpdu->cist_flags.proposal)
	{
		cist_port->co.proposed = true;

		if (!mstp_port->rcvdInternal &&
			!l2_proto_mask_is_clear(&mstp_port->instance_mask))
		{
			for (index = MSTP_INDEX_MIN; index <= MSTP_INDEX_MAX; index++)
			{
				msti_port = MSTP_GET_MSTI_PORT(mstp_port, index);
				if (msti_port == NULL)
					continue;

				msti_port->co.proposed = cist_port->co.proposed;
			}
		}
	}
}

// 13.27.18
void mstp_recordProposalMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;
    MSTP_MSTID mstid =  mstputil_get_mstid(mstp_index);

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	if (!mstp_port->rcvdInternal)
		return;
	
	if(msg && msg->msti_flags.role == MSTP_ROLE_DESIGNATED && msg->msti_flags.proposal)
		msti_port->co.proposed = true;
}

void mstp_setRcvdMsgs(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTID mstid;
	MSTP_INDEX mstp_index;
	MSTP_MSTI_PORT *msti_port;
	UINT16 i;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("Port %d msti_port null", port_number);
		return;
	}

	if (bpdu->type == TCN_BPDU_TYPE)
	{
		mstp_port->rcvdTcn = true;
		if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
		{
			for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
			{
				msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
				
				if (msti_port)
					msti_port->co.rcvdTc = true;
			}
		}
	}

	// process cist message
	mstp_port->cist.co.rcvdMsg = true;
	mstputil_extract_cist_msg_info(port_number, bpdu);

	if (!mstp_port->rcvdInternal)
	{
		// mstp bpdu from different region
		return;
	}

	// process msti messages
	for (i = 0; i < MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(bpdu->v3_length);  i++)
	{
		mstid = bpdu->msti_msgs[i].msti_regional_root.system_id;
		mstp_index = mstputil_get_index(mstid);
		if (mstp_index != MSTP_INDEX_INVALID)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
			if (msti_port != NULL)
			{
				msti_port->co.rcvdMsg = true;
				mstputil_extract_msti_msg_info(port_number, bpdu, &bpdu->msti_msgs[i]);

                /* update rx stats */
                mstputil_update_mst_stats(port_number, msti_port, true);
			}
		}
	}
}

// 13.27.20
void mstp_setReRootTree(MSTP_INDEX mstp_index)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
		if (cport)
		{
			cport->reRoot = true;
		}

		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}
}


void mstp_setReselectTree(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE *cbridge;

	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge)
	{
		cbridge->reselect = true;
	}
}

// 13.26.21
void mstp_setSelectedTree(MSTP_INDEX mstp_index)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
		if (cport)
		{
			cport->selected = true;
		}

		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}
}

// 13.27.22
void mstp_setSyncTree(MSTP_INDEX mstp_index)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
		if (cport)
		{
			cport->sync = true;
		}

		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}
}

// 13.27.23
void mstp_setTcFlags(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
	MSTP_INDEX mstp_idx;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_CIST_PORT *cist_port = MSTP_GET_CIST_PORT(mstp_port);	
	MSTP_MSTI_PORT *msti_port;	
	MSTP_BPDU *bpdu = (MSTP_BPDU *) bufptr;
	MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;

	//Set rcvdTc, rcvdTcAck flags for CIST
	if (bpdu && MSTP_IS_CIST_INDEX(mstp_index))
	{		
		if (bpdu->cist_flags.topology_change_acknowledgement)
			mstp_port->rcvdTcAck = true;
		
		if (bpdu->cist_flags.topology_change)
		{
			cist_port->co.rcvdTc = true;
            STP_LOG_INFO("[MST %d] Recvd TC Port %d", mstputil_get_mstid(mstp_index), port_number);
			if (!mstp_port->rcvdInternal)
			{
				if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
				{
					for (mstp_idx = MSTP_INDEX_MIN; mstp_idx <= MSTP_INDEX_MAX; mstp_idx++)
					{
						msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_idx);
						
						if (msti_port)
                        {
                            STP_LOG_INFO("[MST %d] Recvd TC Port %d", mstputil_get_mstid(mstp_index), port_number);
							msti_port->co.rcvdTc = true;
                        }
					}
				}
			}
		}
		return;
	}

	//Set rcvdTc Flag for MSTI if corresponding Flag is set in received MSTI Message
	if (msg && msg->msti_flags.topology_change)
	{
		mstp_idx = mstputil_get_index(msg->msti_regional_root.system_id);
		if (mstp_idx != MSTP_INDEX_INVALID)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_idx);
			if (msti_port)
				msti_port->co.rcvdTc = true;
		}
	}
	return;
}

// 13.27.24
void mstp_setTcPropTree(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;

	if (mstp_bridge == NULL)
		return;
	
	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		if (port_number != calling_port)
		{
			cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
			if (cport)
				cport->tcProp = true;
		}
		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}
}

// 13.27.26
void mstp_txConfig(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;
	MSTP_BPDU *bpdu;
	VLAN_ID vlan_id, modified_vlan_id;
	MSTP_COMMON_BRIDGE *cbridge;
	MSTP_BRIDGE *mstp_bridge;
	
	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("Port %d msti_port null", port_number);
		return;
	}

	bpdu = mstpdata_get_bpdu();
	bpdu->mac_header.length = htons(STP_SIZEOF_CONFIG_BPDU + sizeof(LLC_HEADER));

	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	bpdu->protocol_version_id = STP_VERSION_ID;
	bpdu->type = CONFIG_BPDU_TYPE;

	memset(&bpdu->cist_flags, 0, sizeof(bpdu->cist_flags));
	bpdu->cist_flags.topology_change = (cist_port->co.tcWhile != 0);
	bpdu->cist_flags.topology_change_acknowledgement = (UINT8) mstp_port->tcAck;

	bpdu->cist_root = cist_port->designatedPriority.root;
	bpdu->cist_ext_path_cost = cist_port->designatedPriority.extPathCost;
	bpdu->cist_regional_root = cist_port->designatedPriority.designatedId;
	bpdu->cist_port = cist_port->designatedPriority.designatedPort;

	bpdu->message_age = cist_port->designatedTimes.messageAge << 8;
	bpdu->hello_time = cist_port->designatedTimes.helloTime << 8;
	bpdu->max_age = cist_port->designatedTimes.maxAge << 8;
	bpdu->forward_delay = cist_port->designatedTimes.fwdDelay << 8;

	if (MSTP_DEBUG_BPDU_TX(MSTP_INDEX_CIST, port_number))
	{
		STP_LOG_DEBUG("Port %d -TX Config BPDU", port_number);
		mstpdebug_display_bpdu(bpdu, false);
	}
	mstputil_transmit_bpdu(port_number, CONFIG_BPDU_TYPE);
}

// 13.27.28
void mstp_txTcn(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_COMMON_BRIDGE *cbridge;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_INDEX mstp_index;
	VLAN_ID vlan_id, modified_vlan_id;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("Port %d msti_port null", port_number);
		return;
	}

	if (MSTP_DEBUG_BPDU_TX(MSTP_INDEX_CIST, port_number))
	{
		STP_LOG_DEBUG("Port %d -TX TCN BPDU",port_number);
		mstpdebug_display_bpdu((MSTP_BPDU *) &g_stp_tcn_bpdu, false);
	}
	mstputil_transmit_bpdu(port_number, TCN_BPDU_TYPE);
}

// 13.27.27
void mstp_txMstp(PORT_ID port_number)
{
	UINT8 cnt;
	MSTP_BPDU *bpdu;
	MSTP_INDEX mstp_index;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_PORT *mstp_port;
	MSTP_CIST_PORT *cist_port;
	MSTP_MSTI_PORT *msti_port;
	MSTI_CONFIG_MESSAGE *msti_msg;
	VLAN_ID vlan_id, modified_vlan_id;
	MSTP_COMMON_BRIDGE *cbridge;
	

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
		STP_LOG_ERR("Port %d msti_port null", port_number);
		return;
	}

	bpdu = mstpdata_get_bpdu();
	mstp_bridge = mstpdata_get_bridge();

	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	bpdu->type = RSTP_BPDU_TYPE;

	bpdu->cist_flags.agreement = (UINT8) cist_port->co.agree;

	/* When agree is set for CIST, it is immediately sent as agreement to the peer port which sets
	   agreed flag for CIST (and also for MSTIs for boundary ports). Due to this CIST and MSTI ports on the
	   other side goes into rapid forwarding for boundary ports causing transient loops. Hence for boundary ports,
	   agreement should be sent for CIST if and only if MSTIs have their agree set too indicating that SYNC is
	   now complete for both CIST and MSTIs */
	if (cist_port->co.agree &&
		!mstp_port->rcvdInternal &&		
		(cist_port->co.role == MSTP_ROLE_ROOT || 
		 cist_port->co.role == MSTP_ROLE_DESIGNATED ||
		 cist_port->co.role == MSTP_ROLE_ALTERNATE) &&
		 !l2_proto_mask_is_clear(&mstp_port->instance_mask))
	{		
		for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
			if (msti_port == NULL)
				continue;

			if(!msti_port->co.agree)
			{
				bpdu->cist_flags.agreement = false;
				
				if (MSTP_DEBUG_EVENT(MSTP_MSTID_CIST, port_number))
                    STP_LOG_INFO("[MST %d] Port %d - Agreement Not Set in BPDU as Agree is false",
                            mstputil_get_mstid(mstp_index), port_number);
                break;
            }
		}
	}
	bpdu->cist_flags.forwarding = (UINT8) cist_port->co.forwarding;
	bpdu->cist_flags.learning = (UINT8) cist_port->co.learning;
	bpdu->cist_flags.proposal = (UINT8) cist_port->co.proposing;
	if (cist_port->co.role == MSTP_ROLE_DISABLED)
	{
		STP_LOG_INFO("Port %d DISABLED role",  port_number);
		return;
	}
	bpdu->cist_flags.role = txRole[cist_port->co.role];
	bpdu->cist_flags.topology_change = (cist_port->co.tcWhile != 0);
	bpdu->cist_flags.topology_change_acknowledgement = 0;

	bpdu->cist_root = cist_port->designatedPriority.root;
	bpdu->cist_ext_path_cost = cist_port->designatedPriority.extPathCost;
	bpdu->cist_regional_root = cist_port->designatedPriority.regionalRoot;
	bpdu->cist_port = cist_port->designatedPriority.designatedPort;

	bpdu->forward_delay = cist_port->designatedTimes.fwdDelay << 8;
	bpdu->max_age = cist_port->designatedTimes.maxAge << 8;
	bpdu->hello_time = cist_port->designatedTimes.helloTime << 8;
	bpdu->message_age = cist_port->designatedTimes.messageAge << 8;

	bpdu->v1_length = 0;

	if (mstp_bridge->forceVersion == MSTP_RSTP_COMPATIBILITY_MODE)
	{
		// rstp bpdu
		bpdu->protocol_version_id = RSTP_VERSION_ID;
		bpdu->mac_header.length = htons(RSTP_SIZEOF_BPDU + sizeof(LLC_HEADER));

		if (MSTP_DEBUG_BPDU_TX(MSTP_INDEX_CIST, port_number))
		{
			STP_LOG_DEBUG("Port %d -TX RST BPDU",port_number);
			mstpdebug_display_bpdu((MSTP_BPDU *) &g_stp_tcn_bpdu, false);
		}		
		mstputil_transmit_bpdu(port_number, RSTP_BPDU_TYPE);
		return;
	}

	// mstp bpdu - encode msti information
	bpdu->protocol_version_id = MSTP_VERSION_ID;

	bpdu->v3_length = MSTP_BPDU_BASE_V3_LENGTH; // will be modified later if needed
	bpdu->mst_config_id = mstp_bridge->mstConfigId;
	bpdu->cist_int_path_cost = cist_port->designatedPriority.intPathCost;
	bpdu->cist_bridge = cist_port->designatedPriority.designatedId;
	bpdu->cist_remaining_hops = cist_port->designatedTimes.remainingHops;

	if (MSTP_DEBUG_BPDU_TX(MSTP_INDEX_CIST, port_number))
	{
		STP_LOG_INFO("Port %d TX MST BPDU",port_number);
		mstpdebug_display_bpdu((MSTP_BPDU *) bpdu, false);
	}

	cnt = 0;
	memset(&bpdu->msti_msgs, 0, sizeof(MSTI_CONFIG_MESSAGE)*MSTP_MAX_INSTANCES_PER_REGION);
	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port == NULL)
			continue;

		msti_msg = &bpdu->msti_msgs[cnt];

		msti_msg->msti_flags.agreement = (UINT8) msti_port->co.agree; // MSTP_BUG: Fix bug in standard.
		msti_msg->msti_flags.forwarding = (UINT8) msti_port->co.forwarding;
		msti_msg->msti_flags.learning = (UINT8) msti_port->co.learning;
		msti_msg->msti_flags.master = (UINT8) msti_port->master;
		msti_msg->msti_flags.proposal = (UINT8) msti_port->co.proposing;
		if (msti_port->co.role == MSTP_ROLE_DISABLED)
		{
			STP_LOG_INFO("[MST %u] Port %d DISABLED role",
				  mstp_index, port_number);
			return;
		}
		msti_msg->msti_flags.role = txRole[msti_port->co.role];
		msti_msg->msti_flags.topology_change = (msti_port->co.tcWhile != 0);
		
		msti_msg->msti_regional_root = msti_port->designatedPriority.regionalRoot;
		msti_msg->msti_int_path_cost = msti_port->designatedPriority.intPathCost;
		msti_msg->msti_bridge_priority = (UINT8) msti_port->designatedPriority.designatedId.priority;
		msti_msg->msti_port_priority = (UINT8) msti_port->co.portId.priority;
		msti_msg->msti_remaining_hops = msti_port->designatedTimes.remainingHops;

		if (MSTP_DEBUG_BPDU_TX(mstp_index, port_number))
		{
			STP_LOG_INFO("[MST %u] Port %d -TX MSTI config message",
				  mstputil_get_mstid(mstp_index), port_number);
			mstpdebug_display_msti_bpdu(msti_msg);
		}

        /* update tx stats */
        mstputil_update_mst_stats(port_number, msti_port, false);

		cnt++;		
		cbridge = mstputil_get_common_bridge(mstp_index);
		
		if (!cbridge)
		{
			STP_LOG_INFO("Common bridge null");
			continue;
		}
	}

	bpdu->v3_length += (cnt * sizeof (MSTI_CONFIG_MESSAGE));
	bpdu->mac_header.length = htons(MSTP_BPDU_BASE_SIZE + sizeof(LLC_HEADER) + bpdu->v3_length);
	mstputil_transmit_bpdu(port_number, RSTP_BPDU_TYPE);
}

// 13.27.29
void mstp_updtBpduVersion(MSTP_PORT *mstp_port, MSTP_BPDU *bpdu)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

	if ((bpdu->type == TCN_BPDU_TYPE) || 
		(bpdu->type == CONFIG_BPDU_TYPE))
	{
		mstp_port->rcvdSTP = true;
	}
	else
	if ((bpdu->type == RSTP_BPDU_TYPE) &&
		(mstp_bridge->forceVersion >= MSTP_RSTP_COMPATIBILITY_MODE))
	{
		mstp_port->rcvdRSTP = true;
	}
}

// 13.27.30
void mstp_updtRcvdInfoWhileCist(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_CIST_PORT *cist_port;
	MSTP_CIST_TIMES *times;

	if (!MSTP_IS_CIST_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST %d] Non-cist info",  mstp_index);
		return;
	}

	if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstp_index,port_number);
		return;
	}

	cist_port = MSTP_GET_CIST_PORT(mstp_port);
	times = &(cist_port->portTimes);

	if (mstp_port->rcvdInternal == false)
	{
		// received from a bridge external to the MST region
		UINT16 effective_age = times->messageAge + 1;
		
		if (times->maxAge < effective_age)
			cist_port->co.rcvdInfoWhile = 0;
		else			
			cist_port->co.rcvdInfoWhile = 3*times->helloTime;
	}
	else
	{
		if (times->remainingHops  > 1)
			cist_port->co.rcvdInfoWhile = 3*times->helloTime;
		else
			cist_port->co.rcvdInfoWhile = 0;
	}
}

// 13.27.30
void mstp_updtRcvdInfoWhileMsti(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_MSTI_PORT *msti_port;
    MSTP_CIST_PORT *cist_port;	
    MSTP_CIST_TIMES *times;
    MSTP_MSTID mstid =  mstputil_get_mstid(mstp_index);

    if (mstp_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
	{
		STP_LOG_ERR("[MST %d] Port %d msti_port null", mstid, port_number);
		return;
	}

	cist_port = MSTP_GET_CIST_PORT(mstp_port);
	times = &(cist_port->portTimes);

	if (mstp_port->rcvdInternal == false)
	{
		// received from a bridge external to the MST region
		UINT16 effective_age = times->messageAge + 1;
		
		if (times->maxAge < effective_age)
			msti_port->co.rcvdInfoWhile = 0;
		else			
			msti_port->co.rcvdInfoWhile = 3*times->helloTime;
    }
    else
    {
        /* As per the standard (802.1Q 2011), 
           "..The values of Message Age, Max Age, remainingHops, and Hello Time used in these calculations are taken
           from the CIST portTimes parameter.
           If 2 switches are connected back to back by 2 ports and if "count-to-infinity" problem happens, then 
           then stale info will circulate forever.Using CIST portTimes wont help to resolve this issue. 
           Adding deviation from standard to use MSTI own remainingHops to get rid this issue */
		if (msti_port->portTimes.remainingHops > 1)
			msti_port->co.rcvdInfoWhile = 3 * times->helloTime;
		else
        {
            msti_port->co.rcvdInfoWhile = 0;
            STP_LOG_INFO("[MST %d] Port %d Remaining Hop is %d ",mstputil_get_mstid(mstp_index), 
                    port_number, msti_port->portTimes.remainingHops);
        }
    }
}

// 13.27.31
void mstp_updtRolesCist(MSTP_INDEX mstp_index)
{
    PORT_ID port_number, root_port, old_root_port;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_PORT *mstp_port;
    MSTP_CIST_PORT *cist_port, *cist_root_port;
    MSTP_CIST_VECTOR rootPathPriority, rootPriority, oldRootPriority;
    bool changedMaster;
    UINT8	root_id_string[20];
    bool flag = true;
    MSTP_COMMON_BRIDGE *cbridge;
    
    if (!MSTP_IS_CIST_INDEX(mstp_index))
    {
        STP_LOG_ERR("[MST %d] Non-cist info", MSTP_MSTID_CIST);
        return;
    }

    root_port = MSTP_INVALID_PORT;
    cist_root_port = NULL;
    mstp_bridge = mstpdata_get_bridge();
    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
    cbridge = &cist_bridge->co;

    rootPriority = cist_bridge->bridgePriority;

    oldRootPriority = cist_bridge->rootPriority;
    old_root_port = cist_bridge->co.rootPortId;

    port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstp_port = mstpdata_get_port(port_number);
        if (mstp_port)
        {
            cist_port = MSTP_GET_CIST_PORT(mstp_port);
            if (cist_port->co.infoIs == MSTP_INFOIS_RECEIVED)
            {
                if (mstputil_compare_cist_bridge_id(&cist_port->portPriority.designatedId,
                            &cist_bridge->co.bridgeIdentifier) == EQUAL_TO)
                {
                    cist_port->co.selectedRole = MSTP_ROLE_BACKUP;
                    STP_LOG_INFO("[MST %u] Port %d Role BACKUP", MSTP_MSTID_CIST, port_number);
                }
                else
                {
                    cist_port->co.selectedRole = MSTP_ROLE_ALTERNATE;
                    rootPathPriority = cist_port->portPriority;

                    if (mstp_port->rcvdInternal)
                    {
                        rootPathPriority.intPathCost += cist_port->co.intPortPathCost;
                    }
                    else
                    {
                        rootPathPriority.extPathCost += cist_port->extPortPathCost;
                        rootPathPriority.regionalRoot = cist_bridge->co.bridgeIdentifier;
                    }

                    if ((mstputil_compare_cist_vectors(&rootPathPriority, &rootPriority) == LESS_THAN) && 
                            (!mstp_port->restrictedRole))
                    {
                        root_port = port_number;
                        cist_root_port = cist_port;
                        rootPriority = rootPathPriority;
                    }
                    if(cist_port->co.selectedRole == MSTP_ROLE_ALTERNATE && mstp_port->restrictedRole)
                    {
                        STP_SYSLOG("MSTP: Root Guard interface %d MST%u inconsistent (Received superior BPDU)",
                                port_number, MSTP_MSTID_CIST);
                        SET_BIT(cist_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT);
                    }
                }
            }
            cist_port->co.updtInfo = false;
        }

        port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
    }


    if(mstputil_compare_bridge_id(&cist_bridge->rootPriority.regionalRoot, &rootPriority.regionalRoot) != EQUAL_TO)
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT);
    if(mstputil_compare_bridge_id(&cist_bridge->rootPriority.root, &rootPriority.root) != EQUAL_TO)
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
    if(cist_bridge->rootPriority.intPathCost != rootPriority.intPathCost)
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_IN_ROOT_PATH_COST_BIT);
    if(cist_bridge->rootPriority.extPathCost != rootPriority.extPathCost)
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT);
	
	// rootPriority is the best vector
	cist_bridge->rootPriority = rootPriority;

	cist_bridge->co.rootPortId = root_port;
	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT);

	changedMaster = ((rootPriority.extPathCost != 0) || (oldRootPriority.extPathCost != 0)) &&
    	(mstputil_compare_cist_bridge_id(&rootPriority.regionalRoot, &oldRootPriority.regionalRoot) != EQUAL_TO);

	if (root_port != MSTP_INVALID_PORT)
    {
        cist_root_port->co.selectedRole = MSTP_ROLE_ROOT;
		if(cist_bridge->rootTimes.maxAge != cist_root_port->portTimes.maxAge)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
    	if(cist_bridge->rootTimes.helloTime != cist_root_port->portTimes.helloTime)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
    	if(cist_bridge->rootTimes.fwdDelay != cist_root_port->portTimes.fwdDelay)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
    	if(cist_bridge->rootTimes.remainingHops != cist_root_port->portTimes.remainingHops)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);

        cist_bridge->rootTimes = cist_root_port->portTimes;

        mstp_port = mstpdata_get_port(root_port);

        if (!mstp_port->rcvdInternal)
            cist_bridge->rootTimes.messageAge++;
        else 
		{
			if(cist_bridge->rootTimes.remainingHops)
			{
            	cist_bridge->rootTimes.remainingHops--;
				SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);
			}
		}
    }
    else
    {
		if(cist_bridge->rootTimes.maxAge != cist_bridge->bridgeTimes.maxAge)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
    	if(cist_bridge->rootTimes.helloTime != cist_bridge->bridgeTimes.helloTime)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
    	if(cist_bridge->rootTimes.fwdDelay != cist_bridge->bridgeTimes.fwdDelay)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
    	if(cist_bridge->rootTimes.remainingHops != cist_bridge->bridgeTimes.remainingHops)
        	SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);

        cist_bridge->rootTimes = cist_bridge->bridgeTimes;
    }

    port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstp_port = mstpdata_get_port(port_number);

        if (mstp_port)
        {
            if (changedMaster)
                mstputil_set_changedMaster(mstp_port);

            cist_port = MSTP_GET_CIST_PORT(mstp_port);
            mstputil_compute_cist_designated_priority(cist_bridge, mstp_port);
            cist_port->designatedTimes = cist_bridge->rootTimes;

            if (port_number != root_port) // root_port may be MSTP_INVALID_PORT
            {
                switch (cist_port->co.infoIs)
                {
                    case MSTP_INFOIS_DISABLED:
                        cist_port->co.selectedRole = MSTP_ROLE_DISABLED;
                        break;

                    case MSTP_INFOIS_AGED:
                        if(cist_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                            STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_AGED)", MSTP_MSTID_CIST, port_number);
                        cist_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                        cist_port->co.updtInfo = true;
                        break;

                    case MSTP_INFOIS_MINE:
                        if(cist_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                            STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_MINE)", MSTP_MSTID_CIST, port_number);
                        cist_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                        if ((mstputil_compare_cist_vectors(&cist_port->portPriority,
                                        &cist_port->designatedPriority) != EQUAL_TO) ||
                                (mstputil_are_cist_times_equal(&cist_port->portTimes,
                                                               &cist_port->designatedTimes) == false))
                        {
                            cist_port->co.updtInfo = true;
                        }
                        break;

                    case MSTP_INFOIS_RECEIVED:
                        if (mstputil_compare_cist_vectors(&cist_port->designatedPriority,
                                    &cist_port->portPriority) == LESS_THAN)
                        {
                            if(cist_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                                STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_RECEIVED)", MSTP_MSTID_CIST, port_number);
                            cist_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                            cist_port->co.updtInfo = true;
                        }
                        break;

                    default:
                        // print error
                        return;
                }
            }
        }
        if(cist_port->co.selectedRole == MSTP_ROLE_ALTERNATE)
            STP_LOG_INFO("[MST %u] Port %d Role Alternate", MSTP_MSTID_CIST, port_number);

        if (mstp_port && !mstp_port->rcvdInternal && (cist_port->co.selectedRole != cist_port->co.role))
        {
            // boundary port - when role changes signal to the mstis as well
            mstp_index = l2_proto_get_first_instance(&mstp_port->instance_mask);
            while (mstp_index != L2_PROTO_INDEX_INVALID)
            {
                mstp_setReselectTree(mstp_index);
                mstp_index = l2_proto_get_next_instance(&mstp_port->instance_mask, mstp_index);
            }
        }
        port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
    }

    mstputil_bridge_to_string(&rootPriority.root, root_id_string, sizeof(root_id_string));
    if (mstputil_compare_cist_bridge_id(&rootPriority.root, &oldRootPriority.root) != EQUAL_TO)
    {
        if (mstputil_is_cist_root_bridge())
        {
            /*This is bridge information we are logging, in root bridge there will be no root port*/
            STP_LOG_INFO("[MST %u] I am the Root %s", MSTP_MSTID_CIST, root_id_string);
        }
        else
        {
            if(root_port != MSTP_INVALID_PORT)
                STP_LOG_INFO("[MST %u] New Root %s Root port %d", MSTP_MSTID_CIST, root_id_string,
                        root_port);
        }
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
    }
    else
        if (root_port != old_root_port &&
                root_port != MSTP_INVALID_PORT &&
                old_root_port != MSTP_INVALID_PORT)
        {
            STP_LOG_INFO("[MST %u] Root port Changed from %d to %d (Root %s)", MSTP_MSTID_CIST,
                    old_root_port, root_port, root_id_string);
        }
}

// 13.27.31
void mstp_updtRolesMsti(MSTP_INDEX mstp_index)
{
    bool masterSelected, masteredSet;
    PORT_ID port_number, root_port, old_root_port;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_PORT *mstp_port;
    MSTP_CIST_PORT *cist_port;
    MSTP_MSTI_PORT *msti_port, *msti_root_port;
    MSTP_MSTI_VECTOR rootPathPriority, rootPriority, oldRootPriority;
    PORT_MASK_LOCAL l_msti_enable_mask;
    PORT_MASK *msti_enable_mask = portmask_local_init(&l_msti_enable_mask);
    MSTP_MSTID mstid;
    UINT8	root_id_string[20];
    bool flag = true;
    MSTP_COMMON_BRIDGE *cbridge;

    mstp_bridge = mstpdata_get_bridge();
    msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);

    if (msti_bridge == NULL)
    {
        return;
    }
    cbridge = &msti_bridge->co;

    root_port = MSTP_INVALID_PORT;
    msti_root_port = NULL;
    masterSelected = masteredSet = false;

    mstid = MSTP_GET_MSTID_FROM_MSTI(msti_bridge);
    and_masks(msti_enable_mask, msti_bridge->co.portmask, mstp_bridge->enable_mask);
    rootPriority = msti_bridge->bridgePriority;

    oldRootPriority = msti_bridge->rootPriority;
    old_root_port = msti_bridge->co.rootPortId;

    /*
     * 1 - go through all msti enabled ports which are internal to the
     *     region where infoIs = RECEIVED to identify msti regional root.
     */
    port_number = port_mask_get_first_port(msti_enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstp_port = mstpdata_get_port(port_number);
        if ((mstp_port == NULL) ||
                ((msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index)) == NULL))
        {
            // clear port so that next loop does not need any of these checks.
            clear_mask_bit(msti_enable_mask, port_number);
        }
        else
        {
            msti_port->co.updtInfo = false;

            if (msti_port->co.infoIs == MSTP_INFOIS_RECEIVED)
            {
                if (mstputil_compare_bridge_id(&msti_port->portPriority.designatedId,
                            &msti_bridge->co.bridgeIdentifier) == EQUAL_TO)
                {
                    msti_port->co.selectedRole = MSTP_ROLE_BACKUP;
                    STP_LOG_INFO("[MST %u] Port %d Role BACKUP", mstid, port_number);
                }
                else
                {
                    msti_port->co.selectedRole = MSTP_ROLE_ALTERNATE;
                    rootPathPriority = msti_port->portPriority;

                    if (mstp_port->rcvdInternal)
                    {
                        rootPathPriority.intPathCost += msti_port->co.intPortPathCost;
                    }

                    if ((mstputil_compare_msti_vectors(&rootPathPriority, &rootPriority) == LESS_THAN) &&
                            (!mstp_port->restrictedRole))
                    {
                        root_port = port_number;
                        msti_root_port = msti_port;
                        rootPriority = rootPathPriority;
                    }
                    if(msti_port->co.selectedRole == MSTP_ROLE_ALTERNATE && mstp_port->restrictedRole)
                    {
                        STP_SYSLOG("MSTP: Root Guard interface %d MST%u inconsistent (Received superior BPDU)",
                                port_number, mstid);
                        SET_BIT(msti_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT);
                    }

                }
            }
        }

        port_number = port_mask_get_next_port(msti_enable_mask, port_number);
    } /* 1 */

    // rootPriority is the best vector (could be bridgePriority)
    if(mstputil_compare_bridge_id(&msti_bridge->rootPriority.regionalRoot, &rootPriority.regionalRoot) != EQUAL_TO)
        SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT);
    if(msti_bridge->rootPriority.intPathCost != rootPriority.intPathCost)
        SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT);

    msti_bridge->rootPriority = rootPriority;

	if(msti_bridge->co.rootPortId != root_port)
        SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT);

    msti_bridge->co.rootPortId = root_port;
	SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT);

	if (root_port != MSTP_INVALID_PORT)
    {
        msti_root_port->co.selectedRole = MSTP_ROLE_ROOT;

		if(msti_bridge->rootTimes.remainingHops != msti_bridge->bridgeTimes.remainingHops)
            SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);
        msti_bridge->rootTimes = msti_root_port->portTimes;

        mstp_port = mstpdata_get_port(root_port);

		if (mstp_port->rcvdInternal)
        {
            if(msti_bridge->rootTimes.remainingHops)
            {
                msti_bridge->rootTimes.remainingHops--;
                SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);
            }
		}
	}
	else
	{
		msti_bridge->rootTimes = msti_bridge->bridgeTimes;
	}

    /*
     * 2 - go through all the msti enabled ports including boundary ports.
     */
    port_number = port_mask_get_first_port(msti_enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstp_port = mstpdata_get_port(port_number);
        msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);

        mstputil_compute_msti_designated_priority(msti_bridge, msti_port);
        msti_port->designatedTimes = msti_bridge->rootTimes;
        cist_port = MSTP_GET_CIST_PORT(mstp_port);

        if ((cist_port->co.infoIs != MSTP_INFOIS_RECEIVED) ||
                (mstp_port->rcvdInternal))
        {
            // received inside the region - look at msti infoIs
            switch (msti_port->co.infoIs)
            {
                case MSTP_INFOIS_DISABLED:
                    msti_port->co.selectedRole = MSTP_ROLE_DISABLED;
                    break;

                case MSTP_INFOIS_AGED:
                    if(msti_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                        STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_AGED)", mstid, port_number);
                    msti_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                    msti_port->co.updtInfo = true;
                    break;

                case MSTP_INFOIS_MINE:
                    if(msti_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                        STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_MINE)", mstid, port_number);

                    msti_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                    if ((mstputil_compare_msti_vectors(&msti_port->portPriority,
                                    &msti_port->designatedPriority) != EQUAL_TO) ||
                            (msti_port->portTimes.remainingHops !=
                             msti_port->designatedTimes.remainingHops))
                    {
                        msti_port->co.updtInfo = true;
                    }
                    break;

                case MSTP_INFOIS_RECEIVED:
                    if (port_number != root_port)
                    {
                        if (mstputil_compare_msti_vectors(&msti_port->designatedPriority,
                                    &msti_port->portPriority) == LESS_THAN)
                        {
                            if(msti_port->co.selectedRole != MSTP_ROLE_DESIGNATED)
                                STP_LOG_INFO("[MST %u] Port %d Role DESIGNATED (INFOIS_RECEIVED)", mstid, port_number);
                            msti_port->co.selectedRole = MSTP_ROLE_DESIGNATED;
                            msti_port->co.updtInfo = true;
                        }
                    }
                    break;

                default:
                    // print error
                    return;
            }
        }
        else
        {
            // received from outside the region - look at cist selected role.
            switch (cist_port->co.selectedRole)
            {
                case MSTP_ROLE_ROOT:
                    if(msti_port->co.selectedRole != MSTP_ROLE_MASTER)
                        STP_LOG_INFO("[MST %u] Port %d Role Master", mstid, port_number);
                    msti_port->co.selectedRole = MSTP_ROLE_MASTER;
                    masterSelected = true;
                    if ((mstputil_compare_msti_vectors(&msti_port->portPriority,
                                    &msti_port->designatedPriority) != EQUAL_TO) ||
                            (msti_port->portTimes.remainingHops !=
                             msti_port->designatedTimes.remainingHops))
                    {
                        msti_port->co.updtInfo = true;
                    }
                    break;

                case MSTP_ROLE_ALTERNATE:
                    if(msti_port->co.selectedRole != MSTP_ROLE_ALTERNATE)
                    {
                        flag = false;
                        STP_LOG_INFO("[MST %u] Port %d Role Alternate", mstid, port_number);
                    }
                    msti_port->co.selectedRole = MSTP_ROLE_ALTERNATE;
                    if ((mstputil_compare_msti_vectors(&msti_port->portPriority,
                                    &msti_port->designatedPriority) != EQUAL_TO) ||
                            (msti_port->portTimes.remainingHops !=
                             msti_port->designatedTimes.remainingHops))
                    {
                        msti_port->co.updtInfo = true;
                    }
                    break;
            }
        }

        if(flag && (msti_port->co.selectedRole == MSTP_ROLE_ALTERNATE))
            STP_LOG_INFO("[MST %u] Port %d Role Alternate", mstid, port_number);

        if ((msti_port->co.selectedRole == MSTP_ROLE_DESIGNATED ||
                    msti_port->co.selectedRole == MSTP_ROLE_ROOT) &&
                msti_port->mastered)
        {
            masteredSet = true;
        }
        msti_port->master = false;

        port_number = port_mask_get_next_port(msti_enable_mask, port_number);
    } /* 2 */

    /*
     * 3 - set the master flag for all ports if mastered is set for any port
     * or the bridge has selected one of the ports as the master port.
     * see section 13.24.13 and 13.24.14 for more information.
     */
    if (masteredSet || masterSelected)
    {
        port_number = port_mask_get_first_port(msti_enable_mask);
        while (port_number != BAD_PORT_ID)
        {
            mstp_port = mstpdata_get_port(port_number);
            msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);

            if (msti_port->co.role == MSTP_ROLE_DESIGNATED ||
                    msti_port->co.role == MSTP_ROLE_ROOT)
            {
                msti_port->master = true;
            }

            port_number = port_mask_get_next_port(msti_enable_mask, port_number);
        }
    } /* 3 */

    mstputil_bridge_to_string(&rootPriority.regionalRoot, root_id_string, sizeof(root_id_string));
    // snmp traps and logging
    if (mstputil_compare_bridge_id(&rootPriority.regionalRoot, &oldRootPriority.regionalRoot) != EQUAL_TO)
    {
        if (mstputil_is_msti_root_bridge(mstp_index))
        {
            STP_LOG_INFO("[MST %u] I am the Root %s", mstid, root_id_string);
        }
        else
        {
            if(root_port != MSTP_INVALID_PORT)
                STP_LOG_INFO("[MST %u] New Root %s Root port %d", mstid, root_id_string,
                    root_port);
        }
        SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT);
    }
    else
        if (root_port != old_root_port &&
                root_port != MSTP_INVALID_PORT &&
                old_root_port != MSTP_INVALID_PORT)
        {
            STP_LOG_INFO("[MST %u] Root port Changed from %d to %d (Root %s)", mstid,
                    old_root_port, root_port, root_id_string);
        }

}

// 13.27.32
void mstp_updtRolesDisabledTree(MSTP_INDEX mstp_index)
{
	PORT_ID port_number;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;

	port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
		if (cport)
		{
			cport->selectedRole = MSTP_ROLE_DISABLED;
		}
		port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
	}
}
// 13.27.15
void mstp_recordDisputeCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
    MSTP_BPDU *bpdu = (MSTP_BPDU *) bufptr;
    MSTP_COMMON_PORT *cport;
    MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
    MSTP_INDEX msti_index;

    cport = mstputil_get_common_port(mstp_index, mstp_port);
    if (cport)
    {
        if(bpdu->cist_flags.learning)
        {
            cport->agreed = false;
            cport->disputed = true;
        }
        if(!mstp_port->rcvdInternal)
        {
            for (msti_index = MSTP_INDEX_MIN; msti_index <= MSTP_INDEX_MAX; msti_index++)
            {
                cport = mstputil_get_common_port(msti_index, mstp_port);
                if (cport)
                {
                    cport->agreed = false;
                    cport->disputed = true;
                }
            }
        }
    }
}
// 13.27.15
void mstp_recordDisputeMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr)
{
    MSTI_CONFIG_MESSAGE *msg = (MSTI_CONFIG_MESSAGE *) bufptr;
    MSTP_COMMON_PORT *cport;

    cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
    if (cport)
    {
        if(msg->msti_flags.learning)
        {
            cport->agreed = false;
            cport->disputed = true;
        }
    }
}
