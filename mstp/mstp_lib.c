/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstplib_port_is_admin_edge_port: returns true if the port is configured   */
/* as an edge port (ie connected to an end-host)                             */
/*****************************************************************************/
bool mstplib_port_is_admin_edge_port(PORT_ID port_number)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

	if (port_number >= MAX_PORT)
		return false;

	return IS_MEMBER(mstp_bridge->admin_edge_mask, port_number);
}

/*****************************************************************************/
/* mstplib_port_is_oper_edge_port: returns true if the port is an operational*/
/* edge port (ie connected to an end-host). not necessarily the same as the  */
/* administratively configured value                                         */
/*****************************************************************************/
bool mstplib_port_is_oper_edge_port(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	return mstp_port->operEdge;
}

/*****************************************************************************/
/* mstplib_get_num_actives_instances: returns the number of active mstp      */
/* instances                                                                 */
/*****************************************************************************/
UINT8 mstplib_get_num_active_instances()
{
    STP_LOG_DEBUG("Active MSTP instances %d",((g_mstp_global.max_instances) - (g_mstp_global.free_instances)));	
	
	return ((g_mstp_global.max_instances) - (g_mstp_global.free_instances));
}

/*****************************************************************************/
/* mstplib_get_max_instances: returns the max number of mstp instances       */
/*****************************************************************************/
UINT8 mstplib_get_max_instances()
{
	return (g_mstp_global.max_instances);
}

/*****************************************************************************/
/* mstplib_is_mstp_active: returns true if 'mstp start' has been executed    */
/* mstp is currently operational                                             */
/*****************************************************************************/
bool mstplib_is_mstp_active()
{
	MSTP_BRIDGE *mstp_bridge = NULL;

    mstp_bridge = mstpdata_get_bridge();

    if(mstp_bridge && mstp_bridge->active)
    {
        return true;
    }

    return false;
}

/*****************************************************************************/
/* mstplib_get_index_from_mstid: returns the mstp index associated with the  */
/* the mst instance id                                                       */
/*****************************************************************************/
MSTP_INDEX mstplib_get_index_from_mstid(MSTP_MSTID mstid)
{
	return mstputil_get_index(mstid);
}

/*****************************************************************************/
/* mstplib_is_root_bridge: returns true if this bridge is the root bridge    */
/* for the input mstid                                                       */
/*****************************************************************************/
bool mstplib_is_root_bridge(MSTP_MSTID mstid)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_MSTI_BRIDGE *msti_bridge;
	MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_COMMON_BRIDGE *cbridge;

	if (MSTP_IS_CIST_MSTID(mstid))
	{
        cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
        if(cist_bridge ==  NULL)
            return false;

        cbridge = &cist_bridge->co;
        if ((cist_bridge->co.rootPortId != MSTP_INVALID_PORT))
            return false;
    
        return true;
    }

	msti_bridge = mstputil_get_msti_bridge(mstid);
	if (msti_bridge == NULL)
		return false;

    cbridge = &msti_bridge->co;
	if (msti_bridge->co.rootPortId != MSTP_INVALID_PORT)
		return false;

	return true;
}

/*****************************************************************************/
/* mstplib_instance_get_vlanmask: returns the list of vlans associated with  */
/* the input mstp index                                                      */
/*****************************************************************************/
VLAN_MASK * mstplib_instance_get_vlanmask(MSTP_INDEX mstp_index)
{
	VLAN_MASK *vlanmask = NULL;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

	if (MSTP_IS_CIST_INDEX(mstp_index))
	{
		MSTP_CIST_BRIDGE *cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
		vlanmask = &cist_bridge->co.vlanmask;
	}
	else
	{
		MSTP_MSTI_BRIDGE *msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
		if (msti_bridge)
		{
			vlanmask = &msti_bridge->co.vlanmask;
		}
	}

	return vlanmask;
}

/*****************************************************************************/
/* mstplib_get_next_mstid: gets the next configured mstid (returns           */
/* MSTP_MSTID_INVALID if no more mstid configured).                          */
/*****************************************************************************/
MSTP_MSTID mstplib_get_next_mstid(MSTP_MSTID prev_mstid)
{
	MSTP_MSTID mstid, start_mstid;

	if (prev_mstid == MSTP_MSTID_INVALID)
		start_mstid = MSTP_MSTID_CIST;
	else
		start_mstid = prev_mstid + 1;

	for (mstid = start_mstid; mstid <= MSTP_MSTID_MAX; mstid++)
	{
		if (mstputil_get_index(mstid) != MSTP_INDEX_INVALID)
			return mstid;
	}

	return MSTP_MSTID_INVALID;
}

/*****************************************************************************/
/* mstplib_get_first_mstid: returns the first configured mstid               */
/*****************************************************************************/
MSTP_MSTID mstplib_get_first_mstid()
{
	return MSTP_MSTID_CIST;
}
/********************************************************************************/
/* mstplib_instance_get_portmask: returns portmask associated with mst instance */
/********************************************************************************/
PORT_MASK * mstplib_instance_get_portmask(MSTP_INDEX mstp_index)
{
	PORT_MASK *portmask = NULL;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

	if (MSTP_IS_CIST_INDEX(mstp_index))
	{
		MSTP_CIST_BRIDGE *cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
		portmask = cist_bridge->co.portmask;
	}
	else
	{
		MSTP_MSTI_BRIDGE *msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
		if (msti_bridge)
		{
			portmask = msti_bridge->co.portmask;
		}
	}

	return portmask;
}
/***********************************************************************************/
/* mstplib_get_port_state: returns port state of mst instance                      */
/***********************************************************************************/
bool mstplib_get_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE *state)
{
    MSTP_COMMON_PORT *cport;

    cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
    if (cport == NULL)
        return false;

    *state = (cport->state);
    return true;
}