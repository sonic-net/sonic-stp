
/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 */

 #ifndef __L2_H__
 #define __L2_H__
 
 /* definitions -------------------------------------------------------------- */
 
 #undef BYTE
 #define BYTE unsigned char
 #undef USHORT
 #define USHORT unsigned short
 #undef UINT8
 #define UINT8 uint8_t
 #undef UINT16
 #define UINT16 uint16_t
 #undef UINT32
 #define UINT32 uint32_t
 #undef UINT
 #define UINT unsigned int
 #undef INT32
 #define INT32 int32_t
 
 // ---------------------------
 // VLAN_ID structure (32-bit value)
 // ---------------------------
 typedef UINT16 VLAN_ID;
 
 #define MIN_VLAN_ID 1
 #define MAX_VLAN_ID 4095
 #define VLAN_ID_INVALID (MAX_VLAN_ID + 1)
 #define L2_ETH_ADD_LEN 6
 #define VLAN_HEADER_LEN 4
 
 #define VLAN_ID_TAG_BITS 0xFFF
 #define GET_VLAN_ID_TAG(vlan_id) (vlan_id & VLAN_ID_TAG_BITS)
 #define IS_VALID_VLAN(vlan_id) ((GET_VLAN_ID_TAG(vlan_id) >= MIN_VLAN_ID) && (GET_VLAN_ID_TAG(vlan_id) < MAX_VLAN_ID))
 
 /* enums -------------------------------------------------------------------- */
 
 enum L2_PORT_STATE
 {
	 DISABLED = 0,
	 BLOCKING = 1,
	 LISTENING = 2,
	 LEARNING = 3,
	 FORWARDING = 4,
	 L2_MAX_PORT_STATE = 5
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
	 BYTE destination_address_DSAP;
	 BYTE source_address_SSAP;
	 BYTE llc_frame_type;
 } __attribute__((__packed__)) LLC_HEADER;
 
 typedef struct SNAP_HEADER
 {
	 BYTE destination_address_DSAP;
	 BYTE source_address_SSAP;
 
	 BYTE llc_frame_type;
 
	 BYTE protocol_id_filler[3];
	 UINT16 protocol_id;
 } __attribute__((__packed__)) SNAP_HEADER;
 
 typedef struct MAC_ADDRESS
 {
	 UINT32 _ulong;
	 UINT16 _ushort;
 } __attribute__((__packed__)) MAC_ADDRESS;
 
 #define COPY_MAC(sptr_mac2, sptr_mac1) \
	 (memcpy(sptr_mac2, sptr_mac1, sizeof(MAC_ADDRESS)))
 
 #define NET_TO_HOST_MAC(sptr_dst_mac, sptr_src_mac)                                           \
	 ((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = ntohl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
	 ((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = ntohs(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);
 
 /* copies src mac to dst mac and converts to network byte ordering */
 #define HOST_TO_NET_MAC(sptr_dst_mac, sptr_src_mac)                                           \
	 ((MAC_ADDRESS *)(sptr_dst_mac))->_ulong = htonl(((MAC_ADDRESS *)(sptr_src_mac))->_ulong); \
	 ((MAC_ADDRESS *)(sptr_dst_mac))->_ushort = htons(((MAC_ADDRESS *)(sptr_src_mac))->_ushort);
 
 typedef struct MAC_HEADER
 {
	 MAC_ADDRESS destination_address;
	 MAC_ADDRESS source_address;
 
	 USHORT length;
 } __attribute__((__packed__)) MAC_HEADER;
 
 #define STP_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(LLC_HEADER))
 #define PVST_BPDU_OFFSET (sizeof(MAC_HEADER) + sizeof(SNAP_HEADER))
 
 
 #define PORT_MASK      						  BITMAP_T
 #define VLAN_MASK      						  STATIC_BITMAP_T
 #define PORT_MASK_LOCAL						  STATIC_BITMAP_T
 
 #define vlan_bmp_init   					  static_bmp_init
 #define portmask_local_init 				  static_portmask_init
 
 #define IS_MEMBER(_bmp, _port)                bmp_isset(_bmp, _port)
 #define is_member(_bmp, _port)                bmp_isset(_bmp, _port)
 #define PORT_MASK_ISSET(_bmp, _port)          bmp_isset(_bmp, _port)
 
 #define PORT_MASK_ISSET_ANY(_bmp)             bmp_isset_any(_bmp)
 #define PORT_MASK_SET_ALL(_bmp)               bmp_set_all(_bmp)
 #define PORT_MASK_ZERO(_bmp)                  bmp_reset_all(_bmp)
 
 #define clear_mask(_bmp)                      bmp_reset_all(_bmp)
 #define is_mask_clear(_bmp)                   (!(bmp_isset_any(_bmp)))
 #define is_mask_equal(_bmp1, _bmp2)           bmp_is_mask_equal(_bmp1, _bmp2)
 #define copy_mask(_dst, _src)                 bmp_copy_mask(_dst, _src)
 #define not_mask(_dst, _src)                  bmp_not_mask(_dst, _src)
 #define and_masks(_tgt, _bmp1, _bmp2)         bmp_and_masks(_tgt, _bmp1, _bmp2)
 #define and_not_masks(_tgt, _bmp1, _bmp2)     bmp_and_not_masks(_tgt, _bmp1, _bmp2)
 #define or_masks(_tgt, _bmp1, _bmp2)          bmp_or_masks(_tgt, _bmp1, _bmp2)
 #define xor_masks(_tgt, _bmp1, _bmp2)         bmp_xor_masks(_tgt, _bmp1, _bmp2)
 
 #define set_mask_bit(_bmp, _port)             bmp_set(_bmp, _port)
 #define clear_mask_bit(_bmp, _port)           bmp_reset(_bmp, _port)
 
 #define port_mask_get_next_port(_bmp,_port)   bmp_get_next_set_bit(_bmp, _port)
 #define port_mask_get_first_port(_bmp)        bmp_get_first_set_bit(_bmp)
 
 #define vlanmask_clear_all(_bmp)              bmp_reset_all((BITMAP_T *)_bmp)
 #define vlanmask_is_clear(_bmp)               (!(bmp_isset_any((BITMAP_T *)_bmp)))
 #define vlanmask_copy(_dst, _src)             bmp_copy_mask((BITMAP_T *)_dst, (BITMAP_T *)_src)
 
 #define vlanmask_set_bit(_bmp, _vlan)         bmp_set((BITMAP_T *)_bmp, _vlan)
 #define vlanmask_clear_bit(_bmp, _vlan)       bmp_reset((BITMAP_T *)_bmp, _vlan)
 #define VLANMASK_ISSET(_bmp, _vlan)           bmp_isset((BITMAP_T *)_bmp, _vlan)
 
 
 /* L2_PROTOCOL_INSTANCE_MASK
  * keeps track of the list of msti instances running inside mstp region
  */
 #define L2_MAX_PROTOCOL_INSTANCES            1024
 #define L2_PROTO_INDEX_MASKS                 (L2_MAX_PROTOCOL_INSTANCES >> 5)
 #define L2_PROTO_INDEX_INVALID               0xFFFF
 
 typedef struct
 {
	 UINT32 m[L2_PROTO_INDEX_MASKS];
 } L2_PROTO_INSTANCE_MASK;
 
 // set the instance bit in the instance mask
 #define L2_PROTO_INSTANCE_MASK_SET(_instance_mask_ptr_, _instance_id_) \
	 ((_instance_mask_ptr_)->m[(_instance_id_) >> 5]) |= (1L<<((_instance_id_)&0x1f))
 
 // clear the instance bit from the instance mask
 #define L2_PROTO_INSTANCE_MASK_CLR(_instance_mask_ptr_, _instance_id_) \
	 ((_instance_mask_ptr_)->m[(_instance_id_) >> 5]) &= ~(1L<<((_instance_id_)&0x1f))
 
 // check if the instance bit is set in the instance mask
 #define L2_PROTO_INSTANCE_MASK_ISSET(_instance_mask_ptr_, _instance_id_) \
	 ((((_instance_mask_ptr_)->m[(_instance_id_) >> 5]) & (1L<<((_instance_id_)&0x1f))) != 0)
 
 // clear all the bits in the instance mask
 #define L2_PROTO_INSTANCE_MASK_ZERO(_instance_mask_ptr_) \
	 memset(_instance_mask_ptr_, 0, sizeof(L2_PROTO_INSTANCE_MASK))
 
 #endif //__L2_H__
 
