
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
 * \file   os_interface_stg.cpp
 */

#include "private/nas_os_if_priv.h"
#include "dell-base-stg.h"
#include "nas_nlmsg.h"
#include "event_log.h"
#include "net_publish.h"

#include <linux/if_link.h>
#include <sys/fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

bool os_bridge_stp_enabled (if_details *details)
{
    const std::string SYSFS_CLASS_NET = "/sys/class/net/";
    std::stringstream str_stream;

    str_stream << SYSFS_CLASS_NET << details->if_name << "/bridge/stp_state";

    std::string path = str_stream.str();

    std::ifstream in(path);
    if(!in.good()) {
        return false;
    }

    std::string s;

    int stp_state = 0;
    if(getline(in, s)) {
        stp_state = stoi(s);
    }

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "STP %s, path %s state %d",
            details->if_name.c_str(), path.c_str(), stp_state);

    return ((stp_state)? true:false);
}

bool INTERFACE::os_interface_stg_attrs_handler(if_details *details, cps_api_object_t obj)
{
    if (details->_attrs[IFLA_PROTINFO]) {
        struct nlattr *brinfo[IFLA_BRPORT_MAX];
        memset(brinfo,0,sizeof(brinfo));
        nla_parse_nested(brinfo,IFLA_BRPORT_MAX,details->_attrs[IFLA_PROTINFO]);
        if (brinfo[IFLA_BRPORT_STATE]) {
            if(!os_bridge_stp_enabled(details))
                return true;

            cps_api_object_t cln_obj = cps_api_object_create();

            cps_api_object_clone(cln_obj,obj);

            cps_api_key_init(cps_api_object_key(cln_obj),cps_api_qualifier_TARGET,
                   (cps_api_object_category_types_t) cps_api_obj_CAT_BASE_STG,BASE_STG_ENTRY_OBJ,0);

            int stp_state = *(int *)nla_data(brinfo[IFLA_BRPORT_STATE]);

            cps_api_attr_id_t ids[2] = {BASE_STG_ENTRY_INTF, BASE_STG_ENTRY_INTF_STATE };
            const int ids_len = sizeof(ids)/sizeof(ids[0]);

            cps_api_object_e_add(cln_obj,ids,ids_len,cps_api_object_ATTR_T_U32,&stp_state,sizeof(stp_state));

            ids[1]= BASE_STG_ENTRY_INTF_IF_INDEX_IFINDEX;

            cps_api_object_e_add(cln_obj,ids,ids_len,cps_api_object_ATTR_T_U32,&details->_ifindex,
                                         sizeof(details->_ifindex));
            net_publish_event(cln_obj);
        }
    }

    return true;
}
