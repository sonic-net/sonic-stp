/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#ifndef __STP_IPC_H__
#define __STP_IPC_H__

#define STPD_SOCK_NAME "/var/run/stpipc.sock"

typedef enum L2_PROTO_MODE{
    L2_NONE,
    L2_PVSTP,
}L2_PROTO_MODE;

typedef enum STP_MSG_TYPE {
    STP_INVALID_MSG,
    STP_INIT_READY,
    STP_BRIDGE_CONFIG,
    STP_VLAN_CONFIG,
    STP_VLAN_INTF_CONFIG,
    STP_INTF_CONFIG,
    STP_VLAN_MEM_CONFIG,
    STP_STPCTL_MSG,
    STP_MAX_MSG
}STP_MSG_TYPE;

typedef enum STP_CTL_TYPE {
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
    STP_CTL_MAX
}STP_CTL_TYPE;

typedef struct STP_IPC_MSG {
    int             msg_type;
    unsigned int    msg_len;
    char            data[0];
}STP_IPC_MSG;

#define STP_SET_COMMAND      1
#define STP_DEL_COMMAND      0

typedef struct STP_INIT_READY_MSG {
    uint8_t     opcode; // enable/disable
    uint16_t    max_stp_instances;
}__attribute__ ((packed))STP_INIT_READY_MSG;

typedef struct STP_BRIDGE_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    uint8_t     stp_mode;
    int         rootguard_timeout;
    uint8_t     base_mac_addr[6];
}__attribute__ ((packed))STP_BRIDGE_CONFIG_MSG;

typedef struct PORT_ATTR {
    char       intf_name[IFNAMSIZ];
    int8_t     mode;
    uint8_t    enabled;
}PORT_ATTR;

typedef struct STP_VLAN_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    uint8_t     newInstance;
    int         vlan_id;
    int         inst_id;
    int         forward_delay;
    int         hello_time;
    int         max_age;
    int         priority;
    int         count;
    PORT_ATTR   port_list[0];
}__attribute__ ((packed))STP_VLAN_CONFIG_MSG;

typedef struct STP_VLAN_INTF_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    int         vlan_id;
    char        intf_name[IFNAMSIZ];
    int         inst_id;
    int         path_cost;
    int         priority;
}__attribute__ ((packed))STP_VLAN_INTF_CONFIG_MSG;

typedef struct VLAN_ATTR {
    int         inst_id;
    int         vlan_id;
    int8_t      mode;
}VLAN_ATTR;

typedef struct STP_INTF_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    char        intf_name[IFNAMSIZ];
    uint8_t     enabled;
    uint8_t     root_guard;
    uint8_t     bpdu_guard;
    uint8_t     bpdu_guard_do_disable;
    uint8_t     portfast;
    uint8_t     uplink_fast;
    int         path_cost;
    int         priority;
    int         count;
    VLAN_ATTR   vlan_list[0];
}__attribute__ ((packed))STP_INTF_CONFIG_MSG;

typedef struct STP_VLAN_MEM_CONFIG_MSG {
    uint8_t     opcode; // enable/disable
    int         vlan_id;
    int         inst_id;
    char        intf_name[IFNAMSIZ];
    uint8_t     enabled;
    int8_t      mode;
    int         path_cost;
    int         priority;
}__attribute__ ((packed))STP_VLAN_MEM_CONFIG_MSG;

typedef struct STP_DEBUG_OPT {
#define STPCTL_DBG_SET_ENABLED  0x0001
#define STPCTL_DBG_SET_VERBOSE  0x0002
#define STPCTL_DBG_SET_BPDU_RX  0x0004
#define STPCTL_DBG_SET_BPDU_TX  0x0008
#define STPCTL_DBG_SET_EVENT    0x0010
#define STPCTL_DBG_SET_PORT     0x0020
#define STPCTL_DBG_SET_VLAN     0x0040
#define STPCTL_DBG_SHOW         0x0080
    uint16_t           flags;

    uint8_t            enabled:1;
    uint8_t            verbose:1;
    uint8_t            bpdu_rx:1;
    uint8_t            bpdu_tx:1;
    uint8_t            event:1;
    uint8_t            port:1;
    uint8_t            vlan:1;
    uint8_t            spare:1;
} STP_DEBUG_OPT;

typedef struct STP_CTL_MSG {
    int             cmd_type;
    int             vlan_id;
    char            intf_name[IFNAMSIZ];
    int             level;
    STP_DEBUG_OPT   dbg;
}__attribute__ ((packed))STP_CTL_MSG;

#endif
