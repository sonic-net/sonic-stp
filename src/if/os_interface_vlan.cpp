
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


/*!
 * \file   os_interface_vlan.cpp
 */

#include "private/nas_os_if_priv.h"
#include "private/os_if_utils.h"
#include "dell-base-if.h"
#include "dell-base-if-vlan.h"

#include "dell-interface.h"
#include "nas_nlmsg.h"

#include "nas_os_vlan_utils.h"

#include "event_log.h"

#include <linux/if_link.h>
#include <linux/if.h>

bool static os_interface_vlan_bridge_handler(hal_ifindex_t mas_idx, hal_ifindex_t mem_idx, bool tag)
{
    if_bridge *br_hdlr = os_get_bridge_db_hdlr();

    if(br_hdlr == nullptr) return true;

    if(br_hdlr->bridge_mbr_present(mas_idx, mem_idx)) {
        EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Duplicate netlink add master %d ifx %d",
                mas_idx, mem_idx);
        return false;
    }

    if(!tag) {
        br_hdlr->bridge_untag_mbr_add(mas_idx, mem_idx);
        // If tagged member list is empty, defer the publishing of untagged ports
        if(br_hdlr->bridge_mbr_list_chk_empty(mas_idx))
            return false;
    } else {
        br_hdlr->bridge_tag_mbr_add(mas_idx, mem_idx);
    }

    return true;
}

inline void os_interface_add_untag_ports(hal_ifindex_t master_idx, cps_api_object_t obj)
{
    if_bridge *br_hdlr = os_get_bridge_db_hdlr();

    if(br_hdlr == nullptr) return;

    if(br_hdlr->bridge_mbr_list_chk_empty(master_idx, false)) return;

    br_hdlr->for_each_untag_mbr(master_idx, [obj](int port) {
        cps_api_object_attr_add_u32(obj,DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS, port);
    });

}

bool INTERFACE::os_interface_vlan_attrs_handler(if_details *details, cps_api_object_t obj)
{
    if_bridge *br_hdlr = os_get_bridge_db_hdlr();
    hal_ifindex_t master_idx = (details->_attrs[IFLA_MASTER]!=NULL)?
                                *(int *)nla_data(details->_attrs[IFLA_MASTER]):0;

    if (details->_attrs[IFLA_MASTER]!=NULL) {
        if(details->_op == cps_api_oper_DELETE) {
            int phy_ifindex = 0;

            nas_os_physical_to_vlan_ifindex(details->_ifindex, 0, false, &phy_ifindex);
            EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received oper del ifidx %d, phy idx %d",
                                   details->_ifindex, phy_ifindex);

            if(details->_ifindex != phy_ifindex) {
                cps_api_object_attr_add_u32(obj,DELL_IF_IF_INTERFACES_INTERFACE_TAGGED_PORTS, phy_ifindex);
                if(br_hdlr) br_hdlr->bridge_tag_mbr_del(master_idx, details->_ifindex);
            } else {
                cps_api_object_attr_add_u32(obj,DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS, phy_ifindex);
                if(br_hdlr) br_hdlr->bridge_untag_mbr_del(master_idx, details->_ifindex);
            }

            details->_type = BASE_CMN_INTERFACE_TYPE_VLAN;
        }
    }

    if (details->_info_kind == nullptr) return true;

    if (!strncmp(details->_info_kind, "tun", 3)) {
         if(details->_attrs[IFLA_MASTER]!=NULL  && ((details->_flags & IFF_SLAVE)==0)) {
             EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received tun %d", details->_ifindex);

             if(!os_interface_vlan_bridge_handler(master_idx, details->_ifindex, false))
                 return true;

             details->_type = BASE_CMN_INTERFACE_TYPE_VLAN;
             cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS,
                      details->_ifindex);
         }
    }

    if(!strncmp(details->_info_kind, "bond", 4) && details->_attrs[IFLA_MASTER]!=nullptr) {
        EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Bond interface index is %d ", details->_ifindex);

        EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received bond %d with master set %d",
                details->_ifindex, *(int *)nla_data(details->_attrs[IFLA_MASTER]));

        if(!os_interface_vlan_bridge_handler(master_idx, details->_ifindex, false))
            return true;

        //Bond with master set, means configured in a bridge
        details->_type = BASE_CMN_INTERFACE_TYPE_VLAN;

        cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS,
                details->_ifindex);

    } // bond interface

    if ((details->_attrs[IFLA_LINKINFO] != nullptr) &&
        (details->_linkinfo[IFLA_INFO_KIND]!=nullptr)) {

        EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "In IFLA_INFO_KIND for %s index %d",
                details->_info_kind, details->_ifindex);

        struct nlattr *vlan[IFLA_VLAN_MAX];
        bool publish_untag = false;

        if(!strncmp(details->_info_kind, "vlan", 4)) {
            if(details->_attrs[IFLA_MASTER]!=NULL) {
                if (details->_linkinfo[IFLA_INFO_DATA]) {

                    memset(vlan,0,sizeof(vlan));

                    nla_parse_nested(vlan,IFLA_VLAN_MAX,details->_linkinfo[IFLA_INFO_DATA]);
                    if (vlan[IFLA_VLAN_ID]) {
                        EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Received VLAN %d", details->_ifindex);
                        cps_api_object_attr_add_u32(obj,BASE_IF_VLAN_IF_INTERFACES_INTERFACE_ID,
                                                *(uint16_t*)nla_data(vlan[IFLA_VLAN_ID]));
                        details->_type = BASE_CMN_INTERFACE_TYPE_VLAN;

                        /*
                         * Check if this is the first tagged port add, if so append
                         * the previously added untagged ports if any
                         */
                        if(br_hdlr && br_hdlr->bridge_mbr_list_chk_empty(master_idx))
                            publish_untag = true;

                        if(!os_interface_vlan_bridge_handler(master_idx, details->_ifindex, true))
                            return false;

                        if(publish_untag) os_interface_add_untag_ports(master_idx, obj);

                        if (details->_attrs[IFLA_LINK]) {
                            //port that is added to vlan
                            uint32_t portIndex = *(uint32_t*)nla_data(details->_attrs[IFLA_LINK]);
                            EV_LOG(INFO, NAS_OS, 3, "NET-MAIN", "Member phy port %d", portIndex);

                            cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_TAGGED_PORTS,
                                                        portIndex);
                        } //IFLA_LINK
                    } //IFLA_VLAN_ID
                }//IFLA_INFO_DATA
            } else { // IFLA_MASTER
                return false; // Need not publish sub-interfaces (e.g e101-001-1.100)
            }
        }// vlan interface

    }

    return true;
}
