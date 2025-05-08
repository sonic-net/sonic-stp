/*
 * Copyright 2025 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_pst_signal: signals to other state machines                          */
/*****************************************************************************/
static void mstp_pst_signal(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
		return;

	/* learning, forwarding - PRT
	 */
	if (!cport->learning &&
		!cport->forwarding)
	{
		mstp_prt_gate(mstp_index, port_number);
	}

	if (!(cport->learn || cport->learning) ||
		(cport->learn || cport->forward))
	{
		mstp_tcm_gate(mstp_index, port_number);
	}
}

/*****************************************************************************/
/* mstp_pst_action: performs actions associated with specific pst states     */
/*****************************************************************************/
void mstp_pst_action(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTP_MSTI_PORT *msti_port;
	MSTP_MSTID mstid=mstputil_get_mstid(mstp_index);
	
	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}

	switch (cport->pstState)
	{
		case MSTP_PST_DISCARDING:
			cport->learning = false;
			cport->forwarding = false;
			mstp_disableForwarding(mstp_index, port_number);
			break;

		case MSTP_PST_LEARNING:
			cport->learning = true;
			mstp_enableLearning(mstp_index, port_number);
			break;

		case MSTP_PST_FORWARDING:
			cport->forwarding = true;
            mstp_enableForwarding(mstp_index, port_number);			
            cport->forwardTransitions++;
            SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_FWD_TRANSITIONS_BIT);
            break;

		default:
			break;
	}
}

/*****************************************************************************/
/* mstp_pst: executes the port state transitions state machine               */
/*****************************************************************************/
static bool mstp_pst(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);	
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	bool flag = false;
	MSTP_COMMON_PORT *cport;
	MSTP_PST_STATE old_state;
    MSTP_COMMON_BRIDGE *cbridge;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return false;
	}	
	if(!mstp_bridge)
		return false;

	old_state = cport->pstState;
	switch (cport->pstState)
	{
		case MSTP_PST_DISCARDING:
			if (cport->learn)
			{
				cport->pstState = MSTP_PST_LEARNING;
				flag = true;
			}
			break;

		case MSTP_PST_LEARNING:
			if (cport->forward)
			{
				cport->pstState = MSTP_PST_FORWARDING;				
                flag = true;
			}
			else
			if (!cport->learn)
			{
				cport->pstState = MSTP_PST_DISCARDING;				
                flag = true;
			}
			break;

		case MSTP_PST_FORWARDING:
			if (!cport->forward)
			{
				cport->pstState = MSTP_PST_DISCARDING;				
                flag = true;
			}
			break;

		default:
			break;
	}

	if (flag)
	{
		if(old_state != cport->pstState)
		{
            STP_LOG_INFO("[MST %d] Port %d PST %s->%s", mstputil_get_mstid(mstp_index), port_number,		
				MSTP_PST_STATE_STRING(old_state),
				MSTP_PST_STATE_STRING(cport->pstState));
		}

		mstp_pst_action(mstp_index, port_number);
    }

    return flag;
}

/*****************************************************************************/
/* mstp_pst_init: initializes the port state transitions state machine       */
/*****************************************************************************/
void mstp_pst_init(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTP_COMMON_BRIDGE *cbridge;

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}
	
	mstputil_set_port_state(mstp_index, port_number, BLOCKING);

	cport->pstState = MSTP_PST_DISCARDING;
    mstp_pst_action(mstp_index, port_number);
}

/*****************************************************************************/
/* mstp_pst_gate: invokes the port state transitions state machine           */
/*****************************************************************************/
void mstp_pst_gate(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	bool flag = false;

	while (mstp_pst(mstp_index, port_number))
	{
		flag = true;
	}

 	if (flag)
	{
        mstp_pst_signal(mstp_index, port_number);
	}
}