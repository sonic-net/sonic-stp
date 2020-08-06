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

#ifndef _STP_MAIN_H_
#define _STP_MAIN_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <event2/event.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <signal.h>

#include "applog.h"
#include "stp_ipc.h"
#include "avl.h"
#include "stp_inc.h"

#define STP_LOG_CRITICAL    APP_LOG_CRITICAL
#define STP_LOG_DEBUG       APP_LOG_DEBUG
#define STP_LOG_ERR         APP_LOG_ERR
#define STP_LOG_WARNING     APP_LOG_WARNING
#define STP_LOG_NOTICE      APP_LOG_NOTICE
#define STP_LOG_INFO        APP_LOG_INFO
#define STP_LOG_INIT        APP_LOG_INIT
#define STP_LOG_SET_LEVEL   APP_LOG_SET_LEVEL
#define STP_LOG_LEVEL_DEBUG APP_LOG_LEVEL_DEBUG
#define STP_LOG_LEVEL_INFO  APP_LOG_LEVEL_INFO

/* Logs directed to /var/log/syslog */
#define STP_SYSLOG(msg, ...) applog_write(APP_LOG_LEVEL_INFO, "STP_SYSLOG: "msg" ", ##__VA_ARGS__)

#define STP_PKTLOG(msg, ...) applog_write(APP_LOG_LEVEL_INFO, "STP_PKT: "msg" ", ##__VA_ARGS__)

//forward declaration
struct netlink_db_s;

#define g_stpd_evbase           stpd_context.evbase
#define g_stpd_netlink_handle   stpd_context.netlink_fd
#define g_stpd_ipc_handle       stpd_context.ipc_fd
#define g_stpd_pkt_handle       stpd_context.pkt_fd
#define g_stpd_netlink_ibuf_sz  stpd_context.netlink_init_buf_sz
#define g_stpd_netlink_cbuf_sz  stpd_context.netlink_curr_buf_sz
#define g_stpd_port_init_done   stpd_context.port_init_done
#define g_stpd_intf_db          stpd_context.intf_avl_tree
#define g_stpd_po_id_pool       stpd_context.po_id_pool
#define g_stpd_ioctl_sock       stpd_context.ioctl_sock
#define g_stpd_sys_max_port     stpd_context.sys_max_port
#define g_stpd_extend_mode      stpd_context.extend_mode

#define STPD_100MS_TIMEOUT      100000

#define STP_ETH_NAME_PREFIX_LEN 8

/*
 * STP-LIBEVENT Priority-Mapping
 * - STP sockets:
 *   1) 100ms Timer
 *   2) PF_SOCKET for packet-rx/tx
 *   3) Netlink
 *   4) IPC
 *
 * - STP is a single threaded process. It relies on Sockets for all its communication.
 * - STP uses libevent to manage socket communication. (viz, get notified for READ/WRITE_READY state)
 *   Libevent Priority Mapping:
 *      - High Priority Q := 100ms Timer. Prio-0 queue.
 *      - Low Priority  Q := All others. Prio-1 queue.
 *   Whenever there is data in both the queues, Prio-0 gets executed first.
 * - We impose the below mentioned restrictions on the low-priority queue using "event_config".
 *
// max_interval :
// Restrict the libevent low-prio-queue processing time to less than 50ms.
//  why 50ms?
//  - tx and rx are the main events. NETLink/IPC fd events are comparitively less disruptive.
//  - by setting tx(timers) in priority-0, we have ensured it executes first on every 100ms.
//  - With the pkt-tx-profiling results, we have learnt 
//    To send 1600 pkts in a 70% loaded system
//    max-time taken = 60ms
//    avg-time taken = 15-20ms 
//    hence we can safely allcate 50ms for each(tx/rx).
//
// max_callbacks :
// Max-sockets to be served at any given instance.
// Pkt_rx, IPC-sock1, IPC-sock2 etc.
// IPC can be run from multiple terminals, in which case there can be a socket opened for each terminal.
// Lets restrict our libevent queue to process only 5 sockets at any given instance.
//
*/
#define STP_LIBEV_PRIO_QUEUES 2
#define STP_LIBEV_HIGH_PRI_Q 0
#define STP_LIBEV_LOW_PRI_Q  1


typedef struct
{
    char ifname[IFNAMSIZ];
    uint32_t kif_id;  // kernel if index
    uint32_t port_id; // local port if index
    char     mac[L2_ETH_ADD_LEN];
    uint32_t speed;
    uint8_t  oper_state;

}stp_if_avl_node_t;

#define g_stpd_stats_libev_no_of_sockets stpd_context.dbg_stats.libev.no_of_sockets
#define g_stpd_stats_libev_timer   stpd_context.dbg_stats.libev.timer_100ms
#define g_stpd_stats_libev_pktrx   stpd_context.dbg_stats.libev.pkt_rx
#define g_stpd_stats_libev_ipc     stpd_context.dbg_stats.libev.ipc
#define g_stpd_stats_libev_netlink stpd_context.dbg_stats.libev.netlink

#define g_stpd_intf_stats          stpd_context.dbg_stats.intf
#define STPD_INCR_PKT_COUNT(x, y)   (g_stpd_intf_stats[x]->y)++
#define STPD_GET_PKT_COUNT(x, y)    (g_stpd_intf_stats[x]->y)

struct tm *g_tm;
struct timespec g_ts;

typedef struct
{
    uint16_t no_of_sockets;
    uint64_t timer_100ms;
    uint64_t pkt_rx;
    uint64_t ipc;
    uint64_t netlink;
}STPD_LIBEV_STATS;

typedef struct
{
    uint64_t pkt_rx;
    uint64_t pkt_tx;
    uint64_t pkt_rx_err_trunc;
    uint64_t pkt_rx_err;
    uint64_t pkt_tx_err;
}STPD_INTF_STATS;

typedef struct 
{
    STPD_INTF_STATS   **intf;
    STPD_LIBEV_STATS libev;
}STPD_DEBUG_STATS;

typedef struct STPD_CONTEXT {
    /*Libevent base to monitor all socket Fd's*/
    struct event_base   *evbase;

    /*Fd's used by STP*/
    int                 netlink_fd;     //netlink Interaction with Kernel
    int                 ipc_fd;         //communication with stpmgrd, etc.
    int                 pkt_fd;

    uint8_t             port_init_done:1;
    uint8_t             extend_mode:1;
    uint8_t             spare:6;
    uint32_t            netlink_init_buf_sz; //default netlink rcv buff size fetched on bootup
    uint32_t            netlink_curr_buf_sz; //updated netlink rcv buff size by stp

    /*INTERFACE Database*/
    //The Interface DB- AVL tree.
    //Key := Interface name
    //Will hold a node for each Interface(Ethernet/PO) in the System.
    //PO node will be created only when 1st Member port is added to the system.
    struct avl_table    *intf_avl_tree;

    //TODO : for performance optimization
    //array of pointers to nodes in avl tree.
    //for faster access by avoiding parsing avl tree.
    int                 **intf_ptr_to_avl_node;

    //Local port-id for Port-channel.
    struct BITMAP_S     *po_id_pool;
    uint32_t            ioctl_sock;
    //Max Phy ports in system. calculated using netlink dump
    uint16_t            sys_max_port;
    STPD_DEBUG_STATS    dbg_stats;
}STPD_CONTEXT;

extern char msgtype_str[][64];

//Function declaration
int stpmgr_avl_compare(const void *user_p, const void *data_p, void *param);
void stpmgr_interface_update(struct netlink_db_s *if_db, uint8_t add);
uint32_t stpmgr_update_if_avl_tree(struct netlink_db_s *if_db, uint8_t add);
void stpmgr_update_portclass(uint32_t port_id, struct netlink_db_s *if_db, uint8_t add);
#endif
