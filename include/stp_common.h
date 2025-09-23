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

#ifndef __STP_COMMON_H__
#define __STP_COMMON_H__

/*****************************************************************************/
/* definitions and macros                                                    */
/*****************************************************************************/
typedef enum SORT_RETURN
{
	LESS_THAN = -1,
	EQUAL_TO = 0,
	GREATER_THAN = 1
} SORT_RETURN;

#define STP_VERSION_ID      0
#define RSTP_VERSION_ID     2
#define MSTP_VERSION_ID     3

#define RSTP_SIZEOF_BPDU	36

#define PORT_MASK BITMAP_T

#define STP_INDEX UINT16
#define STP_INDEX_INVALID 0xFFFF

#define IS_STP_MAC(_mac_ptr_) (SAME_MAC_ADDRESS((_mac_ptr_), &bridge_group_address))
#define IS_PVST_MAC(_mac_ptr_) (SAME_MAC_ADDRESS((_mac_ptr_), &pvst_bridge_group_address))

/*****************************************************************************/
/* enum definitions                                                          */
/*****************************************************************************/

typedef enum STP_BPDU_TYPE
{
	CONFIG_BPDU_TYPE = 0,
	RSTP_BPDU_TYPE = 2,
	TCN_BPDU_TYPE = 128
} STP_BPDU_TYPE;

/*****************************************************************************/
/* structure definitions                                                     */
/*****************************************************************************/

typedef struct BRIDGE_BPDU_FLAGS
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT8 topology_change_acknowledgement : 1;
	UINT8 blank : 6;
	UINT8 topology_change : 1;
#else
	UINT8 topology_change : 1;
	UINT8 blank : 6;
	UINT8 topology_change_acknowledgement : 1;
#endif
} __attribute__((__packed__)) BRIDGE_BPDU_FLAGS;

typedef struct BRIDGE_IDENTIFIER
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT16 priority : 4;
	UINT16 system_id : 12;
#else
	UINT16 system_id : 12;
	UINT16 priority : 4;
#endif // BIG_ENDIAN

	MAC_ADDRESS address;

} __attribute__((__packed__)) BRIDGE_IDENTIFIER;

typedef struct PORT_IDENTIFIER
{
#if __BYTE_ORDER == __BIG_ENDIAN
	UINT16 priority : 4;
	UINT16 number : 12;
#else
	UINT16 number : 12;
	UINT16 priority : 4;
#endif // BIG_ENDIAN
} __attribute__((__packed__)) PORT_IDENTIFIER;

// spanning-tree configuration bpdu
typedef struct STP_CONFIG_BPDU
{
	MAC_HEADER mac_header;
	LLC_HEADER llc_header;
	UINT16 protocol_id;
	UINT8 protocol_version_id;
	UINT8 type;
	BRIDGE_BPDU_FLAGS flags;
	BRIDGE_IDENTIFIER root_id;
	UINT32 root_path_cost;
	BRIDGE_IDENTIFIER bridge_id;
	PORT_IDENTIFIER port_id;
	UINT16 message_age;
	UINT16 max_age;
	UINT16 hello_time;
	UINT16 forward_delay;
} __attribute__((__packed__)) STP_CONFIG_BPDU;

// spanning-tree topology change notification bpdu
typedef struct STP_TCN_BPDU
{
	MAC_HEADER mac_header;
	LLC_HEADER llc_header;
	UINT16 protocol_id;
	UINT8 protocol_version_id;
	UINT8 type;
	UINT8 padding[3];
} __attribute__((__packed__)) STP_TCN_BPDU;

// pvst configuration bpdu
typedef struct PVST_CONFIG_BPDU
{
	MAC_HEADER mac_header;
	SNAP_HEADER snap_header;
	UINT16 protocol_id;
	UINT8 protocol_version_id;
	UINT8 type;
	BRIDGE_BPDU_FLAGS flags;
	BRIDGE_IDENTIFIER root_id;
	UINT32 root_path_cost;
	BRIDGE_IDENTIFIER bridge_id;
	PORT_IDENTIFIER port_id;
	UINT16 message_age;
	UINT16 max_age;
	UINT16 hello_time;
	UINT16 forward_delay;
	UINT8 padding[3];
	UINT16 tag_length;
	UINT16 vlan_id;
} __attribute__((__packed__)) PVST_CONFIG_BPDU;

// pvst topology change notification bpdu
typedef struct PVST_TCN_BPDU
{
	MAC_HEADER mac_header;
	SNAP_HEADER snap_header;
	UINT16 protocol_id;
	UINT8 protocol_version_id;
	UINT8 type;
	UINT8 padding[38];
} __attribute__((__packed__)) PVST_TCN_BPDU;

#endif //__STP_COMMON_H__
