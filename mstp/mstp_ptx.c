/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_ptx_mstiRootPort_and_not_tcWhile: utility function to check that no  */
/* instance root port has tcWhile active                                     */
/*****************************************************************************/
static bool mstp_ptx_mstiRootPort_and_not_tcWhile(MSTP_PORT * mstp_port)
{
	MSTP_MSTI_PORT *msti_port;
	MSTP_INDEX mstp_index;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port &&
			msti_port->co.role == MSTP_ROLE_ROOT &&
			msti_port->co.tcWhile != 0)
		{
			return true;
		}
	}
	return false;
}

/*****************************************************************************/
/* mstp_ptx_selected_and_not_updtInfo: utility function to check that all    */
/* instances have selected set and updtInfo cleared                          */
/*****************************************************************************/
static bool mstp_ptx_selected_and_not_updtInfo(MSTP_PORT *mstp_port)
{
	MSTP_INDEX mstp_index;
	MSTP_MSTI_PORT *msti_port;

	if (mstp_port->cist.co.selected && !mstp_port->cist.co.updtInfo)
	{
		for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
		{
			msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
			if (msti_port)
			{
				if (msti_port->co.selected && !msti_port->co.updtInfo)
				{
					continue;
				}

				return false;
			}
		}

		return true;
	}

	return false;
}

/*****************************************************************************/
/* mstp_ptx_action: performs actions associated with a specific state        */
/* machine state, trigger on transitions                                     */
/*****************************************************************************/
static void mstp_ptx_action(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL)
		return;

	switch (mstp_port->ptxState)
	{
		case MSTP_PTX_TRANSMIT_INIT:
			mstp_port->newInfoCist =
			mstp_port->newInfoMsti = true;
			mstp_port->txCount = 0;
			mstp_port->ptxState = MSTP_PTX_IDLE; // UCT
			break;

		case MSTP_PTX_TRANSMIT_PERIODIC:
			mstp_port->newInfoCist =
				(mstp_port->newInfoCist ||
				 (mstp_cistDesignatedPort(port_number) ||
				  (mstp_cistRootPort(port_number) && mstp_port->cist.co.tcWhile != 0)));
			mstp_port->newInfoMsti =
				(mstp_port->newInfoMsti ||
				 (mstp_mstiDesignatedPort(port_number) ||
				  mstp_ptx_mstiRootPort_and_not_tcWhile(mstp_port)));
			mstp_port->ptxState = MSTP_PTX_IDLE; // UCT
			break;

		case MSTP_PTX_TRANSMIT_CONFIG:
			mstp_port->newInfoCist = false;
			mstp_txConfig(port_number);
			mstp_port->txCount++;
			mstp_port->tcAck = false;
			mstp_port->ptxState = MSTP_PTX_IDLE; // UCT
			break;

		case MSTP_PTX_TRANSMIT_TCN:
			mstp_port->newInfoCist = false;
			mstp_txTcn(port_number);
			mstp_port->txCount++;
			mstp_port->ptxState = MSTP_PTX_IDLE; // UCT
			break;

		case MSTP_PTX_TRANSMIT_RSTP:
			mstp_port->newInfoCist =
			mstp_port->newInfoMsti = false;
			mstp_txMstp(port_number);
			mstp_port->txCount++;
			mstp_port->tcAck = false;
			mstp_port->ptxState = MSTP_PTX_IDLE; // UCT
			break;

		default:
			break;
	}
	
	if (mstp_port->ptxState == MSTP_PTX_IDLE)
	{
		mstp_port->helloWhen = mstp_port->cist.portTimes.helloTime;
	}
}

/*****************************************************************************/
/* mstp_ptx: executes the port transmit state machine for the port           */
/*****************************************************************************/
static bool mstp_ptx(PORT_ID port_number)
{
	bool flag = false;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PTX_STATE old_state;

	if (mstp_bridge == NULL)
		return flag;

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("Port %d mstp_port is NULL", port_number);
		return flag;
	}

	old_state = mstp_port->ptxState;

	if (mstp_ptx_selected_and_not_updtInfo(mstp_port))
	{
		switch (mstp_port->ptxState)
		{
			case MSTP_PTX_IDLE:
				if (mstp_port->helloWhen == 0)
				{
					mstp_port->ptxState = MSTP_PTX_TRANSMIT_PERIODIC;
					flag = true;
				}
				else
				if (mstp_port->txCount < mstp_bridge->txHoldCount &&
					mstp_port->helloWhen != 0)
				{
					if (mstp_port->sendRSTP)
					{
						/* See fig. 13-12 Page 181*/
						if (mstp_port->newInfoCist ||
							(mstp_port->newInfoMsti &&
							!mstp_mstiMasterPort(port_number)))
						{
							mstp_port->ptxState = MSTP_PTX_TRANSMIT_RSTP;
							flag = true;
						}
					}
					else
					{
						if (mstp_port->newInfoCist)
						{
							if (mstp_cistRootPort(port_number))
							{
								mstp_port->ptxState = MSTP_PTX_TRANSMIT_TCN;
								flag = true;
							}
							else
							if (mstp_cistDesignatedPort(port_number))
							{
								mstp_port->ptxState = MSTP_PTX_TRANSMIT_CONFIG;
								flag = true;
							}
						}
					}
				}
				break;

			default:
                STP_LOG_ERR("Port %d unknown state %u", port_number,mstp_port->ptxState);
				break;
		}
	}

	if (flag)
	{
		if (MSTP_DEBUG_PORT_EVENT(port_number))
		{
            if(old_state != mstp_port->ptxState)  
			{  
                STP_LOG_INFO("Port %d PTX %s->%s",port_number,
                    MSTP_PTX_STATE_STRING(old_state),MSTP_PTX_STATE_STRING(mstp_port->ptxState));
			}
		}
		mstp_ptx_action(port_number);
	}
	return flag;
}

/*****************************************************************************/
/* mstp_ptx_init: routine to initialize the port transmit state machine for  */
/* the input port                                                            */
/*****************************************************************************/
void mstp_ptx_init(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("Port %d mstp_port NULL", port_number);
		return;
	}

	mstp_port->ptxState = MSTP_PTX_TRANSMIT_INIT;
	mstp_ptx_action(port_number);
}

/*****************************************************************************/
/* mstp_ptx_gate: entry point to the port transmit state machine for the     */
/* specified port                                                            */
/*****************************************************************************/
void mstp_ptx_gate(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool flag = false;
    MSTP_COMMON_BRIDGE *cbridge;

    if(mstp_port == NULL)
        return;

    if (mstp_ptx_selected_and_not_updtInfo(mstp_port) && !mstp_port->portEnabled)
	{
		mstp_port->ptxState = MSTP_PTX_TRANSMIT_INIT;
		flag = true;
	}
	
	while (mstp_ptx(port_number))
	{
		flag = true;
	}
}
