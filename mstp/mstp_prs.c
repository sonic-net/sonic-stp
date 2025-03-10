/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp_prs_signal: signals to port machines associated with the input mstp  */
/* instance                                                                  */
/*****************************************************************************/
static void mstp_prs_signal(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_MSTI_BRIDGE *msti_bridge;
	MSTP_COMMON_PORT *cport;
	PORT_ID port_number;
	PORT_MASK *mask = 0;
    PORT_MASK_LOCAL l_tmpmask;
    PORT_MASK *tmpmask = portmask_local_init(&l_tmpmask);

	bool cist = MSTP_IS_CIST_INDEX(mstp_index);

	if (cist)
	{
		mask = mstp_bridge->enable_mask;
	}
	else
	{
		msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
		clear_mask(tmpmask);
		and_masks(tmpmask, msti_bridge->co.portmask, mstp_bridge->enable_mask);
		mask = tmpmask;
	}

	port_number = port_mask_get_first_port(mask);
	while (port_number != BAD_PORT_ID)
	{
		cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
		if (cport != NULL)
		{
			/*
			 * selected, updtInfo, changedMaster - PIM
			 * designatedPriority, designatedTimes do not trigger any transition
			 * in the PIM state machine.
			 */
			if (cport->selected ||
				cport->updtInfo ||
				cport->changedMaster)
			{
				mstp_pim_gate(mstp_index, port_number, NULL);
			}

			/*
			 * selectedRole, selected, updtInfo - PRT
			 * selectedRole will cause a transition only when it is not equal to
			 * role.
			 */
			if (cport->selected ||
				!cport->updtInfo ||
				cport->selectedRole != cport->role)
			{
				mstp_prt_gate(mstp_index, port_number);
			}
		}
		port_number = port_mask_get_next_port(mask, port_number);
	}
}

/*****************************************************************************/
/* mstp_prs_action: performs actions associated with the role selection state*/
/*****************************************************************************/
static void mstp_prs_action(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE *cbridge;

	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge == NULL)
		return;

	switch (cbridge->prsState)
	{
		case MSTP_PRS_INIT_BRIDGE:
			mstp_updtRolesDisabledTree(mstp_index);
			cbridge->prsState = MSTP_PRS_ROLE_SELECTION;
			// fall thru

		case MSTP_PRS_ROLE_SELECTION:
			mstp_clearReselectTree(mstp_index);

			if (MSTP_IS_CIST_INDEX(mstp_index))
				mstp_updtRolesCist(mstp_index);
			else
				mstp_updtRolesMsti(mstp_index);

			mstp_setSelectedTree(mstp_index);
			break;

		default:
			break;
	}
}

/*****************************************************************************/
/* mstp_prs: checks for transitions associated with the role selection state */
/*****************************************************************************/
static bool mstp_prs(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE *cbridge;
	bool flag = false;
    MSTP_PRS_STATE old_state;

	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge == NULL)
		return false;

    old_state = cbridge->prsState;

	switch (cbridge->prsState)
	{
		case MSTP_PRS_ROLE_SELECTION:
			flag = mstputil_computeReselect(mstp_index);
			break;

		default:
			break;
	}

	if (flag)
	{
		if(old_state != cbridge->prsState)
		{
            STP_LOG_INFO("[MST %d]PRS %s->%s",mstputil_get_mstid(mstp_index),
                    MSTP_PRS_STATE_STRING(old_state), MSTP_PRS_STATE_STRING(cbridge->prsState));
		}

		mstp_prs_action(mstp_index);
	}

	return flag;
}

/*****************************************************************************/
/* mstp_prs_init: initializes the port role selection state machine          */
/*****************************************************************************/
void mstp_prs_init(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE *cbridge;

	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge == NULL)
		return;
	cbridge->prsState = MSTP_PRS_INIT_BRIDGE;

	mstp_prs_action(mstp_index);

	/* kick start all other state machines */
	mstp_prs_signal(mstp_index);
}

/*****************************************************************************/
/* mstp_prs_gate2: invokes the port role selection state machine              */
/*****************************************************************************/
void mstp_prs_gate2(MSTP_INDEX mstp_index)
{
	MSTP_INDEX index;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_MSTI_BRIDGE *msti_bridge;
	bool flag = false;

	while (mstp_prs(mstp_index))
	{
		flag = true;
	}

	if (flag)
	{
		mstp_prs_signal(mstp_index);
	}
}

/*****************************************************************************/
/* mstp_prs_gate: invokes the port role selection state machine              */
/*****************************************************************************/
void mstp_prs_gate(MSTP_INDEX mstp_index)
{
	MSTP_INDEX index;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_MSTI_BRIDGE *msti_bridge;
	bool flag = false;

	while (mstp_prs(mstp_index))
	{
		flag = true;
	}

	if (flag)
	{
		mstp_prs_signal(mstp_index);
		
		// signaling from CIST to MSTI see earlier part in
		// mstputil_set_changedMaster() function.
		if (MSTP_IS_CIST_INDEX(mstp_index))
		{
			mstp_bridge = mstpdata_get_bridge();
			for (index = MSTP_INDEX_MIN; index <= MSTP_INDEX_MAX; index++)
			{
				msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, index);
				if (msti_bridge == NULL)
					continue;
				mstp_prs_gate2(index);
			}
		}
	}
}
