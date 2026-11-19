/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * See stpd_driver.h.
 */

#include "stp_inc.h"
#include "stp_main.h"

#include "stpd_driver.h"

/* stpd reads tagging mode 0 as untagged. */
#define STPTEST_UNTAGGED_MODE 0

/* What stpmgrd would announce in STP_INIT_READY. */
#define STPTEST_MAX_STP_INSTANCES 64

/* In stp/stp_intf.c, but not exported by include/stp_intf.h. */
extern int stp_intf_init_port_stats(void);
extern int stp_intf_init_po_id_pool(void);

/* Both ends of the datagram pair that stands in for the stpmgrd socket. */
static int g_ipc_tx = -1;
static int g_ipc_rx = -1;

static char g_port_names[STPTEST_NUM_PORTS][IFNAMSIZ];

static const uint8_t g_base_mac[L2_ETH_ADD_LEN] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

/* Registers a port through the callback the netlink reader uses, so
 * port-id derivation follows the production code. */
static void stptest_add_port(uint32_t idx)
{
    netlink_db_t if_db;

    memset(&if_db, 0, sizeof(if_db));
    snprintf(if_db.ifname, IFNAMSIZ, "Ethernet%u", idx);
    if_db.kif_index = idx + 1;
    if_db.oper_state = 1;
    if_db.speed = 100000;
    memcpy(if_db.mac, g_base_mac, L2_ETH_ADD_LEN);

    stp_intf_netlink_cb(&if_db, 1 /* is_add */, true /* init_in_prog */);
}

void stptest_init(void)
{
    uint32_t i;
    int sv[2];

    if (g_stpd_intf_db)
        return;

    /* Logs go to syslog; keep the run quiet. */
    STP_LOG_SET_LEVEL(APP_LOG_LEVEL_NONE);

    memset(&stpd_context, 0, sizeof(STPD_CONTEXT));
    stpmgr_set_extend_mode(true);

    g_stpd_intf_db = avl_create(&stp_intf_avl_compare, NULL, NULL);
    sys_assert(g_stpd_intf_db != NULL);

    for (i = 0; i < STPTEST_NUM_PORTS; i++)
    {
        stptest_add_port(i);
        snprintf(g_port_names[i], IFNAMSIZ, "Ethernet%u", i);
    }

    /* stp_intf_event_mgr_init() doubles this to leave room for PO ids. */
    g_max_stp_port = g_max_stp_port * 2;
    sys_assert(g_max_stp_port != 0);

    stp_intf_init_port_stats();
    sys_assert(stp_intf_init_po_id_pool() == 0);
    g_stpd_port_init_done = 1;

    sys_assert(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) == 0);
    g_ipc_tx = sv[0];
    g_ipc_rx = sv[1];
}

void stptest_reset_protocol(void)
{
    stpmgr_init(STPTEST_MAX_STP_INSTANCES);
    sys_assert(mstpmgr_init());
}

const char *stptest_port_name(uint32_t idx)
{
    sys_assert(idx < STPTEST_NUM_PORTS);
    return g_port_names[idx];
}

void stptest_tick(unsigned int ticks)
{
    unsigned int i;

    for (i = 0; i < ticks; i++)
        stptimer_100ms_tick(-1, 0, NULL);
}

int stptest_protocol_enabled(void)
{
    return stp_global.enable ? 1 : 0;
}

/* Sends one message the way stpmgrd does, and lets the daemon's own read
 * callback pick it up. */
static void stptest_send_ipc(int msg_type, L2_PROTO_MODE proto_mode,
                             const void *data, size_t data_len)
{
    char buffer[4096];
    STP_IPC_MSG *msg = (STP_IPC_MSG *)buffer;
    size_t total = sizeof(STP_IPC_MSG) + data_len;
    ssize_t sent;

    sys_assert(total <= sizeof(buffer));

    memset(buffer, 0, total);
    msg->msg_type = msg_type;
    msg->msg_len = (unsigned int)data_len;
    msg->proto_mode = proto_mode;
    if (data_len)
        memcpy(msg->data, data, data_len);

    sent = send(g_ipc_tx, buffer, total, 0);
    sys_assert(sent == (ssize_t)total);

    stpmgr_recv_client_msg(g_ipc_rx, 0, NULL);
}

static void stptest_bridge_config(L2_PROTO_MODE mode, uint8_t opcode)
{
    STP_BRIDGE_CONFIG_MSG msg;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = opcode;
    msg.stp_mode = (uint8_t)mode;
    msg.rootguard_timeout = 30;
    memcpy(msg.base_mac_addr, g_base_mac, L2_ETH_ADD_LEN);

    stptest_send_ipc(STP_BRIDGE_CONFIG, mode, &msg, sizeof(msg));
}

void stptest_pvst_enable(void)
{
    stptest_bridge_config(L2_PVSTP, STP_SET_COMMAND);
}

void stptest_pvst_disable(void)
{
    stptest_bridge_config(L2_PVSTP, STP_DEL_COMMAND);
}

void stptest_mstp_enable(void)
{
    stptest_bridge_config(L2_MSTP, STP_SET_COMMAND);
}

void stptest_mstp_disable(void)
{
    stptest_bridge_config(L2_MSTP, STP_DEL_COMMAND);
}

static void stptest_pvst_add_vlan_impl(uint16_t vlan_id, uint16_t inst_id, int with_members);

void stptest_pvst_add_vlan(uint16_t vlan_id, uint16_t inst_id)
{
    stptest_pvst_add_vlan_impl(vlan_id, inst_id, 1);
}

void stptest_pvst_add_empty_vlan(uint16_t vlan_id, uint16_t inst_id)
{
    stptest_pvst_add_vlan_impl(vlan_id, inst_id, 0);
}

static void stptest_pvst_add_vlan_impl(uint16_t vlan_id, uint16_t inst_id, int with_members)
{
    char buffer[sizeof(STP_VLAN_CONFIG_MSG) + STPTEST_NUM_PORTS * sizeof(PORT_ATTR)];
    STP_VLAN_CONFIG_MSG *msg = (STP_VLAN_CONFIG_MSG *)buffer;
    uint32_t i;

    memset(buffer, 0, sizeof(buffer));
    msg->opcode = STP_SET_COMMAND;
    msg->newInstance = 1;
    msg->vlan_id = vlan_id;
    msg->inst_id = inst_id;
    msg->forward_delay = STP_DFLT_FORWARD_DELAY;
    msg->hello_time = STP_DFLT_HELLO_TIME;
    msg->max_age = STP_DFLT_MAX_AGE;
    msg->priority = STP_DFLT_PRIORITY;
    msg->count = with_members ? STPTEST_NUM_PORTS : 0;

    for (i = 0; i < msg->count; i++)
    {
        snprintf(msg->port_list[i].intf_name, IFNAMSIZ, "%.*s", IFNAMSIZ - 1, g_port_names[i]);
        msg->port_list[i].enabled = 1;
        msg->port_list[i].mode = STPTEST_UNTAGGED_MODE;
    }

    stptest_send_ipc(STP_VLAN_CONFIG, L2_PVSTP, buffer, sizeof(buffer));
}

void stptest_pvst_port_config(const char *ifname, uint16_t inst_id, int enabled, int portfast)
{
    stptest_pvst_port_config_tagging(ifname, inst_id, enabled, portfast, STPTEST_UNTAGGED_MODE);
}

static void stptest_pvst_port_config_full(const char *ifname, uint16_t inst_id, int enabled,
                                          int portfast, int mode, int bpdu_guard);

void stptest_pvst_port_config_tagging(const char *ifname, uint16_t inst_id, int enabled,
                                      int portfast, int mode)
{
    stptest_pvst_port_config_full(ifname, inst_id, enabled, portfast, mode, 1);
}

void stptest_pvst_port_config_unguarded(const char *ifname, uint16_t inst_id, int enabled)
{
    stptest_pvst_port_config_full(ifname, inst_id, enabled, 0, STPTEST_UNTAGGED_MODE, 0);
}

static void stptest_pvst_port_config_full(const char *ifname, uint16_t inst_id, int enabled,
                                          int portfast, int mode, int bpdu_guard)
{
    char buffer[sizeof(STP_PORT_CONFIG_MSG) + sizeof(VLAN_ATTR)];
    STP_PORT_CONFIG_MSG *msg = (STP_PORT_CONFIG_MSG *)buffer;

    memset(buffer, 0, sizeof(buffer));
    msg->opcode = STP_SET_COMMAND;
    snprintf(msg->intf_name, IFNAMSIZ, "%s", ifname);
    msg->enabled = enabled ? 1 : 0;
    msg->bpdu_guard = bpdu_guard ? 1 : 0;
    msg->portfast = portfast ? 1 : 0;
    msg->path_cost = 0;
    msg->priority = -1;
    msg->count = 1;
    msg->vlan_list[0].inst_id = inst_id;
    msg->vlan_list[0].mode = (int8_t) mode;

    stptest_send_ipc(STP_PORT_CONFIG, L2_PVSTP, buffer, sizeof(buffer));
}

void stptest_pvst_vlan_member_tagging(uint16_t vlan_id, uint16_t inst_id,
                                      const char *ifname, int mode)
{
    STP_VLAN_MEM_CONFIG_MSG msg;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = STP_SET_COMMAND;
    msg.vlan_id = vlan_id;
    msg.inst_id = inst_id;
    snprintf(msg.intf_name, IFNAMSIZ, "%s", ifname);
    msg.enabled = 1;
    msg.mode = (int8_t) mode;
    msg.path_cost = 0;
    msg.priority = -1;

    stptest_send_ipc(STP_VLAN_MEM_CONFIG, L2_PVSTP, &msg, sizeof(msg));
}

void stptest_pvst_vlan_member(uint16_t vlan_id, uint16_t inst_id, const char *ifname)
{
    stptest_pvst_vlan_member_tagging(vlan_id, inst_id, ifname, STPTEST_UNTAGGED_MODE);
}

int stptest_pvst_port_is_untagged(uint16_t vlan_id, const char *ifname)
{
    PORT_ID port_id = stp_intf_get_port_id_by_name((char *) ifname);

    if (port_id == BAD_PORT_ID)
        return 0;

    return stputil_is_port_untag(vlan_id, port_id) ? 1 : 0;
}

void stptest_mstp_global_config(void)
{
    STP_MST_GLOBAL_CONFIG_MSG msg;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = STP_SET_COMMAND;
    msg.revision_number = 1;
    snprintf(msg.name, STP_SYNC_MSTP_NAME_LEN, "%s", "test-region");
    msg.forward_delay = MSTP_DFLT_FORWARD_DELAY;
    msg.hello_time = MSTP_DFLT_HELLO_TIME;
    msg.max_age = MSTP_DFLT_MAX_AGE;
    msg.max_hop = MSTP_DFLT_MAX_HOPS;

    stptest_send_ipc(STP_MST_GLOBAL_CONFIG, L2_MSTP, &msg, sizeof(msg));
}

void stptest_mstp_add_instance(uint16_t mst_id, const uint16_t *vlans, uint16_t vlan_count)
{
    char buffer[sizeof(MST_INST_CONFIG_MSG) + 64 * sizeof(VLAN_LIST)];
    MST_INST_CONFIG_MSG *msg = (MST_INST_CONFIG_MSG *)buffer;
    uint16_t i;

    sys_assert(vlan_count <= 64);

    memset(buffer, 0, sizeof(buffer));
    msg->opcode = STP_SET_COMMAND;
    msg->mst_id = mst_id;
    msg->priority = MSTP_DFLT_PRIORITY;
    msg->vlan_count = vlan_count;
    for (i = 0; i < vlan_count; i++)
        msg->vlan_list[i].vlan_id = vlans[i];

    stptest_send_ipc(STP_MST_INST_CONFIG, L2_MSTP, buffer,
                     sizeof(MST_INST_CONFIG_MSG) + vlan_count * sizeof(VLAN_LIST));
}

void stptest_mstp_vlan_ports(uint16_t vlan_id, int add)
{
    char buffer[sizeof(STP_MST_VLAN_PORT_MAP) + STPTEST_NUM_PORTS * sizeof(PORT_LIST)];
    STP_MST_VLAN_PORT_MAP *msg = (STP_MST_VLAN_PORT_MAP *)buffer;
    uint32_t i;

    memset(buffer, 0, sizeof(buffer));
    msg->vlan_id = vlan_id;
    msg->port_count = STPTEST_NUM_PORTS;
    msg->stp_mode = L2_MSTP;
    msg->add = add ? 1 : 0;
    for (i = 0; i < STPTEST_NUM_PORTS; i++)
    {
        snprintf(msg->port_list[i].intf_name, IFNAMSIZ, "%.*s", IFNAMSIZ - 1, g_port_names[i]);
        msg->port_list[i].tagging_mode = STPTEST_UNTAGGED_MODE;
    }

    stptest_send_ipc(STP_MST_VLAN_PORT_LIST_CONFIG, L2_MSTP, buffer, sizeof(buffer));
}

void stptest_mstp_vlan_port(uint16_t vlan_id, const char *ifname, int mode, int add)
{
    char buffer[sizeof(STP_MST_VLAN_PORT_MAP) + sizeof(PORT_LIST)];
    STP_MST_VLAN_PORT_MAP *msg = (STP_MST_VLAN_PORT_MAP *)buffer;

    memset(buffer, 0, sizeof(buffer));
    msg->vlan_id = vlan_id;
    msg->port_count = 1;
    msg->stp_mode = L2_MSTP;
    msg->add = add ? 1 : 0;
    snprintf(msg->port_list[0].intf_name, IFNAMSIZ, "%s", ifname);
    msg->port_list[0].tagging_mode = (int8_t) mode;

    stptest_send_ipc(STP_MST_VLAN_PORT_LIST_CONFIG, L2_MSTP, buffer, sizeof(buffer));
}

void stptest_mstp_port_config(const char *ifname, int enabled)
{
    STP_PORT_CONFIG_MSG msg;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = STP_SET_COMMAND;
    snprintf(msg.intf_name, IFNAMSIZ, "%s", ifname);
    msg.enabled = enabled ? 1 : 0;
    msg.bpdu_guard = 1;
    msg.edge = 1;
    msg.link_type = AUTO;
    msg.path_cost = 0;
    msg.priority = -1;
    msg.count = 0;

    stptest_send_ipc(STP_PORT_CONFIG, L2_MSTP, &msg, sizeof(msg));
}

void stptest_mstp_inst_port_config(const char *ifname, uint16_t mst_id)
{
    STP_MST_INST_PORT_CONFIG_MSG msg;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = STP_SET_COMMAND;
    snprintf(msg.intf_name, IFNAMSIZ, "%s", ifname);
    msg.mst_id = mst_id;
    msg.path_cost = 0;
    msg.priority = -1;

    stptest_send_ipc(STP_MST_INST_PORT_CONFIG, L2_MSTP, &msg, sizeof(msg));
}

/* --- received BPDUs -------------------------------------------------------
 * Frames are laid out byte by byte in wire order, not through the daemon's
 * bit-field structs, so the test says exactly what is on the wire. */

static void stptest_put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void stptest_put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* Ethernet header plus the Cisco SNAP header PVST carries; returns the
 * offset of the BPDU body. */
static size_t stptest_pvst_frame_head(uint8_t *f, const uint8_t src_mac[6], size_t body_len)
{
    static const uint8_t pvst_dst[6] = {0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcd};
    static const uint8_t snap[8] = {LSAP_SNAP_LLC, LSAP_SNAP_LLC, UNNUMBERED_INFORMATION,
                                    0x00, 0x00, 0x0c, 0x01, 0x0b};
    memcpy(f, pvst_dst, 6);
    memcpy(f + 6, src_mac, 6);
    stptest_put16(f + 12, (uint16_t)(sizeof(snap) + body_len));
    memcpy(f + 14, snap, sizeof(snap));
    return 14 + sizeof(snap);
}

static PORT_ID stptest_port_id(const char *ifname)
{
    PORT_ID port_id = stp_intf_get_port_id_by_name((char *)ifname);
    sys_assert(port_id != BAD_PORT_ID);
    return port_id;
}

void stptest_pvst_rx_config_bpdu(const char *ifname, uint16_t vlan_id, uint16_t root_priority,
                                 const uint8_t root_mac[6], uint32_t root_path_cost, int tc)
{
    uint8_t frame[STP_MAX_PKT_LEN];
    /* protocol id, version, type, flags, root id, cost, bridge id, port id,
     * four timers, three pad bytes, then the PVST VLAN TLV */
    const size_t body_len = 2 + 1 + 1 + 1 + 8 + 4 + 8 + 2 + 8 + 3 + 4;
    uint8_t *b;

    memset(frame, 0, sizeof(frame));
    b = frame + stptest_pvst_frame_head(frame, root_mac, body_len);
    stptest_put16(b, 0);                       /* protocol id */
    b[2] = 0;                                  /* version */
    b[3] = CONFIG_BPDU_TYPE;
    b[4] = tc ? 0x01 : 0x00;                   /* flags: topology change */
    stptest_put16(b + 5, (uint16_t)(root_priority | vlan_id));   /* root id */
    memcpy(b + 7, root_mac, 6);
    stptest_put32(b + 13, root_path_cost);
    stptest_put16(b + 17, (uint16_t)(root_priority | vlan_id));  /* bridge id: the root itself */
    memcpy(b + 19, root_mac, 6);
    stptest_put16(b + 25, 0x8001);             /* port id: priority 128, port 1 */
    stptest_put16(b + 27, 0);                  /* message age */
    stptest_put16(b + 29, STP_DFLT_MAX_AGE << 8);
    stptest_put16(b + 31, STP_DFLT_HELLO_TIME << 8);
    stptest_put16(b + 33, STP_DFLT_FORWARD_DELAY << 8);
    stptest_put16(b + 38, 2);                  /* TLV length */
    stptest_put16(b + 40, vlan_id);

    stpmgr_process_rx_bpdu(vlan_id, stptest_port_id(ifname), frame);
}

void stptest_pvst_rx_tcn_bpdu(const char *ifname, uint16_t vlan_id)
{
    static const uint8_t peer_mac[6] = {0x00, 0x00, 0x5e, 0x00, 0x53, 0x01};
    uint8_t frame[STP_MAX_PKT_LEN];
    const size_t body_len = 2 + 1 + 1 + 38;
    uint8_t *b;

    memset(frame, 0, sizeof(frame));
    b = frame + stptest_pvst_frame_head(frame, peer_mac, body_len);
    stptest_put16(b, 0);
    b[2] = 0;
    b[3] = TCN_BPDU_TYPE;

    stpmgr_process_rx_bpdu(vlan_id, stptest_port_id(ifname), frame);
}

static STP_CLASS *stptest_pvst_class(uint16_t vlan_id)
{
    STP_INDEX stp_index = STP_INDEX_INVALID;
    stputil_get_index_from_vlan(vlan_id, &stp_index);
    sys_assert(stp_index != STP_INDEX_INVALID);
    return GET_STP_CLASS(stp_index);
}

int stptest_pvst_is_root(uint16_t vlan_id)
{
    return root_bridge(stptest_pvst_class(vlan_id)) ? 1 : 0;
}

int stptest_pvst_root_port_is(uint16_t vlan_id, const char *ifname)
{
    return stptest_pvst_class(vlan_id)->bridge_info.root_port == stptest_port_id(ifname) ? 1 : 0;
}

int stptest_pvst_topology_change(uint16_t vlan_id)
{
    return stptest_pvst_class(vlan_id)->bridge_info.topology_change ? 1 : 0;
}

int stptest_pvst_port_state(uint16_t vlan_id, const char *ifname)
{
    STP_CLASS *stp_class = stptest_pvst_class(vlan_id);
    STP_PORT_CLASS *stp_port = GET_STP_PORT_CLASS(stp_class, stptest_port_id(ifname));
    return stp_port ? (int)stp_port->state : -1;
}

void stptest_port_up(const char *ifname)
{
    stpmgr_port_event(stptest_port_id(ifname), true);
}

void stptest_port_down(const char *ifname)
{
    stpmgr_port_event(stptest_port_id(ifname), false);
}

unsigned int stptest_pvst_drop_count(void)
{
    return (unsigned int)stp_global.pvst_drop_count;
}

unsigned int stptest_path_cost(unsigned int speed_mbps, int extend)
{
    return stputil_get_path_cost((STP_PORT_SPEED) speed_mbps, extend ? true : false);
}
