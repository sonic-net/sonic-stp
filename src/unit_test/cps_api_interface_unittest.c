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
 * cps_api_interface_unittest.c
 *
 *  Created on: May 19, 2015
 */

#include "nas_os_interface.h"
#include "dell-base-if.h"
#include "dell-base-if-linux.h"
#include "cps_api_object_key.h"

#include <stdio.h>

int main() {
    cps_api_object_list_t lst = cps_api_object_list_create();
    cps_api_object_t filter = cps_api_object_create();
    nas_os_get_interface(filter,lst);
    size_t ix = 0;
    size_t mx = cps_api_object_list_size(lst);
    for ( ; ix < mx ; ++ix ) {
        cps_api_object_t o = cps_api_object_list_get(lst,ix);
        cps_api_key_t * key = cps_api_object_key(o);
        char buff[1024];
        printf("Key = %s\n",cps_api_key_print(key,buff,sizeof(buff)));
    }
    cps_api_object_list_destroy(lst,true);
    lst = cps_api_object_list_create();
    int ifix = 1;
    cps_api_set_key_data_uint(filter,DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX,&ifix,sizeof(ifix));

    nas_os_get_interface(filter,lst);

    ix = 0;
    mx = cps_api_object_list_size(lst);
    for ( ; ix < mx ; ++ix ) {
        cps_api_object_t o = cps_api_object_list_get(lst,ix);
        cps_api_key_t * key = cps_api_object_key(o);
        char buff[1024];
        printf("Key = %s\n",cps_api_key_print(key,buff,sizeof(buff)));
    }

    return 0;
}
