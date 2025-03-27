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