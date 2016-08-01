/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*
 * ds_api_linux_neigh.c
 */

#include "standard_netlink_requests.h"
#include "cps_api_route.h"
#include "cps_api_operation.h"
#include "cps_api_interface_types.h"
#include "std_error_codes.h"
#include "nas_nlmsg_object_utils.h"
#include "event_log.h"

#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>

#include <stdio.h>
#include <arpa/inet.h>

// @TODO - This file has to conform to new yang model

char *nl_neigh_mac_to_str (hal_mac_addr_t *mac_addr) {
    static char str[18];
    snprintf (str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x",
             (*mac_addr) [0], (*mac_addr) [1], (*mac_addr) [2],
             (*mac_addr) [3], (*mac_addr) [4], (*mac_addr) [5]);

    return str;
}

char *nl_neigh_state_to_str (int state) {
    static char str[18];
        if (state == NUD_INCOMPLETE)
            snprintf (str, sizeof(str), "Incomplete");
        else if (state == NUD_REACHABLE)
            snprintf (str, sizeof(str), "Reachable");
        else if (state == NUD_STALE)
            snprintf (str, sizeof(str), "Stale");
        else if (state == NUD_DELAY)
            snprintf (str, sizeof(str), "Delay");
        else if (state == NUD_PROBE)
            snprintf (str, sizeof(str), "Probe");
        else if (state == NUD_FAILED)
            snprintf (str, sizeof(str), "Failed");
        else if (state == NUD_NOARP)
            snprintf (str, sizeof(str), "NoArp");
        else if (state == NUD_PERMANENT)
            snprintf (str, sizeof(str), "Static");
        else
            snprintf (str, sizeof(str), "None");

    return str;
}

bool nl_neigh_get_all_request(int sock, int family,int req_id) {
    struct ifinfomsg ifm;
    memset(&ifm,0,sizeof(ifm));
    ifm.ifi_family = family;
    return nl_send_request(sock,RTM_GETNEIGH,
            NLM_F_ROOT| NLM_F_DUMP|NLM_F_REQUEST,
            req_id,&ifm,sizeof(ifm));
}

bool nl_to_neigh_info(int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj) {
    struct ndmsg    *ndmsg = (struct ndmsg *)NLMSG_DATA(hdr);
    struct rtattr   *rtatp = NULL;
    unsigned int     attrlen;
    char             addr_str[INET6_ADDRSTRLEN];


    if(hdr->nlmsg_len < NLMSG_LENGTH(sizeof(*ndmsg)))
        return false;

    attrlen = hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*ndmsg));

    if(rt_msg_type == RTM_NEWNEIGH) {
        cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_OPERATION,NBR_ADD);
    } else if(rt_msg_type == RTM_DELNEIGH) {
        cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_OPERATION,NBR_DEL);
    } else {
        return false;
    }
    cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_IFINDEX,ndmsg->ndm_ifindex);
    cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_FLAGS,ndmsg->ndm_flags);
    cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_STATE,ndmsg->ndm_state);
    cps_api_object_attr_add_u32(obj,cps_api_if_NEIGH_A_FAMILY,ndmsg->ndm_family);

    EV_LOG(INFO, NETLINK,3,"NH-EVENT","Op:%s fly:%s(%d) flags:0x%x state:%s(%d) ifx:%d",
           ((rt_msg_type == RTM_NEWNEIGH) ? "Add-NH" : "Del-NH"),
           ((ndmsg->ndm_family == AF_INET) ? "IPv4" : "IPv6"), ndmsg->ndm_family,
           ndmsg->ndm_flags, nl_neigh_state_to_str(ndmsg->ndm_state), ndmsg->ndm_state,
           ndmsg->ndm_ifindex);

    rtatp = ((struct rtattr*)(((char*)(ndmsg)) + NLMSG_ALIGN(sizeof(struct ndmsg))));

    for (; RTA_OK(rtatp, attrlen); rtatp = RTA_NEXT (rtatp, attrlen)) {

        if(rtatp->rta_type == NDA_DST) {
            rta_add_ip((struct nlattr*)rtatp,ndmsg->ndm_family, obj,cps_api_if_NEIGH_A_NBR_ADDR);
            EV_LOG(INFO, NETLINK,3,"NH-EVENT","NextHop IP:%s",
                   ((ndmsg->ndm_family == AF_INET) ?
                    (inet_ntop(ndmsg->ndm_family, ((struct in_addr *) nla_data((struct nlattr*)rtatp)), addr_str, INET_ADDRSTRLEN)) :
                    (inet_ntop(ndmsg->ndm_family, ((struct in6_addr *) nla_data((struct nlattr*)rtatp)), addr_str, INET6_ADDRSTRLEN))));

        }

        if(rtatp->rta_type == NDA_LLADDR) {
            rta_add_mac((struct nlattr*)rtatp,obj,cps_api_if_NEIGH_A_NBR_MAC);
            EV_LOG(INFO, NETLINK,3,"NH-EVENT","NextHop MAC:%s", nl_neigh_mac_to_str(nla_data((struct nlattr*)rtatp)));
        }

    }
    return true;
}

static bool process_neigh_and_add_to_list(int rt_msg_type, struct nlmsghdr *nh, void *context) {
    cps_api_object_list_t *list = (cps_api_object_list_t*) context;
    cps_api_object_t obj=cps_api_object_create();
    if (!cps_api_object_list_append(*list,obj)) {
        cps_api_object_delete(obj);
        return false;
    }
    if (!nl_to_neigh_info(nh->nlmsg_type,nh,obj)) {
        return false;
    }
    return true;
}

static bool read_all_neighbours(cps_api_object_list_t list) {
    int sock = nas_nl_sock_create(nas_nl_sock_T_NEI);
    if (sock<0) return false;

    bool rc = false;
    int RANDOM_ID=21323;
    if (nl_neigh_get_all_request(sock,AF_INET,RANDOM_ID)) {
        char buff[1024];
        rc = netlink_tools_process_socket(sock,
                process_neigh_and_add_to_list,&list,
                buff,sizeof(buff),&RANDOM_ID,NULL);
    }

    if (rc && nl_neigh_get_all_request(sock,AF_INET6,++RANDOM_ID)) {
        char buff[1024];
        rc = netlink_tools_process_socket(sock,
                process_neigh_and_add_to_list,&list,
                buff,sizeof(buff),&RANDOM_ID,NULL);
    }

    close(sock);
    return rc;
}

static cps_api_return_code_t db_read_function (void * context, cps_api_get_params_t * param,
        size_t key_ix) {

    cps_api_return_code_t rc = cps_api_ret_code_OK;
    cps_api_key_t key;
    cps_api_key_init(&key,cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE, cps_api_route_obj_NEIBH, 0);
    if (cps_api_key_matches(&param->keys[key_ix], &key,false)!=0) {
        return cps_api_ret_code_OK;
    }

    read_all_neighbours(param->list);

    return rc;
}

static cps_api_return_code_t db_write_function(void * context, cps_api_transaction_params_t * param,size_t ix) {
    return cps_api_ret_code_ERR;
}

t_std_error ds_api_linux_neigh_init(cps_api_operation_handle_t handle) {
    cps_api_registration_functions_t f;
    memset(&f,0,sizeof(f));
    f.handle = handle;
    f._read_function = db_read_function;
    f._write_function = db_write_function;
    cps_api_key_init(&f.key,cps_api_qualifier_TARGET,cps_api_obj_cat_ROUTE,cps_api_route_obj_NEIBH,0);

    cps_api_return_code_t rc = cps_api_register(&f);

    return STD_ERR_OK_IF_TRUE(rc==cps_api_ret_code_OK,STD_ERR(ROUTE,FAIL,rc));
}
