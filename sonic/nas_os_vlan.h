
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
 * filename: nas_os_vlan.h
 */


#ifndef NAS_OS_VLAN_H_
#define NAS_OS_VLAN_H_

#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "std_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief : Add Vlan : This creates the bridge interface in kernel
 *
 * @obj : CPS API object which contains bridge_name to create
 *
 * @br_index : Returns the bridge index on response during create
 */
t_std_error nas_os_add_vlan(cps_api_object_t obj, hal_ifindex_t *br_index);

/**
 * @brief : Del Vlan : This removes the bridge interface from kernel
 *
 * @obj : CPS API object which contains bridge_name and kernel ifindex
 */

t_std_error nas_os_del_vlan(cps_api_object_t obj);

/**
 * @brief : Add port in the vlan : Creates the vlan interface in kernel and
 * also updates the interface in the bridge
 *
 * @obj : CPS API object which contains vlan_id, the port index for port
 *        and the bridge index.
 */

t_std_error nas_os_add_port_to_vlan(cps_api_object_t obj, hal_ifindex_t *vlan_index);

/**
 * @brief : Del port from vlan : Deletes the vlan interface from bridge and
 * also deletes the interface in the kernel
 *
 * @obj : CPS API object which contains vlan_id, portindex for the port
 */
t_std_error nas_os_del_port_from_vlan(cps_api_object_t obj);

/**
 * @brief : Delete vlan interface: Deletes the vlan interface from kernel
 *
 * @obj : CPS API object which contains kernel vlan_index
 */

t_std_error nas_os_del_vlan_interface(cps_api_object_t obj);


#ifdef __cplusplus
}
#endif


#endif /* NAS_OS_VLAN_H_ */
