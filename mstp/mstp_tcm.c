/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_tcm_action: performs actions associated with tcm state               */
/*****************************************************************************/
void mstp_tcm_action(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);

	if (mstp_port == NULL)
	{
		return;
	}

	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}
	
	switch (cport->tcmState)
	{
		case MSTP_TCM_INACTIVE:
			cport->fdbFlush = true; 
			cport->tcWhile = 0;
			if (cist)
			{
				mstp_port->tcAck = false;
			}
			break;

		case MSTP_TCM_LEARNING:
			if (cist)
			{
				mstp_port->rcvdTcn = mstp_port->rcvdTcAck = false;
			}
			cport->rcvdTc =
			cport->tcProp = false;
			break;

		case MSTP_TCM_DETECTED:
			mstp_newTcWhile(mstp_index, port_number);
			mstp_setTcPropTree(mstp_index, port_number);	
			if (cist)
				mstp_port->newInfoCist = true;
			else
				mstp_port->newInfoMsti = true;	
			cport->tcmState = MSTP_TCM_ACTIVE;			// UCT
			STP_LOG_INFO("[MST %d] Port %d Tcm detected",mstputil_get_mstid(mstp_index), port_number);		
			break;

		case MSTP_TCM_NOTIFIED_TCN:
			mstp_newTcWhile(mstp_index, port_number);
			cport->tcmState = MSTP_TCM_NOTIFIED_TC;		// UCT
			// fall thru

		case MSTP_TCM_NOTIFIED_TC:
			if (cist)
				mstp_port->rcvdTcn = false;
			cport->rcvdTc = false;
			if (cist && (cport->role == MSTP_ROLE_DESIGNATED))
				mstp_port->tcAck = true;
			mstp_setTcPropTree(mstp_index, port_number);
			cport->tcmState = MSTP_TCM_ACTIVE;			// UCT
			break;

		case MSTP_TCM_PROPAGATING:
			mstp_newTcWhile(mstp_index, port_number);
			cport->fdbFlush = true;
			cport->tcProp = false;
			cport->tcmState = MSTP_TCM_ACTIVE;			// UCT
			break;

		case MSTP_TCM_ACKNOWLEDGED:
			cport->tcWhile = 0;
			mstp_port->rcvdTcAck = false;
			cport->tcmState = MSTP_TCM_ACTIVE;			// UCT
			break;

		default:
			break;
	}
	
	if (cport->fdbFlush)
	{
		if (mstp_port->operEdge)
		{
			cport->fdbFlush = false;
			return;
		}
		
		if (true == mstp_flush(mstp_index, port_number))
		{
		    cport->fdbFlush = false;
		}
	}
}

/*****************************************************************************/
/* mstp_tcm: executes the topology change state machine                      */
/*****************************************************************************/
bool mstp_tcm(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	bool flag = false;
	bool cist = MSTP_IS_CIST_INDEX(mstp_index);
	MSTP_TCM_STATE old_state;

	if (mstp_port == NULL)
	{
		return false;
	}
	cport = mstputil_get_common_port(mstp_index, mstp_port);	
	if (cport == NULL)
	{
		return false;
	}
	old_state = cport->tcmState;
	switch (cport->tcmState)
	{
		case MSTP_TCM_INACTIVE:
			if (cport->learn && !cport->fdbFlush)
			{
				cport->tcmState = MSTP_TCM_LEARNING;
				flag = true;
			}
			break;
			
		case MSTP_TCM_LEARNING:
			if (((cport->role == MSTP_ROLE_ROOT) ||
				 (cport->role == MSTP_ROLE_DESIGNATED) ||
				 (cport->role == MSTP_ROLE_MASTER)) &&
				cport->forward && !mstp_port->operEdge)
			{
				cport->tcmState = MSTP_TCM_DETECTED;
				flag = true;
			}
			else
			if ((cist && (mstp_port->rcvdTcn || mstp_port->rcvdTcAck)) ||
				cport->rcvdTc ||
				cport->tcProp)
			{
				flag = true;
			}
			else
			if ((cport->role != MSTP_ROLE_DESIGNATED) &&
				(cport->role != MSTP_ROLE_ROOT) &&
				(cport->role != MSTP_ROLE_MASTER) &&
				!(cport->learn || cport->learning) &&
				!(cport->rcvdTc || 
				  mstp_port->rcvdTcn || 
				  mstp_port->rcvdTcAck ||
				  cport->tcProp))
			{
				cport->tcmState = MSTP_TCM_INACTIVE;
				flag = true;
			}
			break;

		case MSTP_TCM_ACTIVE:
			if (((cport->role != MSTP_ROLE_DESIGNATED) &&
				 (cport->role != MSTP_ROLE_ROOT) &&
				 (cport->role != MSTP_ROLE_MASTER)) ||
				mstp_port->operEdge)
			{
				cport->tcmState = MSTP_TCM_LEARNING;
				flag = true;
			}
			else
			if (cist && mstp_port->rcvdTcn)
			{
				cport->tcmState = MSTP_TCM_NOTIFIED_TCN;
				flag = true;
			}
			else
			if (cport->rcvdTc)
			{
				cport->tcmState = MSTP_TCM_NOTIFIED_TC;
				flag = true;
			}
			else
			if (cport->tcProp && !mstp_port->operEdge)
			{
				cport->tcmState = MSTP_TCM_PROPAGATING;
				flag = true;
			}
			else
			if (mstp_port->rcvdTcAck)
			{
				cport->tcmState = MSTP_TCM_ACKNOWLEDGED;
				flag = true;
			}			
			break;

		default:
			break;
	}

	if (flag)
	{
		if(old_state != cport->tcmState)
		{
			STP_LOG_INFO("[MST %d] Port %d TCM %s->%s", mstputil_get_mstid( mstp_index), port_number,		
				MSTP_TCM_STATE_STRING(old_state),
				MSTP_TCM_STATE_STRING(cport->tcmState));
		}
		mstp_tcm_action(mstp_index, port_number);
	}
	return flag;
}

/*****************************************************************************/
/* mstp_tcm_init: initializes topology change machine for the port           */
/*****************************************************************************/
void mstp_tcm_init(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));

	if (cport == NULL)
	{
		return;
	}

	cport->tcmState = MSTP_TCM_INACTIVE;
	mstp_tcm_action(mstp_index, port_number);
}

/*****************************************************************************/
/* mstp_tcm_signal: signals to other state machines                          */
/*****************************************************************************/
void mstp_tcm_signal(MSTP_INDEX mstp_index, PORT_ID calling_port)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(calling_port);
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_COMMON_PORT *cport;
	PORT_ID port_number;
	bool retVal;

	if (mstp_bridge == NULL || mstp_port == NULL)
	{
		return;
	}

	for (port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
		 port_number != BAD_PORT_ID;
		 port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number))
	{
		if (port_number != calling_port)
		{
			cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));

			if ((cport == NULL) || (cport->state == DISABLED))
				continue;
			
			if (cport->tcProp)
			{
				do {
					retVal = mstp_tcm(mstp_index, port_number);
				}while (retVal);
			}
		}
	}
	
	cport = mstputil_get_common_port(mstp_index, mstp_port);
	if (cport == NULL)
	{
		return;
	}
	/*
	 * tcWhile, newInfo, tcAck - PTX
	 */
	if (cport->tcWhile != 0 ||
		mstp_port->newInfoCist ||
		mstp_port->newInfoMsti ||
		mstp_port->tcAck)
	{
		mstp_ptx_gate(calling_port);
	}
}

/*****************************************************************************/
/* mstp_tcm_gate: entry point to invoke the topology change machine          */
/*****************************************************************************/
void mstp_tcm_gate(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	bool flag = false;
	VLAN_ID vlan_id;
	MSTP_COMMON_BRIDGE *cbridge;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL)
	{
		return;
	}

	while (mstp_tcm(mstp_index, port_number))
	{
		flag = true;
	}

	if (flag)
	{
		MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstp_port);

		if (cport == NULL)
			return;
		
		mstp_tcm_signal(mstp_index, port_number);
	}
}
