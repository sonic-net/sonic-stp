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
 * cps_api_interface_name_tools.c
 */
#include "ds_common_types.h"
#include <stdlib.h>
#include <net/if.h>

int cps_api_interface_name_to_if_index(const char *name) {
    return if_nametoindex(name);
}
const char * cps_api_interface_if_index_to_name(int index, char *buff, unsigned int len) {
    if (len<HAL_IF_NAME_SZ) return NULL;
    return if_indextoname(index,buff);
}
