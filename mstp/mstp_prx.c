/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_prx_signal: signals to other state machines that could be involved   */
/*****************************************************************************/
static void mstp_prx_signal(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	MSTP_INDEX mstp_index;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_COMMON_PORT *cport;
	MSTI_CONFIG_MESSAGE *msg;
	UINT16 i;

	if (mstp_port == NULL)
		return;
	
	cport = mstputil_get_common_port(MSTP_INDEX_CIST, mstp_port);
	if (cport == NULL)
		return;

	// rcvdRSTP, rcvdSTP - PPM
	if (mstp_port->rcvdRSTP ||
		mstp_port->rcvdSTP)
	{
		mstp_ppm_gate(port_number);
	}

	// rcvdMsg - PIM
	if (cport->rcvdMsg)
    {
        mstp_pim_gate(MSTP_INDEX_CIST, port_number, (void *) bpdu);
    }

    if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
    {
        if (mstp_port->rcvdInternal)
        {
            // rcvdMsg - PIM
            for (i = 0; i < MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(bpdu->v3_length); i++)
            {
                msg = &bpdu->msti_msgs[i];

                mstp_index = mstputil_get_index(msg->msti_regional_root.system_id);
                if (mstp_index == MSTP_INDEX_INVALID)
                    continue;

                cport = mstputil_get_common_port(mstp_index, mstp_port);
                if (cport == NULL)
                    continue;

                if (cport->rcvdMsg)
                {
                    mstp_pim_gate(mstp_index, port_number, (void *) msg);
                }
            }
        }
        else
        {
            for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
            {
                mstp_pim_gate(mstp_index, port_number, NULL);
                mstp_prt_gate(mstp_index, port_number);
            }
        }
    }
}

/*****************************************************************************/
/* mstp_prx_action: performs the actions associated with a prx state         */
/*****************************************************************************/
static void mstp_prx_action(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	bool flag = false;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
    MSTP_COMMON_BRIDGE *cbridge;

	flag = mstp_port->rcvdInternal;

	switch (mstp_port->prxState)
	{
		case MSTP_PRX_DISCARD:
			mstp_port->rcvdBpdu =
			mstp_port->rcvdRSTP =
			mstp_port->rcvdSTP = false;
			mstp_clearAllRcvdMsgs(port_number);
			break;

		case MSTP_PRX_RECEIVE:
			mstp_updtBpduVersion(mstp_port, bpdu);
			mstp_port->rcvdInternal = mstp_fromSameRegion(mstp_port, bpdu);
            mstp_setRcvdMsgs(port_number, bpdu);

            mstp_port->operEdge = mstp_port->rcvdBpdu = false;
            break;

		default:
			break;
	}

	// Trigger reselect when rcvdInternal toggles
	if (mstp_port->rcvdInternal != flag)
	{
		// boundary port <-> internal port
        STP_LOG_INFO("Port %d transitions to %s port", port_number,
            mstp_port->rcvdInternal ? "internal" : " boundary");

        if(mstp_port->rcvdRSTP)
        {
            stpsync_update_boundary_port(stp_intf_get_port_name(mstp_port->port_number), 
                    !mstp_port->rcvdInternal, "RSTP") ;
        }
        else if (mstp_port->rcvdSTP)
        {
            stpsync_update_boundary_port(stp_intf_get_port_name(mstp_port->port_number),
                    !mstp_port->rcvdInternal, "STP");
        }

        // trigger reselect for all instances
        mstputil_setReselectAll(port_number);
	}
}

/*****************************************************************************/
/* mstp_prx: executes the port receive state machine.                        */
/*****************************************************************************/
static bool mstp_prx(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	bool flag = false;
	MSTP_PRX_STATE old_state;

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("Port %d mstp_port NULL", port_number);
		return flag;
	}

	old_state = mstp_port->prxState;
	switch (mstp_port->prxState)
	{
		case MSTP_PRX_DISCARD:
			if (mstp_port->rcvdBpdu)
			{
				flag = true;
				if (mstp_port->portEnabled)
				{
					mstp_port->prxState = MSTP_PRX_RECEIVE;
				}
			}
			break;

		case MSTP_PRX_RECEIVE:
			if (mstp_port->rcvdBpdu && 
				mstp_port->portEnabled && 
				!mstp_rcvdAnyMsg(port_number))
			{
				flag = true;
			}
			break;

		default:
			break;
	}

	if (flag)
	{
        if(old_state != mstp_port->prxState)
            STP_LOG_INFO("Port %d PRX %s->%s", port_number,
                MSTP_PRX_STATE_STRING(old_state), MSTP_PRX_STATE_STRING(mstp_port->prxState));

		mstp_prx_action(port_number, bpdu);
	}

	return flag;
}

/*****************************************************************************/
/* mstp_prx_init: initializes the port receive state machine                 */
/*****************************************************************************/
void mstp_prx_init(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port == NULL)
	{
        STP_LOG_ERR("Port %d mstp_port NULL", port_number);
		return;
	}
	mstp_port->prxState = MSTP_PRX_DISCARD;
	mstp_prx_action(port_number, NULL);
}

/*****************************************************************************/
/* mstp_prx_gate: invokes the port receive state machine                     */
/*****************************************************************************/
void mstp_prx_gate(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	bool flag = false;
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

	if (mstp_port->rcvdBpdu && !mstp_port->portEnabled )
		mstp_port->prxState = MSTP_PRX_DISCARD;

	while (mstp_prx(port_number, bpdu))
	{
		flag = true;
	}

	if (flag)
	{
		mstp_prx_signal(port_number, bpdu);
	}
}
