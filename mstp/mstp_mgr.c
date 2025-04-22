/*
 * Copyright 2025 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

extern MAC_ADDRESS g_stp_base_mac_addr;
UINT8 g_stp_base_mac[L2_ETH_ADD_LEN];

/*****************************************************************************/
/* mstpmgr_init_mstp_bridge: initializes mstp bridge structure               */
/*****************************************************************************/
static bool mstpmgr_init_mstp_bridge(UINT16 max_instances)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_MSTID mstid;
    VLAN_ID vlan_id; 
    MSTP_GLOBAL *mstp_global = &g_mstp_global;
    MSTP_COMMON_BRIDGE *cbridge;

    mstp_bridge = mstpdata_alloc_mstp_bridge();
    if (mstp_bridge == NULL)
    {
        STP_LOG_ERR("Alloc mstp_bridge failed");
        return false;
    }

    // initialize non-zero values in mstp_bridge
    for (mstid = MSTP_MSTID_MIN; mstid <= MSTP_MSTID_MAX; mstid++)
    {
        MSTP_SET_INSTANCE_INDEX(mstp_bridge, mstid, MSTP_INDEX_INVALID);
    }

    MSTP_SET_INSTANCE_INDEX(mstp_bridge, MSTP_MSTID_CIST, MSTP_INDEX_CIST);

    mstp_global->max_instances = mstp_global->free_instances = (UINT8) max_instances;
    mstp_bridge->forceVersion = MSTP_DFLT_COMPATIBILITY_MODE;
    mstp_bridge->fwdDelay = MSTP_DFLT_FORWARD_DELAY;
    mstp_bridge->maxHops = MSTP_DFLT_MAX_HOPS;
    mstp_bridge->txHoldCount = MSTP_DFLT_TXHOLDCOUNT;
    mstp_bridge->helloTime = MSTP_DFLT_HELLO_TIME;

    // init mstConfigId
    mstputil_get_default_name(mstp_bridge->mstConfigId.name,
        sizeof(mstp_bridge->mstConfigId.name));
    mstputil_compute_message_digest(false);

    PORT_MASK_SET_ALL(mstp_bridge->admin_pt2pt_mask);

    // initialize non-zero values in the cist bridge structure
    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

    cist_bridge->co.mstid = MSTP_MSTID_CIST;

    cbridge = &cist_bridge->co; 
    
    NET_TO_HOST_MAC(&cist_bridge->co.bridgeIdentifier.address, &g_stp_base_mac_addr);
    
    cist_bridge->co.bridgeIdentifier.priority = MSTP_ENCODE_PRIORITY(MSTP_DFLT_PRIORITY);

    cist_bridge->co.rootPortId = MSTP_INVALID_PORT;
    vlan_bmp_init(&cist_bridge->co.vlanmask);
    bmp_alloc(&cist_bridge->co.portmask, g_max_stp_port);

    cist_bridge->bridgePriority.root = cist_bridge->co.bridgeIdentifier;
    cist_bridge->bridgePriority.extPathCost = 0;
    cist_bridge->bridgePriority.regionalRoot = cist_bridge->co.bridgeIdentifier;
    cist_bridge->bridgePriority.intPathCost = 0;
    cist_bridge->bridgePriority.designatedId = cist_bridge->co.bridgeIdentifier;
    cist_bridge->bridgePriority.designatedPort.number = MSTP_INVALID_PORT;
    cist_bridge->bridgePriority.designatedPort.priority = MSTP_ENCODE_PORT_PRIORITY(MSTP_DFLT_PORT_PRIORITY);

    cist_bridge->rootPriority = cist_bridge->bridgePriority;

    cist_bridge->bridgeTimes.fwdDelay = mstp_bridge->fwdDelay;
    cist_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;
    cist_bridge->bridgeTimes.maxAge = MSTP_DFLT_MAX_AGE;
    cist_bridge->bridgeTimes.messageAge = 0;
    cist_bridge->bridgeTimes.helloTime = mstp_bridge->helloTime;

    cist_bridge->rootTimes = cist_bridge->bridgeTimes;
    SET_ALL_BITS(cist_bridge->modified_fields);

    mstp_global->bridge = mstp_bridge;

    return true;
}

/*****************************************************************************/
/* mstpmgr_init_bpdu: initializes the bpdu structure                         */
/*****************************************************************************/
static void mstpmgr_init_bpdu()
{
    MSTP_BPDU *bpdu;

    bpdu = mstpdata_get_bpdu();

    HOST_TO_NET_MAC(&bpdu->mac_header.destination_address, &bridge_group_address);
    bpdu->llc_header.destination_address_DSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
    bpdu->llc_header.source_address_SSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
    bpdu->llc_header.llc_frame_type = (enum LLC_FRAME_TYPE) UNNUMBERED_INFORMATION;
}

/*****************************************************************************/
/* mstpmgr_init_debug: initializes debugging structure                       */
/*****************************************************************************/
static void mstpmgr_init_debug()
{
    int8_t ret = 0;
    memset(&debugGlobal.mstp, 0, sizeof(DEBUG_MSTP));

    debugGlobal.mstp.all_ports =
        debugGlobal.mstp.all_instance =
        debugGlobal.mstp.bpdu_tx =
        debugGlobal.mstp.bpdu_rx =
        debugGlobal.mstp.event =
        debugGlobal.mstp.verbose = false;

    debugGlobal.mstp.all_instance = true;

    ret |= bmp_alloc(&debugGlobal.mstp.instance_mask, MSTP_MAX_INSTANCES_PER_REGION);
    ret |= bmp_alloc(&debugGlobal.mstp.port_mask, g_max_stp_port);
    if (ret != 0)
    {
        if(debugGlobal.mstp.instance_mask)
        {
            bmp_free(debugGlobal.mstp.instance_mask);
        }
        STP_LOG_ERR("bmp_alloc Failed");
        return;
    }
}

/*****************************************************************************/
/* mstpmgr_init: initializes mstp for max number of instances                */
/*****************************************************************************/
bool mstpmgr_init()
{
    bool    ret_flag = false;

    // reset global structure.
    mstpdata_reset();

    g_mstp_global.port_arr   = (MSTP_PORT **)calloc(1,(g_max_stp_port * sizeof(MSTP_PORT *)));
    if(!g_mstp_global.port_arr)
    {
        STP_LOG_ERR("Allocate MSTP port array failed");
        return false;
    }
    if (!mstpmgr_init_mstp_bridge(MSTP_MAX_INSTANCES_PER_REGION))
    {
        STP_LOG_ERR("Init Bridge failed");
        free(g_mstp_global.port_arr);
        return false;
    }

    mstpmgr_init_bpdu();	
    mstpmgr_init_debug();
    mstpdata_port_vlan_db_init();
    mstpdata_vlan_port_db_init();

    STP_LOG_INFO("Init success");
    return true;
}

/*****************************************************************************/
/* mstpmgr_instance_init_port_state_machines: initializes all relevant port  */
/* state machines for the input mst index                                    */
/*****************************************************************************/
void mstpmgr_instance_init_port_state_machines(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    mstp_pim_init(mstp_index, port_number);
    mstp_prt_init(mstp_index, port_number);
    mstp_pst_init(mstp_index, port_number);
    mstp_tcm_init(mstp_index, port_number);
}

/*****************************************************************************/
/* mstpmgr_init_port_state_machines: initializes all relevant port           */
/* state machines for the input port                                         */
/*****************************************************************************/
static void mstpmgr_init_port_state_machines(PORT_ID port_number)
{
    mstp_ppm_init(port_number);
    mstp_ptx_init(port_number);
    mstp_prx_init(port_number);
    mstpmgr_instance_init_port_state_machines(MSTP_INDEX_CIST, port_number);
}

/*****************************************************************************/
/* mstpmgr_init_mstp_port: initializes mstp port structure for input port    */
/*****************************************************************************/
static bool mstpmgr_init_mstp_port(PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_PORT *cist_port;
    MSTP_PORT *mstp_port;
    MSTP_COMMON_BRIDGE *cbridge;

    // sanity check for presence in configured mask
    if (IS_MEMBER(mstp_bridge->control_mask, port_number))
    {
        return true;
    }

    // sanity check for allocated data structure
    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port != NULL)
    {
        STP_LOG_ERR("[Port %d] mstp_port is present for an unconfigured port",port_number);
        return false;
    }

    // allocate port structure
    mstp_port = mstpdata_alloc_mstp_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("[Port %d] Unable to allocate mstp port", port_number);
        return false;
    }

    // initialize port structure variables.
    mstp_port->port_number = port_number;
    mstp_port->operEdge = IS_MEMBER(mstp_bridge->admin_edge_mask, port_number) ? true : false;
    mstp_port->operPt2PtMac = IS_MEMBER(mstp_bridge->admin_pt2pt_mask, port_number) ? true : false;
    mstp_port->restrictedRole = STP_IS_ROOT_PROTECT_CONFIGURED(port_number);

    cist_port = MSTP_GET_CIST_PORT(mstp_port);
    cist_port->extPortPathCost = stp_intf_get_inst_port_pathcost(port_number, MSTP_INDEX_CIST);
    cist_port->co.portId.priority = stp_intf_get_inst_port_priority(port_number, MSTP_INDEX_CIST);
    cist_port->co.portId.number = port_number;
    cist_port->co.autoConfigPathCost = stp_intf_is_default_port_pathcost(port_number, MSTP_INDEX_CIST);
    cist_port->co.intPortPathCost = cist_port->extPortPathCost;
    cist_port->co.state = DISABLED;
    cist_port->co.role = cist_port->co.selectedRole = MSTP_ROLE_DISABLED;
    
    // mark the port as configured
    set_mask_bit(mstp_bridge->control_mask, port_number);

    SET_ALL_BITS(cist_port->co.modified_fields);
    SET_ALL_BITS(cist_port->modified_fields);

    return true;
}

/*****************************************************************************/
/* mstpmgr_free_mstp_port: frees mstp port structure for input port          */
/*****************************************************************************/
static void mstpmgr_free_mstp_port(PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_PORT *mstp_port;
    char *ifname= stp_intf_get_port_name(port_number);
    MSTP_COMMON_PORT *cport;

    mstp_bridge = mstpdata_get_bridge();
    if(mstp_bridge == NULL)
    {
        STP_LOG_ERR("mstp_bridge NULL");
        return;
    }

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number))
    {
        return;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("[Port %d] mstp_port NULL",  port_number);
        return;
    }

    clear_mask_bit(mstp_bridge->control_mask, port_number);
    cport = mstputil_get_common_port(MSTP_INDEX_CIST, mstp_port);

    if(ifname && cport)
    {
        /* Delete CIST port from APP DB*/
        stpsync_del_mst_port_info(ifname, MSTP_MSTID_CIST);
        stpsync_del_port_state(ifname, MSTP_INDEX_CIST);
    }
    mstpdata_free_port(port_number);
}

/*****************************************************************************/
/* mstpmgr_init_msti: initializes mst instance bridge structure              */
/*****************************************************************************/
static bool mstpmgr_init_msti(MSTP_MSTID mstid)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_INDEX mstp_index;
    MSTP_MSTI_BRIDGE *msti_bridge;
    PORT_ID port_number;
    MSTP_COMMON_BRIDGE *cbridge;

    mstp_bridge = mstpdata_get_bridge();
    if(mstp_bridge == NULL)
    {
        STP_LOG_ERR("mstp_bridge NULL");
        return false;
    }

    // check if mst instance is already configured
    mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
    if (mstp_index != MSTP_INDEX_INVALID)
    {
        STP_LOG_ERR("[Mst %d] Already configured mstid", mstid);
        return false;
    }

    // check that mst instances are available.
    if (g_mstp_global.free_instances == 0)
    {
        STP_LOG_ERR("No mst instances are available");
        return false;
    }

    // get new index for instance
    mstp_index = mstpdata_get_new_index();
    if (mstp_index > MSTP_INDEX_MAX)
    {
        STP_LOG_ERR("No free mst instances available");
        return false;
    }

    msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
    if (msti_bridge == NULL)
    {
        STP_LOG_ERR("[Mst %d] msti_bridge null", mstid);
        return false;
    }

    cbridge = &msti_bridge->co;

    // map mstp index to the appropriate mstp instance id.
    MSTP_SET_INSTANCE_INDEX(mstp_bridge, mstid, mstp_index);

    // initialize non-zero values in msti bridge structure
    msti_bridge->co.mstid = mstid;
    msti_bridge->co.bridgeIdentifier.priority = MSTP_ENCODE_PRIORITY(MSTP_DFLT_PRIORITY);
    msti_bridge->co.bridgeIdentifier.system_id = mstid;

    NET_TO_HOST_MAC(&msti_bridge->co.bridgeIdentifier.address, &g_stp_base_mac_addr);

    msti_bridge->co.rootPortId = MSTP_INVALID_PORT;
    vlan_bmp_init(&msti_bridge->co.vlanmask);
    bmp_alloc(&msti_bridge->co.portmask, g_max_stp_port);

    msti_bridge->bridgePriority.designatedId =
        msti_bridge->bridgePriority.regionalRoot = msti_bridge->co.bridgeIdentifier;
    msti_bridge->bridgePriority.intPathCost = 0;
    msti_bridge->bridgePriority.designatedPort.number = MSTP_INVALID_PORT;
    msti_bridge->bridgePriority.designatedPort.priority = MSTP_ENCODE_PORT_PRIORITY(MSTP_DFLT_PORT_PRIORITY);

    msti_bridge->rootPriority = msti_bridge->bridgePriority;

    msti_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;

    msti_bridge->rootTimes = msti_bridge->bridgeTimes;
    SET_ALL_BITS(msti_bridge->modified_fields);

    return true;
}

/*****************************************************************************/
/* mstpmgr_free_msti: free mst instance bridge structure at index            */
/*****************************************************************************/
static void mstpmgr_free_msti(MSTP_INDEX mstp_index)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_MSTID mstid;
    PORT_ID port_number;

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        STP_LOG_ERR("[MST Index %d] Called with cist index", mstp_index);
        return;
    }

    mstp_bridge = mstpdata_get_bridge();

    msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
    if (msti_bridge == NULL)
    {
        STP_LOG_ERR("[MST Index %d] msti_bridge null", mstp_index);
        return;
    }

    mstid = MSTP_GET_MSTID_FROM_MSTI(msti_bridge);
    MSTP_SET_INSTANCE_INDEX(mstp_bridge, mstid, MSTP_INDEX_INVALID);

    stpsync_del_mst_info(mstid);
    mstpdata_free_index(mstp_index);
}

/*****************************************************************************/
/* mstpmgr_init_msti_port: initializes mst instance port structure           */
/*****************************************************************************/
static MSTP_MSTI_PORT *mstpmgr_init_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    MSTP_MSTI_PORT *msti_port;
    MSTP_COMMON_BRIDGE *cbridge;

    msti_port = mstpdata_alloc_msti_port(mstp_index, port_number);
    if (msti_port == NULL)
    {
        STP_LOG_ERR("[MST Index %u] Port %d Alloc msti port failed", mstp_index, port_number);
        return NULL;
    }

    // initialize non-zero values
    msti_port->co.portId.priority = stp_intf_get_inst_port_priority(port_number, mstp_index);
    msti_port->co.portId.number = port_number;
    msti_port->co.autoConfigPathCost = stp_intf_is_default_port_pathcost(port_number, mstp_index);
    msti_port->co.intPortPathCost = stp_intf_get_inst_port_pathcost(port_number, mstp_index);
    msti_port->co.state = DISABLED;
    msti_port->co.role = msti_port->co.selectedRole = MSTP_ROLE_DISABLED;

    SET_ALL_BITS(msti_port->co.modified_fields);
    SET_ALL_BITS(msti_port->modified_fields);

    return msti_port;
}

/*****************************************************************************/
/* mstpmgr_free_msti_port: frees mst instance port structure                 */
/*****************************************************************************/
static void mstpmgr_free_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_MSTI_BRIDGE *msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);

    if (msti_bridge == NULL)
    {
        return;
    }

    clear_mask_bit(msti_bridge->co.portmask, port_number);
    mstpdata_free_msti_port(mstp_index, port_number);
}

/*****************************************************************************/
/* mstpmgr_instance_activate: activates instance when ports go up            */
/* or mstp start is requested by user.                                       */
/*****************************************************************************/
static bool mstpmgr_instance_activate(MSTP_INDEX mstp_index)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        MSTP_CIST_BRIDGE *cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

        cist_bridge->co.active = true;
        cist_bridge->rootPriority = cist_bridge->bridgePriority;
        cist_bridge->rootTimes = cist_bridge->bridgeTimes;
        SET_ALL_BITS(cist_bridge->modified_fields);
    }
    else
    {
        MSTP_MSTI_BRIDGE *msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL)
        {
            STP_LOG_ERR("[MST Index %u] msti_bridge null", mstp_index);
            return false;
        }

        msti_bridge->co.active = true;
        msti_bridge->rootPriority = msti_bridge->bridgePriority;
        msti_bridge->rootTimes = msti_bridge->bridgeTimes;
        SET_ALL_BITS(msti_bridge->modified_fields);
    }

    mstp_prs_init(mstp_index);
    return true;
}

/*****************************************************************************/
/* mstpmgr_instance_deactivate: deactivates instance when ports go down      */
/* or mstp is stopped by user                                                */
/*****************************************************************************/
static bool mstpmgr_instance_deactivate(MSTP_INDEX mstp_index)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        MSTP_CIST_BRIDGE *cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

        cist_bridge->rootPriority  = cist_bridge->bridgePriority;
        cist_bridge->rootTimes = cist_bridge->bridgeTimes;
        cist_bridge->co.rootPortId = MSTP_INVALID_PORT;
        cist_bridge->co.active = false;
        SET_ALL_BITS(cist_bridge->modified_fields);
    }
    else
    {
        MSTP_MSTI_BRIDGE *msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge)
        {
            msti_bridge->rootPriority  = msti_bridge->bridgePriority;
            msti_bridge->rootTimes = msti_bridge->bridgeTimes;
            msti_bridge->co.rootPortId = MSTP_INVALID_PORT;
            msti_bridge->co.active = false;
            SET_ALL_BITS(msti_bridge->modified_fields);
        }
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_enable_msti_port: initializes all relevant port variables         */
/*****************************************************************************/
bool mstpmgr_enable_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_PORT *mstp_port;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_MSTI_PORT *msti_port;

    if (MSTP_IS_CIST_INDEX(mstp_index))
        return false;

    mstp_bridge = mstpdata_get_bridge();
    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        return false;
    }

    msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
    msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
    if (msti_bridge == NULL || msti_port == NULL)
    {
        return false;
    }

    if (!msti_bridge->co.active)
    {
        mstpmgr_instance_activate(mstp_index);
    }

    if (msti_port->co.autoConfigPathCost)
    {
        msti_port->co.intPortPathCost = stputil_get_default_path_cost(port_number, true);
    }

    mstputil_compute_msti_designated_priority(msti_bridge, msti_port);
    msti_port->designatedTimes = msti_bridge->bridgeTimes;
    msti_port->portPriority = msti_port->designatedPriority;
    msti_port->portTimes = msti_port->designatedTimes;

    mstpmgr_instance_init_port_state_machines(mstp_index, port_number);
    SET_ALL_BITS(msti_port->co.modified_fields);
    SET_ALL_BITS(msti_port->modified_fields);

    return true;
}

/*****************************************************************************/
/* mstpmgr_disable_msti_port: initializes all relevant port variables        */
/*****************************************************************************/
bool mstpmgr_disable_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number, bool del_flag)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_PORT *mstp_port;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_MSTI_PORT *msti_port;
    PORT_MASK_LOCAL l_mask;
    PORT_MASK *mask = portmask_local_init(&l_mask);
    MSTP_COMMON_BRIDGE *cbridge;

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        return false;
    }

    mstp_bridge = mstpdata_get_bridge();
    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
        return false;

    msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
    msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
    if (msti_bridge == NULL || msti_port == NULL)
        return false;

    mstpmgr_instance_init_port_state_machines(mstp_index, port_number);
    msti_port->co.state = DISABLED;

    and_masks(mask, msti_bridge->co.portmask, mstp_bridge->enable_mask);

    cbridge = &msti_bridge->co;
    if (is_mask_clear(mask))
    {
        mstpmgr_instance_deactivate(mstp_index);
    }

    SET_ALL_BITS(msti_port->co.modified_fields);
    SET_ALL_BITS(msti_port->modified_fields);

    stpsync_update_boundary_port(stp_intf_get_port_name(mstp_port->port_number),
            !mstp_port->rcvdInternal, "RSTP");
    return true;
}

/*****************************************************************************/
/* mstpmgr_enable_port: enables input port, called when port comes up        */
/*****************************************************************************/
bool mstpmgr_enable_port(PORT_ID port_number)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_PORT *mstp_port;
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_CIST_PORT *cist_port;
    MSTP_MSTID mstid;

    mstp_bridge = mstpdata_get_bridge();

    if(mstp_bridge == NULL)
        return false;

    if (!mstp_bridge->active)
    {
        STP_LOG_INFO("Port %d MSTP not started", port_number);
        return true;
    }

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number))
    {
        STP_LOG_INFO("Port %d not part of control port", port_number);
        return true;
    }

    if (IS_MEMBER(mstp_bridge->enable_mask, port_number))
    {
        STP_LOG_INFO("Port %d is already enabled", port_number);
        return true;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("Port %d mstp_port is null", port_number);
        return false;
    }

    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
    if (!cist_bridge->co.active)
    {
        STP_LOG_INFO("Port %d Entered mstpmgr_instance_activate port", port_number);
        mstpmgr_instance_activate(MSTP_INDEX_CIST);
    }

    cist_port = MSTP_GET_CIST_PORT(mstp_port);

    if (cist_port->co.autoConfigPathCost)
    {
        cist_port->extPortPathCost =
            cist_port->co.intPortPathCost = stputil_get_default_path_cost(port_number, true);
    }

    mstputil_compute_cist_designated_priority(cist_bridge, mstp_port);
    cist_port->designatedTimes = cist_bridge->bridgeTimes;
    cist_port->portPriority = cist_port->designatedPriority;
    cist_port->portTimes = cist_port->designatedTimes;

    SET_ALL_BITS(cist_port->co.modified_fields);
    SET_ALL_BITS(cist_port->modified_fields);

    mstpmgr_init_port_state_machines(port_number);

    if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
    {
        for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
        {
            mstpmgr_enable_msti_port(mstp_index, port_number);
        }
    }

    mstp_port->portEnabled = true;
    mstp_port->operEdge = IS_MEMBER(mstp_bridge->admin_edge_mask, port_number);
    mstp_port->operPt2PtMac = IS_MEMBER(mstp_bridge->admin_pt2pt_mask, port_number);
    /* By default we will treat port as NON BOUNDARY PORT, 
    correct information will be updated once after MSTP BPDU recvd for this port*/
    mstp_port->rcvdInternal = mstp_port->infoInternal = true;
    stpsync_update_boundary_port(stp_intf_get_port_name(mstp_port->port_number),
            !mstp_port->rcvdInternal, NULL);

    // Change in portEnabled should trigger PPM
    mstp_ppm_gate(port_number);

    set_mask_bit(mstp_bridge->enable_mask, port_number);
    
    mstpmgr_refresh_all();
    return true;
}

/*****************************************************************************/
/* mstpmgr_disable_port: disables input port, called when port goes down     */
/*****************************************************************************/
bool mstpmgr_disable_port(PORT_ID port_number, bool del_flag)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_PORT *mstp_port;
    MSTP_CIST_PORT *cist_port;
    MSTP_COMMON_BRIDGE *cbridge;

    if (!IS_MEMBER(mstp_bridge->enable_mask, port_number))
        return true;

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
        return false;

    mstp_port->portEnabled = false;
    mstp_port->bpdu_guard_active = false;

    // Change in portEnabled should trigger PPM
    mstp_ppm_gate(port_number);

    clear_mask_bit(mstp_bridge->enable_mask, port_number);

    if (!l2_proto_mask_is_clear(&mstp_port->instance_mask))
    {
        for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
        {
            mstpmgr_disable_msti_port(mstp_index, port_number, del_flag);
        }
    }

    mstpmgr_init_port_state_machines(port_number);

    cist_port = MSTP_GET_CIST_PORT(mstp_port);
    if(cist_port)
    {
        cist_port->co.state = DISABLED;
        stpsync_update_boundary_port(stp_intf_get_port_name(mstp_port->port_number),
                !mstp_port->rcvdInternal, "RSTP");
        SET_ALL_BITS(cist_port->co.modified_fields);
        SET_ALL_BITS(cist_port->modified_fields);
    }

    cbridge = mstputil_get_common_bridge(MSTP_INDEX_CIST);
    if (cbridge == NULL)
        return false;
    
    if (is_mask_clear(mstp_bridge->enable_mask))
    {
        mstpmgr_instance_deactivate(MSTP_INDEX_CIST);
    }
    else
    {
        mstpmgr_refresh_all();
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_restart: resets the state machines and starts protocol execution  */
/* required when the MstConfigId changes (described in section 13.23.1).     */
/* this happens when name or revision level or vlan to mstid mapping changes */
/*****************************************************************************/
static bool mstpmgr_restart()
{
    PORT_ID port_number;
    MSTP_BRIDGE *mstp_bridge;

    mstp_bridge = mstpdata_get_bridge();
    if (!mstp_bridge->active)
        return true;

    port_number = port_mask_get_first_port(mstp_bridge->control_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstpmgr_disable_port(port_number, false);
        port_number = port_mask_get_next_port(mstp_bridge->control_mask, port_number);
    }

    port_number = port_mask_get_first_port(mstp_bridge->control_mask);
    while (port_number != BAD_PORT_ID)
    {
        if (stp_intf_is_port_up(port_number))
        {
            mstpmgr_enable_port(port_number);
        }
        port_number = port_mask_get_next_port(mstp_bridge->control_mask, port_number);
    }

    return true;
}
/*****************************************************************************/
/* mstpmgr_add_member_port: adds the port to the instance, allocates all     */
/* necessary structures to accomplish this                                   */
/*****************************************************************************/
void mstpmgr_add_member_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_PORT *mstp_port;
    MSTP_CIST_PORT *cist_port;

    if (!mstpmgr_init_mstp_port(port_number)) 
    {
        return;
    }

    mstp_bridge = mstpdata_get_bridge();

    if (MSTP_IS_CIST_INDEX(mstp_index)) 
    {
        cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
        set_mask_bit(cist_bridge->co.portmask, port_number);

        mstp_port = mstpdata_get_port(port_number);
        if (!mstp_port)
            return;

        cist_port = MSTP_GET_CIST_PORT(mstp_port);
        SET_ALL_BITS(cist_port->co.modified_fields);
        SET_ALL_BITS(cist_port->modified_fields);
    } 
    else 
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL)
            return;

        if (mstpmgr_init_msti_port(mstp_index, port_number) == NULL)
            return;

        set_mask_bit(msti_bridge->co.portmask, port_number);

        if (IS_MEMBER(mstp_bridge->enable_mask, port_number)) 
        {
            mstpmgr_enable_msti_port(mstp_index, port_number);
            mstpmgr_refresh(mstp_index, false);
        }
    }

    if (mstp_bridge->active &&
            stp_intf_is_port_up(port_number) &&
            !IS_MEMBER(mstp_bridge->enable_mask, port_number)) 
    {
        mstpmgr_enable_port(port_number);
    }
}

/*****************************************************************************/
/* mstpmgr_delete_member_port: removes the port from the instance port       */
/* membership. if no instance (incl. the cist) owns this port, deletes the   */
/* port structure.                                                           */
/*****************************************************************************/
void mstpmgr_delete_member_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_PORT *mstp_port;
    char *ifname;
    MSTP_COMMON_PORT *cport;

    ifname = stp_intf_get_port_name(port_number);

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number))
        return;

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
        return;

    cport = mstputil_get_common_port(mstp_index, mstp_port);

    if (!cport)
        return;

    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
    if (MSTP_IS_CIST_INDEX(mstp_index)) 
    {
        if (!IS_MEMBER(cist_bridge->co.portmask, port_number))
            return;
        stpsync_del_port_state(ifname, mstp_index);
        clear_mask_bit(cist_bridge->co.portmask, port_number);
    } 
    else 
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL ||
                !IS_MEMBER(msti_bridge->co.portmask, port_number)) 
        {
            return;
        }
        mstpmgr_free_msti_port(mstp_index, port_number);
        if(ifname) 
        {
            /* Delete MSTI port from APP DB*/
            stpsync_del_mst_port_info(ifname, mstputil_get_mstid(mstp_index));
            stpsync_del_port_state(ifname, mstp_index);
        }
        if (IS_MEMBER(cist_bridge->co.portmask, port_number))
            return;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
        return;

    if (l2_proto_mask_is_clear(&(mstp_port->instance_mask))) 
    {
        mstpmgr_disable_port(port_number, true);
        mstpmgr_free_mstp_port(port_number);
    }
}

/*****************************************************************************/
/* mstpmgr_set_control_mask: sets the current member mask for the mstp       */
/* instance to the input mask.                                               */
/*****************************************************************************/
bool mstpmgr_set_control_mask(MSTP_INDEX mstp_index)
{
    PORT_ID port_number;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_CIST_BRIDGE *cist_bridge;
    PORT_MASK *mask;
    PORT_MASK *del_mask, *add_mask;
    PORT_MASK *t_old, *t_new;
    PORT_MASK_LOCAL l_del_mask, l_add_mask;
    PORT_MASK_LOCAL l_t_old, l_t_new;
    UINT8 mask_string[500] = {0,};
    bool flag = false;

    del_mask = portmask_local_init(&l_del_mask);
    add_mask = portmask_local_init(&l_add_mask);

    mstp_bridge = mstpdata_get_bridge();
    clear_mask(del_mask);
    clear_mask(add_mask);
    mask = mstp_bridge->config_mask[mstp_index];

    if (MSTP_IS_CIST_INDEX(mstp_index)) 
    {
        cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
        and_not_masks(add_mask, mask, cist_bridge->co.portmask);
        and_not_masks(del_mask, cist_bridge->co.portmask, mask);
    } else {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL) 
        {
            STP_LOG_ERR("[MST Index %d] msti_bridge null", mstp_index);
            return false;
        }
        and_not_masks(add_mask, mask, msti_bridge->co.portmask);
        and_not_masks(del_mask, msti_bridge->co.portmask, mask);
    }


    if(!is_mask_clear(add_mask)) 
    {
        memset(mask_string, 0 , sizeof(mask_string));
        mask_to_string((BITMAP_T *)add_mask, mask_string, sizeof(mask_string));
        STP_LOG_INFO("[MST Index %d] ATTACH ports %s",mstp_index, mask_string);
    }

    if(!is_mask_clear(del_mask)) 
    {
        memset(mask_string, 0 , sizeof(mask_string));
        mask_to_string((BITMAP_T *)del_mask, mask_string, sizeof(mask_string));
        STP_LOG_INFO("[MST Index %d] DETTACH ports %s", mstp_index, mask_string);
    }
    // add ports
    port_number = port_mask_get_first_port(add_mask);
    while (port_number != BAD_PORT_ID) 
    {
        mstpmgr_add_member_port(mstp_index, port_number);
        port_number = port_mask_get_next_port(add_mask, port_number);
        flag = true;
    }

    // del ports
    port_number = port_mask_get_first_port(del_mask);
    while (port_number != BAD_PORT_ID) 
    {
        mstpmgr_delete_member_port(mstp_index, port_number);
        port_number = port_mask_get_next_port(del_mask, port_number);
        flag = true;
    }

    return flag;
}

/*****************************************************************************/
/* handler. validates bpdu and triggers the    */
/* appropriate port receive state machine                                    */
/*****************************************************************************/
bool mstpmgr_rx_bpdu(VLAN_ID vlan_id, PORT_ID port_number, void *bufptr, UINT32 size)
{
    MSTP_BRIDGE 	*mstp_bridge = NULL;
    MSTP_PORT 		*mstp_port = NULL;
    MSTP_BPDU 		*bpdu = (MSTP_BPDU *) bufptr;
    MSTP_INDEX		mstp_index = MSTP_INDEX_INVALID;
    uint64_t last_bpdu_rx_time = 0;
    uint64_t current_time = 0, time_diff;
    MAC_ADDRESS	    address;
    MSTP_COMMON_BRIDGE *cbridge;

    // convert the bpdu fields from network to host order
    mstputil_host_order_bpdu(bpdu);

    // update size based on hdr sizes etc. it does not include
    // crc, mac header, llc etc. make sure that the packet length does not
    // wrap around.
    if (size <= sizeof(MAC_HEADER) + sizeof(LLC_HEADER)) 
    {
        STP_LOG_ERR("Port %d input size %u too small", port_number, size);
        return false;
    }

    size = size - sizeof(MAC_HEADER) - sizeof(LLC_HEADER);
    if (!mstputil_validate_bpdu(bpdu, size))
        return false;

    mstp_bridge = mstpdata_get_bridge();

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number)) 
    {
        STP_LOG_ERR("Port %d Dropping bpdu on non-control port", port_number);
        return true;
    }
    if (!IS_MEMBER(mstp_bridge->enable_mask,port_number)) 
    {
        STP_LOG_ERR("Port %d Dropping BPDU, Port not in enable mask",  port_number);
        return true;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL) 
    {
        STP_LOG_ERR("Port %d mstp_port null", port_number);
        return false;
    }

    last_bpdu_rx_time = mstp_port->last_bpdu_rx_time;
    current_time = sys_get_seconds();
    mstp_port->last_bpdu_rx_time = current_time;
    if (current_time < last_bpdu_rx_time) 
    {
        last_bpdu_rx_time = last_bpdu_rx_time - current_time - 1;
        current_time = (uint64_t) -1;
    }
    /* Log when the rx pdu delayed by atleast one sec more than hello time */
    time_diff = current_time - last_bpdu_rx_time;
    if ((time_diff > (mstp_bridge->cist.rootTimes.helloTime * 1000))
            && (last_bpdu_rx_time != 0)) 
    {
        if((time_diff - (mstp_bridge->cist.rootTimes.helloTime * 1000) >= 1000))
            STP_LOG_INFO("Port %d RX BDPU Delayed by %lu sec", port_number,
                         (time_diff/1000));
    }

    if (mstp_port->operEdge) 
    {
        // received bpdu on an operational edge port.
        if (IS_MEMBER(mstp_bridge->admin_edge_mask, port_number)) 
        {
            STP_LOG_INFO("Port %d Bpdu received on admin-edge-port, clearing operational status",
                         port_number);
        }
        mstp_port->operEdge = false;
        //Change in operEdge status should trigger PRT and TCM
        mstp_prt_gate(MSTP_INDEX_CIST, port_number);
        mstp_tcm_gate(MSTP_INDEX_CIST, port_number);
        if (!l2_proto_mask_is_clear(&mstp_port->instance_mask)) 
        {
            for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++) 
            {
                if (mstp_bridge->msti[mstp_index] == NULL)
                    continue;
                mstp_prt_gate(mstp_index, port_number);
                mstp_tcm_gate(mstp_index, port_number);
            }
        }
    }

    mstputil_update_stats(port_number, bpdu, true);

    mstp_port->rcvdBpdu = true;
    mstp_prx_gate(port_number, bpdu);

    return true;
}



/*****************************************************************************/
/* mstpmgr_propagate_times: updates the bridge times with the globally       */
/* configured bridge times                                                   */
/*****************************************************************************/
static void mstpmgr_propagate_times(MSTP_INDEX mstp_index)
{
    MSTP_INDEX start_index, end_index;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_CIST_BRIDGE *cist_bridge;
    bool prop_cist;
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (mstp_index == MSTP_INDEX_INVALID) 
    {
        start_index = MSTP_INDEX_MIN;
        end_index = MSTP_INDEX_MAX;
        prop_cist = true;
    } 
    else 
    {
        start_index = end_index = mstp_index;
        prop_cist = MSTP_IS_CIST_INDEX(mstp_index);
    }

    if (prop_cist) 
    {
        cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
        cist_bridge->bridgeTimes.fwdDelay = mstp_bridge->fwdDelay;
        cist_bridge->bridgeTimes.helloTime = mstp_bridge->helloTime;
        cist_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;
        if (MSTP_IS_CIST_INDEX(mstp_index))
            return;
    }

    for (mstp_index = start_index; mstp_index <= end_index; mstp_index++) 
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL)
            continue;
        msti_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;
    }
}

/*****************************************************************************/
/* mstpmgr_config_start: activates/deactivates bridge operation.             */
/*****************************************************************************/
static bool mstpmgr_config_start(bool start)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    PORT_ID port_number;

    if (!mstp_bridge)
        return false;

    if (!start) {
        if (!mstp_bridge->active) 
        {
            return true;
        }

        // flush port states to vlan manager
        mstputil_timer_tick();

        mstp_bridge->active = false;
        STP_LOG_INFO("Bridge inactive");
    } else {
        if (mstp_bridge->active) 
        {
            STP_LOG_INFO("Bridge already Ative");
            return true;
        }

        STP_LOG_INFO("Bridge Activated");
        // start mstp operation
        mstp_bridge->active = true;

        port_number = port_mask_get_first_port(mstp_bridge->control_mask);
        while (port_number != BAD_PORT_ID) 
        {
            if (stp_intf_is_port_up(port_number)) 
            {
                mstpmgr_enable_port(port_number);
            }
            port_number = port_mask_get_next_port(mstp_bridge->control_mask, port_number);
        }
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_name: configure name associated with the mst configuration */
/* identifier                                                                */
/*****************************************************************************/
static bool mstpmgr_config_name(UINT8 *name)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    if (strncmp(name, mstp_bridge->mstConfigId.name, MSTP_NAME_LENGTH) == 0) 
    {
        return true;
    }
    memset(mstp_bridge->mstConfigId.name, 0, MSTP_NAME_LENGTH);
    strncpy(mstp_bridge->mstConfigId.name, name, (MSTP_NAME_LENGTH - 1));

    mstpmgr_restart();
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_revision_level: configure the revision level associated    */
/* with the mst configuration identifier                                     */
/*****************************************************************************/
static bool mstpmgr_config_revision_level(UINT16 revision_level)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    if (mstp_bridge->mstConfigId.revision_number == revision_level)
        return true;

    mstp_bridge->mstConfigId.revision_number = revision_level;

    mstpmgr_restart();
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_max_hops: configure max hop count associated with bridge   */
/*****************************************************************************/
static bool mstpmgr_config_max_hops(UINT16 max_hops)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;

    if (!mstp_bridge)
        return false;

    mstp_bridge->maxHops = (UINT8) max_hops;

    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

    if (!cist_bridge)
        return false;

    cist_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;
    if (!cist_bridge->co.active) 
    {
        cist_bridge->rootTimes = cist_bridge->bridgeTimes;
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
        SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);
    }

    for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++) 
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge == NULL)
            continue;

        msti_bridge->bridgeTimes.remainingHops = mstp_bridge->maxHops;
        if (!msti_bridge->co.active) 
        {
            msti_bridge->rootTimes = msti_bridge->bridgeTimes;
            SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT);
        }
    }

    if (mstp_bridge->active) 
    {
        mstpmgr_refresh_all();
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_hello_time: configures bridge hello time                   */
/*****************************************************************************/
static bool mstpmgr_config_hello_time(UINT8 hello_time)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    if(mstp_bridge->helloTime == hello_time)
        return false;

    mstp_bridge->helloTime = hello_time;
    mstpmgr_propagate_times(MSTP_INDEX_INVALID);
    if (mstp_bridge->active)
    {
        mstpmgr_refresh_all();
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_max_age: configures bridge max age                         */
/*****************************************************************************/
static bool mstpmgr_config_max_age(UINT8 max_age)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_BRIDGE *cist_bridge = NULL;

    if (!mstp_bridge)
        return false;

    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

    if (!cist_bridge)
        return false;

    if(cist_bridge->bridgeTimes.maxAge == max_age)
        return false;

    cist_bridge->bridgeTimes.maxAge = max_age;

    if (mstp_bridge->active) 
    {
        mstpmgr_refresh(MSTP_INDEX_CIST, true);
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_forward_delay: configures bridge forward delay             */
/*****************************************************************************/
static bool mstpmgr_config_forward_delay(UINT8 forward_delay)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    if(mstp_bridge->fwdDelay == forward_delay)
        return false;

    mstp_bridge->fwdDelay = forward_delay;
    mstpmgr_propagate_times(MSTP_INDEX_INVALID);
    if (mstp_bridge->active) 
    {
        mstpmgr_refresh_all();
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_port_disable: disables/enables mstp operation on input port*/
/*****************************************************************************/
static bool mstpmgr_config_port_disable(PORT_ID port_number, bool disable)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    if (disable) 
    {
        if (IS_MEMBER(mstp_bridge->admin_disable_mask, port_number))
            return true;
        stputil_update_mask(mstp_bridge->admin_edge_mask, port_number, false);
        stputil_update_mask(mstp_bridge->admin_pt2pt_mask, port_number, false);
    } 
    else 
    {
        if (!IS_MEMBER(mstp_bridge->admin_disable_mask, port_number))
            return true;
    }

    stputil_update_mask(mstp_bridge->admin_disable_mask, port_number, disable);
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_port_admin_edge: configures input port to be an            */
/* operational edge port (ie connected to a host machine)                    */
/*****************************************************************************/
static bool mstpmgr_config_port_admin_edge(PORT_ID port_number, bool admin_edge)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_PORT *mstp_port;

    if (!mstp_bridge)
        return false;

    stputil_update_mask(mstp_bridge->admin_edge_mask, port_number, admin_edge);

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number)) 
    {
        return true;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("Port %d mstp_port null", port_number);
        return false;
    }

    mstp_port->operEdge = admin_edge;

    if (mstp_bridge->active)
        mstpmgr_refresh_all();

    return true;
}

/*****************************************************************************/
/* mstpmgr_config_port_admin_pt2pt: configures input port to be an           */
/* operational point-to-point port (ie connected to a single bridge)         */
/*****************************************************************************/
static bool mstpmgr_config_port_admin_pt2pt(PORT_ID port_number, LINK_TYPE link_type)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_PORT *mstp_port;

    if (!mstp_bridge)
        return false;

    stputil_update_mask(mstp_bridge->admin_pt2pt_mask, port_number, (link_type == LINK_TYPE_SHARED?0:1));

    if (!IS_MEMBER(mstp_bridge->control_mask, port_number)) 
    {
        return true;
    }

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL) 
    {
        STP_LOG_ERR("Port %d mstp_port null", port_number);
        return false;
    }

    if(link_type == LINK_TYPE_AUTO) 
    {
        mstp_port->operPt2PtMac = true;
    } 
    else if(link_type == LINK_TYPE_PT2PT) 
    {
        mstp_port->operPt2PtMac = true;
    } 
    else if (link_type == LINK_TYPE_SHARED) 
    {
        mstp_port->operPt2PtMac = false;
    }

    return true;
}

/*****************************************************************************/
/* mstpmgr_config_instance_vlanmask: attaches/detaches vlans to the input    */
/* spanning-tree instance.                                                   */
/*****************************************************************************/
bool mstpmgr_config_instance_vlanmask(MSTP_MSTID mstid, VLAN_MASK *vlanmask)
{
    MSTP_BRIDGE 		*mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_BRIDGE 	*cist_bridge = mstputil_get_cist_bridge();
    MSTP_COMMON_BRIDGE 	*cbridge = NULL;
    MSTP_INDEX 			mstp_index = MSTP_INDEX_INVALID;
    PORT_ID 			port_number = BAD_PORT_ID;
    bool 		    	flag = false;
    UINT8 				vlanmask_string[500] = {0,};
    VLAN_ID             vlan_id = VLAN_ID_INVALID;
    VLAN_MASK           del_vlan_mask, add_vlan_mask;
    enum L2_PORT_STATE state;
    bool state_flag = false;

    vlan_bmp_init(&add_vlan_mask);
    vlan_bmp_init(&del_vlan_mask);

    if (!mstp_bridge)
        return false;

    vlanmask_to_string((BITMAP_T *)vlanmask, vlanmask_string, sizeof(vlanmask_string));

    mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);

    if (mstp_index == MSTP_INDEX_INVALID) 
    {
        /* Allocate and initialize MSTI instance */
        if (!mstpmgr_init_msti(mstid)) 
        {
            STP_LOG_ERR("[MST %u] could not create new MSTP instance", mstid);
            return false;
        }
        mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
    }

    cbridge = mstputil_get_common_bridge(mstp_index);

    if (!cbridge)
        return false;

    and_not_masks((BITMAP_T *)&add_vlan_mask, (BITMAP_T *)vlanmask, (BITMAP_T *)&cbridge->vlanmask);
    and_not_masks((BITMAP_T *)&del_vlan_mask, (BITMAP_T *)&cbridge->vlanmask, (BITMAP_T *)vlanmask);

    for (vlan_id = vlanmask_get_first_vlan(&add_vlan_mask);
            vlan_id != VLAN_ID_INVALID;
            vlan_id = vlanmask_get_next_vlan(&add_vlan_mask, vlan_id)) 
    {
        MSTP_CLASS_SET_MSTID(mstp_bridge, vlan_id, mstid);
        vlanmask_set_bit(&cbridge->vlanmask, vlan_id);
        if(mstpdata_is_vlan_present(vlan_id)) 
        {
            stpsync_add_vlan_to_instance(vlan_id, mstp_index);
        }
        flag = true;
    }
    if(!vlanmask_is_clear(&add_vlan_mask)) 
    {
        memset(vlanmask_string, 0 , sizeof(vlanmask_string));
        vlanmask_to_string((BITMAP_T *)&add_vlan_mask, vlanmask_string, sizeof(vlanmask_string));
        STP_LOG_INFO("[MST %d] ATTACH vlan %s",mstid, vlanmask_string);
    }

    for (vlan_id = vlanmask_get_first_vlan(&del_vlan_mask);
            vlan_id != VLAN_ID_INVALID;
            vlan_id = vlanmask_get_next_vlan(&del_vlan_mask, vlan_id)) 
    {
        /* If the VLAN is mapped to other MSTIs, then dont set to CIST*/
        if(MSTP_GET_MSTID(mstp_bridge, vlan_id) == mstid)
        {
            MSTP_CLASS_SET_MSTID(mstp_bridge, vlan_id, MSTP_MSTID_CIST);
        }
        vlanmask_clear_bit(&cbridge->vlanmask, vlan_id);
        if(mstpdata_is_vlan_present(vlan_id)) 
        {
            stpsync_del_vlan_from_instance(vlan_id, mstp_index);
        }
        flag = true;
    }
    if(!vlanmask_is_clear(&del_vlan_mask)) 
    {
        memset(vlanmask_string, 0 , sizeof(vlanmask_string));
        vlanmask_to_string((BITMAP_T *)&del_vlan_mask, vlanmask_string, sizeof(vlanmask_string));
        STP_LOG_INFO("[MST %d] DETTACH vlan %s",mstid, vlanmask_string);
    }


    if (!MSTP_IS_CIST_INDEX(mstp_index) &&
            vlanmask_is_clear(&cbridge->vlanmask)) 
    {
        // delete member ports
        port_number = port_mask_get_first_port(cbridge->portmask);

        while (port_number != BAD_PORT_ID) 
        {
            mstpmgr_delete_member_port(mstp_index, port_number);
            state_flag = mstplib_get_port_state(mstp_index, port_number, &state);
            if(!state_flag || state != FORWARDING) 
            {
                for (vlan_id = vlanmask_get_first_vlan(&del_vlan_mask);
                        vlan_id != VLAN_ID_INVALID;
                        vlan_id = vlanmask_get_next_vlan(&del_vlan_mask, vlan_id)) 
                {

                    mstputil_set_kernel_bridge_port_state_for_single_vlan(vlan_id, port_number, FORWARDING);
                }
            }
            port_number = port_mask_get_next_port(cbridge->portmask, port_number);
        }

        vlanmask_clear_all(mstp_bridge->config_mask[mstp_index]);

        mstpmgr_free_msti(mstp_index);
    }

    if(flag) 
    {
        // recompute mstConfigId message digest
        mstputil_compute_message_digest(false);
        if(MSTP_IS_CIST_INDEX(mstp_index))
            SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_VLAN_MASK_SET);
        else 
        {
            MSTP_MSTI_BRIDGE *msti_bridge;
            msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
            if (msti_bridge) 
            {
                SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_VLAN_MASK_SET);
            }
        }
    }

    return flag;
}

/*****************************************************************************/
/* mstpmgr_config_cist_priority: configures bridge priority for the common   */
/* spanning-tree instance                                                    */
/*****************************************************************************/
static bool mstpmgr_config_cist_priority(UINT16 priority)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_COMMON_BRIDGE *cbridge;

    if (!mstp_bridge)
        return false;

    cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);

    if(!cist_bridge)
        return false;

    if(mstputil_is_same_bridge_priority(&cist_bridge->co.bridgeIdentifier, priority))
        return false;

    cist_bridge->co.bridgeIdentifier.priority = MSTP_ENCODE_PRIORITY(priority);
    SET_BIT(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT);
    cist_bridge->bridgePriority.root =
        cist_bridge->bridgePriority.regionalRoot =
            cist_bridge->bridgePriority.designatedId = cist_bridge->co.bridgeIdentifier;

    if (mstp_bridge->active) 
    {
        mstpmgr_refresh_all();
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_cist_port_path_cost: configures port path cost for input   */
/* port for the common spanning-tree instance                                */
/*****************************************************************************/
static bool mstpmgr_config_cist_port_path_cost(PORT_ID port_number, UINT32 path_cost, UINT8 is_global)
{
    MSTP_CIST_PORT *cist_port;
    MSTP_BRIDGE *mstp_bridge;

    mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    cist_port = mstputil_get_cist_port(port_number);
    if (cist_port == NULL) 
    {
        STP_LOG_ERR("Port %d cist_port null", port_number);
        return false;
    }

    if (is_global) 
    {
        /* If per mst attributes are set, ignore global attributes */
        if (stp_intf_is_inst_port_pathcost_set(port_number, MSTP_INDEX_CIST))
            return true;
    }

    cist_port->co.autoConfigPathCost = stp_intf_is_default_port_pathcost(port_number, MSTP_INDEX_CIST);

    if((cist_port->co.intPortPathCost == path_cost) &&
            (cist_port->extPortPathCost == path_cost))
        return true;

    if (cist_port->co.autoConfigPathCost) 
    {
        cist_port->co.intPortPathCost =
            cist_port->extPortPathCost = stputil_get_default_path_cost(port_number, true);
    } else 
    {
        cist_port->co.intPortPathCost =
            cist_port->extPortPathCost = path_cost;
    }

    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_PORT_PATH_COST_BIT);

    cist_port->co.selected = false;
    if (mstp_bridge->active) 
    {
        mstpmgr_refresh_all();
    }

    return true;
}

/*****************************************************************************/
/* mstpmgr_config_cist_port_priority: configures port priority for input     */
/* port for the common spanning-tree instance                                */
/*****************************************************************************/
static bool mstpmgr_config_cist_port_priority(PORT_ID port_number, UINT16 priority,
        UINT8 is_global)
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_CIST_PORT *cist_port = mstputil_get_cist_port(port_number);

    if (cist_port == NULL)
        return false;

    if (is_global) 
    {
        /* If per mst attributes are set, ignore global attributes */
        if (stp_intf_is_inst_port_priority_set(port_number, MSTP_INDEX_CIST))
            return true;
    }

    if(cist_port->co.portId.priority == MSTP_ENCODE_PORT_PRIORITY(priority))
        return false;

    cist_port->co.portId.priority = MSTP_ENCODE_PORT_PRIORITY(priority);
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_PORT_PRIORITY_BIT);

    cist_port->co.selected = false;

    if (mstp_bridge && mstp_bridge->active) 
    {
        mstpmgr_refresh_all();
    }

    return true;
}

/*****************************************************************************/
/* mstpmgr_config_msti_priority: configures priority for input mst instance  */
/*****************************************************************************/
bool mstpmgr_config_msti_priority(MSTP_MSTID mstid, UINT16 priority)
{
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_INDEX mstp_index;
    MSTP_COMMON_BRIDGE *cbridge;

    if (MSTP_IS_CIST_MSTID(mstid)) 
    {
        return mstpmgr_config_cist_priority(priority);
    }

    mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    msti_bridge = mstputil_get_msti_bridge(mstid);
    if (msti_bridge == NULL)
        return false;

    if(mstputil_is_same_bridge_priority(&msti_bridge->co.bridgeIdentifier, priority))
        return false;

    msti_bridge->co.bridgeIdentifier.priority = MSTP_ENCODE_PRIORITY(priority);
    SET_BIT(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT);
    msti_bridge->bridgePriority.regionalRoot =
        msti_bridge->bridgePriority.designatedId = msti_bridge->co.bridgeIdentifier;

    if (msti_bridge->co.active) 
    {
        mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
        mstpmgr_refresh(mstp_index, true);
    }
    return true;
}

/*****************************************************************************/
/* mstpmgr_config_msti_port_path_cost: configures port path cost for input   */
/* port for the mst instance                                                 */
/*****************************************************************************/
static bool mstpmgr_config_msti_port_path_cost(MSTP_MSTID mstid, PORT_ID port_number,
        UINT32 path_cost, UINT8 is_global)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_PORT *msti_port;
    UINT32 port_path_cost;

    if (MSTP_IS_CIST_MSTID(mstid)) 
    {
        return mstpmgr_config_cist_port_path_cost(port_number, path_cost, is_global);
    }

    mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    msti_port = mstputil_get_msti_port(mstid, port_number);
    if (msti_port == NULL)
        return false;

    if (is_global) 
    {
        /* If per mst attributes are set, ignore global attributes */
        if (stp_intf_is_inst_port_pathcost_set(port_number, MSTP_GET_INSTANCE_INDEX(mstp_bridge,mstid)))
            return true;
    }

    mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge,mstid);
    msti_port->co.autoConfigPathCost = stp_intf_is_default_port_pathcost(port_number, mstp_index);

    if(msti_port->co.intPortPathCost == path_cost)
        return false;

    if (msti_port->co.autoConfigPathCost)
        msti_port->co.intPortPathCost = stputil_get_default_path_cost(port_number, true);
    else
        msti_port->co.intPortPathCost = path_cost;

    SET_BIT(msti_port->modified_fields, MSTP_PORT_MEMBER_PORT_PATH_COST_BIT);
    msti_port->co.selected = true;

    if (mstp_bridge->active) 
    {
        mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
        mstpmgr_refresh(mstp_index, true);
    }

    return true;
}

/*****************************************************************************/
/* mstpmgr_config_msti_port_priority: configures port priority for input     */
/* port for the mst instance                                                 */
/*****************************************************************************/
static bool mstpmgr_config_msti_port_priority(MSTP_MSTID mstid, PORT_ID port_number,
        UINT16 port_priority, UINT8 is_global)
{
    MSTP_INDEX mstp_index;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_MSTI_PORT *msti_port;

    if (MSTP_IS_CIST_MSTID(mstid)) 
    {
        return mstpmgr_config_cist_port_priority(port_number, port_priority, is_global);
    }

    mstp_bridge = mstpdata_get_bridge();

    if (!mstp_bridge)
        return false;

    msti_port = mstputil_get_msti_port(mstid, port_number);
    if (msti_port == NULL)
        return false;

    if (is_global) 
    {
        /* If per mst attributes are set, ignore global attributes */
        if (stp_intf_is_inst_port_priority_set(port_number, MSTP_GET_INSTANCE_INDEX(mstp_bridge,mstid)))
            return true;
    }

    if(msti_port->co.portId.priority == MSTP_ENCODE_PORT_PRIORITY(port_priority))
        return false;

    msti_port->co.portId.priority = MSTP_ENCODE_PORT_PRIORITY(port_priority);
    SET_BIT(msti_port->modified_fields, MSTP_PORT_MEMBER_PORT_PRIORITY_BIT);

    msti_port->co.selected = true;
    if (mstp_bridge->active) 
    {
        mstp_index = MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
        mstpmgr_refresh(mstp_index, true);
    }
    return true;
}