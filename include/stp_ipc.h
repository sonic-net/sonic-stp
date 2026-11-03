/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 */

#ifndef __STP_IPC_H__
#define __STP_IPC_H__

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
#define STP_IPC_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define STP_IPC_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* stpmgrd (sonic-swss) writes the messages below and stpd reads them as raw
 * bytes. The assertions at the end of this file fix the size of every
 * message and the offset and width of every field, so a change that moves a
 * field fails the build instead of making the receiver misread it. sonic-swss
 * cfgmgr/stpmgr.cpp checks the same numbers on the sender side. */
#define STP_IPC_ASSERT_SIZE(type, bytes) \
    STP_IPC_STATIC_ASSERT(sizeof(type) == (bytes), \
        #type " changed size; the other side of the stpmgrd/stpd socket expects " #bytes)

#define STP_IPC_ASSERT_FIELD(type, field, off, bytes) \
    STP_IPC_STATIC_ASSERT(offsetof(type, field) == (off) && sizeof(((type *)0)->field) == (bytes), \
        #type "." #field " moved or changed width; the other side reads it at offset " #off)

#define STPD_SOCK_NAME "/var/run/stpipc.sock"

#define STP_SYNC_MSTP_NAME_LEN          32

typedef enum L2_PROTO_MODE
{
    L2_NONE,
    L2_PVSTP,
    L2_MSTP
} L2_PROTO_MODE;

typedef enum LINK_TYPE{
    LINK_TYPE_AUTO,
    LINK_TYPE_PT2PT,
    LINK_TYPE_SHARED
}LINK_TYPE;

typedef enum STP_MSG_TYPE
{
    STP_INVALID_MSG,
    STP_INIT_READY,
    STP_BRIDGE_CONFIG,
    STP_VLAN_CONFIG,
    STP_VLAN_PORT_CONFIG,
    STP_PORT_CONFIG,
    STP_VLAN_MEM_CONFIG,
    STP_STPCTL_MSG,
    STP_MST_GLOBAL_CONFIG,
    STP_MST_INST_CONFIG,
    STP_MST_VLAN_PORT_LIST_CONFIG,
    STP_MST_INST_PORT_CONFIG,
    STP_MAX_MSG
} STP_MSG_TYPE;

typedef enum STP_CTL_TYPE
{
    STP_CTL_HELP,
    STP_CTL_DUMP_ALL,
    STP_CTL_DUMP_GLOBAL,
    STP_CTL_DUMP_VLAN_ALL,
    STP_CTL_DUMP_VLAN,
    STP_CTL_DUMP_INTF,
    STP_CTL_SET_LOG_LVL,
    STP_CTL_DUMP_NL_DB,
    STP_CTL_DUMP_NL_DB_INTF,
    STP_CTL_DUMP_LIBEV_STATS,
    STP_CTL_SET_DBG,
    STP_CTL_CLEAR_ALL,
    STP_CTL_CLEAR_VLAN,
    STP_CTL_CLEAR_INTF,
    STP_CTL_CLEAR_VLAN_INTF,
    STP_CTL_DUMP_MST,
    STP_CTL_DUMP_MST_PORT,
    STP_CTL_MAX
} STP_CTL_TYPE;

typedef enum LinkType {
    AUTO =              0,      // Auto
    POINT_TO_POINT =    1,      // Point-to-point
    SHARED =            2       // Shared
} LinkType;

typedef struct STP_IPC_MSG
{
    int msg_type;
    unsigned int msg_len;
    L2_PROTO_MODE proto_mode;
    char data[0];
} __attribute__((packed)) STP_IPC_MSG;

#define STP_SET_COMMAND 1
#define STP_DEL_COMMAND 0

typedef struct STP_INIT_READY_MSG
{
    uint8_t opcode; // enable/disable
    uint16_t max_stp_instances;
} __attribute__((packed)) STP_INIT_READY_MSG;

typedef struct STP_BRIDGE_CONFIG_MSG
{
    uint8_t opcode; // enable/disable
    uint8_t stp_mode;
    int rootguard_timeout;
    uint8_t base_mac_addr[6];
} __attribute__((packed)) STP_BRIDGE_CONFIG_MSG;

typedef struct PORT_ATTR
{
    char intf_name[IFNAMSIZ];
    int8_t mode;
    uint8_t enabled;
    // Add padding to align to 4 bytes
    uint16_t padding;
} __attribute__((packed)) PORT_ATTR;

typedef struct STP_VLAN_CONFIG_MSG
{
    uint8_t opcode; // enable/disable
    uint8_t newInstance;
    int vlan_id;
    int inst_id;
    int forward_delay;
    int hello_time;
    int max_age;
    int priority;
    int count;
    PORT_ATTR port_list[0];
} __attribute__((packed)) STP_VLAN_CONFIG_MSG;

typedef struct STP_VLAN_PORT_CONFIG_MSG
{
    uint8_t opcode; // enable/disable
    int vlan_id;
    char intf_name[IFNAMSIZ];
    int inst_id;
    int path_cost;
    int priority;
} __attribute__((packed)) STP_VLAN_PORT_CONFIG_MSG;

typedef struct VLAN_ATTR
{
    int inst_id;
    int vlan_id;
    int8_t mode;
    uint8_t padding[3];  // Explicit padding for alignment
} __attribute__((packed)) VLAN_ATTR;

typedef struct STP_PORT_CONFIG_MSG
{
    uint8_t opcode; // enable/disable
    char intf_name[IFNAMSIZ];
    uint8_t enabled;
    uint8_t root_guard;
    uint8_t loop_guard;
    uint8_t bpdu_guard;
    uint8_t bpdu_guard_do_disable;
    uint8_t portfast;
    uint8_t uplink_fast;
    uint8_t edge_port;
    LinkType    link_type;          // MSTP only
    int path_cost;
    int priority;
    int count;
    VLAN_ATTR vlan_list[0];
} __attribute__((packed)) STP_PORT_CONFIG_MSG;

typedef struct STP_VLAN_MEM_CONFIG_MSG
{
    uint8_t opcode; // enable/disable
    uint8_t enabled;
    int8_t mode;
    uint8_t padding;  // Explicit padding for alignment
    int vlan_id;
    int inst_id;
    char intf_name[IFNAMSIZ];
    int path_cost;
    int priority;
} __attribute__((packed)) STP_VLAN_MEM_CONFIG_MSG;

typedef struct STP_MST_GLOBAL_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    uint32_t    revision_number;
    char        name[STP_SYNC_MSTP_NAME_LEN];
    uint8_t     forward_delay;
    uint8_t     hello_time;
    uint8_t     max_age;
    uint8_t     max_hops;
}__attribute__((packed)) STP_MST_GLOBAL_CONFIG_MSG;


typedef struct VLAN_LIST{
    uint16_t    vlan_id;
}__attribute__((packed)) VLAN_LIST;

typedef struct STP_MST_INST_CONFIG_MSG
{
    uint8_t     opcode; // enable/disable
    uint16_t    mst_id;
    int         priority;
    uint16_t    vlan_count;
    VLAN_LIST   vlan_list[0];
} __attribute__((packed))  STP_MST_INST_CONFIG_MSG;

typedef struct STP_MST_INSTANCE_CONFIG_MSG {
    uint8_t    mst_count;
    STP_MST_INST_CONFIG_MSG  mst_list[0];
} __attribute__((packed)) STP_MST_INSTANCE_CONFIG_MSG;

typedef struct STP_MST_INST_PORT_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    char        intf_name[IFNAMSIZ];
    uint16_t    mst_id;
    int         path_cost;
    int         priority;
}__attribute__((packed)) STP_MST_INST_PORT_CONFIG_MSG;

typedef struct PORT_LIST
{
    char        intf_name[IFNAMSIZ];
    int8_t      tagging_mode;
}__attribute__((packed)) PORT_LIST;

typedef struct STP_MST_VLAN_PORT_MAP
{
    uint16_t    vlan_id;
    uint16_t    port_count;
    int8_t      stp_mode;
    uint8_t     add;    
    PORT_LIST   port_list[0];
}__attribute__((packed)) STP_MST_VLAN_PORT_MAP;

typedef struct MSTP_INST_VLAN_LIST
{
    uint8_t     opcode; // enable/disable
    uint16_t    mst_id;
    int         priority;
    uint16_t    vlan_count;
    VLAN_LIST   vlan_list[4094];
}__attribute__((packed)) MSTP_INST_VLAN_LIST;

typedef struct STP_DEBUG_OPT
{
#define STPCTL_DBG_SET_ENABLED 0x0001
#define STPCTL_DBG_SET_VERBOSE 0x0002
#define STPCTL_DBG_SET_BPDU_RX 0x0004
#define STPCTL_DBG_SET_BPDU_TX 0x0008
#define STPCTL_DBG_SET_EVENT 0x0010
#define STPCTL_DBG_SET_PORT 0x0020
#define STPCTL_DBG_SET_VLAN 0x0040
#define STPCTL_DBG_SHOW 0x0080
    uint16_t flags;

    uint8_t enabled : 1;
    uint8_t verbose : 1;
    uint8_t bpdu_rx : 1;
    uint8_t bpdu_tx : 1;
    uint8_t event : 1;
    uint8_t port : 1;
    uint8_t vlan : 1;
    uint8_t mst : 1;
} __attribute__((packed))  STP_DEBUG_OPT;

typedef struct STP_CTL_MSG
{
    int cmd_type;
    int vlan_id;
    char intf_name[IFNAMSIZ];
    int level;
    STP_DEBUG_OPT dbg;
    int mst_id;
} __attribute__((packed)) STP_CTL_MSG;

/* Wire layout of the stpmgrd -> stpd messages. Measured from this header,
 * not counted by hand. */
/* STP_IPC_MSG */
STP_IPC_ASSERT_SIZE(STP_IPC_MSG, 12);
STP_IPC_ASSERT_FIELD(STP_IPC_MSG, msg_type, 0, 4);
STP_IPC_ASSERT_FIELD(STP_IPC_MSG, msg_len, 4, 4);
STP_IPC_ASSERT_FIELD(STP_IPC_MSG, proto_mode, 8, 4);
STP_IPC_ASSERT_FIELD(STP_IPC_MSG, data, 12, 0);

/* STP_INIT_READY_MSG */
STP_IPC_ASSERT_SIZE(STP_INIT_READY_MSG, 3);
STP_IPC_ASSERT_FIELD(STP_INIT_READY_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_INIT_READY_MSG, max_stp_instances, 1, 2);

/* STP_BRIDGE_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_BRIDGE_CONFIG_MSG, 12);
STP_IPC_ASSERT_FIELD(STP_BRIDGE_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_BRIDGE_CONFIG_MSG, stp_mode, 1, 1);
STP_IPC_ASSERT_FIELD(STP_BRIDGE_CONFIG_MSG, rootguard_timeout, 2, 4);
STP_IPC_ASSERT_FIELD(STP_BRIDGE_CONFIG_MSG, base_mac_addr, 6, 6);

/* PORT_ATTR */
STP_IPC_ASSERT_SIZE(PORT_ATTR, 20);
STP_IPC_ASSERT_FIELD(PORT_ATTR, intf_name, 0, 16);
STP_IPC_ASSERT_FIELD(PORT_ATTR, mode, 16, 1);
STP_IPC_ASSERT_FIELD(PORT_ATTR, enabled, 17, 1);

/* STP_VLAN_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_VLAN_CONFIG_MSG, 30);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, newInstance, 1, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, vlan_id, 2, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, inst_id, 6, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, forward_delay, 10, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, hello_time, 14, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, max_age, 18, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, priority, 22, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, count, 26, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_CONFIG_MSG, port_list, 30, 0);

/* STP_VLAN_PORT_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_VLAN_PORT_CONFIG_MSG, 33);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, vlan_id, 1, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, intf_name, 5, 16);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, inst_id, 21, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, path_cost, 25, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_PORT_CONFIG_MSG, priority, 29, 4);

/* VLAN_ATTR */
STP_IPC_ASSERT_SIZE(VLAN_ATTR, 12);
STP_IPC_ASSERT_FIELD(VLAN_ATTR, inst_id, 0, 4);
STP_IPC_ASSERT_FIELD(VLAN_ATTR, vlan_id, 4, 4);
STP_IPC_ASSERT_FIELD(VLAN_ATTR, mode, 8, 1);

/* STP_PORT_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_PORT_CONFIG_MSG, 41);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, intf_name, 1, 16);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, enabled, 17, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, root_guard, 18, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, loop_guard, 19, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, bpdu_guard, 20, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, bpdu_guard_do_disable, 21, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, portfast, 22, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, uplink_fast, 23, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, edge_port, 24, 1);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, link_type, 25, 4);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, path_cost, 29, 4);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, priority, 33, 4);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, count, 37, 4);
STP_IPC_ASSERT_FIELD(STP_PORT_CONFIG_MSG, vlan_list, 41, 0);

/* STP_VLAN_MEM_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_VLAN_MEM_CONFIG_MSG, 36);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, enabled, 1, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, mode, 2, 1);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, vlan_id, 4, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, inst_id, 8, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, intf_name, 12, 16);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, path_cost, 28, 4);
STP_IPC_ASSERT_FIELD(STP_VLAN_MEM_CONFIG_MSG, priority, 32, 4);

/* STP_MST_GLOBAL_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_MST_GLOBAL_CONFIG_MSG, 41);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, revision_number, 1, 4);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, name, 5, 32);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, forward_delay, 37, 1);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, hello_time, 38, 1);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, max_age, 39, 1);
STP_IPC_ASSERT_FIELD(STP_MST_GLOBAL_CONFIG_MSG, max_hops, 40, 1);

/* VLAN_LIST */
STP_IPC_ASSERT_SIZE(VLAN_LIST, 2);
STP_IPC_ASSERT_FIELD(VLAN_LIST, vlan_id, 0, 2);

/* STP_MST_INST_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_MST_INST_CONFIG_MSG, 9);
STP_IPC_ASSERT_FIELD(STP_MST_INST_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_MST_INST_CONFIG_MSG, mst_id, 1, 2);
STP_IPC_ASSERT_FIELD(STP_MST_INST_CONFIG_MSG, priority, 3, 4);
STP_IPC_ASSERT_FIELD(STP_MST_INST_CONFIG_MSG, vlan_count, 7, 2);
STP_IPC_ASSERT_FIELD(STP_MST_INST_CONFIG_MSG, vlan_list, 9, 0);

/* STP_MST_INST_PORT_CONFIG_MSG */
STP_IPC_ASSERT_SIZE(STP_MST_INST_PORT_CONFIG_MSG, 27);
STP_IPC_ASSERT_FIELD(STP_MST_INST_PORT_CONFIG_MSG, opcode, 0, 1);
STP_IPC_ASSERT_FIELD(STP_MST_INST_PORT_CONFIG_MSG, intf_name, 1, 16);
STP_IPC_ASSERT_FIELD(STP_MST_INST_PORT_CONFIG_MSG, mst_id, 17, 2);
STP_IPC_ASSERT_FIELD(STP_MST_INST_PORT_CONFIG_MSG, path_cost, 19, 4);
STP_IPC_ASSERT_FIELD(STP_MST_INST_PORT_CONFIG_MSG, priority, 23, 4);

/* PORT_LIST */
STP_IPC_ASSERT_SIZE(PORT_LIST, 17);
STP_IPC_ASSERT_FIELD(PORT_LIST, intf_name, 0, 16);
STP_IPC_ASSERT_FIELD(PORT_LIST, tagging_mode, 16, 1);

/* STP_MST_VLAN_PORT_MAP */
STP_IPC_ASSERT_SIZE(STP_MST_VLAN_PORT_MAP, 6);
STP_IPC_ASSERT_FIELD(STP_MST_VLAN_PORT_MAP, vlan_id, 0, 2);
STP_IPC_ASSERT_FIELD(STP_MST_VLAN_PORT_MAP, port_count, 2, 2);
STP_IPC_ASSERT_FIELD(STP_MST_VLAN_PORT_MAP, stp_mode, 4, 1);
STP_IPC_ASSERT_FIELD(STP_MST_VLAN_PORT_MAP, add, 5, 1);
STP_IPC_ASSERT_FIELD(STP_MST_VLAN_PORT_MAP, port_list, 6, 0);

#endif
