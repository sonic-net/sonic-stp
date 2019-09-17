/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#ifndef __STP_EXTERNS_H__
#define __STP_EXTERNS_H__

/* variable declarations */
extern struct STPD_CONTEXT stpd_context;
extern STP_GLOBAL stp_global;
extern uint32_t g_max_stp_port;
extern uint16_t g_stp_bmp_po_offset;
struct timeval g_stp_100ms_tv;
extern MAC_ADDRESS bridge_group_address;
extern MAC_ADDRESS pvst_bridge_group_address;
extern MAC_ADDRESS g_stp_base_mac_addr;

/* stp.c */
extern void transmit_config(STP_CLASS *stp_class, PORT_ID port_number);
extern bool supercedes_port_info(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu);
extern void record_config_information(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu);
extern void record_config_timeout_values(STP_CLASS *stp_class, STP_CONFIG_BPDU *bpdu);
extern void config_bpdu_generation (STP_CLASS *stp_class);
extern void reply (STP_CLASS *stp_class, PORT_ID port_number);
extern void transmit_tcn(STP_CLASS *stp_class);
extern void configuration_update(STP_CLASS *stp_class);
extern void root_selection(STP_CLASS *stp_class);
extern void designated_port_selection(STP_CLASS *stp_class);
extern void become_designated_port(STP_CLASS *stp_class, PORT_ID port_number);
extern void port_state_selection(STP_CLASS *stp_class);
extern void make_forwarding(STP_CLASS *stp_class, PORT_ID port_number);
extern void make_blocking(STP_CLASS *stp_class, PORT_ID port_number);
extern void topology_change_detection(STP_CLASS *stp_class);
extern void topology_change_acknowledged(STP_CLASS *stp_class);
extern void acknowledge_topology_change(STP_CLASS *stp_class, PORT_ID port_number);
extern void received_config_bpdu(STP_CLASS *stp_class, PORT_ID port_number, STP_CONFIG_BPDU *bpdu);
extern void received_tcn_bpdu(STP_CLASS *stp_class, PORT_ID port_number, STP_TCN_BPDU *bpdu);
extern void hello_timer_expiry(STP_CLASS *stp_class);
extern void message_age_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number);
extern void forwarding_delay_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number);
extern void tcn_timer_expiry(STP_CLASS *stp_class);
extern void topology_change_timer_expiry(STP_CLASS * stp_class);
extern void hold_timer_expiry(STP_CLASS *stp_class, PORT_ID port_number);
extern bool root_bridge (STP_CLASS *stp_class);
extern bool designated_port(STP_CLASS *stp_class, PORT_ID port_number);
extern bool designated_for_some_port(STP_CLASS *stp_class);
extern void send_config_bpdu(STP_CLASS* stp_class, PORT_ID port_number);
extern void send_tcn_bpdu(STP_CLASS* stp_class, PORT_ID port_number);

/* stp_mgr.c */
extern void stpmgr_init(UINT16 max_stp_instances);
extern void stpmgr_initialize_stp_class(STP_CLASS *stp_class, VLAN_ID vlan_id);
extern void stpmgr_initialize_control_port(STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_activate_stp_class(STP_CLASS *stp_class);
extern void stpmgr_deactivate_stp_class(STP_CLASS *stp_class);
extern void stpmgr_initialize_port(STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_enable_port(STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_disable_port (STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_set_bridge_priority (STP_CLASS *stp_class, BRIDGE_IDENTIFIER *bridge_id);
extern void stpmgr_set_port_priority(STP_CLASS *stp_class, PORT_ID port_number, UINT16 priority);
extern void stpmgr_set_path_cost(STP_CLASS *stp_class, PORT_ID port_number, bool auto_config, UINT32 path_cost);
extern void stpmgr_enable_change_detection(STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_disable_change_detection(STP_CLASS *stp_class, PORT_ID port_number);
extern void stpmgr_set_bridge_params(STP_CLASS *stp_class);
extern bool stpmgr_config_bridge_priority(STP_INDEX stp_index, UINT16 priority);
extern bool stpmgr_config_bridge_max_age(STP_INDEX stp_index, UINT16 max_age);
extern bool stpmgr_config_bridge_hello_time(STP_INDEX stp_index, UINT16 hello_time);
extern bool stpmgr_config_bridge_forward_delay(STP_INDEX stp_index, UINT16 fwd_delay);
extern bool stpmgr_config_port_priority(STP_INDEX stp_index, PORT_ID port_number, UINT16 priority, bool is_global);
extern bool stpmgr_config_port_path_cost(STP_INDEX stp_index, PORT_ID port_number, bool auto_config, 
        UINT32 path_cost, bool is_global);
extern bool stpmgr_release_index(STP_INDEX stp_index);
extern bool stpmgr_add_control_port(STP_INDEX stp_index, PORT_ID port_number, uint8_t mode);
extern bool stpmgr_delete_control_port(STP_INDEX stp_index, PORT_ID port_number, bool del_stp_port);
extern bool stpmgr_add_enable_port(STP_INDEX stp_index, PORT_ID port_number);
extern bool stpmgr_delete_enable_port(STP_INDEX stp_index, PORT_ID port_number);
extern void stpmgr_process_stp_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer);
extern void stpmgr_process_pvst_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer);
extern void stpmgr_config_fastuplink(PORT_ID port_number, bool enable);
extern void stpmgr_set_extend_mode(bool enable);
extern void stpmgr_port_event(PORT_ID port_number, bool up);
extern void stpmgr_100ms_timer(evutil_socket_t fd, short what, void *arg);
extern void stpmgr_recv_client_msg(evutil_socket_t fd, short what, void *arg);
extern struct event *stpmgr_libevent_create(struct event_base *base, evutil_socket_t sock, short flags, 
        void *cb_fn, void *arg, const struct timeval *timeout);
extern void stpmgr_process_rx_bpdu(uint16_t vlan_id, uint32_t port_id, unsigned char *pkt);
extern void stpmgr_libevent_destroy(struct event *ev);
extern void stpmgr_clear_statistics(VLAN_ID vlan_id, PORT_ID port_number);

/* stp_util.c */
extern bool stputil_is_protocol_enabled(L2_PROTO_MODE proto_mode);
extern bool stputil_is_port_untag(VLAN_ID vlan_id, PORT_ID port_id);
extern void stputil_bridge_to_string(BRIDGE_IDENTIFIER *bridge_id, UINT8 *buffer, UINT16 size);
extern UINT32 stputil_get_default_path_cost(PORT_ID port_number, bool extend);
extern UINT32 stputil_get_path_cost(STP_PORT_SPEED port_speed, bool extend);
extern void stputil_set_vlan_topo_change(STP_CLASS *stp_class);
extern bool stputil_set_port_state(STP_CLASS * stp_class, STP_PORT_CLASS * stp_port_class);
extern bool stputil_get_index_from_vlan(VLAN_ID vlan_id, STP_INDEX *stp_index);
extern enum SORT_RETURN stputil_compare_mac(MAC_ADDRESS *mac1, MAC_ADDRESS *mac2);
extern enum SORT_RETURN stputil_compare_bridge_id(BRIDGE_IDENTIFIER *id1, BRIDGE_IDENTIFIER *id2);
extern enum SORT_RETURN stputil_compare_port_id(PORT_IDENTIFIER *port_id1,PORT_IDENTIFIER *port_id2);
extern UINT16 stputil_get_bridge_priority(BRIDGE_IDENTIFIER * id);
extern void stputil_set_bridge_priority(BRIDGE_IDENTIFIER * id, UINT16 priority, VLAN_ID vlan_id);
extern bool stputil_is_same_bridge_priority(BRIDGE_IDENTIFIER * id1, UINT16 priority);
extern bool stputil_validate_bpdu(STP_CONFIG_BPDU *bpdu);
extern bool stputil_validate_pvst_bpdu(PVST_CONFIG_BPDU *bpdu);
extern void stputil_encode_bpdu(STP_CONFIG_BPDU *bpdu);
extern void stputil_decode_bpdu(STP_CONFIG_BPDU *bpdu);
extern void stputil_send_bpdu(STP_CLASS* stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type);
extern void stputil_send_pvst_bpdu(STP_CLASS* stp_class, PORT_ID port_number, enum STP_BPDU_TYPE type);
extern void stputil_process_bpdu(STP_INDEX stp_index, PORT_ID port_number, void * buffer);
extern void stputil_update_mask(PORT_MASK *mask, PORT_ID port_number, bool add);
extern bool stputil_start_periodic_timer();
extern bool stputil_stop_periodic_timer();
extern void stputil_set_global_enable_mask(PORT_ID port_id, uint8_t add);
extern void stptimer_tick();
extern void stptimer_update(STP_CLASS *stp_class);
extern void stputil_sync_port_counters(STP_CLASS *stp_class, STP_PORT_CLASS * stp_port);
extern void stptimer_sync_db(STP_CLASS *stp_class);
extern void stptimer_start(TIMER *sptr_timer, UINT32 start_value_in_seconds);
extern void stptimer_stop(TIMER *sptr_timer);
extern bool stptimer_expired(TIMER *timer, UINT32 timer_limit_in_seconds);
extern bool stptimer_is_active(TIMER * timer);
extern int mask_to_string(BITMAP_T *bmp, uint8_t *str, uint32_t maxlen);



/* stp_data.c */
extern bool stpdata_init_global_structures(UINT16 max_instances);
extern bool stpdata_deinit_global_structures();
extern bool stpdata_malloc_port_structures();
extern void stpdata_free_port_structures();
extern int stpdata_init_class(STP_INDEX stp_index, VLAN_ID vlan_id);
extern void stpdata_class_free(STP_INDEX stp_index);
extern void stpdata_init_bpdu_structures();
extern int stpdata_init_debug_structures(void);
extern STP_PORT_CLASS* stpdata_get_port_class(STP_CLASS *stp_class, PORT_ID port_number);

/* stp_debug.c */
extern UINT8 stp_log_msg_src_string[][20];
extern void stpdebug_display_bpdu(STP_CONFIG_BPDU *bpdu, bool verbose,bool rx_flag);
extern void stpdm_class(STP_CLASS *stp_class);
extern void stpdm_port_class(STP_CLASS *stp_class, PORT_ID port_number); 
extern void stpdm_global();
extern void stpdm_clear();

extern void stp_show_debug_log (UINT16 instance_id, UINT16 print_count, UINT8 print_all);
extern void stp_show_debug_log_all (UINT16 print_count);
extern void stp_clear_debug_log (UINT16 instance_id);
extern void stp_clear_debug_log_all ();
extern void stp_debug_log (UINT16 stp_instance, PORT_ID port_number, UINT16 vlan_id, STP_RAS_EVENTS event_type, UINT8* trigger_event);
extern bool stp_create_debug_instance (UINT16 instance_id);
extern void stp_delete_debug_instance (UINT16 instance_id);
extern char* l2_port_state_to_string(uint8_t state, uint32_t port);

//stp_intf.h
extern int stp_intf_get_netlink_fd();
extern char * stp_intf_get_port_name(uint32_t port_id);
extern bool stp_intf_is_port_up(int port_id);
extern uint32_t stp_intf_get_speed(int port_id);
extern INTERFACE_NODE *stp_intf_get_node(uint32_t port_id);
extern INTERFACE_NODE *stp_intf_get_node_by_kif_index(uint32_t kif_index);
extern uint32_t stp_intf_get_kif_index(uint32_t port_id);
extern bool stp_intf_get_mac(int port_id, MAC_ADDRESS *mac);
extern uint32_t stp_intf_get_port_id_by_kif_index(uint32_t kif_index);
extern uint32_t stp_intf_get_kif_index_by_port_id(uint32_t port_id);
extern uint32_t stp_intf_get_port_id_by_name(char *ifname);
extern INTERFACE_NODE *stp_intf_get_node_by_name(char *ifname);
extern struct event_base *stp_intf_get_evbase();
extern int stp_intf_event_mgr_init(void);
extern int stp_intf_avl_compare(const void *user_p, const void *data_p, void *param);
extern void stp_intf_netlink_cb(struct netlink_db_s *if_db, uint8_t is_add, bool init_in_prog);
extern void stp_intf_reset_port_params();

extern void sys_assert(int status);

//stp_netlink.c
extern int stp_set_sock_buf_size(int sock, int optname, int size);
extern int stpdbg_init();
extern void stpdbg_process_msg(void *msg);

extern void stp_pkt_sock_close(INTERFACE_NODE *intf_node);
extern int stp_pkt_sock_create(INTERFACE_NODE *intf_node);
extern void stp_pkt_rx_handler (evutil_socket_t fd, short what, void *arg);
extern int stp_pkt_tx_handler ( uint32_t kif_index, VLAN_ID vlan_id, char *buffer, uint16_t size, bool tagged);
extern void stpdbg_process_ctl_msg(void *msg);
extern PORT_ID stp_intf_handle_po_preconfig(char * ifname);
extern bool stputil_set_kernel_bridge_port_state(STP_CLASS * stp_class, STP_PORT_CLASS * stp_port_class);
#endif //__STP_EXTERNS_H__
