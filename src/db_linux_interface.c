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
 * filename: db_linux_interface.c
 */

/*
 * db_linux_interface.c
 */

#include "event_log.h"

#include "dell-base-stg.h"
#include "ds_api_linux_interface.h"

#include "nas_linux_l2.h"
#include "std_assert.h"
#include "netlink_tools.h"
#include "standard_netlink_requests.h"
#include "nas_os_int_utils.h"
#include "nas_nlmsg.h"
#include "nas_os_vlan_utils.h"
#include "nas_os_interface.h"

#include "cps_api_interface_types.h"
#include "nas_nlmsg_object_utils.h"

#include "cps_api_operation.h"

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <linux/if_bridge.h>
#include <linux/if_vlan.h>

// @TODO - This file has been deprecated with os_interface.cpp

/*
 * Function to update kernel operational
 * state
 */

//db_if_event_t
bool nl_get_if_info (int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj) {

    struct ifinfomsg *ifmsg = (struct ifinfomsg *)NLMSG_DATA(hdr);

    if(hdr->nlmsg_len < NLMSG_LENGTH(sizeof(*ifmsg)))
        return false;

    db_if_type_t _type = DB_IF_TYPE_PHYS_INTER;
    uint_t ifindex = ifmsg->ifi_index;
    uint32_t op = 0;

    EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "nl_get_if_info msgType %d, ifindex %d change %x\n",
           rt_msg_type, ifindex, ifmsg->ifi_change);

    if (rt_msg_type ==RTM_NEWLINK)  {
        op = DB_INTERFACE_OP_CREATE;
    } else if (rt_msg_type ==RTM_DELLINK)  {
        op = DB_INTERFACE_OP_DELETE;
    } else {
        op = DB_INTERFACE_OP_SET;
    }
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_OPERATION,op);
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_FLAGS, ifmsg->ifi_flags);

    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IFINDEX,ifindex);

    if(ifmsg->ifi_flags & IFF_UP) {
        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_ADMIN_STATE,DB_ADMIN_STATE_UP);
    } else {
        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_ADMIN_STATE,DB_ADMIN_STATE_DN);
    }

    int nla_len = nlmsg_attrlen(hdr,sizeof(*ifmsg));
    struct nlattr *head = nlmsg_attrdata(hdr, sizeof(struct ifinfomsg));

    struct nlattr *attrs[__IFLA_MAX];
    memset(attrs,0,sizeof(attrs));

    if (nla_parse(attrs,__IFLA_MAX,head,nla_len)!=0) {
        EV_LOG(ERR,NAS_OS,0,"NL-PARSE","Failed to parse attributes");
        cps_api_object_delete(obj);
        return false;
    }

    if (attrs[IFLA_ADDRESS]!=NULL) {
        rta_add_mac(attrs[IFLA_ADDRESS],obj,cps_api_if_STRUCT_A_IF_MACADDR);
    }
    if(attrs[IFLA_IFNAME]!=NULL) {
        rta_add_name(attrs[IFLA_IFNAME],obj,cps_api_if_STRUCT_A_NAME);
    }

    if(attrs[IFLA_MTU]!=NULL) {
        int *mtu = (int *) nla_data(attrs[IFLA_MTU]);
        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_MTU,(*mtu + NAS_LINK_MTU_HDR_SIZE));
    }

    if(attrs[IFLA_MASTER]!=NULL) {
            /* This gives us the bridge index, which should be sent to
             * NAS for correlation  */
        EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Rcvd master index %d",
                *(int *)nla_data(attrs[IFLA_MASTER]));

        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_MASTER,
                                   *(int *)nla_data(attrs[IFLA_MASTER]));
        if(op == DB_INTERFACE_OP_DELETE) {
            /* With Dell offering VLAN data structures use port index instead of
             * vlan interface */
            int phy_ifindex = 0;
            nas_os_physical_to_vlan_ifindex(ifindex, 0, false, &phy_ifindex);
            cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IFINDEX, phy_ifindex);
            cps_api_object_attr_add_u32(obj, cps_api_if_STRUCT_A_VLAN_PORT_INDEX, ifindex);
            _type = DB_IF_TYPE_VLAN_INTER;
        }
    }

    struct nlattr *linkinfo[IFLA_INFO_MAX];
    struct nlattr *vlan[IFLA_VLAN_MAX];
    char *info_kind;

    if (attrs[IFLA_LINKINFO]) {
        memset(linkinfo,0,sizeof(linkinfo));
        nla_parse_nested(linkinfo,IFLA_INFO_MAX,attrs[IFLA_LINKINFO]);

        if(linkinfo[IFLA_INFO_KIND]) {
            info_kind = (char *)nla_data(linkinfo[IFLA_INFO_KIND]);
            EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "In IFLA_INFO_KIND for %s index %d",
                   info_kind, ifindex);

            if(!strncmp(info_kind, "vlan", 4)) {
                if(attrs[IFLA_MASTER]!=NULL) {
                    if (linkinfo[IFLA_INFO_DATA]) {
                        memset(vlan,0,sizeof(vlan));

                        nla_parse_nested(vlan,IFLA_VLAN_MAX,linkinfo[IFLA_INFO_DATA]);
                        if (vlan[IFLA_VLAN_ID]) {
                            EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Received VLAN %d",
                                   ifindex);
                            cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_VLAN_ID,
                                                    *(uint16_t*)nla_data(vlan[IFLA_VLAN_ID]));
                            _type = DB_IF_TYPE_VLAN_INTER;

                            if (attrs[IFLA_LINK]) {
                                //port that is added to vlan
                                uint32_t portIndex = *(uint32_t*)nla_data(attrs[IFLA_LINK]);

                                if (op == DB_INTERFACE_OP_DELETE) {
                                    cps_api_object_attr_add_u32(obj, cps_api_if_STRUCT_A_VLAN_PORT_INDEX, ifindex);
                                } else {
                                    cps_api_object_attr_add_u32(obj, cps_api_if_STRUCT_A_VLAN_PORT_INDEX,
                                        portIndex);
                                }
                            } //IFLA_LINK
                        } //IFLA_VLAN_ID
                    }//IFLA_INFO_DATA
                }// IFLA_MASTER
            } // vlan interface
            else if(!strncmp(info_kind, "tun", 3)) {
                /* TODO : Revisit this and introduce a logical interface type */
                 if(attrs[IFLA_MASTER]!=NULL) {
                     EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received tun %d",
                                ifindex);

                     _type = DB_IF_TYPE_VLAN_INTER;
                      cps_api_object_attr_add_u32(obj, cps_api_if_STRUCT_A_VLAN_PORT_INDEX,
                                                  ifindex);
                 }

                 if(ifmsg->ifi_flags & IFF_SLAVE)
                 {
                     EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received tun %d and state IFF_UP",
                             ifindex,ifmsg->ifi_flags);
                     _type = DB_IF_TYPE_LAG_INTER;
                 }
            }
            else if(!strncmp(info_kind, "bridge", 6)) {

                EV_LOG(INFO,NAS_OS,3, "NET-MAIN", "Bridge intf index is %d ",
                            ifindex);
                _type = DB_IF_TYPE_BRIDGE_INTER;
            } /* bridge interface */
            else if(!strncmp(info_kind, "bond", 4)) {
                EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Bond interface index is %d ",
                            ifindex);
                _type = DB_IF_TYPE_LAG_INTER;

                if(attrs[IFLA_MASTER]!=NULL) {
                    EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received bond %d with master set %d",
                               ifindex, *(int *)nla_data(attrs[IFLA_MASTER]));

                    /* Bond with master set, means configured in a bridge
                     * Pass the master index as the ifindex for vlan processing to continue */
                    _type = DB_IF_TYPE_VLAN_INTER;

                    cps_api_object_attr_add_u32(obj, cps_api_if_STRUCT_A_VLAN_PORT_INDEX,
                                                ifindex);
                    if (op == DB_INTERFACE_OP_DELETE ) {
                        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IFINDEX,
                            *(int *)nla_data(attrs[IFLA_MASTER]));
                    }
                }
            } // bond interface
        }
    }

    if(attrs[IFLA_OPERSTATE]!=NULL) {
        int *link = (int *) nla_data(attrs[IFLA_OPERSTATE]);
        if (*link==IF_OPER_UP) {
            cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_OPER_STATE,DB_OPER_STATE_UP);
        } else if (*link==IF_OPER_DOWN) {
            cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_OPER_STATE,DB_OPER_STATE_DN);
        }
    }

    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IF_TYPE,_type);

    cps_api_int_if_key_create(cps_api_object_key(obj),true,0,ifindex);

    return true;
}

bool nl_interface_get_request(int sock, int req_id) {

    return nl_route_send_get_all(sock,RTM_GETLINK,AF_PACKET,req_id);
}


static cps_api_return_code_t get_interface(
        const char * name, cps_api_get_params_t *param, size_t *ix) {

    cps_api_object_t obj = cps_api_object_create();
    uint_t ifix = cps_api_interface_name_to_if_index(name);

    cps_api_int_if_key_create(cps_api_object_key(obj),true,0,ifix);

    if (!cps_api_object_list_append(param->list,obj)) {
        cps_api_object_delete(obj);
        return cps_api_ret_code_ERR;
    }

    cps_api_object_attr_add(obj,cps_api_if_STRUCT_A_NAME,name,strlen(name)+1);
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IFINDEX,ifix);

    unsigned int mtu;

    if (nas_os_util_int_mtu_get(name,&mtu)!=STD_ERR_OK) return cps_api_ret_code_ERR;
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_MTU,mtu);

    db_interface_state_t astate;
    db_interface_operational_state_t ostate;

    if (nas_os_util_int_admin_state_get(name,&astate,&ostate)!=STD_ERR_OK) return cps_api_ret_code_ERR;
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_ADMIN_STATE,astate);
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_OPER_STATE,ostate);

    hal_mac_addr_t mac;
    if (nas_os_util_int_mac_addr_get(name,&mac)!=STD_ERR_OK) return cps_api_ret_code_ERR;
    cps_api_object_attr_add(obj,cps_api_if_STRUCT_A_IF_MACADDR,&mac,sizeof(mac));

    return cps_api_ret_code_OK;
}

static bool get_netlink_data(int rt_msg_type, struct nlmsghdr *hdr, void *data) {
    if (rt_msg_type <= RTM_SETLINK) {
        cps_api_object_list_t * list = (cps_api_object_list_t*)data;
        cps_api_object_t obj = cps_api_object_create();
        if (nl_get_if_info(rt_msg_type,hdr,obj)) {
            if (cps_api_object_list_append(*list,obj)) {
                return true;
            }
        }
        return false;
    }
    return true;
}

cps_api_return_code_t get_all_interfaces( cps_api_object_list_t list ) {
    int if_sock = 0;
    if((if_sock = nas_nl_sock_create(nas_nl_sock_T_INT)) < 0) {
       return cps_api_ret_code_ERR;
    }

    const int DEF_BUFF_LEN=10000;
    char *buff=(char*)malloc(DEF_BUFF_LEN);
    if(buff==NULL) {
        close(if_sock);
        return cps_api_ret_code_ERR;
    }
    int RANDOM_REQ_ID = 0xee00;

    if (nl_interface_get_request(if_sock,RANDOM_REQ_ID)) {
        netlink_tools_process_socket(if_sock,get_netlink_data,
                &list,buff,DEF_BUFF_LEN,
            &RANDOM_REQ_ID,NULL);
    }
    close(if_sock);
    free(buff);
    return cps_api_ret_code_OK;
}

cps_api_return_code_t ds_api_linux_interface_get_function (void * context, cps_api_get_params_t * param, size_t ix) {
    cps_api_return_code_t rc = cps_api_ret_code_OK;
    cps_api_key_t key;
    cps_api_int_if_key_create(&key,false,0,0);

    if (cps_api_key_matches(&(param->keys[ix]),&key,false)==0) {
        if (cps_api_key_get_len(&param->keys[ix])<=CPS_OBJ_KEY_APP_INST_POS) {
            return get_all_interfaces(param->list);
        } else {
            uint32_t ifix = cps_api_key_element_at(&param->keys[ix],CPS_API_INT_IF_OBJ_KEY_IFIX);
            char if_name[HAL_IF_NAME_SZ+1];
            if (cps_api_interface_if_index_to_name(ifix,if_name,
                    sizeof(if_name))==NULL) {
                return cps_api_ret_code_OK;
            }
            rc = get_interface(if_name,param,&ix);
            //toss rc on a query unless it fails due to some sw err
        }
    }

    return rc;
}

static cps_api_return_code_t ds_set_interface (cps_api_object_list_t list,
        cps_api_object_t elem) {

    uint_t ifix = cps_api_key_element_at(cps_api_object_key(elem),
            CPS_API_INT_IF_OBJ_KEY_IFIX);

    char if_name[HAL_IF_NAME_SZ+1];
    if (cps_api_interface_if_index_to_name(ifix,if_name,
            sizeof(if_name))==NULL) {
        return cps_api_ret_code_ERR;
    }

    cps_api_object_attr_t mtu = cps_api_object_attr_get(elem,cps_api_if_STRUCT_A_MTU);
    cps_api_object_attr_t astate = cps_api_object_attr_get(elem,cps_api_if_STRUCT_A_ADMIN_STATE);
    cps_api_object_attr_t ostate = cps_api_object_attr_get(elem,cps_api_if_STRUCT_A_OPER_STATE);
    cps_api_object_attr_t mac = cps_api_object_attr_get(elem,cps_api_if_STRUCT_A_IF_MACADDR);

    if (mtu!=CPS_API_ATTR_NULL) {
        if (nas_os_util_int_mtu_set(if_name,cps_api_object_attr_data_u32(mtu))!=STD_ERR_OK)
            return cps_api_ret_code_ERR;
    }

    if (astate!=CPS_API_ATTR_NULL && ostate!=CPS_API_ATTR_NULL) {
        if (nas_os_util_int_admin_state_set(if_name,
                cps_api_object_attr_data_u32(astate),
                cps_api_object_attr_data_u32(ostate))!=STD_ERR_OK) return cps_api_ret_code_ERR;
    }
    if (mac!=CPS_API_ATTR_NULL) {
        hal_mac_addr_t mac_addr;
        void * data = cps_api_object_attr_data_bin(mac);
        memcpy(mac_addr,data,sizeof(mac_addr));
        if (nas_os_util_int_mac_addr_set(if_name,
                &mac_addr)!=STD_ERR_OK) return cps_api_ret_code_ERR;
    }
    return cps_api_ret_code_OK;
}

struct  {
    uint32_t type;
    cps_api_return_code_t (*handler) (cps_api_object_list_t list,
                                        cps_api_object_t elem);
}db_linux_set_functions[] = {
        {cps_api_int_obj_INTERFACE, ds_set_interface},
};

static cps_api_return_code_t execute_set_function(cps_api_object_list_t list,
        cps_api_object_t elem) {

    size_t ix = 0;
    size_t mx = sizeof(db_linux_set_functions)/sizeof(*db_linux_set_functions);
    for ( ; ix < mx ; ++ix ) {
        if (db_linux_set_functions[ix].type ==
                cps_api_key_get_subcat(cps_api_object_key(elem))) {
            return db_linux_set_functions[ix].handler(list,elem);
        }
    }
    return cps_api_ret_code_OK;
}

cps_api_return_code_t ds_api_linux_interface_set_function(void * context, cps_api_transaction_params_t * param, size_t ix) {
    cps_api_object_t obj = cps_api_object_list_get(param->change_list,ix);

    cps_api_operation_types_t op = cps_api_object_type_operation(cps_api_object_key(obj));

    if (op!=cps_api_oper_SET) return cps_api_ret_code_ERR;

    return execute_set_function(param->prev,obj);
}

t_std_error ds_api_linux_interface_init(cps_api_operation_handle_t handle) {
    cps_api_registration_functions_t f;
    memset(&f,0,sizeof(f));
    f.handle = handle;
    f._read_function = ds_api_linux_interface_get_function;
    f._write_function = ds_api_linux_interface_set_function;
    cps_api_key_init(&f.key,cps_api_qualifier_TARGET,cps_api_obj_cat_INTERFACE,0,0);

    cps_api_return_code_t rc = cps_api_register(&f);
    return STD_ERR_OK_IF_TRUE(rc==cps_api_ret_code_OK,STD_ERR(INTERFACE,FAIL,rc));
}
