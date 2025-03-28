/*
 * Copyright 2025 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

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
