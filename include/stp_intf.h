/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __STP_INTF_H__
#define __STP_INTF_H__

///////////////
//BITMAP MACRO's
///////////////

//Ex :- in a system with 64 ports.
// STP_BMP_PO_OFFSET = 256
// 0-255   : regular ports
// 256-511 : port-channels
#define STP_BMP_PO_OFFSET ((g_max_stp_port/2))
//set max po id equivalent to max stp instances
#define STP_MAX_PO_ID 4096
#define STP_UPDATE_MAX_PORT(port) g_stpd_sys_max_port = (g_stpd_sys_max_port < port) ?port:g_stpd_sys_max_port;

typedef uint32_t PORT_ID;
#define BAD_PORT_ID                     (PORT_ID)(-1)
#define MAX_PORT g_max_stp_port
#define MAX_PORT_PER_MODULE MAX_PORT
#define CONFIG_MODULE_NUMBER_OF_PORTS(s) MAX_PORT_PER_MODULE

#define STP_ETH_NAME_PREFIX_LEN 8 //"Ethernet" prefix is expected for all phy ports.
#define STP_PO_NAME_PREFIX_LEN 11
#define STP_IS_ETH_PORT_ID(port_id) (port_id < (g_max_stp_port/2))
#define STP_IS_ETH_PORT(name) (0 == strncmp("Ethernet", name, STP_ETH_NAME_PREFIX_LEN))
#define STP_IS_PO_PORT(name) (0 == strncmp("PortChannel", name, STP_PO_NAME_PREFIX_LEN))

#define IS_MEMBER(_bmp, _port)                              bmp_isset(_bmp, _port)
#define is_member(_bmp, _port)                              bmp_isset(_bmp, _port)
#define PORT_MASK_ISSET(_bmp, _port)                        bmp_isset(_bmp, _port)
#define PORT_MASK_ISSET_ANY(_bmp)                           bmp_isset_any(_bmp)

#define PORT_MASK_SET_ALL(_bmp)                             bmp_set_all(_bmp)
#define PORT_MASK_ZERO(_bmp)                                bmp_reset_all(_bmp)

#define set_mask(_bmp)                                      bmp_set_all(_bmp)
#define clear_mask(_bmp)                                    bmp_reset_all(_bmp)
#define is_mask_clear(_bmp)                                 (!(bmp_isset_any(_bmp)))
#define is_mask_equal(_bmp1, _bmp2)                         bmp_is_mask_equal(_bmp1, _bmp2)
#define copy_mask(_dst, _src)                               bmp_copy_mask(_dst, _src)
#define not_mask(_dst, _src)                                bmp_not_mask(_dst, _src)
#define and_masks(_tgt, _bmp1, _bmp2)                       bmp_and_masks(_tgt, _bmp1, _bmp2)
#define and_not_masks(_tgt, _bmp1, _bmp2)                   bmp_and_not_masks(_tgt, _bmp1, _bmp2)
#define or_masks(_tgt, _bmp1, _bmp2)                        bmp_or_masks(_tgt, _bmp1, _bmp2)
#define xor_masks(_tgt, _bmp1, _bmp2)                       bmp_xor_masks(_tgt, _bmp1, _bmp2)

#define set_mask_bit(_bmp, _port)                           bmp_set(_bmp, _port)
#define clear_mask_bit(_bmp, _port)                         bmp_reset(_bmp, _port)

#define port_mask_get_next_port(_bmp,_port)                 bmp_get_next_set_bit(_bmp, _port)
#define port_mask_get_first_port(_bmp)                      bmp_get_first_set_bit(_bmp)

#define stp_intf_allocate_po_id()                           (STP_BMP_PO_OFFSET + bmp_set_first_unset_bit(g_stpd_po_id_pool))
#define stp_intf_release_po_id(_port_id)                    bmp_reset(g_stpd_po_id_pool, (_port_id - STP_BMP_PO_OFFSET))


////////////////////
//STP INTERFACE DB 
////////////////////

#define MAX_CONFIG_PORTS 16
#define MSTP_MAX_INSTANCES 65               //MSTP_MAX_INSTANCES_PER_REGION+1

typedef struct
{
    #define MSTP_PORT_PRI_FLAG          0x0001
    #define MSTP_PORT_PATH_COST_FLAG    0x0002
    int8_t         flag;
    uint16_t        priority;
    uint32_t        path_cost;
}MST_INFO;

typedef struct
{
    char            ifname[IFNAMSIZ+1];
    uint32_t        kif_index;          // kernel if index
    uint32_t        port_id;            // local port if index
    char            mac[L2_ETH_ADD_LEN];
    uint32_t        speed;
    uint8_t         oper_state;
    uint8_t         is_valid;           /* PO with no member ports are invalid  */
    uint8_t         def_path_cost;
    uint16_t        member_port_count;  /* No of member ports in case of LAG */
    uint32_t        master_ifindex;     /*Applicable only for Member port of LAG */
    uint16_t        priority;
    uint32_t        path_cost;
    int             sock;               //socket created for this kif_index
    MST_INFO        mst_info[MSTP_MAX_INSTANCES];
    struct event *  ev;                 //libevent to handle this sock
}INTERFACE_NODE;

/*
 *  * PORT_SPEED
 *   */

typedef enum STP_PORT_SPEED
{
    STP_SPEED_NONE  = 0L,
    STP_SPEED_1M    = 1,
    STP_SPEED_10M   = 10,
    STP_SPEED_100M  = 100,
    STP_SPEED_1G    = 1000,
    STP_SPEED_10G   = 10000,
    STP_SPEED_25G   = 25000,
    STP_SPEED_40G   = 40000,
    STP_SPEED_100G  = 100000,
    STP_SPEED_400G  = 400000,
    STP_SPEED_800G  = 800000,
    STP_SPEED_1T    = 1000000,
    STP_SPEED_10T   = 10000000,
    STP_SPEED_LAST
}STP_PORT_SPEED;


struct netlink_db_s;

BITMAP_T *static_portmask_init(STATIC_BITMAP_T *bmp);
int stp_intf_avl_compare(const void *user_p, const void *data_p, void *param);
void stp_intf_netlink_cb(struct netlink_db_s *if_db, uint8_t is_add, bool init_in_prog);
char * stp_intf_get_port_name(uint32_t port_id);
uint32_t stp_intf_get_port_id_by_kif_index(uint32_t kif_index);
uint32_t stp_intf_get_port_id_by_name(char *ifname);
bool stp_intf_is_port_up(int port_id);
bool stp_intf_get_mac(int port_id, MAC_ADDRESS *mac);
bool stp_intf_set_port_priority(PORT_ID port_id, uint16_t priority);
uint16_t stp_intf_get_port_priority(PORT_ID port_id);
uint32_t stp_intf_get_path_cost(PORT_ID port_id);
bool stp_intf_set_path_cost(PORT_ID port_id, uint32_t path_cost, uint8_t def_path_cost);
uint16_t stp_intf_get_inst_port_priority(PORT_ID port_id, UINT16 mstp_index);
bool stp_intf_is_inst_port_priority_set(PORT_ID port_id, UINT16 mstp_index);
void stp_intf_set_inst_port_priority(PORT_ID port_id, UINT16 mstp_index, uint16_t priority, UINT8 add);
void stp_intf_set_inst_port_pathcost(PORT_ID port_id, UINT16 mstp_index, UINT32 cost, UINT8 add);
UINT32 stp_intf_get_inst_port_pathcost(PORT_ID port_id, UINT16 mstp_index);
bool stp_intf_is_inst_port_pathcost_set(PORT_ID port_id, UINT16 mstp_index);
bool stp_intf_is_default_port_pathcost(PORT_ID port_id, UINT16 mstp_index);

#endif //__STP_INTF_H__
