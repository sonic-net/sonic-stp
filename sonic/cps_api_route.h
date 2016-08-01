
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
 * cps_api_route.h
 */

#ifndef cps_api_route_H_
#define cps_api_route_H_

typedef enum {
    cps_api_route_obj_ROUTE=1,
    cps_api_route_obj_NEIBH,
    cps_api_route_obj_EVENT,
}cps_api_route_sub_category_t;


//cps_api_route_obj_NEIBH
typedef enum {
    cps_api_if_NEIGH_A_FAMILY=0, //uint32_t
    cps_api_if_NEIGH_A_OPERATION=1, //db_nbr_event_type_t
    cps_api_if_NEIGH_A_NBR_ADDR=2, //hal_ip_addr_t
    cps_api_if_NEIGH_A_NBR_MAC=3, //hal_mac_addr_t
    cps_api_if_NEIGH_A_IFINDEX=4, //hal_ifindex_t
    cps_api_if_NEIGH_A_PHY_IFINDEX=5, //hal_ifindex_t
    cps_api_if_NEIGH_A_VRF=7, //uint32_t
    cps_api_if_NEIGH_A_EXPIRE=8, //uint32_t
    cps_api_if_NEIGH_A_FLAGS=9, //uint32_t
    cps_api_if_NEIGH_A_STATE=10, //uint32_t
    cps_api_if_NEIGH_A_MAX
}cps_api_if_NEIGH_ATTR;

//cps_api_int_obj_ROUTE
typedef enum {
    cps_api_if_ROUTE_A_MSG_TYPE=0, //db_route_msg_t
    cps_api_if_ROUTE_A_DISTANCE=1, //uint32_t
    cps_api_if_ROUTE_A_PROTOCOL=2, //uint32_t
    cps_api_if_ROUTE_A_VRF=3, //uint32_t
    cps_api_if_ROUTE_A_PREFIX=4, //hal_ip_addr_t
    cps_api_if_ROUTE_A_PREFIX_LEN=5, //uint32_t
    cps_api_if_ROUTE_A_NH_IFINDEX=6, //uint32_t
    cps_api_if_ROUTE_A_NEXT_HOP_VRF=7, //uint32_t
    cps_api_if_ROUTE_A_NEXT_HOP_ADDR=8, //hal_ip_addr_t
    cps_api_if_ROUTE_A_NEXT_HOP_FLAGS=9,
    cps_api_if_ROUTE_A_NEXT_HOP_WEIGHT=10,
    cps_api_if_ROUTE_A_HOP_COUNT=11,
    cps_api_if_ROUTE_A_NH=12,
    cps_api_if_ROUTE_A_FAMILY=13,
    cps_api_if_ROUTE_A_MAX
}cps_api_if_ROUTE_ATTR;


#endif /* cps_api_route_H_ */
