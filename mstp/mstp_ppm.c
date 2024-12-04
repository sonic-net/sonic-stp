/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_ppm_action: performs actions associated with specific                */
/* port migration state machine states                                       */
/*****************************************************************************/
static void mstp_ppm_action(PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL || mstp_bridge == NULL)
		return;

	switch (mstp_port->ppmState)
	{
		case MSTP_PPM_CHECKING_RSTP:
			mstp_port->mcheck = false;
			mstp_port->sendRSTP = (mstp_bridge->forceVersion >= RSTP_VERSION_ID);
			mstp_port->mdelayWhile = MSTP_DFLT_MIGRATE_TIME;
			break;

		case MSTP_PPM_SELECTING_STP:
			mstp_port->sendRSTP = false;
			mstp_port->mdelayWhile = MSTP_DFLT_MIGRATE_TIME;
			break;

		case MSTP_PPM_SENSING:
			mstp_port->rcvdRSTP =
			mstp_port->rcvdSTP = false;
			break;

		default:
			break;
	}
}

/*****************************************************************************/
/* mstp_ppm: checks for transition conditions for ppm                        */
/*****************************************************************************/
static bool mstp_ppm(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	bool flag = false;
	MSTP_PPM_STATE old_state;

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("Port %d mstp_port NULL", port_number);
		return flag;
	}

	old_state = mstp_port->ppmState;

	if (mstp_bridge == NULL)
	{
        STP_LOG_ERR("Port %d mstp_bridge NULL", port_number);
		return flag;
	}

	switch (mstp_port->ppmState)
	{
		case MSTP_PPM_CHECKING_RSTP:
			if ((mstp_port->mdelayWhile != MSTP_DFLT_MIGRATE_TIME) && 
				!mstp_port->portEnabled)
			{
					flag = true;
			}
			else
			if (!mstp_port->mdelayWhile)
			{
				mstp_port->ppmState = MSTP_PPM_SENSING;
				flag = true;
			}	
			break;

		case MSTP_PPM_SELECTING_STP:
			if (!mstp_port->mdelayWhile ||
				!mstp_port->portEnabled ||
				mstp_port->mcheck)
			{
				mstp_port->ppmState = MSTP_PPM_SENSING;
				flag = true;
			}
			break;

		case MSTP_PPM_SENSING:
			if (!mstp_port->portEnabled ||
				mstp_port->mcheck ||
				((mstp_bridge->forceVersion >= RSTP_VERSION_ID) &&
				 !mstp_port->sendRSTP && mstp_port->rcvdRSTP))
			{
				mstp_port->ppmState = MSTP_PPM_CHECKING_RSTP;
				flag = true;
			}
			else
			if (mstp_port->sendRSTP && mstp_port->rcvdSTP)
			{
				mstp_port->ppmState = MSTP_PPM_SELECTING_STP;
				flag = true;
			}
			break;

		default:
            STP_LOG_ERR("Port %d unknown ppmState %u",port_number,mstp_port->ppmState);
			break;
	}

	if (flag)
	{
        if(old_state != mstp_port->ppmState)
		{
            STP_LOG_INFO("Port %d PPM %s->%s",port_number,		
				MSTP_PPM_STATE_STRING(old_state),
				MSTP_PPM_STATE_STRING(mstp_port->ppmState));
		}
		mstp_ppm_action(port_number);
	}

	return flag;
}

/*****************************************************************************/
/* mstp_ppm_init: initializes the port migration state machine               */
/*****************************************************************************/
void mstp_ppm_init(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL)
		return;
	mstp_port->ppmState = MSTP_PPM_CHECKING_RSTP;
	mstp_ppm_action(port_number);
}

/*****************************************************************************/
/* mstp_ppm_gate: invokes the port migration state machine for input port    */
/*****************************************************************************/
void mstp_ppm_gate(PORT_ID port_number)
{
	bool flag = false;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	
	while (mstp_ppm(port_number))
	{
		flag = true;
	}

	if (flag)
	{
		mstp_ptx_gate(port_number);
	}
}
