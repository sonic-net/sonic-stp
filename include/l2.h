/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
 */

#ifndef __L2_H__
#define __L2_H__

/* definitions -------------------------------------------------------------- */

#undef BYTE
#define BYTE    unsigned char
#undef USHORT
#define USHORT  unsigned short
#undef UINT8
#define UINT8   uint8_t
#undef UINT16
#define UINT16  uint16_t
#undef UINT32
#define UINT32  uint32_t
#undef UINT
#define UINT    unsigned int
#undef INT32
#define INT32   int32_t


// ---------------------------
// VLAN_ID structure (32-bit value)
// ---------------------------
typedef UINT16 VLAN_ID;

#define MIN_VLAN_ID							1
#define MAX_VLAN_ID							4095
#define VLAN_ID_INVALID						(MAX_VLAN_ID+1)
#define L2_ETH_ADD_LEN                      6

#define VLAN_ID_TAG_BITS         0xFFF
#define GET_VLAN_ID_TAG(vlan_id) (vlan_id & VLAN_ID_TAG_BITS)
#define IS_VALID_VLAN(vlan_id)   ((GET_VLAN_ID_TAG(vlan_id) >= MIN_VLAN_ID) \
                                 && (GET_VLAN_ID_TAG(vlan_id) < MAX_VLAN_ID))


/* enums -------------------------------------------------------------------- */

enum L2_PORT_STATE 
{
	DISABLED				= 0,
	BLOCKING				= 1,
	LISTENING				= 2,
	LEARNING				= 3,
	FORWARDING				= 4,
    L2_MAX_PORT_STATE       = 5
};


enum SNAP_PROTOCOL_ID
{
	SNAP_CISCO_PVST_ID = (0x010b),  
};

enum LLC_FRAME_TYPE
{
	UNNUMBERED_INFORMATION = 3,
};


enum SAP_TYPES
{
	LSAP_SNAP_LLC = 0xaa,
	LSAP_BRIDGE_SPANNING_TREE_PROTOCOL = 0x42,
};

/* structures --------------------------------------------------------------- */

typedef struct LLC_HEADER
{
	BYTE      destination_address_DSAP;
	BYTE      source_address_SSAP;
	BYTE      llc_frame_type;
} __attribute__((__packed__)) LLC_HEADER;

typedef struct SNAP_HEADER
{
	BYTE    destination_address_DSAP;
	BYTE    source_address_SSAP;
	
	BYTE    llc_frame_type;

	BYTE	protocol_id_filler[3];
	UINT16  protocol_id;
} __attribute__((__packed__)) SNAP_HEADER;

typedef struct MAC_ADDRESS
{
	UINT32							_ulong;
	UINT16							_ushort;
}__attribute__ ((packed))MAC_ADDRESS;

#define COPY_MAC(sptr_mac2, sptr_mac1) \
        (memcpy(sptr_mac2, sptr_mac1, sizeof(MAC_ADDRESS)))

#define NET_TO_HOST_MAC(sptr_dst_mac, sptr_src_mac) \
    ((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = ntohl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
    ((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = ntohs(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);

/* copies src mac to dst mac and converts to network byte ordering */
#define HOST_TO_NET_MAC(sptr_dst_mac, sptr_src_mac) \
        ((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = htonl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
    ((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = htons(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);

typedef struct MAC_HEADER
{
	MAC_ADDRESS						destination_address;
	MAC_ADDRESS						source_address;

	USHORT							length;
} __attribute__((__packed__)) MAC_HEADER;


#define STP_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(LLC_HEADER))
#define PVST_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(SNAP_HEADER))

#define STP_MAX_PKT_LEN 68
#define VLAN_HEADER_LEN 4    //4 bytes

#endif //__L2_H__
