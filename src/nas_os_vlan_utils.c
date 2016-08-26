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
 * nas_os_vlan_utils.c
 *
 *  Created on: June 5, 2015
 */

#include "ds_common_types.h"
#include "nas_os_vlan_utils.h"
#include "ds_api_linux_interface.h"
#include "event_log.h"
#include <stdio.h>
#include <string.h>


void nas_os_get_vlan_if_name(char *if_name, int len, hal_vlan_id_t vlan_id, char *vlan_name)
{
    if(vlan_id != 0) {
        snprintf(vlan_name, HAL_IF_NAME_SZ, "%s.%d", if_name, vlan_id);
    }
    else {
        strncpy(vlan_name, if_name, len);
    }
}

bool nas_os_physical_to_vlan_ifindex(hal_ifindex_t intf_index, hal_vlan_id_t vlan_id,
                                            bool to_vlan,hal_ifindex_t * index){
    char intf_name[HAL_IF_NAME_SZ+1];
    char vlan_intf_name[HAL_IF_NAME_SZ+1];
    char *converted_intf_name = NULL;

    if(cps_api_interface_if_index_to_name(intf_index,intf_name,sizeof(intf_name))==NULL){
        EV_LOG(ERR,NAS_OS,0,"NAS-LINUX-INTERFACE","Invalid Interface Index %d ",intf_index);
        return false;
    }

    if(to_vlan){
        snprintf(vlan_intf_name,sizeof(vlan_intf_name),"%s.%d",intf_name,vlan_id);
        converted_intf_name = vlan_intf_name;
    }else{
        char * saveptr;
        converted_intf_name = strtok_r(intf_name,".",&saveptr);
    }

    if(((*index) = cps_api_interface_name_to_if_index(converted_intf_name)) == 0){
        EV_LOG(ERR,NAS_OS,0,"NAS-LINUX-INTERFACE","Invalid Interface name %s ",converted_intf_name);
        return false;
    }

    return true;
}




