
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
 * filename: db_unit_test.cpp
 */


/*
 * db_unit_test.cpp
 */

#include "cps_api_operation.h"
#include "db_api_linux_init.h"

#include "private/netlink_tools.h"
#include "ds_api_linux_interface.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

bool get_netlink_data(int rt_msg_type, struct nlmsghdr *hdr, void *data) {

    cps_api_object_t obj=cps_api_object_create();
    if (!nl_get_if_info(rt_msg_type,hdr,obj)) return false;
    cps_api_object_attr_t attr = cps_api_object_attr_get(obj,cps_api_if_STRUCT_A_NAME);
    if (attr==CPS_API_ATTR_NULL) return false;
    char buff[1024];
    printf("OID = %s\n",cps_api_key_print(cps_api_object_key(obj),buff,sizeof(buff)));
    printf("Name:%s \n", (char*)cps_api_object_attr_data_bin(attr));
    cps_api_object_delete(obj);
    return true;
}

bool netlinl_test() {
    int sock = nas_nl_sock_create(nas_nl_sock_T_INT);
    char buf[4196];
    const int RANDOM_REQ_ID = 23231;
    if (nl_interface_get_request(sock,RANDOM_REQ_ID)) {
        if (netlink_tools_process_socket(sock,get_netlink_data, NULL,buf,sizeof(buf),
                &RANDOM_REQ_ID,NULL)) {
            close(sock);
            return true;
        }
    }
    close(sock);
    return false;
}

int main() {

    if (!netlinl_test()) {
        exit(1);
    }

    cps_api_linux_init();
    {
        cps_api_get_params_t gr;
        cps_api_get_request_init(&gr);

        cps_api_key_t if_key;
        cps_api_int_if_key_create(&if_key,false,0,0);
        gr.key_count = 1;
        gr.keys = &if_key;
        //ignore failures since all we want is to fill up the get request
        if (cps_api_get(&gr) != cps_api_ret_code_OK) {
            printf("cps_api_get() failed\n");
        }

        cps_api_get_request_close(&gr);
    }
    cps_api_get_params_t gp;

    if (cps_api_get_request_init(&gp)!=cps_api_ret_code_OK) {
        exit(1);
    }

    cps_api_key_t key;
    cps_api_int_if_key_create(&key,false,0,0);

    gp.key_count = 1;
    gp.keys = &key;

    cps_api_transaction_params_t tr;

    if (cps_api_transaction_init(&tr)!=cps_api_ret_code_OK) {
        cps_api_get_request_close(&gp);
        return -1;
    }

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);
        char buff[1024];
        for ( ; ix < mx ; ++ix ) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list,ix);
            printf("Obj %d -> %s\n",(int)ix,cps_api_object_to_string(obj,buff,sizeof(buff)));

            cps_api_object_t clone = cps_api_object_create();
            if (!cps_api_object_clone(clone,obj)) {
                printf("Clone Failed %d -> %s\n",(int)ix,cps_api_object_to_string(clone,buff,sizeof(buff)));
            }
            else {
                printf("Clone %d -> %s\n",(int)ix,cps_api_object_to_string(clone,buff,sizeof(buff)));
            }
            cps_api_set(&tr,clone);
        }
    }
    cps_api_get_request_close(&gp);

    if (cps_api_commit(&tr)!=cps_api_ret_code_OK) {
        return -1;
    }
    cps_api_transaction_close(&tr);


    return 0;
}
