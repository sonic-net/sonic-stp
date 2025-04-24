/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #include "stp_inc.h"

/*
 * Input:
 *   pointer to STATIC_BITMAP_T
 * Return:
 *   pointer to BITMAP_T, so static bitmap can levarage and utilize all apis written for dynamic bmp.
 */
BITMAP_T *static_portmask_init(STATIC_BITMAP_T *bmp)
{
    static_bmp_init(bmp);
    return (BITMAP_T *)bmp;
}

int stp_intf_get_netlink_fd()
{
    return stpd_context.netlink_fd;
}

struct event_base *stp_intf_get_evbase()
{
    return stpd_context.evbase;
}

char * stp_intf_get_port_name(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->ifname;
    }
    snprintf(g_stp_invalid_port_name, IFNAMSIZ, "%d", port_id);
    return g_stp_invalid_port_name;
}

bool stp_intf_is_port_up(int port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->oper_state?true:false;
    }
    return false;
}

uint32_t stp_intf_get_speed(int port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->speed;
    }
    return 0;
}

INTERFACE_NODE *stp_intf_get_node(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node;
    }

    return NULL;
}

INTERFACE_NODE *stp_intf_get_node_by_kif_index(uint32_t kif_index)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->kif_index == kif_index)
            return node;
    }

    return NULL;
}

bool stp_intf_get_mac(int port_id, MAC_ADDRESS *mac)
{
    // SONIC has same MAC for all interface
    COPY_MAC(mac, &g_stp_base_mac_addr);

}

uint32_t stp_intf_get_kif_index_by_port_id(uint32_t port_id)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == port_id)
            return node->kif_index;
    }

    return BAD_PORT_ID;
}

uint32_t stp_intf_get_port_id_by_kif_index(uint32_t kif_index)
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->kif_index == kif_index)
            return node->port_id;
    }

    return BAD_PORT_ID;
}


uint32_t stp_intf_get_port_id_by_name(char *ifname)
{
    INTERFACE_NODE *node = 0;
    
    node = stp_intf_get_node_by_name(ifname);
    if (node && (node->port_id != BAD_PORT_ID))
        return node->port_id;
    
    return BAD_PORT_ID;
}

INTERFACE_NODE *stp_intf_get_node_by_name(char *ifname)
{
    INTERFACE_NODE search_node;

    if (!ifname)
        return NULL;

    memset(&search_node, 0, sizeof(INTERFACE_NODE));
    memcpy(search_node.ifname, ifname, IFNAMSIZ);

    return avl_find(g_stpd_intf_db, &search_node);
}

int stp_intf_avl_compare(const void *user_p, const void *data_p, void *param)
{
    stp_if_avl_node_t *pa = (stp_if_avl_node_t *)user_p;
    stp_if_avl_node_t *pb = (stp_if_avl_node_t *)data_p;

    return (strncasecmp(pa->ifname, pb->ifname, IFNAMSIZ));
}

void stp_intf_del_from_intf_db(INTERFACE_NODE *node)
{
    STP_LOG_INFO("AVL Delete :  %s  kif : %d  port_id : %u", node->ifname, node->kif_index, node->port_id);

    if (STP_IS_ETH_PORT(node->ifname))
        stp_pkt_sock_close(node);

    avl_delete(g_stpd_intf_db, node);
    free(node);

    return;
}

uint32_t stp_intf_add_to_intf_db(INTERFACE_NODE *node)
{
    void **ret_ptr = avl_probe(g_stpd_intf_db, node);
    if (*ret_ptr != node) 
    {
        if (*ret_ptr == NULL)
            STP_LOG_CRITICAL("AVL-Insert Malloc Failure, Intf: %s kif: %d", node->ifname, node->kif_index);
        else
            STP_LOG_CRITICAL("DUPLICATE Entry found Intf: %s kif: %d", ((INTERFACE_NODE *)(*ret_ptr))->ifname, ((INTERFACE_NODE *)(*ret_ptr))->kif_index);
        //This should never happen. 
        sys_assert(0);
    }
    else 
    {
        STP_LOG_INFO("AVL Insert :  %s %d %u", node->ifname, node->kif_index, node->port_id);

        //create socket only for Ethernet ports
        if (STP_IS_ETH_PORT(node->ifname))
            stp_pkt_sock_create(node);

        return node->port_id;
    }
}

bool stp_intf_ioctl_get_ifname(uint32_t kif_index, char * if_name)
{
    struct ifreq ifr;

    ifr.ifr_ifindex = kif_index;
    if(ioctl(g_stpd_ioctl_sock, SIOCGIFNAME, &ifr) < 0)
        return false;

    strncpy(if_name, ifr.ifr_name, IFNAMSIZ);
    return true;
}

uint32_t stp_intf_ioctl_get_kif_index(char * if_name)
{
    struct ifreq ifr;
    size_t if_name_len=strlen(if_name);

    if(if_name_len < sizeof(ifr.ifr_name)) 
    {
        memcpy(ifr.ifr_name,if_name,if_name_len);
        ifr.ifr_name[if_name_len]='\0';

        if(ioctl(g_stpd_ioctl_sock, SIOCGIFINDEX, &ifr) < 0)
            return -1;

        return ifr.ifr_ifindex;
    }
    return -1;
}

INTERFACE_NODE * stp_intf_create_intf_node(char * ifname, uint32_t kif_index)
{
    INTERFACE_NODE *node = NULL;

    node = calloc(1, sizeof(INTERFACE_NODE));
    if(!node)
    {
        STP_LOG_CRITICAL("Calloc Failed");
        return NULL;
    }

    /* Initialize to BAD port id */
    node->port_id = BAD_PORT_ID;

    /* Update interface name */
    if(ifname)
    {
        strncpy(node->ifname, ifname, IFNAMSIZ);
    }
    else
    {
        if(stp_intf_ioctl_get_ifname(kif_index, node->ifname))
            STP_LOG_INFO("Kernel ifindex %u name %s", kif_index, node->ifname);
        else
            STP_LOG_ERR("Kernel ifindex %u name fetch failed", kif_index);
    }

    /* Update kernel ifindex*/
    if(kif_index == BAD_PORT_ID)
    {
        /* Update kernel ifindex*/
        node->kif_index = stp_intf_ioctl_get_kif_index(ifname);
        if(node->kif_index == BAD_PORT_ID)
            STP_LOG_ERR("Kernel ifindex fetch for %s failed", ifname);
    }
    else
    {
        node->kif_index = kif_index;
    }

    node->priority = (STP_DFLT_PORT_PRIORITY >> 4);

    stp_intf_add_to_intf_db(node);
    return node;
}

/* Handle PO configuration in case it is recieved before netlink msg is received for PO creation */
PORT_ID stp_intf_handle_po_preconfig(char * ifname)
{
    INTERFACE_NODE *node = NULL;
    
    node = stp_intf_get_node_by_name(ifname);
    if(!node)
    {
        node = stp_intf_create_intf_node(ifname, BAD_PORT_ID);
        if(!node)
            return BAD_PORT_ID;
    }
    
    /* Allocate port id for PO if not yet done */
    if(node->port_id == BAD_PORT_ID && g_stpd_port_init_done)
    {
        node->port_id = stp_intf_allocate_po_id();
        if(node->port_id == BAD_PORT_ID)
            sys_assert(0);
    }
    return node->port_id;
}

void stp_intf_add_po_member(INTERFACE_NODE * if_node)
{
    INTERFACE_NODE *node = NULL;
    
    node = stp_intf_get_node_by_kif_index(if_node->master_ifindex);
    if(!node)
    {
        node = stp_intf_create_intf_node(NULL, if_node->master_ifindex);
        if(!node)
            return;
    }

    /* Increment member port count */
    node->member_port_count++;

    /* Populate PO speed from member port speed */
    if(node->speed == 0)
    {
        node->speed = if_node->speed;

        /* Calculate default Path cost */
        node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
    }

    /* Allocate port id for PO if not yet done */
    if(node->port_id == BAD_PORT_ID && g_stpd_port_init_done)
    {
        node->port_id = stp_intf_allocate_po_id();
        if(node->port_id == BAD_PORT_ID)
            sys_assert(0);
    }

    STP_LOG_INFO("Add PO member kernel_if - %u member_if - %u kif_index - %u", if_node->master_ifindex, if_node->port_id, if_node->kif_index);
}

void stp_intf_del_po_member(uint32_t po_kif_index, uint32_t member_port)
{
    INTERFACE_NODE *node = 0;
    STP_INDEX stp_index = 0;
    
    node = stp_intf_get_node_by_kif_index(po_kif_index);
    if(!node)
    {
        STP_LOG_ERR("PO not found in interface DB kernel_if - %u member_if - %u", po_kif_index, member_port);
        return;
    }

    if(node->member_port_count == 0)
    {
        STP_LOG_ERR("PO member count is 0 kernel_if - %u member_if - %u", po_kif_index, member_port);
        return;
    }

    node->member_port_count--;
    /* if this is last member port, delete PO node in avl tree*/
    if(!node->member_port_count)
    {
        stputil_set_global_enable_mask(node->port_id, false);
         if (STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
        {
            for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
                stpmgr_delete_control_port(stp_index, node->port_id, true);
        }
       /* else if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
        {
            mstpdata_update_vlanport_db_on_po_delete(node->port_id);
            mstpmgr_delete_control_port(node->port_id, true);
        }*/

        stp_intf_release_po_id(node->port_id);
        stp_intf_del_from_intf_db(node);
    }
    STP_LOG_INFO("Del PO member kernel_if - %u member_if - %u", po_kif_index, member_port);
}

bool stp_intf_update_po_members(netlink_db_t * if_db, INTERFACE_NODE * node)
{
    /* Add member port to PO */
    if (!node->master_ifindex && if_db->master_ifindex) 
    {
        node->master_ifindex = if_db->master_ifindex;
        stp_intf_add_po_member(node);
    }

    /* Delete member port from PO */
    if (node->master_ifindex && !if_db->master_ifindex) 
    {
        stp_intf_del_po_member(node->master_ifindex, node->port_id);
        node->master_ifindex = 0;
    }
}

INTERFACE_NODE * stp_intf_update_intf_db(netlink_db_t *if_db, uint8_t is_add, bool init_in_prog, bool eth_if)
{
    INTERFACE_NODE *node = NULL;
    uint32_t port_id = 0;

    if(is_add)
    {
        node = stp_intf_get_node_by_name(if_db->ifname);
        if(!node)
        {
            node = stp_intf_create_intf_node(if_db->ifname, if_db->kif_index);
            if(!node)
                return NULL;

            /* Update port id */
            if(eth_if)
            {
                port_id = strtol(((char *)if_db->ifname + STP_ETH_NAME_PREFIX_LEN), NULL, 10);
                node->port_id = port_id;
                
                /* Derive Max Port */
                if (init_in_prog)
                {
                    if ( port_id + (4 - (port_id % 4)) > g_max_stp_port)
                    {
                        g_max_stp_port = port_id + (4 - (port_id % 4));
                    }
                }
            }
            STP_LOG_INFO("Add Kernel ifindex %d name %s", if_db->kif_index, if_db->ifname);
        }


        /* Update the port speed */
        if (eth_if)
        {
            if(!node->speed)
            {
                node->speed = stpsync_get_port_speed(if_db->ifname);
        
                /* Calculate default Path cost */
                node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
            }

            /* Handle PO member port */
            if(if_db->is_member || node->master_ifindex)
                stp_intf_update_po_members(if_db, node);
        }
        return node;
    }
    else
    {
        //Netlink Delete does not send name, hence traverse and get the node to delete
        node = stp_intf_get_node_by_kif_index(if_db->kif_index);
        if (!node)
        {
            STP_LOG_ERR("Delete FAILED, AVL Node not found, Kif: %d", if_db->kif_index);
            return NULL;
        }
        
        stp_intf_del_from_intf_db(node);
        STP_LOG_INFO("Del Kernel ifindex %x name %s", if_db->kif_index, if_db->ifname);
    }

    return NULL;
}

void stp_intf_netlink_cb(netlink_db_t *if_db, uint8_t is_add, bool init_in_prog)
{
    INTERFACE_NODE *node = NULL, *po_node = NULL;
    bool eth_if;
    g_stpd_stats_libev_netlink++;

    if(STP_IS_ETH_PORT(if_db->ifname))
        eth_if = true;
    else if(STP_IS_PO_PORT(if_db->ifname))
        eth_if = false;
    else
        return;

    node = stp_intf_update_intf_db(if_db, is_add, init_in_prog, eth_if);

    /* Handle oper data change */
    if(node)
    {
        /* Handle oper state change */
        if (if_db->oper_state != node->oper_state)
        {
            node->oper_state = if_db->oper_state;
            if(eth_if)
            {
                node->speed = stpsync_get_port_speed(if_db->ifname);
                /* Calculate default Path cost */
                node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
                if(if_db->master_ifindex) 
                {
                    po_node = stp_intf_get_node_by_kif_index(if_db->master_ifindex);
                
                    if(po_node && (po_node->member_port_count == 1 || !po_node->oper_state))
                    {
                        po_node->speed = node->speed;
                        /* Calculate default Path cost */
                        po_node->path_cost = node->path_cost;
                    }
                }
            }

            if(!init_in_prog && (if_db->master_ifindex == 0) && (node->port_id != BAD_PORT_ID))
            {
                if (STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
                    stpmgr_port_event(node->port_id, if_db->oper_state);
                /*else if(STP_IS_PROTOCOL_ENABLED(L2_MSTP))
                    mstpmgr_port_event(node->port_id, if_db->oper_state);*/
            }
        }
    }
}

int stp_intf_init_port_stats()
{
    uint16_t i = 0;
    // Allocate stats array g_max_stp_port
    g_stpd_intf_stats = calloc(1, ((g_max_stp_port) * sizeof(STPD_INTF_STATS *)));
    if (!g_stpd_intf_stats)
    {
        STP_LOG_CRITICAL("Calloc Failed, g_stpd_intf_stats");
        sys_assert(0);
    }
    for(i = 0; i < g_max_stp_port ; i++)
    {
        g_stpd_intf_stats[i] = calloc(1, sizeof(STPD_INTF_STATS));
        if (!g_stpd_intf_stats[i])
        {
            STP_LOG_CRITICAL("Calloc Failed, g_stpd_intf_stats[%d]", i);
            sys_assert(0);
        }
    }

    return 0;
}
int stp_intf_init_po_id_pool()
{
    int ret = 0;
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    
    //Allocate po-id-pool
    ret = bmp_alloc(&stpd_context.po_id_pool, STP_MAX_PO_ID);
    if (-1 == ret)
    {
        STP_LOG_ERR("bmp_alloc Failed");
        return -1;
    }

    //Allocate port-id for all PO's 
    avl_t_init(&trav, g_stpd_intf_db);
    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id == BAD_PORT_ID && STP_IS_PO_PORT(node->ifname))
        {
            node->port_id = stp_intf_allocate_po_id();
            STP_LOG_INFO("Allocated PO port id %d name %s", node->port_id, node->ifname);
        }
    }
    return 0;
}


int stp_intf_event_mgr_init(void)
{
    struct event *nl_event = 0;
    
    if((g_stpd_ioctl_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        sys_assert(0);

    g_max_stp_port = 0;

    /* Open Netlink comminucation to populate Interface DB */
    g_stpd_netlink_handle = stp_netlink_init(&stp_intf_netlink_cb);
    if(-1 == g_stpd_netlink_handle)
    {
        STP_LOG_CRITICAL("netlink init failed");
        sys_assert(0);
    }

    if (stp_netlink_recv_all(g_stpd_netlink_handle) == -1)
    {
        STP_LOG_CRITICAL("error in intf db creation");
        sys_assert(0);
    }

    g_max_stp_port = g_max_stp_port * 2; // Phy Ports + LAG
    STP_LOG_INFO("intf db done. max port %d", g_max_stp_port);

    if(g_max_stp_port == 0)
        return -1;

    stp_intf_init_port_stats();

    if(-1 == stp_intf_init_po_id_pool())
    {
        STP_LOG_CRITICAL("error Allocating port-id for PO's");
        sys_assert(0);
    }

    g_stpd_port_init_done = 1;

    /* Add libevent to monitor interface events */
    nl_event = stpmgr_libevent_create
            ( stp_intf_get_evbase()
            , stp_intf_get_netlink_fd()
            , EV_READ|EV_PERSIST
            , stp_netlink_events_cb, (char *)"NETLINK", NULL);
    if (!nl_event)
    {
        STP_LOG_ERR("Netlink Event create Failed");
        return -1;
    }
}

bool stp_intf_set_port_priority(PORT_ID port_id, uint16_t priority)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        node->priority = priority >> 4;
        return true;
    }

    return false;
}

uint16_t stp_intf_get_port_priority(PORT_ID port_id)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
        return (node->priority);

    return (STP_DFLT_PORT_PRIORITY >> 4);
}

bool stp_intf_set_path_cost(PORT_ID port_id, uint32_t path_cost, uint8_t def_path_cost)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        node->path_cost = path_cost;
        node->def_path_cost = def_path_cost;
        return true;
    }

    return false;
}


uint32_t stp_intf_get_path_cost(PORT_ID port_id)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
        return (node->path_cost);

    return 0;
}

/* Reinitialize default values to all interface */
void stp_intf_reset_port_params()
{
    struct avl_traverser trav;
    INTERFACE_NODE *node = 0;
    avl_t_init(&trav, g_stpd_intf_db);

    while(NULL != (node = avl_t_next(&trav)))
    {
        if (node->port_id != BAD_PORT_ID)
        {
            node->priority = STP_DFLT_PORT_PRIORITY >> 4;
            node->path_cost = stputil_get_path_cost(node->speed, g_stpd_extend_mode);
            node->def_path_cost = true;
            memset((char *)node->mst_info, 0, (sizeof(MST_INFO) * MSTP_MAX_INSTANCES));
        }
    }
}

BITMAP_T *static_mask_init(STATIC_BITMAP_T *bmp)
{
    static_bmp_init(bmp);
    return (BITMAP_T *)bmp;
}

void stp_intf_set_inst_port_priority(PORT_ID port_id, UINT16 mstp_index, uint16_t priority, UINT8 add)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if(add)
        {
            node->mst_info[mstp_index].flag |= MSTP_PORT_PRI_FLAG;
            node->mst_info[mstp_index].priority = priority >> 4;
        }
        else
        {
            node->mst_info[mstp_index].flag &= ~MSTP_PORT_PRI_FLAG;
            node->mst_info[mstp_index].priority = STP_DFLT_PORT_PRIORITY >> 4;
        }
    }
}

uint16_t stp_intf_get_inst_port_priority(PORT_ID port_id, UINT16 mstp_index)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if((mstp_index != MSTP_INDEX_INVALID) && (node->mst_info[mstp_index].flag & MSTP_PORT_PRI_FLAG))
        {
            return (node->mst_info[mstp_index].priority);
        }
        else
        {
            return(node->priority);
        }
    }
    return (STP_DFLT_PORT_PRIORITY >> 4);
}

bool stp_intf_is_inst_port_priority_set(PORT_ID port_id, UINT16 mstp_index)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if(node->mst_info[mstp_index].flag & MSTP_PORT_PRI_FLAG)
            return true;
        else
            return false;
    }
    return false;
}

void stp_intf_set_inst_port_pathcost(PORT_ID port_id, UINT16 mstp_index, UINT32 cost, UINT8 add)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if(add)
        {
            node->mst_info[mstp_index].flag |= MSTP_PORT_PATH_COST_FLAG;
            node->mst_info[mstp_index].path_cost = cost;
        }
        else
        {
            node->mst_info[mstp_index].flag &= ~MSTP_PORT_PATH_COST_FLAG;
            node->mst_info[mstp_index].path_cost = cost;
        }
    }
}

UINT32 stp_intf_get_inst_port_pathcost(PORT_ID port_id, UINT16 mstp_index)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if((mstp_index != MSTP_INDEX_INVALID) &&(node->mst_info[mstp_index].flag & MSTP_PORT_PATH_COST_FLAG))
        {
            return (node->mst_info[mstp_index].path_cost);
        }
        else
        {
            /* Node path cost will be port level cost */
            return(node->path_cost);
        }
    }
    return false;
}

bool stp_intf_is_inst_port_pathcost_set(PORT_ID port_id, UINT16 mstp_index)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if(node->mst_info[mstp_index].flag & MSTP_PORT_PATH_COST_FLAG)
            return true;
        else
            return false;
    }
    return false;
}

bool stp_intf_is_default_port_pathcost(PORT_ID port_id, UINT16 mstp_index)
{
    INTERFACE_NODE *node = NULL;

    node = stp_intf_get_node(port_id);
    if (node)
    {
        if((node->mst_info[mstp_index].flag & MSTP_PORT_PATH_COST_FLAG) ||
                !node->def_path_cost)
            return false;
        else
            return true;
    }
    return true;
}