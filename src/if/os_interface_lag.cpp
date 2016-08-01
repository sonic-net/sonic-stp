
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
 * \file   os_interface_lag.cpp
 */

#include "private/nas_os_if_priv.h"
#include "private/os_if_utils.h"
#include "dell-base-if.h"
#include "dell-base-if-lag.h"
#include "dell-interface.h"
#include "ds_api_linux_interface.h"

#include "nas_nlmsg.h"

#include "nas_os_vlan_utils.h"
#include "nas_os_int_utils.h"

#include "event_log.h"

#include <linux/if_link.h>
#include <linux/if.h>

static inline bool os_interface_lag_add_member_name(hal_ifindex_t ifix, cps_api_object_t obj)
{
    char if_name[HAL_IF_NAME_SZ+1];
    if (cps_api_interface_if_index_to_name(ifix,if_name,  sizeof(if_name))==NULL) {
        return false;
    }

    cps_api_object_attr_add(obj, DELL_IF_IF_INTERFACES_INTERFACE_MEMBER_PORTS_NAME, if_name, strlen(if_name)+1);
    return true;
}

bool INTERFACE::os_interface_lag_attrs_handler(if_details *details, cps_api_object_t obj)
{
    if_bond *bond_hdlr = os_get_bond_db_hdlr();
    hal_ifindex_t master_idx = (details->_attrs[IFLA_MASTER]!=NULL)?
                                *(int *)nla_data(details->_attrs[IFLA_MASTER]):0;

    if (details->_info_kind!=nullptr && !strncmp(details->_info_kind, "tun", 3)) {
         if(details->_attrs[IFLA_MASTER]!=NULL  && ((details->_flags & IFF_SLAVE)!=0)) {
            EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Received tun %d and state 0x%x",
                        details->_ifindex,details->_flags);
            if(bond_hdlr && bond_hdlr->bond_mbr_present(master_idx, details->_ifindex)){
                EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Bond mbr present in master %d, slave %d",
                            master_idx, details->_ifindex);
                return true;
            }
            if(bond_hdlr) bond_hdlr->bond_mbr_add(master_idx, details->_ifindex);
            // Unmask the event publish for this interface after lag addition
            os_interface_mask_event(details->_ifindex, OS_IF_CHANGE_NONE);

            if(!os_interface_lag_add_member_name(details->_ifindex, obj)) return false;
            details->_type = BASE_CMN_INTERFACE_TYPE_LAG;

         } else if ((details->_flags & IFF_SLAVE)!=0) {
             if(bond_hdlr && (master_idx = bond_hdlr->bond_master_get(details->_ifindex)))
                 bond_hdlr->bond_mbr_del(master_idx, details->_ifindex);

             if(!master_idx) {
                 EV_LOGGING(NAS_OS, DEBUG, "NET-MAIN", "No master found, this could be an if update for %d",
                             details->_ifindex);
                 return true;
             }

             EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Lag member delete: tun %d and state 0x%x",
                         details->_ifindex,details->_flags);

             if(!os_interface_lag_add_member_name(details->_ifindex, obj)) return false;

             cps_api_object_attr_delete(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
             cps_api_object_attr_delete(obj, IF_INTERFACES_INTERFACE_NAME);
             cps_api_object_attr_add_u32(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX,master_idx);

             details->_type = BASE_CMN_INTERFACE_TYPE_LAG;
             details->_op = cps_api_oper_DELETE;
         }
    }

    if(details->_info_kind!=nullptr && !strncmp(details->_info_kind, "bond", 4)) {
        EV_LOG(INFO, NAS_OS,3, "NET-MAIN", "Bond interface index is %d ",
                details->_ifindex);

        if (details->_attrs[IFLA_MASTER]==nullptr) {
            details->_type = BASE_CMN_INTERFACE_TYPE_LAG;
        }

    } // bond interface

    return true;
}
