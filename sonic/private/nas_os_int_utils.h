
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
 * nas_os_int_utils.h
 *
 *  Created on: May 19, 2015
 */

#ifndef CPS_API_LINUX_INC_PRIVATE_NAS_OS_INT_UTILS_H_
#define CPS_API_LINUX_INC_PRIVATE_NAS_OS_INT_UTILS_H_

#include "std_error_codes.h"
#include "cps_api_interface_types.h"
#include "ds_common_types.h"
#include "cps_api_object.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OS_IF_CHANGE_NONE = 0x0,
    OS_IF_ADM_CHANGE  = 0x1,
    OS_IF_MTU_CHANGE  = 0x2,
    OS_IF_PHY_CHANGE  = 0x4,
    OS_IF_CHANGE_ALL  = 0xf
}if_change_t;

t_std_error nas_os_util_int_admin_state_get(const char *name,
        db_interface_state_t *state, db_interface_operational_state_t *ostate) ;
t_std_error nas_os_util_int_admin_state_set(const char *name,
        db_interface_state_t state, db_interface_operational_state_t ostate);

t_std_error nas_os_util_int_mac_addr_get(const char *name, hal_mac_addr_t *macAddr);
t_std_error nas_os_util_int_mac_addr_set(const char *name, hal_mac_addr_t *macAddr);

t_std_error nas_os_util_int_mtu_set(const char *name, unsigned int mtu);
t_std_error nas_os_util_int_mtu_get(const char *name, unsigned int *mtu);

t_std_error nas_os_util_int_get_if(cps_api_object_t obj, hal_ifindex_t ifix);

t_std_error nas_os_util_int_flags_get(const char *name, unsigned *flags);

t_std_error nas_os_util_int_get_if_details(const char *name, cps_api_object_t obj);

bool os_interface_mask_event(hal_ifindex_t ifix, if_change_t mask_val);

#ifdef __cplusplus
}
#endif

#endif /* CPS_API_LINUX_INC_PRIVATE_NAS_OS_INT_UTILS_H_ */
