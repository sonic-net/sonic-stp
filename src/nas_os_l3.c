/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: nas_os_l3.c
 */


#include "cps_api_object_attr.h"
#include "dell-base-routing.h"
#include "standard_netlink_requests.h"
#include "std_error_codes.h"
#include "netlink_tools.h"
#include "nas_os_l3.h"
#include "event_log.h"
#include "cps_api_operation.h"
#include "cps_api_route.h"
#include "nas_nlmsg.h"
#include "net_publish.h"
#include "std_ip_utils.h"
#include "nas_nlmsg_object_utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define NL_RT_MSG_BUFFER_LEN 16384

typedef enum msg_type {
    NAS_RT_ADD,
    NAS_RT_DEL,
    NAS_RT_SET
}msg_type;

extern char *nl_neigh_mac_to_str (hal_mac_addr_t *mac_addr);
static inline uint16_t nas_os_get_nl_flags(msg_type m_type)
{
    uint16_t flags = (NLM_F_REQUEST | NLM_F_ACK);
    if(m_type == NAS_RT_ADD) {
        flags |= NLM_F_CREATE | NLM_F_EXCL;
    } else if (m_type == NAS_RT_SET) {
        flags |= NLM_F_CREATE | NLM_F_REPLACE;
    }
    return flags;
}

#define MAX_CPS_MSG_SIZE 10000

/*
 * Publish route from here is mainly to handle route delete cases.
 * When application try to delete a route and if it is already deleted in kernel
 * (e.g interface down scenario), then NAS-L3 need to get this delete message.
 * NH and other related info is currently not needed. (Please check the invocation below)
 *
 * @Todo, This has to be revisited for any generic solution. This approach is to handle
 * the current open issues related to interface "shutdown"
 */

static t_std_error nas_os_publish_route(int rt_msg_type, cps_api_object_t obj)
{
    static char buff[MAX_CPS_MSG_SIZE];

    cps_api_object_t new_obj = cps_api_object_init(buff,sizeof(buff));
    cps_api_key_init(cps_api_object_key(new_obj),cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,cps_api_route_obj_ROUTE,0 );

    cps_api_object_attr_t prefix   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    cps_api_object_attr_t af       = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_AF);
    cps_api_object_attr_t pref_len = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);
    cps_api_object_attr_t nh_count = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_NH_COUNT);

    if(rt_msg_type == RTM_NEWROUTE) {
        cps_api_object_attr_add_u32(new_obj, cps_api_if_ROUTE_A_MSG_TYPE,ROUTE_ADD);
    } else if(rt_msg_type == RTM_DELROUTE) {
        cps_api_object_attr_add_u32(new_obj, cps_api_if_ROUTE_A_MSG_TYPE,ROUTE_DEL);
    } else {
        return false;
    }

    cps_api_object_attr_add_u32(new_obj,cps_api_if_ROUTE_A_FAMILY,
            cps_api_object_attr_data_u32(af));

    uint32_t addr_len;
    hal_ip_addr_t ip;
    if(cps_api_object_attr_data_u32(af) == AF_INET) {
        struct in_addr *inp = (struct in_addr *) cps_api_object_attr_data_bin(prefix);
        std_ip_from_inet(&ip,inp);
        addr_len = HAL_INET4_LEN;
    } else {
        struct in6_addr *inp6 = (struct in6_addr *) cps_api_object_attr_data_bin(prefix);
        std_ip_from_inet6(&ip,inp6);
        addr_len = HAL_INET6_LEN;
    }

    cps_api_attr_id_t attr = cps_api_if_ROUTE_A_PREFIX;
    cps_api_object_e_add(new_obj, &attr, 1, cps_api_object_ATTR_T_BIN, &ip, sizeof(ip));

    cps_api_object_attr_add_u32(new_obj,cps_api_if_ROUTE_A_PREFIX_LEN,
            cps_api_object_attr_data_u32(pref_len));

    uint32_t nhc = 0;
    if (nh_count != CPS_API_ATTR_NULL) nhc = cps_api_object_attr_data_u32(nh_count);

    if (nhc == 1) {
        cps_api_object_attr_add_u32(new_obj,cps_api_if_ROUTE_A_HOP_COUNT,nhc);

        cps_api_attr_id_t ids[3] = { BASE_ROUTE_OBJ_ENTRY_NH_LIST,
                                 0, BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR};
        const int ids_len = sizeof(ids)/sizeof(*ids);

        cps_api_object_attr_t gw = cps_api_object_e_get(obj,ids,ids_len);

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
        cps_api_object_attr_t gwix = cps_api_object_e_get(obj,ids,ids_len);

        cps_api_attr_id_t new_ids[3];
        new_ids[0] = cps_api_if_ROUTE_A_NH;
        new_ids[1] = 0;

        if (gw != CPS_API_ATTR_NULL) {
            new_ids[2] = cps_api_if_ROUTE_A_NEXT_HOP_ADDR;

            hal_ip_addr_t ip;
            if(addr_len == HAL_INET4_LEN) {
                ip.af_index = AF_INET;
                memcpy(&(ip.u.v4_addr), cps_api_object_attr_data_bin(gw),addr_len);
            } else {
                ip.af_index = AF_INET6;
                memcpy(&(ip.u.v6_addr), cps_api_object_attr_data_bin(gw),addr_len);
            }
            cps_api_object_e_add(new_obj, new_ids, ids_len, cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
        }

        if (gwix != CPS_API_ATTR_NULL) {
            new_ids[2] = cps_api_if_ROUTE_A_NH_IFINDEX;
            uint32_t gw_idx = cps_api_object_attr_data_u32(gwix);
            cps_api_object_e_add(new_obj,new_ids,ids_len,cps_api_object_ATTR_T_U32,
                     (void *)&gw_idx, sizeof(uint32_t));
        }
    }

    EV_LOG(INFO, NAS_OS, 2,"ROUTE-UPD","Publishing object");

    net_publish_event(new_obj);

    return STD_ERR_OK;
}

cps_api_return_code_t nas_os_update_route (cps_api_object_t obj, msg_type m_type)
{
    static char buff[NL_RT_MSG_BUFFER_LEN]; // Allocate from DS
    char            addr_str[INET6_ADDRSTRLEN];
    memset(buff,0,sizeof(buff));

    cps_api_object_attr_t prefix   = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX);
    cps_api_object_attr_t af       = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_AF);
    cps_api_object_attr_t nh_count = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_NH_COUNT);
    cps_api_object_attr_t pref_len = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN);

    if (prefix == CPS_API_ATTR_NULL || af == CPS_API_ATTR_NULL ||  pref_len == CPS_API_ATTR_NULL
        || (m_type != NAS_RT_DEL && nh_count == CPS_API_ATTR_NULL)) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "ROUTE-UPD", "Missing route params");
        return cps_api_ret_code_ERR;
    }

    struct nlmsghdr *nlh = (struct nlmsghdr *)
                         nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct rtmsg * rm = (struct rtmsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct rtmsg));

    uint16_t flags = nas_os_get_nl_flags(m_type);
    uint16_t type = (m_type == NAS_RT_DEL)?RTM_DELROUTE:RTM_NEWROUTE;

    nas_os_pack_nl_hdr(nlh, type, flags);

    rm->rtm_table = RT_TABLE_MAIN;
    rm->rtm_protocol = RTPROT_UNSPEC; // This could be assigned to correct owner in future

    /* For route delete, initialize scope to no-where and
     * this will get updated to link when Nh addr/ifx is provided.
     */
    if (type != RTM_DELROUTE)
        rm->rtm_scope = RT_SCOPE_UNIVERSE;
    else
        rm->rtm_scope = RT_SCOPE_NOWHERE;

    rm->rtm_type = RTN_UNICAST;

    rm->rtm_dst_len = cps_api_object_attr_data_u32(pref_len);
    rm->rtm_family = (unsigned char) cps_api_object_attr_data_u32(af);

    uint32_t addr_len = (rm->rtm_family == AF_INET)?HAL_INET4_LEN:HAL_INET6_LEN;
    nlmsg_add_attr(nlh,sizeof(buff),RTA_DST,cps_api_object_attr_data_bin(prefix),addr_len);

    uint32_t nhc = 0;
    if (nh_count != CPS_API_ATTR_NULL) nhc = cps_api_object_attr_data_u32(nh_count);

    EV_LOG(INFO, NAS_OS,2,"ROUTE-UPD","NH count:%d family:%s msg:%s for prefix:%s len:%d scope:%d", nhc,
           ((rm->rtm_family == AF_INET) ? "IPv4" : "IPv6"), ((m_type == NAS_RT_ADD) ? "Route-Add" : ((m_type == NAS_RT_DEL) ? "Route-Del" : "Route-Set")),
           ((rm->rtm_family == AF_INET) ?
            (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(prefix), addr_str, INET_ADDRSTRLEN)) :
            (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(prefix), addr_str, INET6_ADDRSTRLEN))),
           rm->rtm_dst_len, rm->rtm_scope);

    if (nhc == 1) {
        cps_api_attr_id_t ids[3] = { BASE_ROUTE_OBJ_ENTRY_NH_LIST,
                                     0, BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR};
        const int ids_len = sizeof(ids)/sizeof(*ids);
        cps_api_object_attr_t gw = cps_api_object_e_get(obj,ids,ids_len);
        if (gw != CPS_API_ATTR_NULL) {
            nlmsg_add_attr(nlh,sizeof(buff),RTA_GATEWAY,cps_api_object_attr_data_bin(gw),addr_len);
            rm->rtm_scope = RT_SCOPE_UNIVERSE; // set scope to universe when gateway is specified
            EV_LOG(INFO, NAS_OS, 2,"ROUTE-UPD","NH:%s scope:%d",
                   ((rm->rtm_family == AF_INET) ?
                    (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(gw), addr_str, INET_ADDRSTRLEN)) :
                    (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(gw), addr_str, INET6_ADDRSTRLEN))),
                   rm->rtm_scope);
        } else {
            EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "ROUTE-UPD", "Missing Gateway, could be intf route");
            /*
             * This could be an interface route, do not return from here!
             */
        }

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
        cps_api_object_attr_t gwix = cps_api_object_e_get(obj,ids,ids_len);
        if (gwix != CPS_API_ATTR_NULL) {
            if (gw == CPS_API_ATTR_NULL) {
                rm->rtm_scope = RT_SCOPE_LINK;
            }

            EV_LOG(INFO, NAS_OS,2,"ROUTE-UPD","out-intf: %d scope:%d",
                   (int)cps_api_object_attr_data_u32(gwix), rm->rtm_scope);
            nas_nl_add_attr_int(nlh,sizeof(buff),RTA_OIF,gwix);
        }

        ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_WEIGHT;
        cps_api_object_attr_t weight = cps_api_object_e_get(obj,ids,ids_len);
        if (weight != CPS_API_ATTR_NULL) nas_nl_add_attr_int(nlh,sizeof(buff),RTA_PRIORITY,weight);

    } else if (nhc > 1){
        struct nlattr * attr_nh = nlmsg_nested_start(nlh, sizeof(buff));

        attr_nh->nla_len = 0;
        attr_nh->nla_type = RTA_MULTIPATH;
        size_t ix = 0;
        for (ix = 0; ix < nhc ; ++ix) {
            struct rtnexthop * rtnh =
                (struct rtnexthop * )nlmsg_reserve(nlh,sizeof(buff), sizeof(struct rtnexthop));
            memset(rtnh,0,sizeof(*rtnh));

            cps_api_attr_id_t ids[3] = { BASE_ROUTE_OBJ_ENTRY_NH_LIST,
                                         ix, BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR};
            const int ids_len = sizeof(ids)/sizeof(*ids);
            cps_api_object_attr_t attr = cps_api_object_e_get(obj,ids,ids_len);
            if (attr != CPS_API_ATTR_NULL) {
                nlmsg_add_attr(nlh,sizeof(buff),RTA_GATEWAY,
                               cps_api_object_attr_data_bin(attr),addr_len);
                rm->rtm_scope = RT_SCOPE_UNIVERSE; // set scope to universe when gateway is specified
                EV_LOG(INFO, NAS_OS,2,"ROUTE-UPD","MP-NH:%d %s scope:%d",ix,
                       ((rm->rtm_family == AF_INET) ?
                        (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(attr), addr_str, INET_ADDRSTRLEN)) :
                        (inet_ntop(rm->rtm_family, cps_api_object_attr_data_bin(attr), addr_str, INET6_ADDRSTRLEN))),
                       rm->rtm_scope);
            } else {
                EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "ROUTE-UPD", "Error - Missing Gateway");
                return cps_api_ret_code_ERR;
            }

            ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;
            attr = cps_api_object_e_get(obj,ids,ids_len);
            if (attr != CPS_API_ATTR_NULL)
                rtnh->rtnh_ifindex = (int)cps_api_object_attr_data_u32(attr);

            ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_WEIGHT;
            attr = cps_api_object_e_get(obj,ids,ids_len);
            if (attr != CPS_API_ATTR_NULL) rtnh->rtnh_hops = (char)cps_api_object_attr_data_u32(attr);

            rtnh->rtnh_len = (char*)nlmsg_tail(nlh) - (char*)rtnh;
        }
        nlmsg_nested_end(nlh,attr_nh);
    }

    t_std_error rc = nl_do_set_request(nas_nl_sock_T_ROUTE,nlh,buff,sizeof(buff));
    int err_code = STD_ERR_EXT_PRIV (rc);
    EV_LOG(INFO, NAS_OS,2,"ROUTE-UPD","Netlink error_code %d", err_code);

    /*
     * Return success if the error is exist, in case of addition, or
     * no-exist, in case of deletion. This is because, kernel might have
     * deleted the route entries (when interface goes down) but has not sent netlink
     * events for those routes and RTM is trying to delete after that.
     * Similarly, during ip address configuration, kernel may add the routes
     * before RTM tries to configure kernel.
     *
     */
    if(err_code == ESRCH || err_code == EEXIST ) {
        EV_LOG(INFO, NAS_OS,2,"ROUTE-UPD","No such process or Entry already exists");
        /*
         * Kernel may or may not have the routes but NAS routing needs to be informed
         * as is from kernel netlink to program NPU for the route addition/deletion to
         * ensure stale routes are cleaned
         */
        if(err_code == ESRCH)
            nas_os_publish_route(RTM_DELROUTE, obj);
        else
            nas_os_publish_route(RTM_NEWROUTE, obj);
        rc = STD_ERR_OK;
    }

    return rc;

}

t_std_error nas_os_add_route (cps_api_object_t obj)
{

    if (nas_os_update_route(obj, NAS_RT_ADD) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "ROUTE-ADD", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_set_route (cps_api_object_t obj)
{

    if (nas_os_update_route(obj, NAS_RT_SET) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "ROUTE-SET", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_del_route (cps_api_object_t obj)
{

    if (nas_os_update_route(obj, NAS_RT_DEL) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "ROUTE-DEL", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}

cps_api_return_code_t nas_os_update_neighbor(cps_api_object_t obj, msg_type m_type)
{
    static char buff[NL_RT_MSG_BUFFER_LEN];
    hal_mac_addr_t mac_addr;
    char            addr_str[INET6_ADDRSTRLEN];
    memset(buff,0,sizeof(buff));
    memset(mac_addr, 0, sizeof(mac_addr));

    cps_api_object_attr_t ip  = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_ADDRESS);
    cps_api_object_attr_t af  = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_AF);
    cps_api_object_attr_t mac = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR);
    cps_api_object_attr_t if_index = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_IFINDEX);
    cps_api_object_attr_t nbr_type = cps_api_object_attr_get(obj, BASE_ROUTE_OBJ_NBR_TYPE);

    if (ip == CPS_API_ATTR_NULL || af == CPS_API_ATTR_NULL || if_index == CPS_API_ATTR_NULL
        || (m_type != NAS_RT_DEL && mac == CPS_API_ATTR_NULL)) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NEIGH-UPD", "Missing neighbor params");
        return cps_api_ret_code_ERR;
    }

    struct nlmsghdr *nlh = (struct nlmsghdr *)
                         nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ndmsg * ndm = (struct ndmsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ndmsg));


    ndm->ndm_ifindex = cps_api_object_attr_data_u32(if_index);
    ndm->ndm_family = (unsigned char) cps_api_object_attr_data_u32(af);
    if (nbr_type != CPS_API_ATTR_NULL) {
        if (((unsigned char) cps_api_object_attr_data_u32(nbr_type))
            == BASE_ROUTE_RT_TYPE_STATIC){
            /* Static ARP handling */
            ndm->ndm_state = NUD_PERMANENT;
            /* Set this flag to replace the dynamic ARP to static if exists */
            if (m_type == NAS_RT_ADD)
                m_type = NAS_RT_SET;
        }else{
            ndm->ndm_state = NUD_REACHABLE;
        }
    } else {
        /* if NH type is not given, assume the state as permanent */
        ndm->ndm_state = NUD_PERMANENT;
        /* Set this flag to replace the dynamic ARP to static if exists */
        if (m_type == NAS_RT_ADD)
            m_type = NAS_RT_SET;
    }
    uint16_t flags = nas_os_get_nl_flags(m_type);
    uint16_t type = (m_type == NAS_RT_DEL)?RTM_DELNEIGH:RTM_NEWNEIGH;
    nas_os_pack_nl_hdr(nlh, type, flags);

    ndm->ndm_type = RTN_UNICAST;

    uint32_t addr_len = (ndm->ndm_family == AF_INET)?HAL_INET4_LEN:HAL_INET6_LEN;
    nlmsg_add_attr(nlh,sizeof(buff),NDA_DST,cps_api_object_attr_data_bin(ip),addr_len);

    if (mac != CPS_API_ATTR_NULL) {
        void * data = cps_api_object_attr_data_bin(mac);
        memcpy(mac_addr,data,sizeof(mac_addr));
        nlmsg_add_attr(nlh,sizeof(buff),NDA_LLADDR, &mac_addr, HAL_MAC_ADDR_LEN);
    }

    t_std_error rc = nl_do_set_request(nas_nl_sock_T_NEI,nlh,buff,sizeof(buff));
    int err_code = STD_ERR_EXT_PRIV (rc);
    EV_LOG(INFO, NAS_OS, 2,"NEIGH-UPD","Operation:%s family:%s NH:%s MAC:%s out-intf:%d state:%s rc:%d",
           ((m_type == NAS_RT_DEL) ? "Arp-Del" : "Arp-Add"), ((ndm->ndm_family == AF_INET) ? "IPv4" : "IPv6"),
           ((ndm->ndm_family == AF_INET) ?
            (inet_ntop(ndm->ndm_family, cps_api_object_attr_data_bin(ip), addr_str, INET_ADDRSTRLEN)) :
            (inet_ntop(ndm->ndm_family, cps_api_object_attr_data_bin(ip), addr_str, INET6_ADDRSTRLEN))),
           nl_neigh_mac_to_str(&mac_addr), ndm->ndm_ifindex,
           ((ndm->ndm_state == NUD_REACHABLE) ? "Dynamic" : "Static"), err_code);
    return rc;
}

t_std_error nas_os_add_neighbor (cps_api_object_t obj)
{

    if (nas_os_update_neighbor(obj, NAS_RT_ADD) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NEIGH-ADD", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_set_neighbor (cps_api_object_t obj)
{

    if (nas_os_update_neighbor(obj, NAS_RT_SET) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NEIGH-SET", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_del_neighbor (cps_api_object_t obj)
{

    if (nas_os_update_neighbor(obj, NAS_RT_DEL) != cps_api_ret_code_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NEIGH-DEL", "Kernel write failed");
        return (STD_ERR(NAS_OS, FAIL, 0));
    }

    return STD_ERR_OK;
}
