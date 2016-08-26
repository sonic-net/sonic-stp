
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
 * filename: nas_os_if_priv.h
 * Created on: Apr 30, 2016
 */

#ifndef NAS_OS_IF_PRIV_H_
#define NAS_OS_IF_PRIV_H_

#include "cps_api_object.h"
#include "ds_common_types.h"
#include "dell-base-common.h"
#include "std_error_codes.h"
#include "nas_os_int_utils.h"

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <string>

#include <functional>
#include <unordered_map>
#include <utility>

bool os_interface_to_object (int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj);
cps_api_return_code_t _get_interfaces( cps_api_object_list_t list, hal_ifindex_t ifix, bool get_all );

typedef struct {
    bool admin;
    if_change_t ev_mask; // Mask interface netlink event publish
    int mtu;
    hal_mac_addr_t phy_addr;
}if_info_t;

using os_if_map_t = std::unordered_map <hal_ifindex_t, if_info_t>;

struct if_details {
    cps_api_operation_types_t _op;
    BASE_CMN_INTERFACE_TYPE_t _type;
    unsigned int _family;
    unsigned int _flags;
    const char * _info_kind;
    int _ifindex;
    std::string if_name;
    struct nlattr *_attrs[__IFLA_MAX];
    struct nlattr *_linkinfo[IFLA_INFO_MAX];
};

class INTERFACE {

    os_if_map_t if_map_;

    enum {
        PHY=0, LAG, VLAN, STG, IP, MAX
    };
    bool (INTERFACE::*fptr[MAX]) (if_details *, cps_api_object_t);

    bool os_interface_phy_attrs_handler(if_details *, cps_api_object_t obj) { return true; };
    bool os_interface_vlan_attrs_handler(if_details *, cps_api_object_t obj);
    bool os_interface_lag_attrs_handler(if_details *, cps_api_object_t obj);
    bool os_interface_stg_attrs_handler(if_details *, cps_api_object_t obj);
    bool os_interface_ip_attrs_handler(if_details *, cps_api_object_t obj) { return true; };

public:

    INTERFACE () {
        fptr[PHY] = &INTERFACE::os_interface_phy_attrs_handler;
        fptr[LAG] = &INTERFACE::os_interface_lag_attrs_handler;
        fptr[VLAN] = &INTERFACE::os_interface_vlan_attrs_handler;
        fptr[STG] = &INTERFACE::os_interface_stg_attrs_handler;
        fptr[IP] = &INTERFACE::os_interface_ip_attrs_handler;
    }

    bool if_hdlr(if_details* if_d, cps_api_object_t obj) {
        for (int ix=0; ix < MAX; ++ix) {
            if(!(this->*fptr[ix])(if_d, obj))
                return false;
        }
        return true;
    }

    int  if_info_update(hal_ifindex_t ifx, if_info_t& if_info);
    bool if_info_setmask(hal_ifindex_t ifx, if_change_t mask_val);
    if_change_t if_info_getmask(hal_ifindex_t ifx);
    bool if_info_delete(hal_ifindex_t ifx);
    bool if_info_get(hal_ifindex_t ifx, if_info_t& if_info) const;
    void for_each_mbr(std::function <void (int ix, if_info_t& if_info)> fn);
};

t_std_error os_interface_object_reg(cps_api_operation_handle_t handle);

#endif /* NAS_OS_IF_PRIV_H_ */
