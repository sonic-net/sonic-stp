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
 * ds_linux_init.c
 */
#include "std_error_codes.h"
#include "ds_api_linux_interface.h"
#include "ds_api_linux_route.h"
#include "ds_api_linux_neigh.h"

t_std_error (*init_functions[])(cps_api_operation_handle_t handle) = {
        ds_api_linux_interface_init,
        ds_api_linux_route_init,
        ds_api_linux_neigh_init,
};

static cps_api_operation_handle_t handle;

t_std_error cps_api_linux_init(void) {

    if (cps_api_operation_subsystem_init(&handle,1)!=cps_api_ret_code_OK) {
        return STD_ERR(CPSNAS,FAIL,0);
    }

    size_t ix = 0;
    size_t mx = sizeof(init_functions)/sizeof(*init_functions);
    for ( ; ix < mx ; ++ix ) {
        t_std_error rc = init_functions[ix](handle);
        if (rc!=STD_ERR_OK) return rc;
    }
    return STD_ERR_OK;
}
