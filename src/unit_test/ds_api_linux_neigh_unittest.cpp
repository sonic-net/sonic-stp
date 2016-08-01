
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
 * ds_api_linux_neigh_unittest.cpp
 */


#include <gtest/gtest.h>

#include "std_mac_utils.h"

#include "cps_api_operation.h"
#include "cps_api_route.h"

#include "private/netlink_tools.h"
#include "db_api_linux_init.h"
#include "ds_api_linux_neigh.h"
#include "std_ip_utils.h"

bool get_netlink_data(int rt_msg_type, struct nlmsghdr *hdr, void *data) {
    cps_api_object_t obj = cps_api_object_create();

    if (nl_to_neigh_info(rt_msg_type,hdr, obj)) {
        cps_api_object_attr_t mac = cps_api_object_attr_get(obj,cps_api_if_NEIGH_A_NBR_MAC);
        hal_mac_addr_t m;
        memcpy(m,cps_api_object_attr_data_bin(mac),sizeof(m));
        char buff[50];
        printf("Neigh: %s (state:%d)\n",std_mac_to_string(&m,buff,sizeof(buff)),
                cps_api_object_attr_data_u32(cps_api_object_attr_get(obj,cps_api_if_NEIGH_A_STATE)));
    }
    cps_api_object_delete(obj);
    return true;
}

bool test_netlink() {
    int sock = nas_nl_sock_create(nas_nl_sock_T_NEI);
    int RANDOM_REQ_ID = 1231231;
    if (nl_neigh_get_all_request(sock,AF_INET,RANDOM_REQ_ID)) {
        char buf[2048];
        netlink_tools_process_socket(sock,get_netlink_data,
                NULL,buf,sizeof(buf),&RANDOM_REQ_ID,NULL);
    }
    if (nl_neigh_get_all_request(sock,AF_INET6,++RANDOM_REQ_ID)) {
        char buf[2048];
        netlink_tools_process_socket(sock,get_netlink_data,
                NULL,buf,sizeof(buf),&RANDOM_REQ_ID,NULL);
    }
    if (nl_neigh_get_all_request(sock,AF_PACKET,++RANDOM_REQ_ID)) {
        char buf[2048];
        netlink_tools_process_socket(sock,get_netlink_data,
                NULL,buf,sizeof(buf),&RANDOM_REQ_ID,NULL);
    }
    return true;
}

bool test_get_all_neigh(void) {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);
    cps_api_key_t key;

    cps_api_key_init(&key,cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,cps_api_route_obj_NEIBH,0);
    gp.key_count = 1;
    gp.keys = &key;

    bool rc = false;
    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);
        for ( ; ix < mx ; ++ix ) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list,ix);
            cps_api_object_attr_t mac = cps_api_object_attr_get(obj,cps_api_if_NEIGH_A_NBR_MAC);
            hal_mac_addr_t m;
            memcpy(m,cps_api_object_attr_data_bin(mac),sizeof(m));
            char buff[50];
            printf("Neigh: %s\n",std_mac_to_string(&m,buff,sizeof(buff)));
        }
        rc = true;
    }
    cps_api_get_request_close(&gp);
    return rc;
}

TEST(std_route_test, get_all_routes) {
    ASSERT_EQ(true,test_netlink());
    ASSERT_EQ(true,test_get_all_neigh());
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (cps_api_linux_init()!=STD_ERR_OK) {

  }

  return RUN_ALL_TESTS();
}
