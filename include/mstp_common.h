/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */
#ifndef __MSTP_COMMON_H__
#define __MSTP_COMMON_H__

/*****************************************************************************/
/* definitions                                                               */
/*****************************************************************************/
#define MSTP_MSTID_MIN                      1
#define MSTP_MSTID_MAX                      4094
#define MSTP_MSTID_CIST                     0
#define MSTP_MSTID_INVALID                  (MSTP_MSTID_MAX+1)

#define MSTP_INDEX_MIN                      0
#define MSTP_INDEX_MAX                     (MSTP_MAX_INSTANCES_PER_REGION-1)
#define MSTP_INDEX_CIST                     MSTP_MAX_INSTANCES_PER_REGION
#define MSTP_INDEX_INVALID                  0xFFFF

#define MSTP_TABLE_SIZE                     4096
#define MSTP_MIN_INSTANCES                  1
#define MSTP_MAX_INSTANCES_PER_REGION		64

#define MSTP_BPDU_BASE_SIZE                 38
#define MSTP_BPDU_BASE_V3_LENGTH            64

#define MSTP_NAME_LENGTH                    32

#define MSTP_PORT_IDENTIFIER                PORT_IDENTIFIER
#define MSTP_INVALID_PORT                   0xFFF

/*****************************************************************************/
/* mst bridge identifier                                                     */
/*****************************************************************************/
typedef struct
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT16                  priority:4;  // bridge priority
	UINT16                  system_id:12;// system-id
#else
	UINT16                  system_id:12; // system-id
	UINT16                  priority:4; // bridge priority
#endif
	// mac-address for the bridge
	MAC_ADDRESS             address;

}__attribute__((__packed__)) MSTP_BRIDGE_IDENTIFIER;

/*****************************************************************************/
/* msti configuration message structure                                      */
/*****************************************************************************/
typedef struct
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT8                   master:1;
	UINT8                   agreement:1;
	UINT8                   forwarding:1;
	UINT8                   learning:1;
	UINT8                   role:2;
	UINT8                   proposal:1;
	UINT8                   topology_change:1;
#else
	UINT8                   topology_change:1;
	UINT8                   proposal:1;
	UINT8                   role:2;
	UINT8                   learning:1;
	UINT8                   forwarding:1;
	UINT8                   agreement:1;
	UINT8                   master:1;
#endif
} __attribute__((__packed__))MSTI_FLAGS;

/*****************************************************************************/
/*  msti configuration message structure                                     */
/*****************************************************************************/
typedef struct
{
	// flags for the mst instance
	MSTI_FLAGS              msti_flags;

	// the mst instance regional root - 4 bits of which constitute priority
	MSTP_BRIDGE_IDENTIFIER  msti_regional_root;

	// the mst instance internal root path cost
	UINT32                  msti_int_path_cost;

#if __BYTE_ORDER == __BIG_ENDIAN
	// mst instance bridge priority encoded in 4 bits.
	UINT8                   msti_bridge_priority:4;
	UINT8                   unused1:4;

	// mst instance port priority encoded in 4 bits
	UINT8                   msti_port_priority:4;
	UINT8                   unused2:4;
#else
	// mst instance bridge priority encoded in 4 bits.
	UINT8                   unused1:4;
	UINT8                   msti_bridge_priority:4;

	// mst instance port priority encoded in 4 bits
	UINT8                   unused2:4;
	UINT8                   msti_port_priority:4;
#endif // !BIG_ENDIAN

	// mst instance remaining hops
	UINT8                   msti_remaining_hops;

} __attribute__((__packed__)) MSTI_CONFIG_MESSAGE;

/*****************************************************************************/
/*  mst configuration identification                                         */
/*****************************************************************************/
#ifndef MD5_DIGEST_LENGTH 
#define MD5_DIGEST_LENGTH (16) 
#endif // MD5_DIGEST_LENGTH 

typedef struct
{
	// configuration identifier format selector - encoded as 0
	UINT8                   format_selector; 

	// configuration name - variable text string conforming to rfc 2271
	// definition snmpAdminString
	UINT8                   name[MSTP_NAME_LENGTH];

	// revision level
	UINT16                  revision_number;

	// configuration digest - 16 octet signature of type hmac-md4 created from
	// the mst configuration table.
	UINT8                   config_digest[16];

} __attribute__((__packed__)) MSTP_CONFIG_IDENTIFIER;

typedef struct RSTP_BPDU_FLAGS
{
#if __BYTE_ORDER == __BIG_ENDIAN
        UINT8                   topology_change_acknowledgement:1;
        UINT8                   agreement:1;
        UINT8                   forwarding:1;
        UINT8                   learning:1;
        UINT8                   role:2;
        UINT8                   proposal:1;
        UINT8                   topology_change:1;
#else
        UINT8                   topology_change:1;
        UINT8                   proposal:1;
        UINT8                   role:2;
        UINT8                   learning:1;
        UINT8                   forwarding:1;
        UINT8                   agreement:1;
        UINT8                   topology_change_acknowledgement:1;
#endif
} __attribute__((__packed__)) RSTP_BPDU_FLAGS;


/*****************************************************************************/
/* mst bpdu structure                                                        */
/*****************************************************************************/
typedef struct
{
	// mac header
	MAC_HEADER              mac_header;

	// llc header
	LLC_HEADER              llc_header;

	// protocol identifier
	UINT16                  protocol_id;

	// protocol version: 0 - stp, 2 - rstp, 3 and greater - mstp
	UINT8                   protocol_version_id;

	// type of bpdu 0 - configuration, 128 - tcn, 2 - rstp/mstp
	UINT8                   type;

	// flags associated with common spanning tree instance
	RSTP_BPDU_FLAGS         cist_flags;

	// common spanning tree root
	MSTP_BRIDGE_IDENTIFIER  cist_root;

	// common spanning tree path cost
	UINT32                  cist_ext_path_cost;

	// common spanning tree regional root
	MSTP_BRIDGE_IDENTIFIER  cist_regional_root;

	// common spanning tree port for transmitting bridge port
	MSTP_PORT_IDENTIFIER    cist_port;

	// message age timer value
	UINT16                  message_age;

	// max age timer value
	UINT16                  max_age;

	// hello time timer value used by the transmitting bridge port
	UINT16                  hello_time;

	// forward delay timer value
	UINT16                  forward_delay;

	// version 1 length - should be 0
	UINT8                   v1_length;

	// version 3 length - number of octets taken by parameters that follow
	UINT16                  v3_length;

	// mst configuration identifier
	MSTP_CONFIG_IDENTIFIER  mst_config_id;

	// cist internal root path cost
	UINT32                  cist_int_path_cost;

	// cist bridge identifier of the transmitting bridge - 12 bit system-id should be 0
	MSTP_BRIDGE_IDENTIFIER  cist_bridge;

	// value of remaining hops for the cist
	UINT8                   cist_remaining_hops;

	// a sequence of zero or more msti configuration messages
	MSTI_CONFIG_MESSAGE     msti_msgs[MSTP_MAX_INSTANCES_PER_REGION];

} __attribute__((__packed__)) MSTP_BPDU;

// rstp bpdu
typedef struct RSTP_BPDU
{
        MAC_HEADER              mac_header;
        LLC_HEADER              llc_header;
        UINT16                  protocol_id;
        UINT8                   protocol_version_id;
        UINT8                   type;
        RSTP_BPDU_FLAGS         flags;
        BRIDGE_IDENTIFIER       root_id;
        UINT32                  root_path_cost;
        BRIDGE_IDENTIFIER       bridge_id;
        PORT_IDENTIFIER         port_id;
        UINT16                  message_age;
        UINT16                  max_age;
        UINT16                  hello_time;
        UINT16                  forward_delay;
        UINT8                   version1_length;
} __attribute__((__packed__)) RSTP_BPDU;

#endif // //__MSTP_COMMON_H__
