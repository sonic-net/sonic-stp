/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 */

#ifndef __STP_H__
#define __STP_H__

/* #defines ----------------------------------------------------------------- */
#define STP_MESSAGE_AGE_INCREMENT 1
#define STP_INVALID_PORT 0xFFF


#define STP_DFLT_PRIORITY 32768
#define STP_MIN_PRIORITY 0
#define STP_MAX_PRIORITY 65535

#define STP_DFLT_FORWARD_DELAY 15
#define STP_MIN_FORWARD_DELAY 4
#define STP_MAX_FORWARD_DELAY 30

#define STP_DFLT_MAX_AGE 20
#define STP_MIN_MAX_AGE 6
#define STP_MAX_MAX_AGE 40

#define STP_DFLT_HELLO_TIME 2
#define STP_MIN_HELLO_TIME 1
#define STP_MAX_HELLO_TIME 10

#define STP_DFLT_HOLD_TIME 1

#define STP_DFLT_ROOT_PROTECT_TIMEOUT 30
#define STP_MIN_ROOT_PROTECT_TIMEOUT 5
#define STP_MAX_ROOT_PROTECT_TIMEOUT 600

#define STP_DFLT_PORT_PRIORITY 128
#define STP_MIN_PORT_PRIORITY 0
#define STP_MAX_PORT_PRIORITY 240

#define STP_FASTSPAN_FORWARD_DELAY 2
#define STP_FASTUPLINK_FORWARD_DELAY 1

// 802.1d path costs - for backward compatibility with BI

#define STP_LEGACY_MIN_PORT_PATH_COST 1
#define STP_LEGACY_MAX_PORT_PATH_COST 65535
#define STP_LEGACY_PORT_PATH_COST_10M 100
#define STP_LEGACY_PORT_PATH_COST_100M 19
#define STP_LEGACY_PORT_PATH_COST_1G 4
#define STP_LEGACY_PORT_PATH_COST_10G 2
#define STP_LEGACY_PORT_PATH_COST_25G 1
#define STP_LEGACY_PORT_PATH_COST_40G 1
#define STP_LEGACY_PORT_PATH_COST_100G 1
#define STP_LEGACY_PORT_PATH_COST_400G 1
#define STP_LEGACY_PORT_PATH_COST_800G 1

// 802.1t path costs - calculated as 20,000,000,000 / LinkSpeedInKbps
#define STP_MIN_PORT_PATH_COST 1
#define STP_MAX_PORT_PATH_COST 200000000
#define STP_PORT_PATH_COST_1M 20000000
#define STP_PORT_PATH_COST_10M 2000000
#define STP_PORT_PATH_COST_100M 200000
#define STP_PORT_PATH_COST_1G 20000
#define STP_PORT_PATH_COST_10G 2000
#define STP_PORT_PATH_COST_25G 800
#define STP_PORT_PATH_COST_40G 500
#define STP_PORT_PATH_COST_100G 200
#define STP_PORT_PATH_COST_400G 50
#define STP_PORT_PATH_COST_800G 25
#define STP_PORT_PATH_COST_1T 20
#define STP_PORT_PATH_COST_10T 2

#define STP_SIZEOF_CONFIG_BPDU 35
#define STP_SIZEOF_TCN_BPDU 4
#define STP_BULK_MESG_LENGTH 350
#define STP_MAX_PKT_LEN 1500 /* eth header + llc + MSTP(102) + MSTP(16 * 64 instance) */ 

#define g_stp_instances stp_global.max_instances
#define g_stp_active_instances stp_global.active_instances
#define g_stp_class_array stp_global.class_array
#define g_stp_port_array stp_global.port_array
#define g_stp_tick_id stp_global.tick_id
#define g_stp_bpdu_sync_tick_id stp_global.bpdu_sync_tick_id

#define g_stp_config_bpdu stp_global.config_bpdu
#define g_stp_tcn_bpdu stp_global.tcn_bpdu
#define g_stp_pvst_config_bpdu stp_global.pvst_config_bpdu
#define g_stp_pvst_tcn_bpdu stp_global.pvst_tcn_bpdu

#define g_fastspan_mask stp_global.fastspan_mask
#define g_fastspan_config_mask stp_global.fastspan_admin_mask
#define g_fastuplink_mask stp_global.fastuplink_admin_mask

#define g_stp_protect_mask stp_global.protect_mask
#define g_stp_protect_do_disable_mask stp_global.protect_do_disable_mask
#define g_stp_root_protect_mask stp_global.root_protect_mask
#define g_stp_protect_disabled_mask stp_global.protect_disabled_mask
#define g_stp_enable_mask stp_global.enable_mask
#define g_stp_enable_config_mask stp_global.enable_admin_mask

#define g_stp_invalid_port_name stp_global.invalid_port_name

#define STP_GET_MIN_PORT_PATH_COST ((UINT32)(g_stpd_extend_mode ? STP_MIN_PORT_PATH_COST : STP_LEGACY_MIN_PORT_PATH_COST))
#define STP_GET_MAX_PORT_PATH_COST ((UINT32)(g_stpd_extend_mode ? STP_MAX_PORT_PATH_COST : STP_LEGACY_MAX_PORT_PATH_COST))

#define GET_STP_INDEX(stp_class) ((stp_class) - g_stp_class_array)
#define GET_STP_CLASS(stp_index) (g_stp_class_array + (stp_index))
#define GET_STP_PORT_CLASS(class, port) stpdata_get_port_class(class, port)
#define GET_STP_PORT_IFNAME(port) stp_intf_get_port_name(port->port_id.number)

#define STP_TICKS_TO_SECONDS(x) ((x) >> 1)
#define STP_SECONDS_TO_TICKS(x) ((x) << 1)

#define STP_IS_FASTSPAN_ENABLED(port) is_member(g_fastspan_mask, (port))
#define STP_IS_ENABLED(port) is_member(g_stp_enable_mask, (port))

#define STP_IS_FASTUPLINK_CONFIGURED(port) is_member(g_fastuplink_mask, (port))
#define STP_IS_FASTSPAN_CONFIGURED(port) is_member(g_fastspan_config_mask, (port))
#define STP_IS_PROTECT_CONFIGURED(_port_) IS_MEMBER(stp_global.protect_mask, (_port_))
#define STP_IS_PROTECT_DO_DISABLE_CONFIGURED(_port_) IS_MEMBER(stp_global.protect_do_disable_mask, (_port_))
#define STP_IS_PROTECT_DO_DISABLED(_port_) IS_MEMBER(stp_global.protect_disabled_mask, (_port_))
#define STP_IS_ROOT_PROTECT_CONFIGURED(_p_) IS_MEMBER(stp_global.root_protect_mask, (_p_))
#define STP_IS_PROTOCOL_ENABLED(mode) (stp_global.enable == true && stp_global.proto_mode == mode)

// _port_ is a pointer to either STP_PORT_CLASS or RSTP_PORT_CLASS
#define STP_ROOT_PROTECT_TIMER_EXPIRED(_port_) \
	timer_expired(&((_port_)->root_protect_timer), STP_SECONDS_TO_TICKS(stp_global.root_protect_timeout))

#define IS_STP_PER_VLAN_FLAG_SET(_stp_port_class, _flag) (_stp_port_class->flags & _flag)
#define SET_STP_PER_VLAN_FLAG(_stp_port_class, _flag) (_stp_port_class->flags |= _flag)
#define CLR_STP_PER_VLAN_FLAG(_stp_port_class, _flag) (_stp_port_class->flags &= ~(_flag))

// debug macros
typedef struct
{
	UINT8 enabled : 1;
	UINT8 verbose : 1;
	UINT8 bpdu_rx : 1;
	UINT8 bpdu_tx : 1;
	UINT8 event : 1;
	UINT8 all_vlans : 1;
	UINT8 all_ports : 1;
	UINT8 spare : 1;
	BITMAP_T *vlan_mask;
	PORT_MASK *port_mask;
} DEBUG_STP;

typedef struct
{
    UINT8            enabled:1;
    UINT8            verbose:1;
    UINT8            bpdu_rx:1;
    UINT8            bpdu_tx:1;
    UINT8            event:1;
    UINT8            all_instance:1;
    UINT8            all_ports:1;
    UINT8           spare:1;
    BITMAP_T        *instance_mask;
    PORT_MASK       *port_mask;
} DEBUG_MSTP;

typedef struct DEBUG_GLOBAL_TAG
{
	DEBUG_STP stp;
	DEBUG_MSTP mstp;
} DEBUG_GLOBAL;
extern DEBUG_GLOBAL debugGlobal;

#define STP_DEBUG_VP(vlan_id, port_number)                                           \
	((debugGlobal.stp.enabled) &&                                                    \
	 (debugGlobal.stp.all_vlans || is_member(debugGlobal.stp.vlan_mask, vlan_id)) && \
	 (debugGlobal.stp.all_ports || is_member(debugGlobal.stp.port_mask, port_number)))

#define STP_DEBUG_BPDU_RX(vlan_id, port_number) \
	(debugGlobal.stp.bpdu_rx && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_BPDU_TX(vlan_id, port_number) \
	(debugGlobal.stp.bpdu_tx && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_EVENT(vlan_id, port_number) \
	(debugGlobal.stp.event && STP_DEBUG_VP(vlan_id, port_number))

#define STP_DEBUG_VERBOSE debugGlobal.stp.verbose

enum STP_CLASS_STATE
{
	STP_CLASS_FREE = 0,
	STP_CLASS_CONFIG = 1,
	STP_CLASS_ACTIVE = 2,
};

/*
 * NOTE: if fields are added to this enum, please also add them
 * to stp_log_msg_src_string defined in stp_debug.c
 */
enum STP_LOG_MSG_SRC
{
	STP_SRC_NOT_IMPORTANT = 0,
	STP_DISABLE_PORT,
	STP_CHANGE_PRIORITY,
	STP_MESSAGE_AGE_EXPIRY,
	STP_FWD_DLY_EXPIRY,
	STP_BPDU_RECEIVED,
	STP_MAKE_BLOCKING,
	STP_MAKE_FORWARDING,
	STP_ROOT_SELECTION
};

enum STP_KERNEL_STATE
{
	STP_KERNEL_STATE_FORWARD = 1,
	STP_KERNEL_STATE_BLOCKING
};

/* struct definitions ------------------------------------------------------- */

typedef struct
{
	BRIDGE_IDENTIFIER root_id;
	UINT32 root_path_cost;

	PORT_ID root_port;
	UINT8 max_age;
	UINT8 hello_time;

	UINT8 forward_delay;
	UINT8 bridge_max_age;
	UINT8 bridge_hello_time;
	UINT8 bridge_forward_delay;

	BRIDGE_IDENTIFIER bridge_id;

	UINT32 topology_change_count;
	UINT32 topology_change_tick; // time of last tc event

	UINT8 hold_time : 6;
	UINT8 topology_change_detected : 1;
	UINT8 topology_change : 1;
	UINT8 topology_change_time; // fwd dly + max age
#define STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT 0
#define STP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT 1
#define STP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT 2
#define STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT 3
#define STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT 4
#define STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT 5
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT 6
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT 7
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT 8
#define STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT 9
#define STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_COUNT_BIT 10
#define STP_BRIDGE_DATA_MEMBER_TOPO_CHNG_TIME_BIT 11
#define STP_BRIDGE_DATA_MEMBER_HOLD_TIME_BIT 12
	UINT32 modified_fields;
} __attribute__((aligned(4))) BRIDGE_DATA;

typedef struct
{
	VLAN_ID vlan_id; // UINT16
	UINT16 fast_aging : 1;
	UINT16 spare : 11;
	UINT16 state : 4; /* encode using enum STP_CLASS_STATE */
	BRIDGE_DATA bridge_info;

	PORT_MASK *enable_mask;
	PORT_MASK *control_mask;
	PORT_MASK *untag_mask;

	TIMER hello_timer;
	TIMER tcn_timer;
	TIMER topology_change_timer;
	UINT32 last_expiry_time;  /* for RAS to log delay events */
	UINT32 last_bpdu_rx_time; /* for RAS to log Rx delay events */
	UINT32 rx_drop_bpdu;

#define STP_CLASS_MEMBER_VLAN_BIT 0
#define STP_CLASS_MEMBER_BRIDEGINFO_BIT 1
#define STP_CLASS_MEMBER_ALL_PORT_CLASS_BIT 31
	UINT32 modified_fields;
} __attribute__((aligned(4))) STP_CLASS;

typedef struct
{
	PORT_IDENTIFIER port_id;
	UINT8 state; /* encode using enum L2_PORT_STATE */
	UINT8 topology_change_acknowledge : 1;
	UINT8 config_pending : 1;
	UINT8 change_detection_enabled : 1;
	UINT8 self_loop : 1;
	UINT8 auto_config : 1;
	UINT8 operEdge : 1;
	UINT8 kernel_state : 2; // STP_KERNEL_STATE
	UINT8 bpdu_guard_active:1;
    UINT8 unused_field:7;

	UINT32 path_cost;

	BRIDGE_IDENTIFIER designated_root;
	UINT32 designated_cost;
	BRIDGE_IDENTIFIER designated_bridge;
	PORT_IDENTIFIER designated_port;

	TIMER message_age_timer;
	TIMER forward_delay_timer;
	TIMER hold_timer;
	TIMER root_protect_timer;

	UINT32 forward_transitions;
	UINT32 rx_config_bpdu;
	UINT32 tx_config_bpdu;
	UINT32 rx_tcn_bpdu;
	UINT32 tx_tcn_bpdu;
	UINT32 rx_delayed_bpdu;
	UINT32 rx_drop_bpdu;

#define STP_CLASS_PORT_PRI_FLAG 0x0001
#define STP_CLASS_PATH_COST_FLAG 0x0002
	UINT16 flags;

#define STP_PORT_CLASS_MEMBER_PORT_ID_BIT 0
#define STP_PORT_CLASS_MEMBER_PORT_STATE_BIT 1
#define STP_PORT_CLASS_MEMBER_PATH_COST_BIT 2
#define STP_PORT_CLASS_MEMBER_DESIGN_ROOT_BIT 3
#define STP_PORT_CLASS_MEMBER_DESIGN_COST_BIT 4
#define STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT 5
#define STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT 6
#define STP_PORT_CLASS_MEMBER_FWD_TRANSITIONS_BIT 7
#define STP_PORT_CLASS_MEMBER_BPDU_SENT_BIT 8
#define STP_PORT_CLASS_MEMBER_BPDU_RECVD_BIT 9
#define STP_PORT_CLASS_MEMBER_TC_SENT_BIT 10
#define STP_PORT_CLASS_MEMBER_TC_RECVD_BIT 11
#define STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT 12
/* These are not member of STP Port class but port level information that need to be synced */
#define STP_PORT_CLASS_UPLINK_FAST_BIT 13
#define STP_PORT_CLASS_PORT_FAST_BIT 14
#define STP_PORT_CLASS_ROOT_PROTECT_BIT 15
#define STP_PORT_CLASS_BPDU_PROTECT_BIT 16
#define STP_PORT_CLASS_CLEAR_STATS_BIT 17
	UINT32 modified_fields;
} __attribute__((aligned(4))) STP_PORT_CLASS;

typedef struct
{
	UINT16 max_instances;
	UINT16 active_instances;

	STP_CLASS *class_array;
	STP_PORT_CLASS *port_array;

	STP_CONFIG_BPDU config_bpdu;
	STP_TCN_BPDU tcn_bpdu;
	PVST_CONFIG_BPDU pvst_config_bpdu;
	PVST_TCN_BPDU pvst_tcn_bpdu;

	UINT8 tick_id;
	UINT8 bpdu_sync_tick_id;
	UINT8 fast_span : 1;
	UINT8 enable : 1;
	UINT8 sstp_enabled : 1;
	UINT8 pvst_protect_do_disable : 1; // global config to disable pvst bpdu received port
	UINT8 spare1 : 4;

	PORT_MASK *enable_mask;
	PORT_MASK *enable_admin_mask;

	PORT_MASK *fastspan_mask;
	PORT_MASK *fastspan_admin_mask;

	PORT_MASK *fastuplink_admin_mask;

	PORT_MASK *protect_mask;
	PORT_MASK *protect_do_disable_mask;
	PORT_MASK *protect_disabled_mask;
	PORT_MASK *root_protect_mask;
	uint16_t root_protect_timeout;
	L2_PROTO_MODE proto_mode;

	UINT32 stp_drop_count;
	UINT32 tcn_drop_count;
	UINT32 pvst_drop_count;
	char invalid_port_name[IFNAMSIZ];
} __attribute__((aligned(4))) STP_GLOBAL;

#define INVALID_STP_PARAM ((UINT32)0xffffffff)

/*Below ENUM for RAS STP, please update "stp_ras_state_string" structure when you are adding new event here*/
typedef enum
{
	STP_RAS_BLOCKING = 1,
	STP_RAS_FORWARDING,
	STP_RAS_INFERIOR_BPDU_RCVD,
	STP_RAS_MES_AGE_TIMER_EXPIRY,
	STP_RAS_ROOT_PROTECT_TIMER_EXPIRY,
	STP_RAS_ROOT_PROTECT_VIOLATION,
	STP_RAS_ROOT_ROLE,
	STP_RAS_DESIGNATED_ROLE,
	STP_RAS_MP_RX_DELAY_EVENT,
	STP_RAS_TIMER_DELAY_EVENT,
	STP_RAS_TCM_DETECTED
} STP_RAS_EVENTS;

#endif //__STP_H__
