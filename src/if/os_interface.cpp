
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


/*!
 * \file   os_interface.cpp
 */

#include "private/nas_os_if_priv.h"
#include "private/os_if_utils.h"

#include "netlink_tools.h"
#include "nas_nlmsg.h"
#include "nas_nlmsg_object_utils.h"
#include "nas_os_vlan_utils.h"
#include "nas_os_int_utils.h"
#include "nas_os_interface.h"


#include "cps_api_operation.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"


#include "std_assert.h"
#include "std_mac_utils.h"
#include "event_log.h"

#include "dell-interface.h"
#include "dell-base-if.h"
#include "dell-base-if-linux.h"
#include "dell-base-if-vlan.h"
#include "dell-base-common.h"
#include "ds_api_linux_interface.h"

#include "iana-if-type.h"
#include "ietf-interfaces.h"
#include "ietf-ip.h"

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
#include <unordered_map>

#define NAS_OS_IF_OBJ_ID_RES_START 4
#define NAS_OS_IF_FLAGS_ID (NAS_OS_IF_OBJ_ID_RES_START) //reserve 4 inside the object for flags
#define NAS_OS_IF_ALIAS (NAS_OS_IF_OBJ_ID_RES_START+1)

#define NAS_LINK_MTU_HDR_SIZE 32
#define NL_MSG_INTF_BUFF_LEN 2048

/*
 * API to mask *any* interface publish event.
 * e.g admin-state in case of LAG member port add
 */
extern "C" bool os_interface_mask_event(hal_ifindex_t ifix, if_change_t mask_val)
{
    INTERFACE *fill = os_get_if_db_hdlr();

    if(fill) {
        return fill->if_info_setmask(ifix, mask_val);
    }
    return true;
}

bool os_interface_to_object (int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj)
{
    struct ifinfomsg *ifmsg = (struct ifinfomsg *)NLMSG_DATA(hdr);

    if(hdr->nlmsg_len < NLMSG_LENGTH(sizeof(*ifmsg)))
        return false;

    int track_change = OS_IF_CHANGE_NONE;
    if_details details;
    if_info_t ifinfo;

    details._op = cps_api_oper_NULL;
    details._family = ifmsg->ifi_family;

    cps_api_object_attr_add_u32(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX,ifmsg->ifi_index);

    static const std::unordered_map<int,cps_api_operation_types_t> _to_op_type = {
            { RTM_NEWLINK, cps_api_oper_CREATE },
            { RTM_DELLINK, cps_api_oper_DELETE }
    };

    EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "msgType %d, ifindex %d change %x\n",
           rt_msg_type, ifmsg->ifi_index, ifmsg->ifi_change);

    auto it = _to_op_type.find(rt_msg_type);
    if (it==_to_op_type.end()) {
        details._op = cps_api_oper_SET;
    } else {
        details._op = it->second;
    }

    details._flags = ifmsg->ifi_flags;

    details._type = BASE_CMN_INTERFACE_TYPE_L3_PORT;

    details._ifindex = ifmsg->ifi_index;

    cps_api_object_attr_add_u32(obj,BASE_IF_LINUX_IF_INTERFACES_INTERFACE_IF_FLAGS, details._flags);

    cps_api_object_attr_add_u32(obj,IF_INTERFACES_INTERFACE_ENABLED,
        (ifmsg->ifi_flags & IFF_UP) ? true :false);

    ifinfo.ev_mask = OS_IF_CHANGE_NONE;
    ifinfo.admin = (ifmsg->ifi_flags & IFF_UP) ? true :false;

    int nla_len = nlmsg_attrlen(hdr,sizeof(*ifmsg));
    struct nlattr *head = nlmsg_attrdata(hdr, sizeof(struct ifinfomsg));

    memset(details._attrs,0,sizeof(details._attrs));
    memset(details._linkinfo,0,sizeof(details._linkinfo));
    details._info_kind = nullptr;

    if (nla_parse(details._attrs,__IFLA_MAX,head,nla_len)!=0) {
        EV_LOG(ERR,NAS_OS,0,"NL-PARSE","Failed to parse attributes");
        cps_api_object_delete(obj);
        return false;
    }

    if (details._attrs[IFLA_LINKINFO]) {
        nla_parse_nested(details._linkinfo,IFLA_INFO_MAX,details._attrs[IFLA_LINKINFO]);
    }

    if (details._attrs[IFLA_LINKINFO] != nullptr && details._linkinfo[IFLA_INFO_KIND]!=nullptr) {
        details._info_kind = (const char *)nla_data(details._linkinfo[IFLA_INFO_KIND]);
    }

    if (details._attrs[IFLA_ADDRESS]!=NULL) {
        char buff[40];
        const char *_p = std_mac_to_string((const hal_mac_addr_t *)(nla_data(details._attrs[IFLA_ADDRESS])), buff, sizeof(buff));
        cps_api_object_attr_add(obj,DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS,_p,strlen(_p)+1);
        memcpy(ifinfo.phy_addr, (nla_data(details._attrs[IFLA_ADDRESS])), sizeof(hal_mac_addr_t));
    }

    if(details._attrs[IFLA_IFNAME]!=NULL) {
        rta_add_name(details._attrs[IFLA_IFNAME],obj,IF_INTERFACES_INTERFACE_NAME);
        details.if_name = static_cast <char *> (nla_data(details._attrs[IFLA_IFNAME]));
    }

    if(details._attrs[IFLA_MTU]!=NULL) {
        int *mtu = (int *) nla_data(details._attrs[IFLA_MTU]);
        cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_MTU,(*mtu + NAS_LINK_MTU_HDR_SIZE));
        ifinfo.mtu = *mtu;
    }

    if(details._attrs[IFLA_MASTER]!=NULL) {
            /* This gives us the bridge index, which should be sent to
             * NAS for correlation  */
        EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Rcvd master index %d",
                *(int *)nla_data(details._attrs[IFLA_MASTER]));

        cps_api_object_attr_add_u32(obj,BASE_IF_LINUX_IF_INTERFACES_INTERFACE_IF_MASTER,
                                   *(int *)nla_data(details._attrs[IFLA_MASTER]));
    }

    if (details._info_kind!=nullptr && (!strncmp(details._info_kind, "bridge", 6))) {
        EV_LOG(INFO,NAS_OS,3, "NET-MAIN", "Bridge intf index is %d ",
                details._ifindex);
        details._type = BASE_CMN_INTERFACE_TYPE_L2_PORT;
    }

    INTERFACE *fill = os_get_if_db_hdlr();
    if_change_t mask = OS_IF_CHANGE_NONE;
    if(fill && !(fill->if_hdlr(&details, obj)))
        return false; // Return in case of sub-interfaces etc (Handler will return false)

    if (!fill)
        track_change = OS_IF_CHANGE_ALL;
    else
        track_change = fill->if_info_update(ifmsg->ifi_index, ifinfo);

    if(details._type != BASE_CMN_INTERFACE_TYPE_L3_PORT && details._attrs[IFLA_MASTER]!=NULL) {
        /*
         * Use master for Vlan and Lag
         */
        char if_name[HAL_IF_NAME_SZ+1];
        int ifix = *(int *)nla_data(details._attrs[IFLA_MASTER]);
        if (cps_api_interface_if_index_to_name(ifix,if_name,  sizeof(if_name))==NULL) {
            return false;
        }

        // Delete the previously filled attributes in case of Vlan/Lag member add/del
        cps_api_object_attr_delete(obj, DELL_IF_IF_INTERFACES_INTERFACE_MTU);
        cps_api_object_attr_delete(obj, DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS);
        cps_api_object_attr_delete(obj, IF_INTERFACES_INTERFACE_NAME);
        cps_api_object_attr_delete(obj, IF_INTERFACES_INTERFACE_ENABLED);
        cps_api_object_attr_delete(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
        cps_api_object_attr_add(obj, IF_INTERFACES_INTERFACE_NAME, if_name, (strlen(if_name)+1));
        cps_api_object_attr_add_u32(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX,ifix);
    } else if (!track_change) {
        /*
         * If track change is false, check for interface type - Return false ONLY in the individual update case
         * If the handler has identified it as VLAN or Bond member addition, then continue with publishing
         */
        if (details._op != cps_api_oper_DELETE && details._type != BASE_CMN_INTERFACE_TYPE_VLAN)
            return false;

    // If mask is set to disable admin state publish event, remove the attribute
    } else if(fill && (mask = fill->if_info_getmask(ifmsg->ifi_index))) {
        EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Masking set for %d, mask %d, track_chg %d",
                                 ifmsg->ifi_index, mask, track_change);
        if(track_change != OS_IF_ADM_CHANGE && mask == OS_IF_ADM_CHANGE)
            cps_api_object_attr_delete(obj, IF_INTERFACES_INTERFACE_ENABLED);
        else if (mask == OS_IF_ADM_CHANGE)
            return false;
    }

    EV_LOG(INFO,NAS_OS,3, "NET-MAIN", "Publishing index %d type %d", details._ifindex, details._type);

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),BASE_IF_LINUX_IF_INTERFACES_INTERFACE_OBJ,
            cps_api_qualifier_OBSERVED);
    cps_api_object_set_type_operation(cps_api_object_key(obj),details._op);
    cps_api_object_attr_add_u32(obj, BASE_IF_LINUX_IF_INTERFACES_INTERFACE_DELL_TYPE, details._type);

    return true;
}

static bool get_netlink_data(int rt_msg_type, struct nlmsghdr *hdr, void *data) {
    if (rt_msg_type <= RTM_SETLINK) {
        cps_api_object_list_t * list = (cps_api_object_list_t*)data;
        cps_api_object_guard og(cps_api_object_create());
        if (!og.valid()) return false;

        if (os_interface_to_object(rt_msg_type,hdr,og.get())) {
            if (cps_api_object_list_append(*list,og.get())) {
                og.release();
                return true;
            }
        }
        return false;
    }
    return true;
}

static bool os_interface_info_to_object(hal_ifindex_t ifix, if_info_t& ifinfo, cps_api_object_t obj)
{
    char if_name[HAL_IF_NAME_SZ+1];
    if(cps_api_interface_if_index_to_name(ifix, if_name, sizeof(if_name)) == NULL) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure getting interface name for %d", ifix);
        return false;
    } else
        cps_api_object_attr_add(obj, IF_INTERFACES_INTERFACE_NAME, if_name, (strlen(if_name)+1));
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), BASE_IF_LINUX_IF_INTERFACES_INTERFACE_OBJ,
            cps_api_qualifier_OBSERVED);

    cps_api_object_attr_add_u32(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX, ifix);

    cps_api_object_attr_add_u32(obj,IF_INTERFACES_INTERFACE_ENABLED, ifinfo.admin);

    char buff[HAL_INET6_TEXT_LEN];
    const char *_p = std_mac_to_string((const hal_mac_addr_t *)ifinfo.phy_addr, buff, sizeof(buff));
    cps_api_object_attr_add(obj,DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS,_p,strlen(_p)+1);
    cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_MTU,
                                (ifinfo.mtu  + NAS_LINK_MTU_HDR_SIZE));
    return true;
}

static cps_api_return_code_t _get_db_interface( cps_api_object_list_t *list, hal_ifindex_t ifix, bool get_all )
{
    INTERFACE *fill = os_get_if_db_hdlr();

    if (!fill) return cps_api_ret_code_ERR;

    if_info_t ifinfo;
    if(!get_all && fill->if_info_get(ifix, ifinfo)) {
        EV_LOG(INFO,NAS_OS,3, "NET-MAIN", "Get ifinfo for %d", ifix);

        cps_api_object_t obj = cps_api_object_create();
        if(!os_interface_info_to_object(ifix, ifinfo, obj)) return cps_api_ret_code_ERR;

        cps_api_object_set_type_operation(cps_api_object_key(obj),cps_api_oper_NULL);
        if (cps_api_object_list_append(*list,obj)) {
            return cps_api_ret_code_OK;
        }
        else {
            return cps_api_ret_code_ERR;
        }
    } else if (get_all) {
        fill->for_each_mbr([&list](int idx, if_info_t& ifinfo) {
            EV_LOG(INFO,NAS_OS,3, "NET-MAIN", "Get all ifinfo for %d", idx);

            cps_api_object_t obj = cps_api_object_create();
            if(!os_interface_info_to_object(idx, ifinfo, obj)) return;

            cps_api_object_set_type_operation(cps_api_object_key(obj),cps_api_oper_NULL);
            if (cps_api_object_list_append(*list,obj)) return;
        });
        return cps_api_ret_code_OK;
    }

    return cps_api_ret_code_ERR;
}

cps_api_return_code_t _get_interfaces( cps_api_object_list_t list, hal_ifindex_t ifix, bool get_all ) {

    if(_get_db_interface(&list, ifix, get_all) == cps_api_ret_code_OK)
        return cps_api_ret_code_OK;

    int if_sock = 0;
    if((if_sock = nas_nl_sock_create(nas_nl_sock_T_INT)) < 0) {
       return cps_api_ret_code_ERR;
    }

    const int BUFF_LEN=4196;
    char buff[BUFF_LEN];
    memset(buff,0,sizeof(buff));

    struct ifinfomsg ifmsg;
    memset(&ifmsg,0,sizeof(ifmsg));

    nas_os_pack_if_hdr(&ifmsg, AF_NETLINK, 0, ifix );

    int seq = (int)pthread_self();
    int dump_flags = NLM_F_ROOT| NLM_F_DUMP;

    if (nl_send_request(if_sock, RTM_GETLINK,
            (NLM_F_REQUEST | NLM_F_ACK | (get_all ? dump_flags : 0)),
            seq,&ifmsg, sizeof(ifmsg))) {
        netlink_tools_process_socket(if_sock,get_netlink_data,
                &list,buff,sizeof(buff),&seq,NULL);
    }

    size_t mx = cps_api_object_list_size(list);
    for (size_t ix = 0 ; ix < mx ; ++ix ) {
        cps_api_object_t ret = cps_api_object_list_get(list,ix);
        STD_ASSERT(ret!=NULL);
        cps_api_object_set_type_operation(cps_api_object_key(ret),cps_api_oper_NULL);
    }

    close(if_sock);
    return cps_api_ret_code_OK;
}

cps_api_return_code_t __rd(void * context, cps_api_get_params_t * param,  size_t key_ix) {

    cps_api_object_t obj = cps_api_object_list_get(param->filters,key_ix);
    STD_ASSERT(obj!=nullptr);

    if (nas_os_get_interface(obj,param->list)==STD_ERR_OK) {
        return cps_api_ret_code_OK;
    }

    return cps_api_ret_code_ERR;
}

cps_api_return_code_t __wr(void * context, cps_api_transaction_params_t * param,size_t ix) {

    return cps_api_ret_code_ERR;
}

t_std_error os_interface_object_reg(cps_api_operation_handle_t handle) {
    cps_api_registration_functions_t f;
    memset(&f,0,sizeof(f));

    char buff[CPS_API_KEY_STR_MAX];
    if (!cps_api_key_from_attr_with_qual(&f.key, BASE_IF_LINUX_IF_INTERFACES_INTERFACE,cps_api_qualifier_TARGET)) {
        EV_LOG(ERR,INTERFACE,0,"NAS-IF-REG","Could not translate %d to key %s",
            (int)(BASE_IF_LINUX_IF_INTERFACES_INTERFACE),cps_api_key_print(&f.key,buff,sizeof(buff)-1));
        return STD_ERR(INTERFACE,FAIL,0);
    }

    f.handle = handle;
    f._read_function = __rd;
    f._write_function = __wr;

    if (cps_api_register(&f)!=cps_api_ret_code_OK) {
        return STD_ERR(INTERFACE,FAIL,0);
    }

    return STD_ERR_OK;

}

