
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
 * filename: cps_api_interface_types.h
 */



#ifndef cps_api_iNTERFACE_TYPES_H_
#define cps_api_iNTERFACE_TYPES_H_

#include "ds_common_types.h"
#include "cps_api_operation.h"

/**
 * Interface Category subtypes
 */
typedef enum {
    cps_api_int_obj_INTERFACE=1,//!< db_int_obj_INTERFACE
    cps_api_int_obj_INTERFACE_ADDR,
    cps_api_int_obj_HW_LINK_STATE,
}cps_api_interface_sub_category_t ;

static inline void cps_api_int_if_key_create(
        cps_api_key_t *key, bool use_inst,
        uint32_t vrf, uint32_t ifix) {

    cps_api_key_init(key,
        cps_api_qualifier_TARGET,
        cps_api_obj_cat_INTERFACE,
        cps_api_int_obj_INTERFACE, use_inst ? 2 : 0,
                vrf, ifix);
}

#define CPS_API_INT_IF_OBJ_KEY_IFIX 4

typedef enum  {
    ADMIN_DEF,
    DB_ADMIN_STATE_UP,
    DB_ADMIN_STATE_DN
}db_interface_state_t;

typedef enum  {
    DB_INTERFACE_OP_SET,
    DB_INTERFACE_OP_CREATE,
    DB_INTERFACE_OP_DELETE
}db_interface_operation_t;

typedef enum {
    OPER_DEF,
    DB_OPER_STATE_UP,
    DB_OPER_STATE_DN
}db_interface_operational_state_t ;


typedef enum {
    ADDR_DEFAULT,
    ADDR_ADD,
    ADDR_DEL
}db_if_addr_msg_type_t ;

typedef enum {
    ROUTE_DEFAULT,
    ROUTE_ADD,
    ROUTE_DEL,
    ROUTE_UPD
}db_route_msg_t;

typedef enum {
    NBR_DEFAULT,
    NBR_ADD,
    NBR_DEL,
    NBR_UPD
}db_nbr_event_type_t;

typedef enum {
    DB_IF_TYPE_PHYS_INTER,
    DB_IF_TYPE_VLAN_INTER,
    DB_IF_TYPE_LAG_INTER,
    DB_IF_TYPE_BRIDGE_INTER,
    DB_IF_TYPE_LOGICAL
} db_if_type_t;

/**
 * The size of an interface name
 */
typedef char hal_ifname_t[HAL_IF_NAME_SZ];


//cps_api_int_obj_INTERFACE_ADDR

typedef enum {
    cps_api_if_ADDR_A_NAME=0, //char *
    cps_api_if_ADDR_A_IFINDEX=1,//uint32_t
    cps_api_if_ADDR_A_FLAGS=2, //uint32_t
    cps_api_if_ADDR_A_IF_ADDR=3,
    cps_api_if_ADDR_A_IF_MASK=4, //hal_ip_addr_t
    cps_api_if_ADDR_A_OPER=5,    //db_if_addr_msg_type_t
}cps_api_if_ADDR_ATTR;

//cps_api_int_obj_INTERFACE
typedef enum {
    cps_api_if_STRUCT_A_NAME=0, //char *
    cps_api_if_STRUCT_A_IFINDEX=1,//uint32_t
    cps_api_if_STRUCT_A_FLAGS=2, //uint32_t
    cps_api_if_STRUCT_A_IF_MACADDR=3,    //hal_mac_addr_t
    cps_api_if_STRUCT_A_IF_VRF=4, //uint32_t
    cps_api_if_STRUCT_A_IF_FAMILY=5, //uint32_t
    cps_api_if_STRUCT_A_ADMIN_STATE=6,    //db_interface_state_t
    cps_api_if_STRUCT_A_OPER_STATE=7,    //db_interface_operational_state_t
    cps_api_if_STRUCT_A_MTU=8,    //uint32_t
    cps_api_if_STRUCT_A_IF_TYPE=9,    //db_if_type_t
    cps_api_if_STRUCT_A_VLAN_ID=10,    //uint32_t(hal_vlan_id_t)
    cps_api_if_STRUCT_A_OPERATION=11,    //db_interface_operation_t
    cps_api_if_STRUCT_A_MASTER=12,
    cps_api_if_STRUCT_A_STP_STATE=13,
    cps_api_if_STRUCT_A_VLAN_PORT_INDEX=14, //uint32_t
    cps_api_if_STRUCT_A_BRIDGE_INDEX=15, //uint32_t
    cps_api_if_STRUCT_A_MAX
}cps_api_if_STRUCT_ATTR;


#endif /* DB_EVENT_INTERFACE_H_ */
