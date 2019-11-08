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

#include "stp_netlink.h"

int stp_intf_get_netlink_fd();


int stp_set_sock_buf_size(int sock, int optname, int size)
{
    int new_size = 0;
    int old_size = 0;
    int size_len = sizeof(size);
    int ret = 0;

    ret = getsockopt(sock, SOL_SOCKET, optname, &old_size, &size_len); 
    if (-1 == ret)
    {
        STP_LOG_ERR("sock[%d] Getsockopt Failed : %s", sock, strerror(errno));
        return -1;
    }

    ret = setsockopt(sock, SOL_SOCKET, optname, &size, size_len); 
    if (-1 == ret)
    {
        STP_LOG_ERR("sock[%d] Setsockopt size : %d Failed : %s",sock, size, strerror(errno));
        return -1;
    }

    ret = getsockopt(sock, SOL_SOCKET, optname, &new_size, &size_len); 
    if (-1 == ret)
    {
        STP_LOG_ERR("sock[%d] Getsockopt Failed : %s",sock, strerror(errno));
        return -1;
    }
    STP_LOG_INFO("socket[%d] buf_size old,req,new : [%d][%d][%d]", sock, old_size, size, new_size);
    return new_size;
}

int stp_netlink_init(stp_netlink_cb_ptr *fn)
{
    int ret = 0;
    int nl_fd = 0;
    int len = sizeof(g_stpd_netlink_ibuf_sz);

    struct sockaddr_nl sa = {
        .nl_family = AF_NETLINK,
        .nl_pad    = 0,
        .nl_pid    = 0,
        .nl_groups = RTMGRP_LINK
    };

    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (nl_fd == -1)
    {
        STP_LOG_ERR("nl_fd CREATE Failed : %s", strerror(errno));
        return -1;
    }

    g_stpd_netlink_ibuf_sz = stp_set_sock_buf_size(nl_fd, SO_RCVBUF, STP_NETLINK_SOCK_MAX_BUF_SIZE);
    if (-1 == g_stpd_netlink_ibuf_sz)
    {
        STP_LOG_ERR("stp_netlink_set_buf_size Failed");
        return -1;
    }
    STP_LOG_INFO("Netlink initial rcv buf size : %d", g_stpd_netlink_ibuf_sz);
    g_stpd_netlink_cbuf_sz  = g_stpd_netlink_ibuf_sz;

    if(bind(nl_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
    {
        STP_LOG_ERR("nl_fd BIND Failed : %s", strerror(errno));
        return -1;
    }

    stp_netlink_cb = fn;

    return nl_fd;
}

int stp_netlink_request(int nl_fd)
{
    int ret = 0;

    struct nlmsghdr nh;
    struct iovec iov;
    struct sockaddr_nl dst;
    struct msghdr msg;

    nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    nh.nlmsg_type = RTM_GETLINK;
    nh.nlmsg_flags = (NLM_F_REQUEST | NLM_F_DUMP);
    nh.nlmsg_pid = getpid();

    iov.iov_base = &nh;
    iov.iov_len  = nh.nlmsg_len;

    memset(&dst, 0, sizeof(struct sockaddr_nl));
    dst.nl_family = AF_NETLINK;

    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_name = &dst;
    msg.msg_namelen = sizeof(struct sockaddr_nl);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = sendmsg(nl_fd, (struct msghdr *)&msg, 0);
    if (ret == -1)
    {
        STP_LOG_ERR("Send failed : %s, nl_fd : %d", strerror(errno), nl_fd);
    }
    return ret;
}

void stp_netlink_parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
    memset(tb, 0, sizeof(struct rtattr *)*(max));
	while (RTA_OK(rta, len)) {
		if (rta->rta_type < max) {
			tb[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}
}

bool stp_netlink_intf_is_valid(char *name)
{
    if ((strncmp(name, "Ethernet", strlen("Ethernet")) == 0) || 
        (strncmp(name, "PortChannel", strlen("PortChannel")) == 0))
        return true;

    return false; 
}

static int stp_netlink_recv(int nl_fd, bool read_all)
{
    int ret = 0;
    uint8_t retry = 0;
    int i =0;
    int len = 0;
    int nh_len = 0;
    uint8_t read_more_msg = 0;
    struct iovec iov;
    struct sockaddr_nl nl_addr;
    struct msghdr msg;
    struct rtattr *rt_list[IFLA_MAX+1];
    struct rtattr *linkinfo_list[IFLA_INFO_MAX+1];
    struct nlmsghdr *nh = 0;
    struct ifinfomsg *ifi = 0;
    struct rtattr *ptr = 0;
    netlink_db_t if_db;
    int flags = MSG_PEEK;
    int new_buf_size = 0;

    void *new_buf = calloc(1, STP_NETLINK_MSG_SIZE);
    iov.iov_base  = new_buf;
    iov.iov_len   = STP_NETLINK_MSG_SIZE;

    nl_addr.nl_family = AF_NETLINK;

    msg.msg_name = (void*)&nl_addr;
    msg.msg_namelen = sizeof(nl_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    do {
        len = recvmsg(nl_fd, &msg, flags);
        if (-1 == len)
        {
            if (errno == ENOBUFS)
            {
                new_buf_size = g_stpd_netlink_cbuf_sz + g_stpd_netlink_ibuf_sz;
                /*Netlink buffer size not enough, increase it*/
                if ( new_buf_size <= STP_NETLINK_SOCK_MAX_BUF_SIZE)  
                { 
                    g_stpd_netlink_cbuf_sz = new_buf_size;
                    ret = stp_set_sock_buf_size(nl_fd, SO_RCVBUF, g_stpd_netlink_cbuf_sz);
                    if (-1 == ret)
                        STP_LOG_ERR("stp_netlink_set_buf_size Failed");
                    else
                    {
                        STP_LOG_INFO("Netlink new rcv buf size : %d", g_stpd_netlink_cbuf_sz);
                        retry = 1;
                        continue;
                    }
                }
                else
                {
                    STP_LOG_CRITICAL("new_buf_size [%u] is beyond max limit", new_buf_size);
                }
            }
            else
                STP_LOG_ERR("errno : %s", strerror(errno));
            break;
        }

        if ((msg.msg_flags & MSG_TRUNC) || (len > iov.iov_len)) 
        { /*Application buffer size not enough*/
            STP_LOG_INFO("Packet truncated");

            if (len < STP_NETLINK_MAX_MSG_SIZE)
            {
                iov.iov_len = len;
                new_buf = 0;
                new_buf = realloc(iov.iov_base, len);
                if (!new_buf)
                {
                    STP_LOG_CRITICAL("Relloc Failed");
                    break;
                }
                iov.iov_base = new_buf; 
                flags = 0;
                retry = 1;
                continue;
            }
            else
            {
                //TODO: 
                //This is a very unlikely condition to hit.
                //STP_NETLINK_APPL_MAX_BUF_SIZE needs to be revisited only if we hit the error
                STP_LOG_CRITICAL("Netlink msg len[%u] is too big", len);
                /*sys_assert(0);*/
                break;
            }
        }

        if (flags != 0)
        {
            //ALL set read without flags
            flags = 0;
            retry = 1;
            continue;
        }

        //A message successfully read , disable retry attempts... continue processing.
        retry = 0;

        nh = (struct nlmsghdr *) (void*)(iov.iov_base); 
        for (; NLMSG_OK (nh, len);
                nh = NLMSG_NEXT (nh, len)) {

            if (nh->nlmsg_flags & NLM_F_MULTI)
                read_more_msg = 1;
            else
                read_more_msg = 0;

            if (nh->nlmsg_type == NLMSG_DONE)
            {
                read_more_msg = 0;
                read_all = 0;
                break;
            }

            if (nh->nlmsg_type == NLMSG_ERROR)
            {
                continue; // move on to next nh
            }

            if ((nh->nlmsg_type == RTM_NEWLINK)
                    || (nh->nlmsg_type == RTM_DELLINK))
            {
                ifi = NLMSG_DATA(nh);
                if (!ifi)
                {
                    continue; // move on to next nh
                }

                if (ifi->ifi_type != ARPHRD_ETHER)
                {
                    continue; // move on to next nh
                }

                memset(&if_db, 0, sizeof(if_db));
                if_db.kif_index = ifi->ifi_index;
                nh_len = nh->nlmsg_len;

                stp_netlink_parse_rtattr(rt_list, IFLA_MAX, IFLA_RTA(ifi), nh_len);
                if (rt_list[IFLA_IFNAME])
                {
                    ptr = rt_list[IFLA_IFNAME];
                    strncpy(if_db.ifname, (char *)RTA_DATA(ptr), IFNAMSIZ);

                    if(!stp_netlink_intf_is_valid(if_db.ifname))
                    {
                        continue;
                    }

                    if (ifi->ifi_family == AF_BRIDGE && nh->nlmsg_type == RTM_DELLINK)
                    {
                        /* For last vlan removed from the port(phy/PO) 
                         * Bridge sends RTM_DELLINK, ignore that
                         */
                        STP_LOG_DEBUG("Ignore AF_BRIDGE RTM_DELLINK for %s kif:%u",if_db.ifname,if_db.kif_index);
                        continue;
                    }

                    if (ifi->ifi_flags & IFF_RUNNING)
                    {
                        if_db.oper_state = 1;
                    }
                    else
                    {
                        if_db.oper_state = 0;
                    }

                    if (rt_list[IFLA_ADDRESS])
                    {
                        ptr = rt_list[IFLA_ADDRESS];
                        memcpy(if_db.mac, RTA_DATA(ptr), L2_ETH_ADD_LEN);
                    }

                    if (rt_list[IFLA_LINKINFO])
                    {
                        ptr = rt_list[IFLA_LINKINFO];

                        stp_netlink_parse_rtattr(linkinfo_list, IFLA_INFO_MAX, RTA_DATA(ptr), ptr->rta_len);

                        if (linkinfo_list[IFLA_INFO_KIND])
                        {
                            ptr = linkinfo_list[IFLA_INFO_KIND];
                            if ((0 == strncmp((char *)RTA_DATA(ptr), "team", 4)) || 
                                (0 == strncmp((char *)RTA_DATA(ptr), "bond", 4)))
                                if_db.is_bond = 1;
                        }
                        if (linkinfo_list[IFLA_INFO_SLAVE_KIND])
                        {
                            ptr = linkinfo_list[IFLA_INFO_SLAVE_KIND];
                            if ((0 == strncmp((char *)RTA_DATA(ptr), "team", 4)) || 
                                (0 == strncmp((char *)RTA_DATA(ptr), "bond", 4)))
                                if_db.is_member = 1;
                        }
                    }

                    if (if_db.is_member)
                    { //find my master
                        if (rt_list[IFLA_MASTER])
                        {
                            ptr = rt_list[IFLA_MASTER];
                            if_db.master_ifindex = *(uint32_t *)RTA_DATA(ptr);
                        }
                    }
                    STP_LOG_INFO("RTM-%s IF:%s KIF:%u Oper:%d Bond:%d Mem:%d Master:%u", (nh->nlmsg_type == RTM_NEWLINK)?"UPDATE":"DELETE", 
                            if_db.ifname, if_db.kif_index, if_db.oper_state, if_db.is_bond, if_db.is_member, if_db.master_ifindex); 
                }
                else
                {
                    STP_LOG_DEBUG("No ifname for kif_index :%d ", if_db.kif_index);
                }
            }
            stp_netlink_cb(&if_db, (nh->nlmsg_type == RTM_NEWLINK)?1:0, read_all);
        } //end for loop


        if (read_more_msg || read_all)
        {
            STP_LOG_DEBUG("%s : Waiting for more msgs until NLMSG_DONE", read_all?"READ_ALL":"READ_MORE");
            flags = MSG_PEEK;
        }

    }while(retry || read_more_msg || read_all);

    free (iov.iov_base);
    return 0;
}

//process all netlink msgs until EAGAIN/EWOULDBLOCK
int stp_netlink_recv_all(int nl_fd)
{
    if (stp_netlink_request(nl_fd) == -1)
        return -1;

    return stp_netlink_recv(nl_fd, true);
}

//process one msg only
int stp_netlink_recv_msg(int nl_fd)
{
    return stp_netlink_recv(nl_fd, false);
}

//Libevent callback for netlink socket
void stp_netlink_events_cb (evutil_socket_t fd, short what, void *arg)
{
    const char *data = arg;

    if (what & EV_READ)
        stp_netlink_recv_msg(stp_intf_get_netlink_fd());
    else
        STP_LOG_ERR("Invalid event : %x",what);

}


