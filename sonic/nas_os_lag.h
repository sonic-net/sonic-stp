
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
 * filename: nas_os_lag.h
 */


#ifndef NAS_OS_LAG_H_
#define NAS_OS_LAG_H_

#include "cps_api_operation.h"
#include "cps_api_object.h"
#include "std_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup NASLAGOS
 *
 This file consists of the NAS API's to create,delete and manage ports to LAG Group from OS.

 Important APIs are:
 @{
 nas_os_create_lag
 nas_os_delete_lag
 nas_os_add_port_to_lag
 nas_os_delete_port_from_lag

 */


/**
 * creates lag interface in kernel
 *
 * @obj : CPS API object for create operation
 *
 * @lag_index : Returns the lag index on response during create
 *
 * @return STD_ERR_OK if successful, or error code
 */
t_std_error nas_os_create_lag(cps_api_object_t obj, hal_ifindex_t *lag_index);

/**
 * Deletes lag interface in kernel
 *
 * @obj : CPS API object for delete operation
 *
 * @return STD_ERR_OK if successful, or error code
 */

t_std_error nas_os_delete_lag(cps_api_object_t obj);

/**
 * Add ports to lag interface in kernel
 *
 * @obj : CPS API object with port index for port
 *        and lag index for set operation.
 *
 * @return STD_ERR_OK if successful, or error code
 */

t_std_error nas_os_add_port_to_lag(cps_api_object_t obj);

/**
 *Delete ports from lag interface in kernel
 *
 * @obj : CPS API object which contains portindex for the port
 *        and lag index for set operation.
 *
 * @return STD_ERR_OK if successful, or error code
 */
t_std_error nas_os_delete_port_from_lag(cps_api_object_t obj);

/**
  @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NAS_OS_LAG_H_ */
