
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
 * nas_os_l3_unittest.cpp
 *
 *  Created on: May 20, 2015
 *      Author: prince
 */

#include <gtest/gtest.h>

#include "std_mac_utils.h"

#include "cps_api_operation.h"
#include "dell-base-routing.h"
#include "nas_os_l3.h"
#include "db_api_linux_init.h"
#include "ds_api_linux_neigh.h"
#include "std_ip_utils.h"
#include "ds_common_types.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

cps_api_object_list_t list_of_objects;

TEST(std_route_test, add_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;


    inet_aton("20.11.11.20",&a);
    ip=a.s_addr;

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    //cps_api_object_t cl = cps_api_object_create();
    //cps_api_object_clone(cl,obj);

    nas_os_add_route(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, add_ifx_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("2.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;

    uint32_t port_index = if_nametoindex("e101-021-0");

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_U32,
                    &port_index,sizeof(port_index));

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    nas_os_add_route(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, set_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);
    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;


    inet_aton("127.0.0.2",&a);
    ip=a.s_addr;

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    //cps_api_object_t cl = cps_api_object_create();
    //cps_api_object_clone(cl,obj);

    nas_os_set_route(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, del_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));

    nas_os_del_route(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, del_nh_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_NH_ADDR;


    inet_aton("20.11.11.20",&a);
    ip=a.s_addr;

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                    &ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);


    nas_os_del_route(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, del_ifx_route) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_AF,AF_INET);
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    uint32_t ip;
    struct in_addr a;
    inet_aton("2.2.3.5",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_ENTRY_ROUTE_PREFIX,&ip,sizeof(ip));
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_PREFIX_LEN,32);

    cps_api_attr_id_t ids[3];
    const int ids_len = sizeof(ids)/sizeof(*ids);
    ids[0] = BASE_ROUTE_OBJ_ENTRY_NH_LIST;
    ids[1] = 0;
    ids[2] = BASE_ROUTE_OBJ_ENTRY_NH_LIST_IFINDEX;


    uint32_t port_index = if_nametoindex("e101-021-0");

    cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_U32,
                    &port_index,sizeof(port_index));

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_ENTRY_NH_COUNT,1);

    nas_os_del_route(obj);

    cps_api_object_delete(obj);
}


TEST(std_route_test, add_neighbor) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.1.1.46",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e00-1");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x00, 0x00, 0x00, 0x1c, 0x1b, 0x1a};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    nas_os_add_neighbor(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, set_neighbor) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.1.1.46",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e00-1");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    hal_mac_addr_t mac_addr = {0x00, 0x00, 0x00, 0x1c, 0x1b, 0x1b};

    cps_api_object_attr_add(obj, BASE_ROUTE_OBJ_NBR_MAC_ADDR, &mac_addr, HAL_MAC_ADDR_LEN);

    nas_os_set_neighbor(obj);

    cps_api_object_delete(obj);
}

TEST(std_route_test, del_neighbor) {

    cps_api_object_t obj = cps_api_object_create();
    cps_api_key_init(cps_api_object_key(obj),cps_api_qualifier_TARGET,
            cps_api_obj_CAT_BASE_ROUTE, BASE_ROUTE_OBJ_OBJ,0 );

    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_AF,AF_INET);

    uint32_t ip;
    struct in_addr a;
    inet_aton("1.1.1.46",&a);
    ip=a.s_addr;

    cps_api_object_attr_add(obj,BASE_ROUTE_OBJ_NBR_ADDRESS,&ip,sizeof(ip));
    int port_index = if_nametoindex("e00-1");
    cps_api_object_attr_add_u32(obj,BASE_ROUTE_OBJ_NBR_IFINDEX, port_index);

    nas_os_del_neighbor(obj);

    cps_api_object_delete(obj);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  cps_api_unittest_init();
  cps_api_linux_init();

  return RUN_ALL_TESTS();
}
