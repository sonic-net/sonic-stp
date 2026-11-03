/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * C driver for the stpd protocol core, used by the gtest binary.
 *
 * stpd_main() needs netlink, a packet socket, an AF_UNIX listener and a
 * libevent loop, none of which config handling touches. This stands up
 * only the state stpmgr_process_ipc_msg() depends on, then feeds it real
 * STP_IPC_MSG buffers through the daemon's own receive callback.
 *
 * It is C so the daemon's headers never get compiled as C++.
 */

#ifndef __STPD_DRIVER_H__
#define __STPD_DRIVER_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ethernet0..Ethernet3. Kept small: MSTP teardown walks every
 * (port, vlan) pair. */
#define STPTEST_NUM_PORTS 4

/* Builds the interface DB and port-stat array. Idempotent. */
void stptest_init(void);

/* Fresh PVST and MSTP global structures, as STP_INIT_READY would create. */
void stptest_reset_protocol(void);

/* "Ethernet<n>", 0 <= idx < STPTEST_NUM_PORTS. */
const char *stptest_port_name(uint32_t idx);

/* PVST config messages. */
void stptest_pvst_enable(void);
void stptest_pvst_disable(void);
void stptest_pvst_add_vlan(uint16_t vlan_id, uint16_t inst_id);

/* Same, with no members, so a test can enrol ports itself. */
void stptest_pvst_add_empty_vlan(uint16_t vlan_id, uint16_t inst_id);
void stptest_pvst_port_config(const char *ifname, uint16_t inst_id, int enabled, int portfast);

/* Same, but chooses the tagging mode carried in the per-VLAN attribute. */
void stptest_pvst_port_config_tagging(const char *ifname, uint16_t inst_id, int enabled,
                                      int portfast, int mode);
void stptest_pvst_vlan_member(uint16_t vlan_id, uint16_t inst_id, const char *ifname);
/* Port config with BPDU guard off, for tests that feed the port BPDUs. The
 * other port_config helpers enable the guard, as the CLI defaults do. */
void stptest_pvst_port_config_unguarded(const char *ifname, uint16_t inst_id, int enabled);
/* BPDU guard with --shutdown: a BPDU err-disables the port. */
void stptest_pvst_port_config_guard_shutdown(const char *ifname, uint16_t inst_id, int enabled);

/* Same, with the tagging mode: 0 untagged, 1 tagged. */
void stptest_pvst_vlan_member_tagging(uint16_t vlan_id, uint16_t inst_id,
                                      const char *ifname, int mode);

/* Nonzero when BPDUs would leave this port untagged. Reads the same
 * predicate the transmit path uses, so it answers what goes on the wire. */
int stptest_pvst_port_is_untagged(uint16_t vlan_id, const char *ifname);

/* MSTP config messages. */
void stptest_mstp_enable(void);
void stptest_mstp_disable(void);
void stptest_mstp_global_config(void);
void stptest_mstp_add_instance(uint16_t mst_id, const uint16_t *vlans, uint16_t vlan_count);
/* One instance record as stpmgrd sends it: SET or DEL, with a priority. */
void stptest_mstp_instance_msg(uint16_t mst_id, int set, int priority,
                               const uint16_t *vlans, uint16_t vlan_count);
/* The instance's bridge priority (0 is the CIST), -1 if it does not exist. */
int stptest_mstp_bridge_priority(uint16_t mst_id);
void stptest_mstp_vlan_ports(uint16_t vlan_id, int add);
/* One port's membership of the VLAN, with its tagging mode: 0 untagged, 1 tagged. */
void stptest_mstp_vlan_port(uint16_t vlan_id, const char *ifname, int mode, int add);
/* One instance message mapping `count` consecutive VLANs from `first_vlan`. */
void stptest_mstp_add_instance_range(uint16_t mst_id, uint16_t first_vlan, uint16_t count);
/* How many VLANs the daemon holds in the instance, 0 if it does not exist. */
unsigned int stptest_mstp_instance_vlan_count(uint16_t mst_id);
void stptest_mstp_port_config(const char *ifname, int enabled);
void stptest_mstp_inst_port_config(const char *ifname, uint16_t mst_id);
/* Recomputes the MST configuration digest and copies its 16 bytes to `out`. */
void stptest_mstp_config_digest(uint8_t *out);

/* Runs the daemon's 100ms timer callback n times. The VLAN and MST rows
 * are written by the periodic sync, so a scenario has to tick before
 * APPL_DB looks like a running switch. */
/* Malformed frames, for the IPC bounds tests. The messages are built here
 * rather than in the test so the daemon's headers stay out of C++.
 * Each sends a frame carrying room for one element while claiming more.
 */
void stptest_send_mst_instance_bogus_vlan_count(uint16_t mst_id, int claimed_count);
void stptest_send_mst_vlan_ports_bogus_port_count(uint16_t vlan_id, int claimed_count);
void stptest_send_pvst_vlan_bogus_port_count(uint16_t vlan_id, int claimed_count);

/* Well formed on the wire, but msg_len claims room for claimed_count. */
void stptest_send_mst_instance_inflated_msg_len(uint16_t mst_id, int claimed_count);

/* A payload too short for the message's own fixed part. */
void stptest_send_mst_instance_truncated(void);

/* A frame shorter than the IPC header itself. */
void stptest_send_short_frame(size_t total_bytes);

/* A msg_type outside the enum, which is used to index a name table. */
void stptest_send_unknown_msg_type(int msg_type);

void stptest_tick(unsigned int ticks);

/* Received BPDUs, handed to the daemon's receive path as the packet socket
 * callback would after the socket bookkeeping it does not need. */
/* A PVST configuration BPDU from a bridge with this root identity and path
 * cost, with the topology-change flag if tc is nonzero. */
void stptest_pvst_rx_config_bpdu(const char *ifname, uint16_t vlan_id, uint16_t root_priority,
                                 const uint8_t root_mac[6], uint32_t root_path_cost, int tc);
/* A PVST topology change notification. */
void stptest_pvst_rx_tcn_bpdu(const char *ifname, uint16_t vlan_id);
/* A CIST BPDU from a designated port of a bridge in another region arrives
 * on `ifname`: `active` sets the learning and forwarding flags, `tc` the
 * topology change flag. Whether it is superior is up to the priority. */
void stptest_mstp_rx_cist_bpdu(const char *ifname, uint16_t root_priority,
                               const uint8_t root_mac[6], int active, int tc);
int stptest_mstp_cist_root_port_is(const char *ifname);
/* Whether the daemon treats the port as an operational edge port. */
int stptest_mstp_port_is_edge(const char *ifname);

/* The daemon's own view of the VLAN's tree. */
int stptest_pvst_is_root(uint16_t vlan_id);
int stptest_pvst_root_port_is(uint16_t vlan_id, const char *ifname);
int stptest_pvst_topology_change(uint16_t vlan_id);
/* The port's state in the VLAN's tree (DISABLED 0, BLOCKING 1, LISTENING 2, LEARNING 3, FORWARDING 4), or -1. */
int stptest_pvst_port_state(uint16_t vlan_id, const char *ifname);
/* A link-up event for the port, as the netlink reader would deliver it. */
void stptest_port_up(const char *ifname);
void stptest_port_down(const char *ifname);
/* What the daemon told the kernel: the newest /sbin/bridge command that
 * contains `needle`, or "" if none has. Reset clears the record. */
const char *stptest_last_kernel_command_containing(const char *needle);
void stptest_reset_kernel_commands(void);
/* Frames the daemon refused: the global PVST drop counter. */
unsigned int stptest_pvst_drop_count(void);

/* True while stp_global says the protocol is up. */
int stptest_protocol_enabled(void);

/* The default path cost the daemon gives a port of `speed_mbps`. */
unsigned int stptest_path_cost(unsigned int speed_mbps, int extend);

#ifdef __cplusplus
}
#endif

#endif /* __STPD_DRIVER_H__ */
