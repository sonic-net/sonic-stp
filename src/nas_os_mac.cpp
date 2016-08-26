
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



#include "dell-base-l2-mac.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "nas_nlmsg.h"
#include "netlink_tools.h"
#include "nas_os_vlan_utils.h"
#include "std_mac_utils.h"

#include <string>
#include <netinet/in.h>
#include <linux/if_bridge.h>
#include <sys/socket.h>

#define NL_MSG_BUFF_LEN 4096
#define MAC_STRING_LEN 20

extern "C"{

t_std_error nas_os_mac_update_entry(cps_api_object_t obj){

    cps_api_object_attr_t ifindex_attr = cps_api_object_attr_get(obj,BASE_MAC_TABLE_IFINDEX);
    cps_api_object_attr_t mac_attr = cps_api_object_attr_get(obj,BASE_MAC_TABLE_MAC_ADDRESS);
    cps_api_object_attr_t static_attr = cps_api_object_attr_get(obj,BASE_MAC_TABLE_STATIC);
    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if(ifindex_attr == NULL ||  mac_attr == NULL){
        EV_LOG(ERR,NAS_OS,0,"NAS-OS-MAC","Ifindex/MAC Missing for creating/updating MAC"
            "Entry in the kernel bridge");
        return STD_ERR(L2MAC,PARAM,0);
    }

    char buff[NL_MSG_BUFF_LEN];
    memset(buff,0,sizeof(buff));
    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ndmsg *req = (struct ndmsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ndmsg));

    bool is_static = false;
    if(static_attr){
        is_static = cps_api_object_attr_data_u32(static_attr);
    }
    nlh->nlmsg_flags = NLM_F_REQUEST;
    std::string s;
    if(op == cps_api_oper_CREATE){
        nlh->nlmsg_flags |=   NLM_F_CREATE | (is_static ? NLM_F_APPEND : NLM_F_EXCL) ;
        nlh->nlmsg_type = RTM_NEWNEIGH ;
        s = "create";
    }else if (op == cps_api_oper_SET){
        nlh->nlmsg_flags |=   NLM_F_CREATE | NLM_F_APPEND ;
        nlh->nlmsg_type = RTM_NEWNEIGH ;
        s = "set";
    }else if(op == cps_api_oper_DELETE){
        nlh->nlmsg_type = RTM_DELNEIGH ;
        s = "delete";

    }else{
        EV_LOG(ERR,NAS_OS,0,"NAS-L2-MAC","Invalid/No operation passed when configuring MAC "
                "entry in the kernel");
        return STD_ERR(L2MAC,PARAM,0);
    }

    req->ndm_family = PF_BRIDGE;
    req->ndm_state = is_static ? NUD_PERMANENT : NUD_REACHABLE;
    req->ndm_flags = NTF_MASTER;
    req->ndm_ifindex = cps_api_object_attr_data_u32(ifindex_attr);
    nlmsg_add_attr(nlh,sizeof(buff),NDA_LLADDR,(void *)cps_api_object_attr_data_bin(mac_attr),sizeof(hal_mac_addr_t));
    hal_mac_addr_t *mac_addr = (hal_mac_addr_t*)cps_api_object_attr_data_bin(mac_attr);
    char mac_buff[MAC_STRING_LEN];

    if(nl_do_set_request(nas_nl_sock_T_NEI,nlh, buff, sizeof(buff)) != STD_ERR_OK){
        EV_LOG(ERR,NAS_OS,0,"NAS-L2-MAC","Failed to %s mac address entry %s for Interface %d "
                "with cps operation %d in Kernel",s.c_str(),std_mac_to_string(mac_addr,mac_buff,sizeof(mac_buff)),
                req->ndm_ifindex,op);
        return STD_ERR(L2MAC,FAIL,0);
    }
    EV_LOG(INFO,NAS_OS,0,"NAS-L2-MAC","%sd mac address entry %s for Interface %d with"
            "cps operation %d in Kernel",s.c_str(),std_mac_to_string(mac_addr,mac_buff,sizeof(mac_buff)),
            req->ndm_ifindex,op);
    return STD_ERR_OK;
}

}

extern "C"{

t_std_error nas_os_mac_change_learning(hal_ifindex_t ifindex,bool enable){

    char buff[NL_MSG_BUFF_LEN];
    memset(buff,0,sizeof(buff));
    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    nlh->nlmsg_pid = 0 ;
    nlh->nlmsg_seq = 0 ;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_type = RTM_SETLINK ;
    ifmsg->ifi_family = PF_BRIDGE;
    ifmsg->ifi_index = ifindex;

    struct nlattr *mac_attr = nlmsg_nested_start(nlh, sizeof(buff));
    mac_attr->nla_len = 0;
    mac_attr->nla_type = IFLA_PROTINFO | NLA_F_NESTED;
    uint8_t learning = (uint8_t)enable;
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_BRPORT_LEARNING,(void *)&learning,sizeof(uint8_t));
    nlmsg_nested_end(nlh, mac_attr);

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh, buff, sizeof(buff)) != STD_ERR_OK){
        EV_LOG(ERR,NAS_OS,0,"NAS-L2-MAC","Failed to set mac learn mode to %d for interface %d "
                "in Kernel",enable,ifindex);
        return STD_ERR(L2MAC,FAIL,0);
    }

    EV_LOG(INFO,NAS_OS,3,"NAS-L2-MAC","Set the MAC learning for interface index %d to %d in kernel",ifindex,enable);
    return STD_ERR_OK;
}

}
