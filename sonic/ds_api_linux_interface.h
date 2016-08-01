
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
 * ds_api_linux_interface.h
 */

#ifndef DS_API_LINUX_INTERFACE_H_
#define DS_API_LINUX_INTERFACE_H_

#include "std_error_codes.h"

#include "cps_api_interface_types.h"

#include "cps_api_events.h"
#include "cps_api_operation.h"

#include <linux/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This API initializes the hooks for the linux interface mapping
 */
t_std_error ds_api_linux_interface_init(cps_api_operation_handle_t handle);

/**
 * Some common netlink level functionality - currently located here
 */

bool nl_get_if_info(int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj);
bool nl_get_stg_info(int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj);
bool nl_get_ip_info(int rt_msg_type, struct nlmsghdr *hdr, cps_api_object_t obj);

bool nl_interface_get_request(int sock, int req_id);

/**
 * Convert to and from interface name to index.. needs to be updated to support VRF
 !TODO support for VRF needed here
 */

int cps_api_interface_name_to_if_index(const char *name);
const char * cps_api_interface_if_index_to_name(int index, char *buff, unsigned int len);


#ifdef __cplusplus
}
#endif

#endif /* DS_API_LINUX_INTERFACE_H_ */
