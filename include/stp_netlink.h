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

#ifndef __STP_NETLINK_H__
#define __STP_NETLINK_H__

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <event2/event.h>
#include <linux/if_arp.h>

#include "stp_inc.h"

// Kernel always allocates 2 times the socket buffer size requested by the process
// Kernel will allocate only upto the max value defined in sysctl net.core.rmem_max 
// NOTE :-
// - Setting higher size for socket buffer doesnt mean , this size will be statically mapped/allocated to the applications memory which is holding the socket.
//   It is only a information for the kernel to use upto MAX configured socket buff size if required.
//   Until unless STP recieves a bulk of packets or STP doesnt process recieved packets in time, socket buffer will NOT grow upto the configured max value.
// - Socket buffers will be immediately released once recvmsg call is executed for that packet. i.e once pkt is delivered to application.
//
#define STP_MAX_SOCKET_RECV_Q_SIZE      (8 * 1024 * 1024) // 8 MB

//Netlink is not reliable, any undesirable action can trigger a burst of netlink messages.
//So setting up the Q buff size to max possible value.
#define STP_NETLINK_SOCK_MAX_BUF_SIZE    STP_MAX_SOCKET_RECV_Q_SIZE

// size of a netlink message should not exceed 32KB.
// Just to be on safer side we shall realloc and retry upto 1 MB
#define STP_NETLINK_MAX_MSG_SIZE         (1024 * 1024) // 1 MB

// On kernel >= linux-4.9 netlink messages can go upto 32KB(1 Page size) 
#define STP_NETLINK_MSG_SIZE             (32 * 1024)

// With MTU = 9000, sockets buffers are restricted to hold very less number of packets. 
//   sk_buff = any kernel-overhead + sizeof(sk_buff) + STP-pkt-size + padding + MTU.
// Based on tests,
//  - 200 KB socket recv buff can hold 13 pkts. 
// This is very small number.
// Even in the worst case, STP does not want to loose any packets. Hence setting pkt-rx buf to max value.
//  - 2 MB socket is able to hold 253 pkts. yes it doesnt add up, but thats how it works.
#define STP_PKT_RX_BUF_SZ                (2 * 1024 * 1024) // 2 MB

#define L2_ETH_ADD_LEN 6

//TODO: remove once linux version is upgraded  
#define IFLA_INFO_SLAVE_KIND 4

typedef struct netlink_db_s
{
    uint32_t kif_index; //kernel if index
    uint32_t master_ifindex; //kernel if index
    char ifname[IFNAMSIZ];
    uint8_t is_bond:1;
    uint8_t is_member:1;
    uint8_t oper_state:1;
    uint8_t unused:5;
    char mac[L2_ETH_ADD_LEN];
    uint32_t speed;
}netlink_db_t;

typedef void stp_netlink_cb_ptr (netlink_db_t *if_db, uint8_t add, bool init_in_prog);
extern stp_netlink_cb_ptr *stp_netlink_cb;

#define PRINT_MAC_FORMAT "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define PRINT_MAC_VAL(x) *(char *)x,*((char *)x)+1,*((char *)x)+2,*((char *)x)+3,*((char *)x)+4,*((char *)x)+5

int stp_netlink_init(stp_netlink_cb_ptr *fn);
int stp_netlink_recv_all(int fd);
int stp_netlink_recv_msg(int fd);
void stp_netlink_event_mgr_init();
void stp_netlink_events_cb (evutil_socket_t fd, short what, void *arg);

#endif //__STP_NETLINK_H__
