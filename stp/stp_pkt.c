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

#include <net/if.h>
#include "stp_inc.h"


MAC_ADDRESS bridge_group_address = { 0x0180c200L, 0x0000};
MAC_ADDRESS pvst_bridge_group_address = { 0x01000cccL, 0xcccd};

/*
 * Psuedocode::
 * if (pkt-len < 1500)
 * {
 *   if (pkt[0-5] == 01 00 0c cc cc cd)
 *      return Match-Found;
 *   else if(pkt[14] == 42)
 *      return Match-Found;
 * }
 * return No-Match-Found;
 * 
 *
 * Filter to match STP & PVST packets
 * 1) check if pkt-len < 1500 
 *      >> (pkt[12:13] < 1500)
 * 2) check if PVST , verify DA Mac
 *      >> (pkt[0-5] == 01 00 0c cc cc cd)
 * 3) Else if STP, check LLC[0] == 42
 *      >> (pkt[14] == 42)
 * 4) Return 
 *      >> 0xffff : Match
 *      >> 0      : No-Match
 *
 * Unknowns::
 * - purpose of BPF_K??
 * - What should be return value on success.??
 */
//next - next instruction 
//next+n - nth instruction after next
struct sock_filter g_stp_filter[] = {
    //1. Load Half-Word @12 into BPF register
    BPF_STMT(BPF_LD |BPF_ABS|BPF_H, 0xc),
    //2. Jump if Greater than 1500, 
    //true=next+7   >> invalid size of pkt , return FAILURE
    //false=next    >> check for PVST mac
    BPF_JUMP(BPF_JMP|BPF_JGT|BPF_K, 0x5dc, 7, 0),
    //3. Load Word @0
    BPF_STMT(BPF_LD |BPF_ABS|BPF_W, 0x0),
    //4. Jump if Equal to 0x01000ccc, 
    //true=next,    >> 1st HAlf of PVST mac Matched
    //false=next+2  >> check for STP
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0x01000ccc, 0, 2),
    //5. Load Half-Word @4
    BPF_STMT(BPF_LD |BPF_ABS|BPF_H, 0x4),
    //6. Jump if Equal to 0xcccd, 
    //true=next+2, >> PVST MAC matched return SUCCESS
    //false=next   >> check for STP
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xcccd, 2, 0),
    //7. Load byte @14
    BPF_STMT(BPF_LD |BPF_ABS|BPF_B, 0xe),
    //8. Jump if Equal to 0x42,
    //true=next,   >> STP pkt , return SUCCESS
    //false=next+1 >> return FAILURE
    BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0x42, 0, 1),
    //9. Match Found trap packet to Application
    BPF_STMT(BPF_RET|BPF_K, 0xffff),
    //10. NO Match skip packet, return 0
    BPF_STMT(BPF_RET|BPF_K, 0),
};
    
void stp_pkt_sock_close(INTERFACE_NODE *intf_node)
{
    stpmgr_libevent_destroy(intf_node->ev);
    close(intf_node->sock);
    intf_node->sock = 0;
    STP_LOG_INFO("SOCKET closed for port : %u kif : %u", intf_node->port_id, intf_node->kif_index);
}

int stp_pkt_sock_create(INTERFACE_NODE *intf_node)
{
    int val = 0;
    struct sock_fprog prog;
    struct sockaddr_ll sa;
    struct event *evpkt = 0;

    if (-1 == (intf_node->sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))) 
    {
        STP_LOG_ERR("SOCKET for (%u) Failed, errno : %s"
                , intf_node->kif_index, strerror(errno));
        sys_assert(0);
    }

    //get vlan info from socket
    val = 1;
    if (-1 == setsockopt(intf_node->sock, SOL_PACKET, PACKET_AUXDATA, &val, sizeof(val)))
    {
        STP_LOG_ERR("setsock PACKET_AUXDATA  for (%u) Failed, errno : %s"
                , intf_node->kif_index, strerror(errno));
        sys_assert(0);
    }

    //filter STP/PVST packets only
    prog.filter = g_stp_filter;
    prog.len = (sizeof(g_stp_filter) / sizeof(struct sock_filter));
    if (-1 == setsockopt(intf_node->sock, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog))) {
        STP_LOG_ERR("setsockopt SO_ATTACH_FILTER for (%u) Failed, errno : %s"
                , intf_node->kif_index, strerror(errno));
        sys_assert(0);
    }


    if (-1 == stp_set_sock_buf_size(intf_node->sock, SO_RCVBUF, STP_PKT_RX_BUF_SZ))
    {
        STP_LOG_ERR("Intf: %u, setsock buf size FAILED, errno : %s"
                , intf_node->kif_index, strerror(errno));
        sys_assert(0);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sll_family = AF_PACKET;
    sa.sll_ifindex = intf_node->kif_index;
    if (-1 == (bind(intf_node->sock, (struct sockaddr *)&sa, sizeof(sa))))
    {
        STP_LOG_ERR("BIND for (%u) Failed, errno : %s"
                , intf_node->kif_index, strerror(errno));
        sys_assert(0);
    }

    /*Add to libevent list */
    intf_node->ev = stpmgr_libevent_create(g_stpd_evbase, intf_node->sock, EV_PERSIST|EV_READ, 
            stp_pkt_rx_handler, intf_node, NULL);

    if (!intf_node->ev)
    {
        STP_LOG_CRITICAL("Packet-handler Event Create failed");
        sys_assert(0);
    }

    STP_LOG_INFO("port-%u, kif-%u, sock-%d, ev-%p", intf_node->port_id, intf_node->kif_index, intf_node->sock, intf_node->ev);

    return intf_node->sock;
}

static void stp_pkt_dump(INTERFACE_NODE *intf_node, VLAN_ID vlan_id, char * pkt, uint16_t pkt_len, bool is_rx)
{
    static char pkt_str[256];
    char * pkt_parser = (char *)&pkt_str;
    UINT32 pkt_count = is_rx ? STPD_GET_PKT_COUNT(intf_node->port_id, pkt_rx):STPD_GET_PKT_COUNT(intf_node->port_id, pkt_tx);
    int i = 0;
    
    memset(pkt_str, 0, sizeof(pkt_str));

    STP_PKTLOG("%s - [port:%3u][vlan:%4u][len:%4lu][cnt:%lu]",(is_rx?"RX":"TX"),
            intf_node->port_id, vlan_id, pkt_len, pkt_count); 

    if (STP_DEBUG_VERBOSE)
    {
        for(i = 0; (i < pkt_len && i < 256); i++)
        {
            pkt_parser += sprintf(pkt_parser, "%02x", pkt[i] & 0xff);
            if (0 == ((i+1)%4))
                pkt_parser += sprintf(pkt_parser, " ");
        }
        STP_PKTLOG("%s",pkt_str);
    }
}

static void stp_pkt_fill_tx_buf(uint16_t size, VLAN_ID vlan_id, char *buffer, char *tx_buf)
{
    uint8_t prio = (7 << 5); 
    //Copy MAC header
    memcpy(tx_buf, buffer, (L2_ETH_ADD_LEN * 2));
    tx_buf   += (L2_ETH_ADD_LEN * 2);
    buffer   += (L2_ETH_ADD_LEN * 2);
    size      = size - (L2_ETH_ADD_LEN * 2);

    if (vlan_id)
    {
        //INSERT VLAN HEADER
        *tx_buf++ = 0x81;
        *tx_buf++ = 0x00;
        *tx_buf++ = (prio | (vlan_id & 0xf00) >> 8);
        *tx_buf++ = (vlan_id & 0xff);

        size      = size - VLAN_HEADER_LEN;
    }

    //Copy the rest of buffer
    memcpy(tx_buf, buffer, size);

    return;

}

/* buffer : contains the entire packet including mac */
int stp_pkt_tx_handler(uint32_t port_id, VLAN_ID vlan_id, char *buffer, uint16_t size, bool tagged)
{
    int                     i = 0;
    int                   ret = 0;
    struct sockaddr_ll sa;
    char            send_buf[STP_MAX_PKT_LEN];
    uint32_t        kif_index = 0;
    INTERFACE_NODE *intf_node = 0;

    intf_node = stp_intf_get_node(port_id);
    if (!intf_node)
    {
        STPD_INCR_PKT_COUNT(port_id, pkt_tx_err);
        return -1;
    }

    if (tagged)
        size += VLAN_HEADER_LEN;

    memset(send_buf, 0, STP_MAX_PKT_LEN);
    stp_pkt_fill_tx_buf(size, tagged?vlan_id:0, buffer, send_buf);

    if (STP_DEBUG_BPDU_TX(vlan_id, intf_node->port_id))
        stp_pkt_dump(intf_node, vlan_id, send_buf, size, false);

    memset(&sa, 0, sizeof(struct sockaddr_ll));
    sa.sll_family = AF_PACKET;
    sa.sll_ifindex = intf_node->kif_index;

    ret = sendto(g_stpd_pkt_handle, send_buf, size, 0,
            (const struct sockaddr*)&sa, sizeof(struct sockaddr_ll));

    if (-1 == ret)
    {
        STP_LOG_ERR("sendto Failed : %s", strerror(errno));
        STPD_INCR_PKT_COUNT(port_id, pkt_tx_err);
    }

    STPD_INCR_PKT_COUNT(port_id, pkt_tx);
    return ret;
}


void stp_pkt_rx_handler (evutil_socket_t fd, short what, void *arg)
{
    g_stpd_stats_libev_pktrx++;

    INTERFACE_NODE *intf_node = (INTERFACE_NODE *)arg;
    INTERFACE_NODE *intf_node_member = 0;
    int                     i = 0;
    uint16_t          vlan_id = 0;
    ssize_t        packet_len = 0;
    static char pkt[STP_MAX_PKT_LEN];

	struct tpacket_auxdata *aux;
    struct sockaddr_ll  from;
    struct iovec        iov;
    struct msghdr       msg;
    struct cmsghdr      *cmsg;
    union {         /* Ancillary data buffer, wrapped in a union
                       in order to ensure it is suitably aligned */
        char buf[CMSG_SPACE(STP_MAX_PKT_LEN)];
        struct cmsghdr align;
    } cmsg_buf;
    int new_buf_size = 0;
    char ifname[IF_NAMESIZE] = {0};

    if (!intf_node)
    {
        STP_LOG_CRITICAL("Invalid Intf_node for socket : %d", fd);
        return;
    }

    memset(pkt, 0, sizeof(pkt));
    memset(&from, 0, sizeof(struct sockaddr_ll));
    memset(&iov, 0, sizeof(struct iovec));
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_name        = &from;
    msg.msg_namelen     = sizeof(from);
    msg.msg_iov         = &iov;
    msg.msg_iovlen      = 1;
    msg.msg_control     = &cmsg_buf;
    msg.msg_controllen  = sizeof(cmsg_buf);
    msg.msg_flags       = 0;

    iov.iov_len         = STP_MAX_PKT_LEN;
    iov.iov_base        = pkt;

    packet_len = recvmsg(fd, &msg, MSG_TRUNC);

    if (-1 == packet_len)
    {
        if (errno == ENETDOWN)
            STP_LOG_INFO("%s : errno : Network is down", intf_node->ifname);
        else
            STP_LOG_ERR("%s : errno : %s", intf_node->ifname, strerror(errno));

        STPD_INCR_PKT_COUNT(intf_node->port_id, pkt_rx_err);
        return;
    }

    if (msg.msg_flags & MSG_TRUNC)
    {
        //Anyway we miss this message, cant do much about that.
        //Instead of parsing the incomplete message return error
        STPD_INCR_PKT_COUNT(intf_node->port_id, pkt_rx_err_trunc);
        return;
    }

    if (from.sll_ifindex != intf_node->kif_index)
    {
        if_indextoname(from.sll_ifindex, ifname);
        if ((strncmp("lo",ifname,2) == 0) || (strncmp("eth",ifname,3) == 0))
        {
            /*
             * STP never expects any packet from "lo" or "eth0/eth1/eth2 etc"
             */
            STP_LOG_DEBUG("Drop pkts recvd on %s pkt-kif:%u my-kif-%u my-port:%u",ifname,from.sll_ifindex, intf_node->kif_index,intf_node->port_id);
            return;
        }
        STP_LOG_ERR("INVALID src_port : pkt-kif:%u my-kif-%u my-port:%u",from.sll_ifindex, intf_node->kif_index, intf_node->port_id);
        STPD_INCR_PKT_COUNT(intf_node->port_id, pkt_rx_err);
        return;
    }

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) 
	{
		if (cmsg->cmsg_len < CMSG_LEN(sizeof(struct tpacket_auxdata)) ||
				cmsg->cmsg_level != SOL_PACKET ||
				    cmsg->cmsg_type != PACKET_AUXDATA) 
		{
			STP_LOG_DEBUG(" isn't a PACKET_AUXDATA auxiliary");
			continue;
		}

		aux = (struct tpacket_auxdata *)CMSG_DATA(cmsg);
        if (aux->tp_status & TP_STATUS_VLAN_VALID)
        {
            vlan_id = (aux->tp_vlan_tci & 0x0fff);
            break;
        }
	}

    //if PO-member port, assign intf_node to PO node.
    if (intf_node->master_ifindex)
    {
        intf_node_member = intf_node;
        intf_node = stp_intf_get_node_by_kif_index(intf_node->master_ifindex);
        if (!intf_node)
        {
            STP_LOG_ERR("Master not found, master_ifindex [%u] port-id [%u]", intf_node_member->kif_index, intf_node_member->port_id);
            STPD_INCR_PKT_COUNT(intf_node_member->port_id, pkt_rx_err);
            return;
        }
    }

    STPD_INCR_PKT_COUNT(intf_node->port_id, pkt_rx);

    if (STP_DEBUG_BPDU_RX(vlan_id, intf_node->port_id))
        stp_pkt_dump(intf_node, vlan_id, pkt, packet_len, true);

    if (STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
    { 
        stpmgr_process_rx_bpdu(vlan_id, intf_node->port_id, &pkt[0]);
    }
    else if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
    {
        if (stpmgr_protect_process(intf_node->port_id, vlan_id))
            return;
        if ((unsigned char)pkt[1] == 128)
        {
             mstpmgr_rx_bpdu(vlan_id, intf_node->port_id, &pkt[0], packet_len);
        }
    }

    return;
}

