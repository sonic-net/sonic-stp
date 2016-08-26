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
 * nas_os_stp.c
 *
 *  Created on: May 19, 2015
 */


#include "dell-base-stg.h"
#include "event_log.h"
#include "std_error_codes.h"
#include "nas_nlmsg.h"
#include "netlink_tools.h"
#include "nas_os_vlan_utils.h"

#include <netinet/in.h>
#include <linux/if_bridge.h>
#include <sys/socket.h>

#define NL_MSG_BUFF_LEN 4096

//@TODO - This function shall be deprecated with the generic interface API

bool nl_get_stg_info (int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj) {

    struct ifinfomsg *ifmsg = (struct ifinfomsg *)NLMSG_DATA(hdr);
    if(hdr->nlmsg_len < NLMSG_LENGTH(sizeof(*ifmsg))) return false;

    unsigned int op;

    if (rt_msg_type ==RTM_NEWLINK)  {
        op = DB_INTERFACE_OP_CREATE;
    } else if (rt_msg_type ==RTM_DELLINK)  {
        op = DB_INTERFACE_OP_DELETE;
    } else {
        op = DB_INTERFACE_OP_SET;
    }

    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_OPERATION,op);
    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_FLAGS, ifmsg->ifi_flags);

    db_if_type_t _type =DB_IF_TYPE_PHYS_INTER;
    hal_ifindex_t ifindex = ifmsg->ifi_index;
    hal_ifindex_t phy_ifindex;

    if(op != DB_INTERFACE_OP_DELETE ){
        if(!nas_os_physical_to_vlan_ifindex(ifindex,0,false,&phy_ifindex)) return false;
        ifindex = phy_ifindex;
    }

    int nla_len = nlmsg_attrlen(hdr,sizeof(*ifmsg));
    struct nlattr *head = nlmsg_attrdata(hdr, sizeof(struct ifinfomsg));

    struct nlattr *attrs[__IFLA_MAX];
    memset(attrs,0,sizeof(attrs));

    if (nla_parse(attrs,__IFLA_MAX,head,nla_len)!=0) {
        EV_LOG_TRACE(ev_log_t_NAS_L2,ev_log_s_WARNING,"STG-NL-PARSE",
                "Failed to parse L2-STG attributes");
        cps_api_object_delete(obj);
        return false;
    }

    if(attrs[IFLA_MASTER]!=NULL) {
        int *master = (int *) nla_data(attrs[IFLA_MASTER]);
        cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_MASTER,*master);
    }

    struct nlattr *linkinfo[IFLA_INFO_MAX];
    struct nlattr *vlan[IFLA_VLAN_MAX];
    struct nlattr *brinfo[IFLA_BRPORT_MAX];

    if (attrs[IFLA_LINKINFO]) {
        memset(linkinfo,0,sizeof(linkinfo));
        nla_parse_nested(linkinfo,IFLA_INFO_MAX,attrs[IFLA_LINKINFO]);

        if (linkinfo[IFLA_INFO_KIND] != NULL) {
            char *name = nla_data(linkinfo[IFLA_INFO_KIND]);
            if(strncmp(name,"bridge",strlen(name)) == 0){
                _type = DB_IF_TYPE_BRIDGE_INTER;
            }
        }

        if (linkinfo[IFLA_INFO_DATA]) {
            memset(vlan,0,sizeof(vlan));
            nla_parse_nested(vlan,IFLA_VLAN_MAX,linkinfo[IFLA_INFO_DATA]);
            if (vlan[IFLA_VLAN_ID]) {
                cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_VLAN,
                        *(uint16_t*)nla_data(vlan[IFLA_VLAN_ID]));
                _type = DB_IF_TYPE_VLAN_INTER;
            }
        }
    }

    if (attrs[IFLA_PROTINFO]) {
        memset(brinfo,0,sizeof(brinfo));
        nla_parse_nested(brinfo,IFLA_BRPORT_MAX,attrs[IFLA_PROTINFO]);
        if (brinfo[IFLA_BRPORT_STATE]) {
            int stp_state = *(int *)nla_data(brinfo[IFLA_BRPORT_STATE]);

            cps_api_attr_id_t ids[2] = {BASE_STG_ENTRY_INTF, BASE_STG_ENTRY_INTF_STATE };
            const int ids_len = sizeof(ids)/sizeof(ids[0]);

            cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_U32,&stp_state,sizeof(stp_state));

            ids[1]= BASE_STG_ENTRY_INTF_IF_INDEX_IFINDEX;

            cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_U32,&ifindex,sizeof(ifindex));
        }
    }

    cps_api_object_attr_add_u32(obj,cps_api_if_STRUCT_A_IF_TYPE,_type);
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
           (cps_api_object_category_types_t) cps_api_obj_CAT_BASE_STG,BASE_STG_ENTRY_OBJ,0);
    return true;
}


t_std_error nl_int_update_stp_state(cps_api_object_t obj){

    cps_api_object_attr_t ifindex = cps_api_object_attr_get(obj,BASE_STG_ENTRY_INTF_IF_INDEX_IFINDEX);
    cps_api_object_attr_t stp_state = cps_api_object_attr_get(obj,BASE_STG_ENTRY_INTF_STATE);
    cps_api_object_attr_t vlan_id_attr = cps_api_object_attr_get(obj,BASE_STG_ENTRY_VLAN);


    if(ifindex == NULL || stp_state == NULL){
        EV_LOG(ERR,NAS_L2,0,"NAS-LINUX-STG","Ifindex/STP state/VLAN Id Missing for updating STP kernel state");
        return STD_ERR(STG,PARAM,0);
    }

    hal_ifindex_t vlan_ifindex;
    hal_ifindex_t phy_ifindex = cps_api_object_attr_data_u32(ifindex);
    vlan_ifindex = phy_ifindex;

    if(vlan_id_attr != NULL){
        hal_vlan_id_t vlan_id = cps_api_object_attr_data_u32(vlan_id_attr);

        // Check if a tagged interface exist otherwise continue with the untagged
        if(!nas_os_physical_to_vlan_ifindex(phy_ifindex,vlan_id,true, &vlan_ifindex)){
            EV_LOG(INFO,NAS_L2,0,"NAS-LINUX-STG","Failed to get the vlanned interface index for physical"
                "interface %d and vlan %d",phy_ifindex,vlan_id);
            vlan_ifindex = phy_ifindex;
        }
    }

    char buff[NL_MSG_BUFF_LEN];
    memset(buff,0,sizeof(buff));
    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    nlh->nlmsg_pid = 0 ;
    nlh->nlmsg_seq = 0 ;
    nlh->nlmsg_flags =  NLM_F_REQUEST | BRIDGE_FLAGS_MASTER |NLM_F_ACK ;
    nlh->nlmsg_type = RTM_SETLINK ;

    ifmsg->ifi_family = AF_BRIDGE;
    ifmsg->ifi_index = vlan_ifindex;

    uint8_t state = cps_api_object_attr_data_u32(stp_state);

    struct nlattr *stp_attr = nlmsg_nested_start(nlh, sizeof(buff));
    stp_attr->nla_len = 0;
    stp_attr->nla_type = IFLA_PROTINFO | NLA_F_NESTED;
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_BRPORT_STATE,(void *)&state,sizeof(uint8_t));
    nlmsg_nested_end(nlh, stp_attr);

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh, buff, sizeof(buff)) != STD_ERR_OK){
        EV_LOG(ERR,NAS_L2,0,"NAS_LINUX-STG","Failed to updated STP State to %d for Interface %d "
                "in Kernel",state,vlan_ifindex);
        return STD_ERR(STG,FAIL,0);
    }

    return STD_ERR_OK;
}
