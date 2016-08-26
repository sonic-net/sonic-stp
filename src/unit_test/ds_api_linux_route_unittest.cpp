
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
 * ds_api_linux_route.cpp
 */
#include <gtest/gtest.h>

#include "std_mac_utils.h"

#include "cps_api_interface_types.h"
#include "cps_api_operation.h"
#include "cps_api_route.h"

#include "private/netlink_tools.h"
#include "db_api_linux_init.h"
#include "ds_api_linux_route.h"
#include "std_ip_utils.h"
#include "std_envvar.h"


#define ROUTE_NEXT_HOP_MAX_COUNT   64
#define ROUTE_NEXT_HOP_DEF_WEIGHT (10)

typedef struct  {
    db_route_msg_t  msg_type;
    unsigned short  distance;
    unsigned short  protocol;
    unsigned long   vrfid;
    hal_ip_addr_t       prefix;
    unsigned short      prefix_masklen;
    hal_ifindex_t   nh_if_index;
    unsigned long   nh_vrfid;
    hal_ip_addr_t         nh_addr;
    /* WECMP nh_list*/
    struct {
        hal_ifindex_t   nh_if_index;
        hal_ip_addr_t   nh_addr;
        uint32_t         nh_weight;
    } nh_list[ROUTE_NEXT_HOP_MAX_COUNT];

    size_t hop_count;
} db_route_t;

void cps_obj_to_route(cps_api_object_t obj, db_route_t *r) {
    memset(r,0,sizeof(*r));
    cps_api_object_attr_t list[cps_api_if_ROUTE_A_MAX];
    char buff[1000], buff1[1000];

    cps_api_object_attr_fill_list(obj,0,list,sizeof(list)/sizeof(*list));

    if (list[cps_api_if_ROUTE_A_MSG_TYPE]!=NULL)
        r->msg_type = (db_route_msg_t) cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_MSG_TYPE]);
    if (list[cps_api_if_ROUTE_A_DISTANCE]!=NULL)
        r->distance = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_DISTANCE]);
    if (list[cps_api_if_ROUTE_A_PROTOCOL]!=NULL)
        r->protocol = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_PROTOCOL]);
    if (list[cps_api_if_ROUTE_A_VRF]!=NULL)
        r->vrfid = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_VRF]);

    if (list[cps_api_if_ROUTE_A_PREFIX]!=NULL)
        r->prefix = *(hal_ip_addr_t*)cps_api_object_attr_data_bin(list[cps_api_if_ROUTE_A_PREFIX]);
    if (list[cps_api_if_ROUTE_A_PREFIX_LEN]!=NULL)
        r->prefix_masklen = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_PREFIX_LEN]);
   if (list[cps_api_if_ROUTE_A_NH_IFINDEX]!=NULL)
        r->nh_if_index = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_NH_IFINDEX]);
    if (list[cps_api_if_ROUTE_A_NEXT_HOP_VRF]!=NULL)
        r->nh_vrfid = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_NEXT_HOP_VRF]);
    if (list[cps_api_if_ROUTE_A_NEXT_HOP_ADDR]!=NULL)
        r->nh_addr = *(hal_ip_addr_t*)cps_api_object_attr_data_bin(list[cps_api_if_ROUTE_A_NEXT_HOP_ADDR]);

    if (list[cps_api_if_ROUTE_A_HOP_COUNT]!=NULL)
           r->hop_count = cps_api_object_attr_data_u32(list[cps_api_if_ROUTE_A_HOP_COUNT]);

    /*
     * If multiple NHs are present, check cps_api_if_ROUTE_A_NH attribute
     */
    if (list[cps_api_if_ROUTE_A_NH]) {
       cps_api_object_it_t nhit;
       cps_api_object_it_from_attr(list[cps_api_if_ROUTE_A_NH],&nhit);
       cps_api_object_it_inside(&nhit);
       size_t hop = 0;
       for ( ; cps_api_object_it_valid(&nhit) ;
               cps_api_object_it_next(&nhit), ++hop) {
           cps_api_object_it_t node = nhit;
           cps_api_object_it_inside(&node);
           r->nh_list[hop].nh_weight = ROUTE_NEXT_HOP_DEF_WEIGHT;
           for ( ; cps_api_object_it_valid(&node) ;
                   cps_api_object_it_next(&node)) {

               switch(cps_api_object_attr_id(node.attr)) {
               case cps_api_if_ROUTE_A_NH_IFINDEX:
                   r->nh_list[hop].nh_if_index = cps_api_object_attr_data_u32(node.attr);
                   break;
               case cps_api_if_ROUTE_A_NEXT_HOP_ADDR:
                   r->nh_list[hop].nh_addr = *(hal_ip_addr_t*)cps_api_object_attr_data_bin(node.attr);

                   printf("Prefix:%s NH(%d): %s, nhIndex %d \r\n",
                           std_ip_to_string(&r->prefix,buff,sizeof(buff)), (int)hop,
                           std_ip_to_string(&r->nh_list[hop].nh_addr, buff1,
                           sizeof(buff1)), (int)hop);

                   break;
               case cps_api_if_ROUTE_A_NEXT_HOP_WEIGHT:
                   r->nh_list[hop].nh_weight = cps_api_object_attr_data_u32(node.attr);
                   break;
               default:
                   break;
               }

           }
        }
    }
}

bool get_netlink_data(int rt_msg_type, struct nlmsghdr *hdr, void *data) {
    cps_api_object_t obj = cps_api_object_create();



    if (nl_to_route_info(rt_msg_type,hdr, obj)) {
        db_route_t r;
        cps_obj_to_route(obj,&r);
    }

    cps_api_object_delete(obj);
    return true;
}

bool test_netlink() {

    int sock = nas_nl_sock_create(nas_nl_sock_T_ROUTE);
    const int RANDOM_REQ_ID = 1231231;
    if (nl_request_existing_routes(sock,0,RANDOM_REQ_ID)) {
        const char *len = std_getenv("NL_ROUTE_BUFFER");
        printf("Route buffer environment variable %s is %s",
                "NL_ROUTE_BUFFER", len==NULL? "" : len );
        if (len==NULL) {
            len = "16384";
        }

        char buf[atoi(len)];

        return netlink_tools_process_socket(sock,get_netlink_data,
                NULL,buf,sizeof(buf),&RANDOM_REQ_ID,NULL);
    }
    return false;
}

bool test_get_all_routes() {
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);
    cps_api_key_t key;

    cps_api_key_init(&key,cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,cps_api_route_obj_ROUTE,0);
    gp.key_count = 1;
    gp.keys = &key;

    bool rc = false;
    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t ix = 0;
        size_t mx = cps_api_object_list_size(gp.list);
        for ( ; ix < mx ; ++ix ) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list,ix);

            cps_api_object_attr_t type = cps_api_object_attr_get(obj,cps_api_if_ROUTE_A_MSG_TYPE);
            cps_api_object_attr_t ip = cps_api_object_attr_get(obj,cps_api_if_ROUTE_A_PREFIX);
            cps_api_object_attr_t nh = cps_api_object_attr_get(obj,cps_api_if_ROUTE_A_NEXT_HOP_ADDR);
            cps_api_object_attr_t ifix = cps_api_object_attr_get(obj,cps_api_if_ROUTE_A_NH_IFINDEX);

            char pref[100];
            char dest[100];

            printf("OP: %d, Prefix: %s, GW: %s, IF: %d\n",
                    type==NULL ? -1 : (int)cps_api_object_attr_data_u32(type),
                    (ip==NULL) ? "NULL" : std_ip_to_string((hal_ip_addr_t*)cps_api_object_attr_data_bin(ip),pref,sizeof(pref)),
                    (nh==NULL) ? "NULL" : std_ip_to_string((hal_ip_addr_t*)cps_api_object_attr_data_bin(nh),dest,sizeof(dest)),
                    ifix==NULL ? -1 : (int)cps_api_object_attr_data_u32(ifix));
        }

        rc = true;
    }
    cps_api_get_request_close(&gp);
    return rc;
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
    ASSERT_EQ(STD_ERR_OK,cps_api_linux_init()); //test that after registration service starts up
    ASSERT_TRUE(cps_api_unittest_init());
    sleep(2);
    ASSERT_EQ(true,test_get_all_routes());
    ASSERT_EQ(true,test_netlink());
    ASSERT_EQ(true,test_get_all_neigh());
}

TEST(std_route_test_nl, get_nl_routes) {
    ASSERT_EQ(true,test_netlink());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (cps_api_linux_init()!=STD_ERR_OK) {

  }

  return RUN_ALL_TESTS();
}



