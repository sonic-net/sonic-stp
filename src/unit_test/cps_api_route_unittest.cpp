
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
 * cps_api_route_unittest.cpp
 *
 *  Created on: Feb 23, 2015
 *      Author: cwichmann
 */


#include <gtest/gtest.h>

#include "std_mac_utils.h"

#include "cps_api_operation.h"
#include "cps_api_route.h"


#include "db_api_linux_init.h"
#include "ds_api_linux_neigh.h"
#include "std_ip_utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

cps_api_object_list_t list_of_objects;

TEST(std_route_test, get_all_routes) {
    list_of_objects = cps_api_object_list_create();

    cps_api_get_params_t gp;
    ASSERT_TRUE(cps_api_get_request_init(&gp)==cps_api_ret_code_OK);
    cps_api_key_t key;

    cps_api_key_init(&key,cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,cps_api_route_obj_ROUTE,0);

    gp.key_count = 1;
    gp.keys = &key;


    ASSERT_TRUE(cps_api_get(&gp)==cps_api_ret_code_OK);
    size_t ix = 0;
    size_t mx = cps_api_object_list_size(gp.list);
    for ( ; ix < mx ; ++ix ) {
        cps_api_object_t obj = cps_api_object_list_get(gp.list,ix);
        char buff[1000];
        printf("Object: %s\n",cps_api_object_to_string(obj,buff,sizeof(buff)));

        cps_api_object_attr_t list[cps_api_if_ROUTE_A_MAX];

        cps_api_object_attr_fill_list(obj,0,list,sizeof(list)/sizeof(*list));

        ASSERT_TRUE(list[cps_api_if_ROUTE_A_HOP_COUNT]);
        ASSERT_TRUE(list[cps_api_if_ROUTE_A_PREFIX_LEN]);

        int cnt = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_HOP_COUNT]);
        printf("Next Hops... %d\n",cnt);

        if (cnt>1) {
            ASSERT_TRUE(list[cps_api_if_ROUTE_A_NH]);
        }
        if (list[cps_api_if_ROUTE_A_NH]) {
            cps_api_object_it_t nhit;
            cps_api_object_it_from_attr(list[cps_api_if_ROUTE_A_NH],&nhit);
            cps_api_object_it_inside(&nhit);

            for ( ; cps_api_object_it_valid(&nhit) ;
                    cps_api_object_it_next(&nhit)) {
                cps_api_object_it_t node = nhit;
                cps_api_object_it_inside(&node);

                cps_api_object_attr_t hostaddr = cps_api_object_it_find(&node,cps_api_if_ROUTE_A_NEXT_HOP_ADDR);
                if (hostaddr==NULL) continue;

                cps_api_object_t clone=cps_api_object_create();

                if (clone != NULL) {
                    cps_api_object_clone(clone,obj);

                    if (!cps_api_object_list_append(list_of_objects,clone)) {
                        printf("failed to add clone %d\n",(int)ix);
                        cps_api_object_delete(clone);
                        break;
                    }
                    printf("Adding clone %d\n",(int)ix);
                }
                //or...
                for ( ; cps_api_object_it_valid(&node) ;
                        cps_api_object_it_next(&node)) {

                    cps_api_attr_id_t id = cps_api_object_attr_id(node.attr);
                    printf("NH Data: %d\n",(int)id);
                }
            }

        }
    }
    cps_api_get_request_close(&gp);
}



TEST(std_route_test, set_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
               cps_api_obj_cat_ROUTE,cps_api_route_obj_ROUTE,0 );

    cps_api_object_attr_add_u32(obj,cps_api_if_ROUTE_A_FAMILY,AF_INET);
    cps_api_object_attr_add_u32(obj,cps_api_if_ROUTE_A_PREFIX_LEN,32);

    hal_ip_addr_t ip;
    ip.af_index = AF_INET;
    struct in_addr a;
    inet_aton("1.2.3.4",&a);
    ip.u.v4_addr=a.s_addr;

    cps_api_object_attr_add(obj,cps_api_if_ROUTE_A_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,cps_api_if_ROUTE_A_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = cps_api_if_ROUTE_A_NH;
    ids[1] = 0;
    ids[2] =cps_api_if_ROUTE_A_NEXT_HOP_ADDR;

    inet_aton("127.0.0.1",&a);
    ip.u.v4_addr=a.s_addr;

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));

    cps_api_object_attr_add_u32(obj,cps_api_if_ROUTE_A_HOP_COUNT,1);

    cps_api_object_t cl = cps_api_object_create();
    cps_api_object_clone(cl,obj);

    cps_api_transaction_params_t tr;
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_set(&tr,cl);
    ASSERT_TRUE(cps_api_commit(&tr)!=cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

    cl = cps_api_object_create();
    cps_api_object_clone(cl,obj);
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_create(&tr,cl);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);

    cl = cps_api_object_create();
    cps_api_object_clone(cl,obj);
    ASSERT_TRUE(cps_api_transaction_init(&tr)==cps_api_ret_code_OK);
    cps_api_delete(&tr,cl);
    ASSERT_TRUE(cps_api_commit(&tr)==cps_api_ret_code_OK);
    cps_api_transaction_close(&tr);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  cps_api_unittest_init();
  cps_api_linux_init();

  return RUN_ALL_TESTS();
}
