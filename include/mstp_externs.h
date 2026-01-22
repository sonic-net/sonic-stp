/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */
#ifndef __MSTP_EXTERNS_H__
#define __MSTP_EXTERNS_H__

extern MSTP_ROLE txRole[];
extern MSTP_GLOBAL g_mstp_global;

/* mstp_debug.c */
extern UINT8 mstp_err_string[];
extern UINT8 mstp_pim_state_string[][20];
extern UINT8 mstp_prs_state_string[][20];
extern UINT8 mstp_prt_state_string[][20];
extern UINT8 mstp_pst_state_string[][20];
extern UINT8 mstp_tcm_state_string[][20];
extern UINT8 mstp_ppm_state_string[][20];
extern UINT8 mstp_ptx_state_string[][20];
extern UINT8 mstp_prx_state_string[][20];
extern UINT8 mstp_rcvdinfo_string[][20];
extern UINT8 mstp_role_string[][20];

/* mstp_debug.c */
extern void mstpdebug_display_bpdu(MSTP_BPDU *bpdu, bool rx);
extern void mstpdebug_display_msti_bpdu(MSTI_CONFIG_MESSAGE *bpdu);
extern void mstpdebug_print_config_digest(UINT8 * config_digest);
extern void mstpdm_mstp_bridge();
extern void mstpdm_mstp_port(PORT_ID port_number);
extern void mstpdm_cist();
extern void mstpdm_cist_port(PORT_ID port_number);
extern void mstpdm_msti(MSTP_MSTID mstid);
extern void mstpdm_msti_port(MSTP_MSTID mstid, PORT_ID port_number);
extern void mstpdbg_process_ctl_msg(void *msg);

/* mstp_pim.c */
extern void mstp_pim_init(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_pim_gate(MSTP_INDEX mstp_index, PORT_ID port_number, void *bpdu);

/* mstp_ppm.c */
extern void mstp_ppm_init(PORT_ID port_number);
extern void mstp_ppm_gate(PORT_ID port_number);

/* mstp_prs.c */
extern void mstp_prs_init(MSTP_INDEX mstp_index);
extern void mstp_prs_gate(MSTP_INDEX mstp_index);

/* mstp_prt.c */
extern void mstp_prt_init(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_prt_gate(MSTP_INDEX mstp_index, PORT_ID port_number);

/* mstp_prx.c */
extern void mstp_prx_init(PORT_ID port_number);
extern void mstp_prx_gate(PORT_ID port_number, MSTP_BPDU *bpdu);

/* mstp_pst.c */
extern void mstp_pst_init(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_pst_gate(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_pst_action(MSTP_INDEX mstp_index, PORT_ID port_number);

/* mstp_ptx.c */
extern void mstp_ptx_init(PORT_ID port_number);
extern void mstp_ptx_gate(PORT_ID port_number);

/* mstp_tcm.c */
extern void mstp_tcm_init(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_tcm_gate(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_tcm_action(MSTP_INDEX mstp_index, PORT_ID port_number);

/* mstp_data.c */
extern MSTP_PORT * mstpdata_get_port(PORT_ID port_number);
extern MSTP_BPDU *mstpdata_get_bpdu();
extern MSTP_BRIDGE * mstpdata_get_bridge();
extern void mstpdata_reset();
extern MSTP_INDEX mstpdata_get_new_index();
extern bool mstpdata_free_index(MSTP_INDEX mstp_index);
extern MSTP_BRIDGE * mstpdata_alloc_mstp_bridge();
extern void mstpdata_free_mstp_bridge();
extern MSTP_PORT * mstpdata_alloc_mstp_port(PORT_ID port_number);
extern void mstpdata_free_port(PORT_ID port_number);
extern MSTP_MSTI_PORT * mstpdata_alloc_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstpdata_free_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstpdata_port_vlan_db_init();
extern void mstpdata_vlan_port_db_init();
extern PORT_MASK* mstpdata_get_port_mask_for_vlan(VLAN_ID vlan_id);
extern BITMAP_T* mstpdata_get_vlan_mask_for_port(PORT_ID port_id);
extern void mstpdata_update_port_mask_for_vlan(VLAN_ID vlan_id, PORT_MASK *port_mask, UINT8 add);
extern void mstpdata_update_port_vlan(PORT_ID port_id, VLAN_ID vlan_id, UINT8 add);
extern bool mstpdata_is_vlan_present(VLAN_ID vlan_id);
extern void mstpdata_update_untag_vlan_for_port(PORT_ID port_id, VLAN_ID vlan_id);
extern VLAN_ID mstpdata_get_untag_vlan_for_port(PORT_ID port_id);
extern void mstpdata_reset_vlan_port_db();
extern void mstpdata_reset_port_vlan_db();
extern void mstpdata_update_vlanport_db_on_po_delete(PORT_ID port_id);

/* mstp_lib.c */
bool mstplib_port_is_admin_edge_port(PORT_ID port_number);
VLAN_MASK * mstplib_instance_get_vlanmask(MSTP_INDEX mstp_index);
extern PORT_MASK * mstplib_instance_get_portmask(MSTP_INDEX mstp_index);
UINT8 mstplib_get_num_active_instances();
bool mstplib_is_mstp_active();
bool mstplib_is_root_bridge(MSTP_MSTID mstid);
extern MSTP_MSTID mstplib_get_next_mstid(MSTP_MSTID prev_mstid);
extern MSTP_MSTID mstplib_get_first_mstid();
extern bool mstplib_port_is_oper_edge_port(PORT_ID port_number);
extern bool mstplib_get_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE *state);


/* mstp.c */
extern bool mstp_allSynced(MSTP_INDEX mstp_index, PORT_ID calling_port);
extern bool mstp_cistRootPort(PORT_ID port_number);
extern bool mstp_cistDesignatedPort(PORT_ID port_number);
extern bool mstp_mstiRootPort(PORT_ID port_number);
extern bool mstp_mstiDesignatedPort(PORT_ID port_number);
extern bool mstp_mstiMasterPort(PORT_ID port_number);
extern bool mstp_rcvdAnyMsg(PORT_ID port_number);
extern bool mstp_rcvdCistInfo(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_rcvdMstiInfo(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_reRooted(MSTP_INDEX mstp_index, PORT_ID calling_port);
extern bool mstp_updtCistInfo(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_updtMstiInfo(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_betterorsameInfoCist(MSTP_INDEX mstp_index, PORT_ID port_number, MSTP_INFOIS newInfoIs);
extern bool mstp_betterorsameInfoMsti(MSTP_INDEX mstp_index, PORT_ID port_number, MSTP_INFOIS newInfoIs);
extern void mstp_clearAllRcvdMsgs(PORT_ID port_number);
extern void mstp_setReselectTree(MSTP_INDEX mstp_index);
extern void mstp_clearReselectTree(MSTP_INDEX mstp_index);
extern void mstp_disableForwarding(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_disableLearning(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_enableForwarding(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_enableLearning(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_flush(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstp_fromSameRegion(MSTP_PORT *mstp_port, MSTP_BPDU *bpdu);
extern void mstp_newTcWhile(MSTP_INDEX mstp_index, PORT_ID port_number);
extern MSTP_RCVD_INFO mstp_rcvInfoCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern MSTP_RCVD_INFO mstp_rcvInfoMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordAgreementCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordAgreementMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordMasteredCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordMasteredMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordProposalCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordProposalMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_setRcvdMsgs(PORT_ID port_number, MSTP_BPDU *bpdu);
extern void mstp_setReRootTree(MSTP_INDEX mstp_index);
extern void mstp_setSelectedTree(MSTP_INDEX mstp_index);
extern void mstp_setSyncTree(MSTP_INDEX mstp_index);
extern void mstp_setTcFlags(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_setTcPropTree(MSTP_INDEX mstp_index, PORT_ID calling_port);
extern void mstp_txConfig(PORT_ID port_number);
extern void mstp_txTcn(PORT_ID port_number);
extern void mstp_txMstp(PORT_ID port_number);
extern void mstp_updtBpduVersion(MSTP_PORT *mstp_port, MSTP_BPDU *bpdu);
extern void mstp_updtRcvdInfoWhileCist(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_updtRcvdInfoWhileMsti(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstp_updtRolesCist(MSTP_INDEX mstp_index);
extern void mstp_updtRolesMsti(MSTP_INDEX mstp_index);
extern void mstp_updtRolesDisabledTree(MSTP_INDEX mstp_index);
extern void mstp_recordDisputeCist(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);
extern void mstp_recordDisputeMsti(MSTP_INDEX mstp_index, PORT_ID port_number, void *bufptr);


/* mstp_util.c */
extern MSTP_COMMON_BRIDGE *mstputil_get_common_bridge(MSTP_INDEX mstp_index);
extern MSTP_COMMON_PORT *mstputil_get_common_port(MSTP_INDEX mstp_index, MSTP_PORT *mstp_port);
extern MSTP_INDEX mstputil_get_index(MSTP_MSTID mstid);
extern MSTP_MSTID mstputil_get_mstid(MSTP_INDEX mstp_index);
extern MSTP_CIST_BRIDGE * mstputil_get_cist_bridge();
extern MSTP_MSTI_BRIDGE * mstputil_get_msti_bridge(MSTP_MSTID mstid);
extern MSTP_CIST_PORT * mstputil_get_cist_port(PORT_ID port_number);
extern MSTP_MSTI_PORT * mstputil_get_msti_port(MSTP_MSTID mstid, PORT_ID port_number);
extern void mstputil_get_default_name(UINT8 * name, UINT32 name_length);
extern UINT8 * mstputil_bridge_to_string(MSTP_BRIDGE_IDENTIFIER * bridge_id, UINT8 * buffer, UINT16 size);
extern void mstputil_port_to_string(MSTP_PORT_IDENTIFIER *portId, UINT8 *buffer, UINT16 size);
extern void mstputil_compute_message_digest(bool print);
extern bool mstputil_computeReselect(MSTP_INDEX mstp_index);
extern bool mstputil_setReselectAll(PORT_ID port_number);
extern void mstputil_set_changedMaster(MSTP_PORT *mstp_port);
extern SORT_RETURN mstputil_compare_bridge_id(MSTP_BRIDGE_IDENTIFIER *id1, MSTP_BRIDGE_IDENTIFIER *id2);
extern SORT_RETURN mstputil_compare_cist_bridge_id(MSTP_BRIDGE_IDENTIFIER *id1, MSTP_BRIDGE_IDENTIFIER *id2);
extern SORT_RETURN mstputil_compare_port_id(MSTP_PORT_IDENTIFIER *p1, MSTP_PORT_IDENTIFIER *p2);
extern SORT_RETURN mstputil_compare_cist_vectors(MSTP_CIST_VECTOR *vec1, MSTP_CIST_VECTOR *vec2);
extern SORT_RETURN mstputil_compare_msti_vectors(MSTP_MSTI_VECTOR *vec1, MSTP_MSTI_VECTOR *vec2);
extern bool mstputil_are_cist_times_equal(MSTP_CIST_TIMES *t1, MSTP_CIST_TIMES *t2);
extern bool mstputil_is_cist_root_bridge();
extern bool mstputil_is_msti_root_bridge(MSTP_INDEX mstp_index);
extern void mstputil_compute_cist_designated_priority(MSTP_CIST_BRIDGE *cist_bridge, MSTP_PORT *mstp_port);
extern void mstputil_compute_msti_designated_priority(MSTP_MSTI_BRIDGE *msti_bridge, MSTP_MSTI_PORT *msti_port);
extern void mstputil_extract_cist_msg_info(PORT_ID port_number, MSTP_BPDU *bpdu);
extern void mstputil_extract_msti_msg_info(PORT_ID port_number, MSTP_BPDU *bpdu, MSTI_CONFIG_MESSAGE *msg);
extern void mstputil_network_order_bpdu(MSTP_BPDU *bpdu);
extern void mstputil_host_order_bpdu(MSTP_BPDU *bpdu);
extern void mstputil_transmit_bpdu(PORT_ID port_number, enum STP_BPDU_TYPE type);
extern void mstputil_update_stats(PORT_ID port_number, MSTP_BPDU *bpdu, bool rx);
extern void mstputil_update_mst_stats(PORT_ID port_number, MSTP_MSTI_PORT *msti_port, bool rx);
extern bool mstputil_validate_bpdu(MSTP_BPDU *bpdu, UINT16 size);
extern bool mstputil_set_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE state);
extern bool mstputil_flush(MSTP_INDEX mstp_index, PORT_ID port_number);
extern void mstputil_timer_tick();
extern void mstputil_clear_timer(UINT16 * timer);
extern bool mstputil_set_kernel_bridge_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE state);
extern bool mstputil_set_kernel_bridge_port_state_for_single_vlan(VLAN_ID vlan_id, PORT_ID port_number, enum L2_PORT_STATE state);
extern void mstputil_timer_sync_db(MSTP_INDEX mstp_index);
extern void mstputil_timer_sync_bpdu_counters();
extern bool mstputil_is_same_bridge_priority(MSTP_BRIDGE_IDENTIFIER *id, UINT16 priority);
extern void mstputil_sync_mst_port(MSTP_INDEX mstp_index, PORT_ID port_number);
extern bool mstputil_is_member_port_of_instance(MSTP_INDEX mstp_index, PORT_ID port_id);
extern UINT16 l2_proto_get_first_instance(L2_PROTO_INSTANCE_MASK *mask);
extern UINT16 l2_proto_get_next_instance(L2_PROTO_INSTANCE_MASK *mask, UINT16 index);
extern bool l2_proto_mask_is_clear(L2_PROTO_INSTANCE_MASK *mask);
extern bool l2_proto_mask_to_string(L2_PROTO_INSTANCE_MASK *mask, UINT8 *str, UINT32 maxlen);

/* mstp_mgr.c */
extern bool mstpmgr_init();
extern bool mstpmgr_rx_bpdu(VLAN_ID vlan_id, PORT_ID port_number, void * bufptr, UINT32 size);
extern bool mstpmgr_set_control_mask(MSTP_INDEX mstp_index);
extern bool mstpmgr_enable_port(PORT_ID port_number);
extern bool mstpmgr_disable_port(PORT_ID port_number, bool del_flag);
extern bool mstpmgr_refresh(MSTP_INDEX input_index, bool trigger_bpdu);
extern bool mstpmgr_config_instance_vlanmask(MSTP_MSTID mstid, VLAN_MASK *vlanmask);
extern void mstpmgr_process_bridge_config_msg(void *msg);
extern void mstpmgr_port_event(PORT_ID port_number, bool up);
extern void mstpmgr_process_inst_vlan_config_msg(void *msg);
extern void mstpmgr_process_vlan_mem_config_msg(void *msg);
extern void mstpmgr_process_intf_config_msg(void *msg);
extern void mstpmgr_process_mstp_global_config_msg(void *msg);
extern void mstpmgr_update_vlan_port_mask(void *msg);
extern void mstpmgr_process_inst_port_config_msg(void *msg);
extern bool mstpmgr_add_control_port(PORT_ID port_id);
extern bool mstpmgr_delete_control_port(PORT_ID port_id, bool flag);
extern bool mstpmgr_config_root_protect(PORT_ID port_number, bool enable);
extern void mstpmgr_reset_bpdu_guard_active(PORT_ID port_id);
extern bool mstpmgr_is_bpdu_rcvd_on_bpdu_guard_enabled_port(PORT_ID port_id);
extern void mstpmgr_clear_statistics_all();
extern void mstpmgr_clear_port_statistics(PORT_ID port_number);
extern bool mstpmgr_config_msti_priority(MSTP_MSTID mstid, UINT16 priority);
extern bool mstpmgr_refresh_all();
extern void mstpmgr_instance_init_port_state_machines(MSTP_INDEX mstp_index, PORT_ID port_number);

#endif //  __MSTP_EXTERNS_H__