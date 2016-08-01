
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
 * nas_os_interface.h
 *
 */


#ifndef NAS_OS_INTERFACE_H_
#define NAS_OS_INTERFACE_H_

#include "cps_api_object.h"
#include "std_error_codes.h"
#include "ds_common_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup NASOS The NAS OS Abstraction
 *  This file contains NAS OS operations - to be used in the NAS only.
 *
 */

/*  LINK HDR size is added in IP MTU before programming the MTU in NPU
 *  Ethernet (untagged)                    :18 bytes
 *  Vlan Tag                               :22 bytes
 *  untagged packet with vlan-stack header :22 bytes
 *  Tagged packet with vlan-stack header   :26 bytes
 *  With 2 bytes padding  and 4 bytes extra for double tagging
 */
#define NAS_LINK_MTU_HDR_SIZE 32

#define NAS_OS_IF_OBJ_ID_RES_START 4
#define NAS_OS_IF_FLAGS_ID (NAS_OS_IF_OBJ_ID_RES_START) //reserve 4 inside the object for flags
#define NAS_OS_IF_ALIAS (NAS_OS_IF_OBJ_ID_RES_START+1)

/**
 * @brief : API to delete interface from kernel
 * @param if_index : Kernel interface index
 * @param if_name : Interface name
 * @return standard error
 */
t_std_error nas_os_del_interface(hal_ifindex_t if_index);


/**
 * Query a one or more interfaces based on the filter specified and return them into the list
 * @param filter the filter
 * @param result the result list
 * @return STD_ERR_OK if successful otherwise an error code
 */
t_std_error nas_os_get_interface(cps_api_object_t filter,cps_api_object_list_t result);

/**
 * Get the details of a kernel interface object using the attributes of the base-port/interface
 * @param ifix the if index of the interface to query
 * @param obj the returned object attributes
 * @return STD_ERR_OK if successful otherwise an error code
 */
t_std_error nas_os_get_interface_obj(hal_ifindex_t ifix, cps_api_object_t obj);

/**
 * Get the NAS OS object representing a os interface, based on the name provided
 * @param ifname - the name of the interface
 * @param obj the object to return
 * @return STD_ERR_OK if successcul otherwise an error
 */
t_std_error nas_os_get_interface_obj_by_name(const char *ifname, cps_api_object_t obj);

/**
 * Look through the interface and set the retrieved attribute into the kernel
 *
 * @param obj the object hold the attributes to look at
 * @param id the attribute ID to update
 * @return STD_ERR_OK if the request was successful otherwise a specific return code
 */
t_std_error nas_os_interface_set_attribute(cps_api_object_t obj,cps_api_attr_id_t id);


/**
 *  \}
 */

#ifdef __cplusplus
}
#endif


#endif /* NAS_OS_INTERFACE_H_ */
