
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
 * filename: nas_os_l3.h
 */


#ifndef NAS_OS_L3_H_
#define NAS_OS_L3_H_

#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "std_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief : Add Route : This adds an IPv4/v6 unicast route in kernel
 *
 * @obj : CPS API object which contains route params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_add_route (cps_api_object_t obj);

/**
 * @brief : Delete Route : This deletes an IPv4/v6 unicast route in kernel
 *
 * @obj : CPS API object which contains route params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_del_route (cps_api_object_t obj);

/**
 * @brief : Set Route : This replaces an IPv4/v6 unicast route in kernel
 *
 * @obj : CPS API object which contains route params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_set_route (cps_api_object_t obj);

/**
 * @brief : Add Neighbor : This adds a neighbor entry in kernel
 *
 * @obj : CPS API object which contains arp/nd params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_add_neighbor (cps_api_object_t obj);

/**
 * @brief : Delete Neighbor : This deletes a neighbor entry in kernel
 *
 * @obj : CPS API object which contains arp/nd params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_del_neighbor (cps_api_object_t obj);

/**
 * @brief : Set Neighbor : This replaces a neighbor entry in kernel
 *
 * @obj : CPS API object which contains arp/nd params
 *
 * @return : STD_ERR_OK if successful, otherwise different error code
 */
t_std_error nas_os_set_neighbor (cps_api_object_t obj);

#ifdef __cplusplus
}
#endif

#endif //NAS_OS_L3_H_
