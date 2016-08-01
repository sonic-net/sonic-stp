
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
 * ds_api_linux_neigh.h
 */

#ifndef DS_API_LINUX_NEIGH_H_
#define DS_API_LINUX_NEIGH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cps_api_object.h"

bool nl_neigh_get_all_request(int sock, int family, int req_id) ;
bool nl_to_neigh_info(int rt_msg_type, struct nlmsghdr *hdr,cps_api_object_t obj);

t_std_error ds_api_linux_neigh_init(cps_api_operation_handle_t handle);

#ifdef __cplusplus
}
#endif


#endif /* DS_API_LINUX_NEIGH_H_ */
