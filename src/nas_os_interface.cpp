
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*
 * filename: nas_os_interface.c
 */


#include "event_log.h"
#include "netlink_tools.h"
#include "nas_nlmsg.h"

#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"
#include "std_assert.h"
#include "dell-interface.h"
#include "dell-base-if.h"
#include "dell-base-if-linux.h"
#include "nas_os_interface.h"
#include "nas_os_if_priv.h"
#include "nas_os_int_utils.h"

#include "nas_nlmsg_object_utils.h"
#include "ds_api_linux_interface.h"
#include "std_mac_utils.h"

#include <sys/socket.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>


#include <pthread.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <map>


#define NL_MSG_INTF_BUFF_LEN 2048

extern "C" t_std_error nas_os_del_interface(hal_ifindex_t if_index)
{
    char buff[NL_MSG_INTF_BUFF_LEN];

    memset(buff,0,sizeof(buff));

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Del Interface %d", if_index);

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    nas_os_pack_nl_hdr(nlh, RTM_DELLINK, (NLM_F_REQUEST | NLM_F_ACK));

    nas_os_pack_if_hdr(ifmsg, AF_UNSPEC, 0, if_index);

    if(nl_do_set_request(nas_nl_sock_T_INT, nlh, buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure deleting interface in kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }
    return STD_ERR_OK;
}

extern "C" t_std_error nas_os_get_interface_obj_by_name(const char *ifname, cps_api_object_t obj) {
    hal_ifindex_t ifix = cps_api_interface_name_to_if_index(ifname);
    if (ifix==0) return STD_ERR(NAS_OS,PARAM,0);
    return nas_os_get_interface_obj(ifix,obj);
}

extern "C" t_std_error nas_os_get_interface_obj(hal_ifindex_t ifix,cps_api_object_t obj) {
    cps_api_object_list_guard lg(cps_api_object_list_create());
    if (lg.get()==NULL) return STD_ERR(NAS_OS,FAIL,0);

    if (_get_interfaces(lg.get(),ifix,false)!=cps_api_ret_code_OK) {
        return STD_ERR(NAS_OS,FAIL,0);
    }

    cps_api_object_t ret = cps_api_object_list_get(lg.get(),0);
    if (ret==nullptr) {
        return STD_ERR(NAS_OS,FAIL,0);
    }
    cps_api_object_clone(obj,ret);
    return STD_ERR_OK;
}

extern "C" t_std_error nas_os_get_interface(cps_api_object_t filter,cps_api_object_list_t result) {
    cps_api_object_attr_t ifix = cps_api_object_attr_get(filter, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
    hal_ifindex_t ifindex = 0;
    if (ifix!=NULL) {
        ifindex = cps_api_object_attr_data_u32(ifix);
    }
    _get_interfaces(result,ifindex, ifix==NULL);
    return STD_ERR_OK;
}

static void _set_mac(cps_api_object_t obj, struct nlmsghdr *nlh, struct ifinfomsg * inf,size_t len) {
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS);
    if (attr==NULL) return;
    void *addr = cps_api_object_attr_data_bin(attr);
    hal_mac_addr_t mac_addr;

    int addr_len = strlen(static_cast<char *>(addr));
    if (std_string_to_mac(&mac_addr, static_cast<const char *>(addr), addr_len)) {
        char mac_str[40] = {0};
        EV_LOG(INFO, NAS_OS, ev_log_s_MAJOR, "NAS-OS", "Setting mac address %s, actual string %s, len %d",
                std_mac_to_string(&mac_addr,mac_str,sizeof(mac_str)), static_cast<char *>(addr), addr_len);
        nlmsg_add_attr(nlh,len,IFLA_ADDRESS, mac_addr , cps_api_object_attr_len(attr));
    }
}

static void _set_mtu(cps_api_object_t obj, struct nlmsghdr *nlh, struct ifinfomsg * inf,size_t len) {
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_MTU);
    if (attr==NULL) return;
    int mtu = (int)cps_api_object_attr_data_uint(attr) - NAS_LINK_MTU_HDR_SIZE;
    nlmsg_add_attr(nlh,len,IFLA_MTU, &mtu , sizeof(mtu));
}

static void _set_name(cps_api_object_t obj, struct nlmsghdr *nlh, struct ifinfomsg * inf,size_t len) {
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj, IF_INTERFACES_INTERFACE_NAME);
    if (attr==NULL) return;
    const char *name = (const char*)cps_api_object_attr_data_bin(attr);
    nlmsg_add_attr(nlh,len,IFLA_IFNAME, name , strlen(name)+1);
}

static void _set_ifalias(cps_api_object_t obj, struct nlmsghdr *nlh, struct ifinfomsg * inf,size_t len) {
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj,NAS_OS_IF_ALIAS);
    if (attr==NULL) return;
    const char *name = (const char*)cps_api_object_attr_data_bin(attr);
    nlmsg_add_attr(nlh,len,IFLA_IFALIAS, name , strlen(name)+1);
}

static void _set_admin(cps_api_object_t obj, struct nlmsghdr *nlh, struct ifinfomsg * inf,size_t len) {
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj,IF_INTERFACES_INTERFACE_ENABLED);
    if (attr==NULL) return;
    if ((int)cps_api_object_attr_data_uint(attr) == true) {
        inf->ifi_flags |= IFF_UP;
    } else if ((int)cps_api_object_attr_data_uint(attr) == false) {
        inf->ifi_flags &= ~IFF_UP;
    }
}

extern "C" t_std_error nas_os_interface_set_attribute(cps_api_object_t obj,cps_api_attr_id_t id) {
    char buff[NL_MSG_INTF_BUFF_LEN];
    memset(buff,0,sizeof(buff));

    cps_api_object_attr_t _ifix = cps_api_object_attr_get(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
    if (_ifix==NULL) return (STD_ERR(NAS_OS,FAIL, 0));

    hal_ifindex_t if_index = cps_api_object_attr_data_uint(_ifix);
    if (if_index==0) return STD_ERR(NAS_OS,FAIL, 0);

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    nas_os_pack_nl_hdr(nlh, RTM_SETLINK, (NLM_F_REQUEST | NLM_F_ACK));
    nas_os_pack_if_hdr(ifmsg, AF_UNSPEC, 0, if_index);

    char if_name[HAL_IF_NAME_SZ+1];
    if(cps_api_interface_if_index_to_name(if_index, if_name, sizeof(if_name)) == NULL) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure getting interface name for %d", if_index);
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    unsigned flags = 0;
    if(nas_os_util_int_flags_get(if_name, &flags) == STD_ERR_OK) {
        ifmsg->ifi_flags = flags;
    }

    static const std::map<cps_api_attr_id_t,void (*)( cps_api_object_t ,struct nlmsghdr *,
            struct ifinfomsg *,size_t)> _funcs = {
            {DELL_IF_IF_INTERFACES_INTERFACE_MTU, _set_mtu},
            {IF_INTERFACES_INTERFACE_NAME, _set_name},
            {IF_INTERFACES_INTERFACE_ENABLED, _set_admin},
            {NAS_OS_IF_ALIAS, _set_ifalias },
            {DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS, _set_mac},

    };

    auto it =_funcs.find(id);
    if (it==_funcs.end()) return STD_ERR_OK;
    it->second(obj,nlh,ifmsg,sizeof(buff));

    if(nl_do_set_request(nas_nl_sock_T_INT, nlh, buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure updating interface in kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }
    return STD_ERR_OK;
}

