
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
 * filename: nas_linux_l2.h
 *
 */

#ifndef NAS_LINUX_L2_H_
#define NAS_LINUX_L2_H_

#include "std_error_codes.h"
#include "cps_api_object.h"
#include "ds_common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief Update the Interface STP state in the kernel
 *
 * @obj - CPS API object which contains ifindex and STP state to be updated.
 *
 * @return STD_ERR_OK if successful, otherwise different error code
 */

t_std_error nl_int_update_stp_state(cps_api_object_t obj);

/*
 * @brief Add/Update MAC entry in the kernel
 *
 * @obj - CPS API object which contains mac entry details to be configured
 *
 * @return STD_ERR_OK if successful, otherwise different error code
 */

t_std_error nas_os_mac_update_entry(cps_api_object_t obj);

/*
 * @brief Change the MAC learning in the kernel for a given interface
 *
 * @ifindex - interface index to identify the interface
 * @enable     - disable/enable learning
 *
 * @return STD_ERR_OK if successful, otherwise different error code
 */

t_std_error nas_os_mac_change_learning(hal_ifindex_t ifindex,bool enable);

#ifdef __cplusplus
}
#endif

#endif /* NAS_LINUX_L2_H_ */
