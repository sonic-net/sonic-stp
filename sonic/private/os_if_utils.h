
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
 * os_if_utils.h
 *
 *  Created on: Aug 21, 2015
 */

#ifndef NAS_LINUX_INC_PRIVATE_OS_IF_UTILS_H_
#define NAS_LINUX_INC_PRIVATE_OS_IF_UTILS_H_

#define CPS_API_INTERFACE_DEFAULT_VLAN_DOMAIN 4095

#include "cps_api_object.h"
#include "ds_common_types.h"
#include "dell-base-common.h"
#include "std_error_codes.h"
#include "nas_os_if_priv.h"

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <utility>

using os_port_list_t = std::unordered_set <hal_ifindex_t>;
using os_member_map_t = std::unordered_map <hal_ifindex_t, os_port_list_t>;
using os_member_pair = std::pair <hal_ifindex_t, os_port_list_t>;

class if_mbr_data {
    os_member_map_t mbr_map_;

public:
    if_mbr_data () { };

    void member_add(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx);
    bool member_del(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx);
    bool member_present(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) const;
    bool member_list_check_empty(hal_ifindex_t master_idx) const;
    void for_each_mbr(hal_ifindex_t master_idx, std::function <void (int mbr)> fn);

    ~if_mbr_data () { };
};

class if_bridge {
    if_mbr_data tag_map_;
    if_mbr_data untag_map_;

public:
    if_bridge () { };

    void bridge_tag_mbr_add(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
        tag_map_.member_add(master_idx, mbr_idx);
    }

    bool bridge_tag_mbr_del(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
        return tag_map_.member_del(master_idx, mbr_idx);
    }

    bool bridge_tag_mbr_present(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) const {
        return tag_map_.member_present(master_idx, mbr_idx);
    }

    void bridge_untag_mbr_add(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
         untag_map_.member_add(master_idx, mbr_idx);
    }

    bool bridge_untag_mbr_del(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
        return untag_map_.member_del(master_idx, mbr_idx);
    }

    bool bridge_mbr_list_chk_empty(hal_ifindex_t master_idx, bool tag = true) {
        if(tag)
            return tag_map_.member_list_check_empty(master_idx);
        else
            return untag_map_.member_list_check_empty(master_idx);
    }

    bool bridge_mbr_present(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) const {
        if(tag_map_.member_present(master_idx, mbr_idx))
            return true;
        else if (untag_map_.member_present(master_idx, mbr_idx))
            return true;
        return false;
    }

    void for_each_untag_mbr(int m_idx, std::function <void (int mbr)> fn) {
        untag_map_.for_each_mbr(m_idx, fn);
    }

    ~if_bridge() { };
};

using os_slave_map_t = std::unordered_map <hal_ifindex_t , hal_ifindex_t>;

class if_bond : public if_mbr_data {
    os_slave_map_t s_map_;

public:
    if_bond () { };

    void bond_mbr_add(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
        member_add(master_idx, mbr_idx);
        s_map_.insert(std::make_pair(mbr_idx, master_idx));
    }

    bool bond_mbr_del(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) {
        s_map_.erase(mbr_idx);
        return member_del(master_idx, mbr_idx);
    }

    bool bond_mbr_present(hal_ifindex_t master_idx, hal_ifindex_t mbr_idx) const {
        return member_present(master_idx, mbr_idx);
    }

    hal_ifindex_t bond_master_get(hal_ifindex_t s_idx) {
        auto it = s_map_.find(s_idx);

        if(it == s_map_.end()) {
            return 0;
        } else {
            return it->second;
        }
    }

    bool bond_mbr_list_chk_empty(hal_ifindex_t master_idx) {
        return member_list_check_empty(master_idx);
    }

    ~if_bond() { };
};

INTERFACE *os_get_if_db_hdlr();
if_bridge *os_get_bridge_db_hdlr();
if_bond   *os_get_bond_db_hdlr();

#endif /* NAS_LINUX_INC_PRIVATE_OS_IF_UTILS_H_ */
