/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
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

#define PORT_TO_MODULE_PORT BMP_GET_ARR_POS
#define PORT_TO_MODULE_ID   BMP_GET_ARR_ID

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

#define L2_MAX_PROTOCOL_INSTANCES           1024
#define L2_PROTO_INDEX_MASKS                (L2_MAX_PROTOCOL_INSTANCES >> 5)
#define L2_PROTO_INDEX_INVALID              0xFFFF


typedef struct
{
    char            ifname[IFNAMSIZ+1];
    uint32_t        kif_index;          // kernel if index
    uint32_t        port_id;            // local port if index
    char            mac[L2_ETH_ADD_LEN];
    uint32_t        speed;
    uint8_t         oper_state;
    uint8_t         is_valid;           /* PO with no member ports are invalid  */
    uint16_t        member_port_count;  /* No of member ports in case of LAG */
    uint32_t        master_ifindex;     /*Applicable only for Member port of LAG */
    uint16_t        priority;
    uint32_t        path_cost;
    int             sock;               //socket created for this kif_index
    struct event *  ev;                 //libevent to handle this sock
}INTERFACE_NODE;

typedef struct
{
    UINT32 m[L2_PROTO_INDEX_MASKS];
} L2_PROTO_INSTANCE_MASK;


#define MAX_MGMT_SLOT           2
#define MAX_SLAVE_SLOT          2
#define MAX_SLAVE_PORT          0
#define FIRST_MGMT_PORT         (MAX_SLAVE_PORT)
#define FIRST_MGMT_PORT         (MAX_SLAVE_PORT)
#define LAST_MGMT_PORT          (FIRST_MGMT_PORT+MAX_MGMT_PORT-1)
#define FIRST_MGMT_SLOT         MAX_SLAVE_SLOT
#define LAST_MGMT_SLOT          (FIRST_MGMT_SLOT+MAX_MGMT_SLOT-1)
#define IS_MGMT_SLOT(slot)      (((slot)>=FIRST_MGMT_SLOT)&&((slot)<=LAST_MGMT_SLOT))
#define IS_MGMT_PORT(port)      (((port)>=FIRST_MGMT_PORT)&&((port)<=LAST_MGMT_PORT))

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
    STP_SPEED_1T    = 1000000,
    STP_SPEED_10T   = 10000000,
    STP_SPEED_LAST
}STP_PORT_SPEED;


struct netlink_db_s;

int stp_intf_avl_compare(const void *user_p, const void *data_p, void *param);
void stp_intf_netlink_cb(struct netlink_db_s *if_db, uint8_t is_add, bool init_in_prog);
char * stp_intf_get_port_name(uint32_t port_id);
uint32_t stp_intf_get_port_id_by_kif_index(uint32_t kif_index);
uint32_t stp_intf_get_port_id_by_name(char *ifname);
bool stp_intf_is_port_up(int port_id);
char * stp_intf_get_port_name(uint32_t port_id);
bool stp_intf_get_mac(int port_id, MAC_ADDRESS *mac);
bool stp_intf_set_port_priority(PORT_ID port_id, uint16_t priority);
uint16_t stp_intf_get_port_priority(PORT_ID port_id);
bool stp_intf_set_path_cost(PORT_ID port_id, uint32_t path_cost);
uint32_t stp_intf_get_path_cost(PORT_ID port_id);
#endif //__STP_INTF_H__
