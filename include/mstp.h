/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#ifndef __MSTP_H__
#define __MSTP_H__

/*****************************************************************************/
/* definitions                                                               */
/*****************************************************************************/

#define MSTP_MIN_REVISION_LEVEL             0
#define MSTP_MAX_REVISION_LEVEL             65535
#define MSTP_DFLT_REVISION_LEVEL            MSTP_MIN_REVISION_LEVEL

#define MSTP_STP_COMPATIBILITY_MODE         STP_VERSION_ID
#define MSTP_RSTP_COMPATIBILITY_MODE        RSTP_VERSION_ID
#define MSTP_DFLT_COMPATIBILITY_MODE        MSTP_VERSION_ID

#define MSTP_DFLT_TXHOLDCOUNT               6

#define MSTP_MIN_MAX_HOPS                   1
#define MSTP_MAX_MAX_HOPS                   40
#define MSTP_DFLT_MAX_HOPS                  20

#define MSTP_ENCODE_PRIORITY(p)             ((p) >> 12)
#define MSTP_DECODE_PRIORITY(p)             ((p) << 12)
#define MSTP_MIN_PRIORITY                   0
#define MSTP_MAX_PRIORITY                   61440
#define MSTP_DFLT_PRIORITY                  32768

#define MSTP_ENCODE_PORT_PRIORITY(p)        ((p) >> 4)
#define MSTP_DECODE_PORT_PRIORITY(p)        ((p) << 4)
#define MSTP_MIN_PORT_PRIORITY              0
#define MSTP_MAX_PORT_PRIORITY              240
#define MSTP_DFLT_PORT_PRIORITY             128

#define MSTP_MIN_HELLO_TIME                 STP_MIN_HELLO_TIME
#define MSTP_MAX_HELLO_TIME                 STP_MAX_HELLO_TIME
#define MSTP_DFLT_HELLO_TIME                STP_DFLT_HELLO_TIME

#define MSTP_DFLT_MIGRATE_TIME				3

#define MSTP_MIN_FORWARD_DELAY              STP_MIN_FORWARD_DELAY
#define MSTP_MAX_FORWARD_DELAY              STP_MAX_FORWARD_DELAY
#define MSTP_DFLT_FORWARD_DELAY             STP_DFLT_FORWARD_DELAY

#define MSTP_MIN_MAX_AGE                    STP_MIN_MAX_AGE
#define MSTP_MAX_MAX_AGE                    STP_MAX_MAX_AGE
#define MSTP_DFLT_MAX_AGE                   STP_DFLT_MAX_AGE

#define MSTP_CONFIG_DIGEST_SIGNATURE_KEY    { 0x13,0xAC,0x06,0xA6,0x2E,0x47,0xFD,0x51,0xF9,0x5D,0x2B,0xA2,0x43,0xCD,0x03,0x46 }

/*****************************************************************************/
/* macros                                                                    */
/*****************************************************************************/

// get mstp index from the input mstid
#define MSTP_GET_INSTANCE_INDEX(_mstp_bridge_, _mstid_) \
	((_mstp_bridge_)->index_table.index[(_mstid_)])

// get mstid from the input vlan id
#define MSTP_GET_MSTID(_mstp_bridge_, _vlan_) \
	ntohs((_mstp_bridge_)->mstid_table.mstid[_vlan_])

// set the mstp index for the input mstid
#define MSTP_SET_INSTANCE_INDEX(_mstp_bridge_, _mstid_, _index_) \
	(_mstp_bridge_)->index_table.index[(_mstid_)] = (_index_)

// set the mstid for the input vlan id
#define MSTP_CLASS_SET_MSTID(_mstp_bridge_, _vlan_, _mstid_) \
	((_mstp_bridge_)->mstid_table.mstid[(_vlan_)]) = ntohs(_mstid_)

// check if the input index is common spanning tree instance index
#define MSTP_IS_CIST_INDEX(_index_) ((_index_) == MSTP_INDEX_CIST)

// check if the input index is a valid mstp instance index
#define MSTP_IS_VALID_MSTI_INDEX(_index_) ((_index_) <= MSTP_INDEX_MAX)

// check if the input mstid is cist mstid
#define MSTP_IS_CIST_MSTID(_mstid_) ((_mstid_) == MSTP_MSTID_CIST)

// check if the input mstid is in the valid range
#define MSTP_IS_VALID_MSTID(_mstid_) ((_mstid_) >= MSTP_MSTID_MIN && (_mstid_) <= MSTP_MSTID_MAX)

// gets the common spanning tree instance bridge information
#define MSTP_GET_CIST_BRIDGE(_mstp_bridge_) &(_mstp_bridge_->cist)

// gets the common spanning tree instance bridge information
#define MSTP_GET_MSTI_BRIDGE(_mstp_bridge_, _index_) \
	(MSTP_IS_VALID_MSTI_INDEX(_index_) ? (_mstp_bridge_)->msti[(_index_)] : NULL)

// get the mstid from the input msti bridge
#define MSTP_GET_MSTID_FROM_MSTI(_msti_bridge_) ((_msti_bridge_)->co.mstid)

// get the common spanning tree instance port
#define MSTP_GET_CIST_PORT(_mstp_port_) (&(_mstp_port_->cist))

// get the mst instance spanning tree port from index
#define MSTP_GET_MSTI_PORT(_mstp_port_, _index_) \
	(MSTP_IS_VALID_MSTI_INDEX(_index_) ? ((_mstp_port_)->msti[_index_]) : NULL)

// get the number of msti config messages in the bpdu based on the input
// length (this is the length of the bpdu)
#define MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(_length_) \
	(((_length_) <= MSTP_BPDU_BASE_V3_LENGTH) ? 0 : (((_length_)-MSTP_BPDU_BASE_V3_LENGTH)/sizeof(MSTI_CONFIG_MESSAGE)))

// check that the input configuration identifiers are equal
#define MSTP_CONFIG_ID_IS_EQUAL(_cnfg1_, _cnfg2_) \
	(memcmp(&(_cnfg1_), &(_cnfg2_), sizeof(MSTP_CONFIG_IDENTIFIER)) == 0)
	
// debugging macros
#define MSTP_DEBUG_MST_PORT(_mstp_index_, port_number) \
    ((debugGlobal.mstp.enabled) && \
        (debugGlobal.mstp.all_instance || is_member(debugGlobal.mstp.instance_mask,  (_mstp_index_))) && \
        (debugGlobal.mstp.all_ports || is_member(debugGlobal.mstp.port_mask, port_number)))

#define MSTP_DEBUG_PORT(_port_) \
	 ((debugGlobal.mstp.all_ports || IS_MEMBER(debugGlobal.mstp.port_mask, (_port_))))

#define MSTP_DEBUG_BPDU_TX(_mstp_index_, _port_) \
	(MSTP_DEBUG_MST_PORT(_mstp_index_, _port_) && (debugGlobal.mstp.bpdu_tx))

#define MSTP_DEBUG_BPDU_RX(_mstp_index_, _port_) \
	(MSTP_DEBUG_MST_PORT(_mstp_index_, _port_) && (debugGlobal.mstp.bpdu_rx))

#define MSTP_DEBUG_EVENT(_mstp_index_, _port_) \
	(MSTP_DEBUG_MST_PORT(_mstp_index_, _port_) && (debugGlobal.mstp.event))

#define MSTP_DEBUG_PORT_EVENT(_port_) \
	((MSTP_DEBUG_PORT(_port_) && (debugGlobal.mstp.event) && (debugGlobal.mstp.enabled)))

#define MSTP_DEBUG() debugGlobal.mstp.enabled
	
// debugging output macros
#define MSTP_PORT_ROLE_STRING(_role_) \
	(((_role_) <= MSTP_ROLE_DISABLED) ? mstp_role_string[(_role_)] : mstp_err_string)

#define MSTP_RCVDINFO_STRING(_rcvdinfo_) \
	(((_rcvdinfo_) <= MSTP_OTHER_INFO) ? mstp_rcvdinfo_string[(_rcvdinfo_)] : mstp_err_string)

#define MSTP_INFOIS_STRING(_infoIs_) \
	(((_infoIs_) <= MSTP_INFOIS_DISABLED) ? mstp_infoIs_string[(_infoIs_)] : mstp_err_string)

#define MSTP_PIM_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PIM_RECEIVE) ? mstp_pim_state_string[(_s_)] : mstp_err_string)

#define MSTP_PRS_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PRS_ROLE_SELECTION) ? mstp_prs_state_string[(_s_)] : mstp_err_string)

#define MSTP_PRT_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PRT_ACTIVE_PORT) ? mstp_prt_state_string[(_s_)] : mstp_err_string)

#define MSTP_PST_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PST_FORWARDING) ? mstp_pst_state_string[(_s_)] : mstp_err_string)

#define MSTP_TCM_STATE_STRING(_s_) \
	(((_s_) <= MSTP_TCM_ACTIVE) ? mstp_tcm_state_string[(_s_)] : mstp_err_string)

#define MSTP_PPM_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PPM_SENSING) ? mstp_ppm_state_string[(_s_)] : mstp_err_string)

#define MSTP_PTX_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PTX_TRANSMIT_RSTP) ? mstp_ptx_state_string[(_s_)] : mstp_err_string)

#define MSTP_PRX_STATE_STRING(_s_) \
	(((_s_) <= MSTP_PRX_RECEIVE) ? mstp_prx_state_string[(_s_)] : mstp_err_string)

#define MSTP_STATE_STRING(_s_, _p_) (((_s_) == BLOCKING) ? "DISCARDING" : L2_STATE_STRING(_s_, _p_))

/*****************************************************************************/
/* typedefs and enum declarations                                            */
/*****************************************************************************/

typedef UINT16 	MSTP_INDEX;
typedef UINT16 	MSTP_MSTID;

typedef enum
{
	MSTP_ROLE_MASTER = 0,
	MSTP_ROLE_ALTERNATE,
	MSTP_ROLE_ROOT,
	MSTP_ROLE_DESIGNATED,
	MSTP_ROLE_BACKUP4,	
	MSTP_ROLE_DISABLED
} MSTP_ROLE;

typedef enum
{
	MSTP_UNKNOWN_INFO = 0,
	MSTP_SUPERIOR_DESIGNATED_INFO,
	MSTP_REPEATED_DESIGNATED_INFO,
	MSTP_ROOT_INFO,
	MSTP_INFERIOR_DESIGNATED_INFO,
	MSTP_OTHER_INFO
} MSTP_RCVD_INFO;

typedef enum
{
	MSTP_INFOIS_UNKNOWN = 0,
	MSTP_INFOIS_RECEIVED ,
	MSTP_INFOIS_MINE,
	MSTP_INFOIS_AGED,
	MSTP_INFOIS_DISABLED
} MSTP_INFOIS;

typedef enum
{
	MSTP_PRX_UNKNOWN = 0,
	MSTP_PRX_DISCARD,
	MSTP_PRX_RECEIVE
} MSTP_PRX_STATE;

typedef enum
{
	MSTP_PPM_UNKNOWN = 0,
	MSTP_PPM_CHECKING_RSTP,
	MSTP_PPM_SELECTING_STP,
	MSTP_PPM_SENSING
} MSTP_PPM_STATE;

typedef enum
{
	MSTP_PTX_UNKNOWN = 0,
	MSTP_PTX_TRANSMIT_INIT,
	MSTP_PTX_IDLE,
	MSTP_PTX_TRANSMIT_PERIODIC,
	MSTP_PTX_TRANSMIT_CONFIG,
	MSTP_PTX_TRANSMIT_TCN,
	MSTP_PTX_TRANSMIT_RSTP
} MSTP_PTX_STATE;

typedef enum
{
	MSTP_PIM_UNKNOWN = 0,
	MSTP_PIM_DISABLED,
	MSTP_PIM_AGED,
	MSTP_PIM_UPDATE,
	MSTP_PIM_SUPERIOR_DESIGNATED,
	MSTP_PIM_REPEATED_DESIGNATED,
	MSTP_PIM_INFERIOR_DESIGNATED,
	MSTP_PIM_ROOT,
	MSTP_PIM_OTHER,
	MSTP_PIM_CURRENT,
	MSTP_PIM_RECEIVE
} MSTP_PIM_STATE;

typedef enum
{
	MSTP_PRS_UNKNOWN = 0,
	MSTP_PRS_INIT_BRIDGE,
	MSTP_PRS_ROLE_SELECTION 
} MSTP_PRS_STATE;

typedef enum
{
	MSTP_PRT_UNKNOWN = 0,

	MSTP_PRT_INIT_PORT,
	MSTP_PRT_BLOCK_PORT,
	MSTP_PRT_BACKUP_PORT,
	MSTP_PRT_BLOCKED_PORT,

	MSTP_PRT_PROPOSED,
	MSTP_PRT_PROPOSING,
	MSTP_PRT_AGREES,
	MSTP_PRT_SYNCED,
	MSTP_PRT_REROOT,
	MSTP_PRT_FORWARD,
	MSTP_PRT_LEARN,
	MSTP_PRT_DISCARD,
	MSTP_PRT_REROOTED,
	MSTP_PRT_ROOT,
	MSTP_PRT_ACTIVE_PORT
} MSTP_PRT_STATE;

typedef enum
{
	MSTP_PST_UNKNOWN = 0,
	MSTP_PST_DISCARDING,
	MSTP_PST_LEARNING,
	MSTP_PST_FORWARDING
} MSTP_PST_STATE;

typedef enum
{
	MSTP_TCM_UNKNOWN = 0,
	MSTP_TCM_INACTIVE,	
	MSTP_TCM_LEARNING,
	MSTP_TCM_DETECTED,
	MSTP_TCM_NOTIFIED_TCN,
	MSTP_TCM_NOTIFIED_TC,
	MSTP_TCM_PROPAGATING,
	MSTP_TCM_ACKNOWLEDGED,
	MSTP_TCM_ACTIVE
} MSTP_TCM_STATE;

/* Vlan port DBs structure */
typedef struct
{
    BITMAP_T    *port_mask;
}MSTP_VLAN_PORT_DB;

typedef struct
{
    BITMAP_T    *vlan_mask;
    VLAN_ID     untag_vlan;
}MSTP_PORT_VLAN_DB;

/*****************************************************************************/
/* bpdu rx/tx statistics structure                                           */
/*****************************************************************************/
typedef struct
{
	UINT32          mstp_bpdu;
	UINT32          rstp_bpdu;
	UINT32          config_bpdu;
	UINT32          tcn_bpdu;
    UINT8           update_flag;
} MSTP_BPDU_STATS;

typedef struct
{
    UINT32          tx_bpdu;
    UINT32          rx_bpdu;
    UINT8           update_flag;
} MSTP_STATS;

/*****************************************************************************/
/* structure definitions common to msti and cists                            */
/*****************************************************************************/

typedef struct
{
	// active indicates that this instance is started
	UINT16                  active:1;

	// reselect is set to force execution of port role selection state machine
	UINT16                  reselect:1;

	// mstid associated with this bridge - 0 for cist, 1-4094 for msti
	UINT16                  mstid:12;

	// spare bits
	UINT16                  spare:2;

	// root port for the bridge
	PORT_ID                 rootPortId;

	// vlans that are members of this instance
	VLAN_MASK               vlanmask;

	// ports that are members of this instance
	PORT_MASK               *portmask;

	// bridge
	MSTP_BRIDGE_IDENTIFIER  bridgeIdentifier;

	// port role selection state machine state
	MSTP_PRS_STATE          prsState;
    
    PORT_MASK				*untag_mask;

} MSTP_COMMON_BRIDGE;

typedef struct
{
	// port information for this interface
	MSTP_PORT_IDENTIFIER    portId;

	// timers
	UINT16                  fdWhile;
	UINT16                  rrWhile;
	UINT16                  rbWhile;
	UINT16                  tcWhile;
	UINT16                  rcvdInfoWhile;

	// variables
	UINT32                  agree:1;
	UINT32                  agreed:1;
	UINT32                  autoConfigPathCost:1;
	UINT32                  changedMaster:1;
	UINT32                  fdbFlush:1;
	UINT32                  forward:1;
	UINT32                  forwarding:1;
	UINT32                  learn:1;
	UINT32                  learning:1;
	UINT32                  proposed:1;
	UINT32                  proposing:1;
	UINT32                  rcvdMsg:1;
	UINT32                  rcvdTc:1;
	UINT32                  reRoot:1;
	UINT32                  reselect:1;
	UINT32                  selected:1;
	UINT32                  sync:1;
	UINT32                  synced:1;
	UINT32                  tcProp:1;
	UINT32                  updtInfo:1;
	UINT32                  disputed:1;
	UINT32                  spareBits:11;

	MSTP_INFOIS             infoIs;
	MSTP_RCVD_INFO          rcvdInfo;
	MSTP_ROLE               role;
	MSTP_ROLE               selectedRole;

	// port information state machine state
	MSTP_PIM_STATE          pimState;

	// port role transitions state machine state
	MSTP_PRT_STATE          prtState;

	// port state transition state machine state
	MSTP_PST_STATE          pstState;

	// topology change state machine state
	MSTP_TCM_STATE          tcmState;

	// l2 port state
	enum L2_PORT_STATE      state;


	// number of transitions to forwarding
	UINT32                  forwardTransitions;

	// port path cost associated with this port
	UINT32                  intPortPathCost;

	// bpdu receive and send statistics for the instance port
	MSTP_STATS              stats;
	
#define MSTP_PORT_MEMBER_PORT_STATE_BIT            0
#define MSTP_PORT_MEMBER_PORT_ROLE_BIT             1
#define MSTP_PORT_MEMBER_FWD_TRANSITIONS_BIT       2
#define MSTP_PORT_MEMBER_BPDU_SENT_BIT             3
#define MSTP_PORT_MEMBER_BPDU_RECVD_BIT            4
#define MSTP_PORT_MEMBER_CLEAR_STATS_BIT           5
#define MSTP_PORT_MEMBER_REM_TIME_BIT              6
    UINT32                  modified_fields;
} MSTP_COMMON_PORT;

/*****************************************************************************/
/* cist specific structures                                                  */
/*****************************************************************************/

typedef struct
{
	UINT8                   maxAge;
	UINT8                   fwdDelay;
	UINT8                   helloTime;
	UINT8                   messageAge;
	UINT8                   remainingHops;
} MSTP_CIST_TIMES;

typedef struct
{
	MSTP_BRIDGE_IDENTIFIER  root;
	UINT32                  extPathCost;
	MSTP_BRIDGE_IDENTIFIER  regionalRoot;
	UINT32                  intPathCost;
	MSTP_BRIDGE_IDENTIFIER  designatedId;
	MSTP_PORT_IDENTIFIER    designatedPort;
} MSTP_CIST_VECTOR;

#define MSTP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT               0
#define MSTP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT        1
#define MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT             2
#define MSTP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT               3
#define MSTP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT            4
#define MSTP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT             5
#define MSTP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT        6
#define MSTP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT     7
#define MSTP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT      8
#define MSTP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT             9
#define MSTP_BRIDGE_DATA_MEMBER_VLAN_MASK_SET             10
#define MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT           11
#define MSTP_BRIDGE_DATA_MEMBER_EX_ROOT_PATH_COST_BIT     12
#define MSTP_BRIDGE_DATA_MEMBER_MAX_HOP_BIT               13
#define MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT              14
#define MSTP_BRIDGE_DATA_MEMBER_TX_HOLD_COUNT_BIT         15  
#define MSTP_BRIDGE_DATA_MEMBER_IN_ROOT_PATH_COST_BIT     16

typedef struct
{
	MSTP_COMMON_BRIDGE      co;

	// priority vectors
	MSTP_CIST_VECTOR        bridgePriority;
	MSTP_CIST_VECTOR        rootPriority;

	// times
	MSTP_CIST_TIMES         bridgeTimes;
	MSTP_CIST_TIMES         rootTimes;

	UINT8                   pad[2]; // to word align
    UINT32                  modified_fields;
} MSTP_CIST_BRIDGE;

#define MSTP_PORT_MEMBER_PORT_ID_BIT               0
#define MSTP_PORT_MEMBER_PORT_PATH_COST_BIT        1
#define MSTP_PORT_MEMBER_DESIGN_ROOT_BIT           2
#define MSTP_PORT_MEMBER_DESIGN_COST_BIT           3
#define MSTP_PORT_MEMBER_DESIGN_BRIDGE_BIT         4
#define MSTP_PORT_MEMBER_DESIGN_PORT_BIT           5
#define MSTP_PORT_MEMBER_PORT_PRIORITY_BIT         6
#define MSTP_PORT_MEMBER_EX_PATH_COST_BIT          7
#define MSTP_PORT_MEMBER_IN_PATH_COST_BIT          8
#define MSTP_PORT_MEMBER_DESIGN_REG_ROOT_BIT       9
 
typedef struct
{
	MSTP_COMMON_PORT        co;

	// external path cost
	UINT32                  extPortPathCost;

	// priority vectors
	MSTP_CIST_VECTOR        designatedPriority;
	MSTP_CIST_VECTOR        portPriority;
	MSTP_CIST_VECTOR        msgPriority;

	// times vectors
	MSTP_CIST_TIMES         designatedTimes;
	MSTP_CIST_TIMES         msgTimes;
	MSTP_CIST_TIMES         portTimes;

    UINT8                   pad[1]; // to word align
    UINT32                  modified_fields;

} MSTP_CIST_PORT;

/*****************************************************************************/
/* msti specific structures                                                  */
/*****************************************************************************/

typedef struct
{
	UINT8                   remainingHops;
} MSTP_MSTI_TIMES;

typedef struct
{
	MSTP_BRIDGE_IDENTIFIER  regionalRoot;
	UINT32                  intPathCost;
	MSTP_BRIDGE_IDENTIFIER  designatedId;
	MSTP_PORT_IDENTIFIER    designatedPort;
} MSTP_MSTI_VECTOR;

typedef struct
{
	MSTP_COMMON_BRIDGE      co;

	// priority vectors
	MSTP_MSTI_VECTOR        bridgePriority;
	MSTP_MSTI_VECTOR        rootPriority;

	// times
	MSTP_MSTI_TIMES         bridgeTimes;
	MSTP_MSTI_TIMES         rootTimes;

    UINT8                   pad[2]; // to word align

 
    UINT32                  modified_fields;
} MSTP_MSTI_BRIDGE;

typedef struct
{
	MSTP_COMMON_PORT        co;
	
	// priority vectors
	MSTP_MSTI_VECTOR        designatedPriority;
	MSTP_MSTI_VECTOR        msgPriority;
	MSTP_MSTI_VECTOR        portPriority;

	// times
	MSTP_MSTI_TIMES         designatedTimes;
	MSTP_MSTI_TIMES         msgTimes;
	MSTP_MSTI_TIMES         portTimes;
	UINT8                   master:1;
	UINT8                   mastered:1;
	UINT8                   spare1:6;

    UINT32                  modified_fields;
	
} MSTP_MSTI_PORT;

/*****************************************************************************/
/* mstp bridge related structures                                            */
/*****************************************************************************/

// mstp configuration table - maps vlans to mstids
typedef struct { MSTP_MSTID mstid[MSTP_TABLE_SIZE]; } MSTP_CONFIG_TABLE;

// mstp index table - maps mstids to mstp indices
typedef struct { MSTP_INDEX index[MSTP_TABLE_SIZE]; } MSTP_INDEX_TABLE;

// mstp bridge structure
typedef struct
{
	// ports controlled by mstp
	PORT_MASK               *control_mask;

	// ports that are up
	PORT_MASK               *enable_mask;

	// ports that have been administratively disabled for mstp
	PORT_MASK               *admin_disable_mask;

	// ports that have been configured as point-to-point ports
	PORT_MASK               *admin_pt2pt_mask;

	// ports that have been configured as edge ports
	PORT_MASK               *admin_edge_mask;
	
	// configuration port mask requested by vlan (+1 is for cist instance)
	PORT_MASK               *config_mask[MSTP_MAX_INSTANCES_PER_REGION+1];

	// mstid to mstp_index mapping table
	MSTP_INDEX_TABLE        index_table;

	// vlan to mstid mapping table
	MSTP_CONFIG_TABLE       mstid_table;

	// set to true when mstp is activated
	bool                 	active;

	// set to true when automatic edge port detection is disabled.
	bool                 	disable_auto_edge_port;

	// current operating version
	UINT8                   forceVersion;

	// configured forward delay
	UINT8                   fwdDelay;

	// configured hello time
	UINT8                   helloTime;

	// configured max hops
	UINT8                   maxHops;

	// maximum packets that can be transmitted per second
	UINT8                   txHoldCount;

	// mstp configuration identifier
	MSTP_CONFIG_IDENTIFIER  mstConfigId;

	// to word align
	UINT8                   pad;

	// common spanning tree bridge information
	MSTP_CIST_BRIDGE         cist;

	// mst instance bridge information.
	MSTP_MSTI_BRIDGE         *msti[MSTP_MAX_INSTANCES_PER_REGION];

} MSTP_BRIDGE;
/*****************************************************************************/
/* mstp port structure                                                       */
/*****************************************************************************/
typedef struct
{
	// mstp port variables that control all spanning-tree instances
	UINT32                  infoInternal:1;
	UINT32                  mcheck:1;
	UINT32                  newInfoCist:1;
	UINT32                  newInfoMsti:1;
	UINT32                  portEnabled:1;
	UINT32                  operEdge:1;
	UINT32                  operPt2PtMac:1;
	UINT32                  rcvdInternal:1;
	UINT32                  rcvdBpdu:1;
	UINT32                  rcvdRSTP:1;
	UINT32                  rcvdSTP:1;
	UINT32                  rcvdTcAck:1;
	UINT32                  rcvdTcn:1;
	UINT32                  sendRSTP:1;
	UINT32                  tcAck:1;
	UINT32                  restrictedRole:1;
	UINT32                  bpdu_guard_active:1;
	UINT32                  unusedBits:15;

	// number of packets transmitted in the last second
	UINT16                  txCount;

	// port number associated with this port
	PORT_ID                 port_number;

	// port migration timer
	UINT16                  mdelayWhile;

	// port hello timer
	UINT16                  helloWhen;

	// Last Timer Expiry ticks
	UINT32                  last_expiry_time;
	uint64_t                last_bpdu_rx_time;

	// port migration state machine state
	MSTP_PPM_STATE          ppmState;

	// port receive state machine state
	MSTP_PRX_STATE          prxState;

	// port transmit state machine state
	MSTP_PTX_STATE          ptxState;

    UINT16                  chksum;

	// bpdu receive and send statistics for the port
	struct { MSTP_BPDU_STATS rx,tx;} stats;

	// mask of the set of mstp index that are currently configured (excluding cist)
	L2_PROTO_INSTANCE_MASK  instance_mask;

	// common spanning tree port
	MSTP_CIST_PORT          cist;

	// mst instance ports
	MSTP_MSTI_PORT          *msti[MSTP_MAX_INSTANCES_PER_REGION];
} MSTP_PORT;

typedef struct
{
	// mstp bridge information
	MSTP_BRIDGE             *bridge;
	
	UINT8 					configured_bpdu_mac;
	
	// mstp bpdu definition
	MSTP_BPDU               bpdu;

	// mstp per port information
    // array of pointers to MSTP_PORT
	MSTP_PORT               **port_arr;

	// number of available mst instances
	UINT8                   free_instances;

	// number of total mst instances
	UINT8                   max_instances;
	
	// to word align.
	UINT8                   pad;
 } MSTP_GLOBAL;


typedef enum
{
	MSTP_CONFIG_ADD_REGION_ID = 1, //FOR PBB
	MSTP_CONFIG_DEL_REGION_ID,

	// common bridge parameters
	MSTP_CONFIG_NAME,
	MSTP_CONFIG_REVISION_LEVEL,
	MSTP_CONFIG_BPDU_MAC,
	MSTP_CONFIG_MAX_HOPS,
	MSTP_CONFIG_FORCE_VERSION,
	MSTP_CONFIG_HELLO_TIME,
	MSTP_CONFIG_MAX_AGE,
	MSTP_CONFIG_FORWARD_DELAY,

	// common port parameters
	MSTP_CONFIG_PORT_DISABLE,
	MSTP_CONFIG_PORT_ADMIN_EDGE,
	MSTP_CONFIG_PORT_ADMIN_PT2PT,
	MSTP_CONFIG_PORT_MCHECK,

	// instance specific bridge and port parameters
	MSTP_CONFIG_MSTID_PRIORITY,
	MSTP_CONFIG_MSTID_PORT_PATH_COST,
	MSTP_CONFIG_MSTID_PORT_PRIORITY,
	MSTP_CONFIG_MSTID_ATTACH_VLAN,
	MSTP_CONFIG_MSTID_ATTACH_VLAN_GROUP,

	// activate/deactivate configuration
	MSTP_CONFIG_START,
	MSTP_CONFIG_MAX
} MSTP_CONFIG_OPCODE;


#define MSTP_CONFIG_CLEAR_OPCODE(_op_)          (_op_) = 0
#define MSTP_CONFIG_ISSET_ANY(_op_)             ((_op_) != 0)
#define MSTP_CONFIG_SET_OPCODE(_tgt_,_op_)      (_tgt_) |= BIT(_op_)
#define MSTP_CONFIG_ISSET_OPCODE(_tgt_,_op_)    ((_tgt_) & BIT(_op_))

typedef struct
{
	UINT8                   max_hops;
	UINT8                   hello_time;
	UINT8                   max_age;
	UINT8                   forward_delay;
	UINT8                   force_version;
	UINT16                  priority;
} MSTP_CONFIG_BRIDGE;

typedef struct
{
	PORT_ID                 port_number;
	UINT32                  path_cost;
	UINT16                  priority;
	UINT16                  disable:1;
	UINT16                  admin_edge:1;
	UINT16                  admin_pt2pt:1;
	UINT16                  auto_config:1;
	UINT16                  mcheck:1;
	UINT16                  spare:11;
} MSTP_CONFIG_PORT;

typedef struct
{
	bool			attach;
	VLAN_MASK		vlanmask;
	UINT16			group_id;
} MSTP_CONFIG_MSTI;

typedef enum
{
    MSTP_ADD = 1,
    MSTP_DELETE,
    MSTP_CLEAR,
    MSTP_COPY
}MSTP_FLAG;

#endif// __MSTP_H__