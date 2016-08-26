/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: nl_api.c
 */

/*
 * nl_api.c
 */

#include "netlink_tools.h"
#include "std_socket_tools.h"
#include "event_log.h"
#include "nas_nlmsg.h"

#include <string.h>
#include <unistd.h>

#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>

typedef struct {
    struct nlattr **tb;
    size_t max_type;
}nl_param_t;

static void netlink_tool_attr(struct nlattr *attr, void *context) {
    nl_param_t * p = (nl_param_t*) context;
    int type = nla_type(attr);
    if (type== 0 || type < p->max_type) {
        p->tb[type] = attr;
    }
}

int nla_parse(struct nlattr *tb[], int max_type, struct nlattr * head, int len)  {
    memset(tb,0,sizeof(struct nlattr*)*max_type);
    nl_param_t param = { tb, max_type };

    nla_for_each_attr((struct nlattr*)head,len,netlink_tool_attr,&param);

    return 0;
}

int nl_sock_create(int ln_groups, int type) {
    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));

    sa.nl_family = AF_NETLINK;
    sa.nl_groups = ln_groups;
    int sock = socket (AF_NETLINK, SOCK_RAW, type);
    if(sock < 0) {
        EV_LOG_ERRNO(ev_log_t_NETLINK,0,"NK-SOCKCR",errno);
        return -1;
    }
    std_sock_set_rcvbuf(sock,NL_SOCKET_BUFFER_LEN);
    if (bind(sock, (struct sockaddr *) &sa, sizeof(sa))!=0) {
        close(sock);
        return -1;
    }
    return sock;
}

bool netlink_tools_process_socket(int sock,
            fun_process_nl_message func,
            void * context, char * scratch_buff, size_t scratch_buff_len,
            const int * seq, int *error_code) {

    int error_rc ;//will init below...
    if(error_code==NULL) error_code = &error_rc;
    //zap out existing error code
    *error_code = 0;

    bool rc =false;
    bool cont=true;

    while (cont) {
        struct nlmsghdr *nh = (struct nlmsghdr *)scratch_buff;
        struct iovec iov = { scratch_buff,sizeof(struct nlmsghdr) };
        struct sockaddr_nl snl;
        struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };

        int len = recvmsg (sock, &msg, MSG_PEEK | MSG_TRUNC);
        if (len<0  && ((errno==EINTR) || (errno==EAGAIN))) continue;

        if (len!=-1 && (msg.msg_flags & MSG_TRUNC)) {
            if (iov.iov_len == len) {
                /*
                 * In cases where the input buffer is less then the required space
                 * read just one message - higher overhead but at least no truncated messages
                 *
                 * */
                iov.iov_len = nh->nlmsg_len;
            } else {
                iov.iov_len = len;
            }
        }
        /*
         * Block possible buffer overwrite - truncate the message if the buffer size is smaller then even a single
         * message - likely only the case when people are querying with less then 1024 bytes
         * */
        if (iov.iov_len > scratch_buff_len) {
            iov.iov_len =  nh->nlmsg_len < scratch_buff_len ?
                    nh->nlmsg_len : scratch_buff_len;
        }

        len = recvmsg (sock, &msg, 0);

        if (len<0) {
            if ((errno==EINTR) || (errno==EAGAIN)) continue;
            *error_code = errno;
            return false;
        }

        int nlmsg_type = nh->nlmsg_type;

        for(nh = (struct nlmsghdr *) scratch_buff; NLMSG_OK (nh, len);
                nh = NLMSG_NEXT (nh, len)) {

            nlmsg_type = nh->nlmsg_type;

            if ((seq!=NULL) && ((*seq)!=nh->nlmsg_seq)) {
                EV_LOG(INFO,NETLINK,2,"ACK/ERR","sock %d, out of sequence, msg_type %d",
                       sock, nlmsg_type);
                continue;
            }

            cont = (nh->nlmsg_flags & NLM_F_MULTI); // continue to wait
                            //for more messages since more on their way

            if (nh->nlmsg_flags & NLM_F_DUMP_INTR) {
                *error_code = EINTR;
                return false;    //current messages are incomplete.
            }

            if (nh->nlmsg_type == NLMSG_DONE) {
                cont = false;
                rc = true;
                continue;
            }

            if (nh->nlmsg_type == NLMSG_NOOP) {
                EV_LOG(INFO,NETLINK,3,"ACK/ERR","Received a NOOP message");
                continue;
            }

            if (nh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (nh);
                EV_LOG(INFO,NETLINK,0,"ACK/ERR","Received response errid:%d msg:%d",err->error,err->msg);
                if (err->error==0) {
                    rc = true;
                    continue;
                }
                /*
                 * Netlink error is returned as a -ve number but all other errorno is +ve.
                 * Converting to a positive error code for putting to STD_ERR private space
                 */
                *error_code = -(err->error);
                return false;
            }

            if (!func(nh->nlmsg_type,nh,context)) {
                return false;
            } else {
                rc = true;
            }
        }
        if (msg.msg_flags & MSG_TRUNC) {
            EV_LOG(INFO,NETLINK,0,"ACK/ERR","Truncated message %d (type:%d)",msg.msg_iovlen,
                    nlmsg_type);
            return false;
        }
    }
    return rc;

}

bool nl_send_nlmsg(int sock, struct nlmsghdr *m) {
    struct sockaddr_nl nladdr ;
    memset(&nladdr,0,sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_groups = 0;

    struct iovec iov[1] = {
        { .iov_base = m, .iov_len = m->nlmsg_len }

    };
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen =     sizeof(nladdr),
        .msg_iov = iov,
        .msg_iovlen = 1,
    };

    return sendmsg(sock,&msg,0)==(m->nlmsg_len);
}

bool nl_send_request(int sock, int type, int flags, int seq, void * req, size_t len ) {
    struct nlmsghdr nlh;

    struct sockaddr_nl nladdr = { .nl_family = AF_NETLINK };

    struct iovec iov[2] = {
        { .iov_base = &nlh, .iov_len = sizeof(nlh) },
        { .iov_base = req, .iov_len = len }
    };
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen =     sizeof(nladdr),
        .msg_iov = iov,
        .msg_iovlen = 2,
    };

    nlh.nlmsg_len = NLMSG_LENGTH(len);
    nlh.nlmsg_type = type;
    nlh.nlmsg_flags = flags;
    nlh.nlmsg_pid = 0;
    nlh.nlmsg_seq = seq;

    return sendmsg(sock,&msg,0)==(sizeof(nlh)+len);
}

void * nlmsg_reserve(struct nlmsghdr * m, int maxlen, int len) {
    void * p = nlmsg_tail(m);
    if ((NLMSG_ALIGN(m->nlmsg_len) + RTA_ALIGN(len)) > maxlen) {
        return NULL;
    }
    m->nlmsg_len = NLMSG_ALIGN(m->nlmsg_len) + RTA_ALIGN(len);
    return p;
}

struct nlattr * nlmsg_nested_start(struct nlmsghdr * m, int maxlen) {
    return (struct nlattr *)(nlmsg_reserve(m,maxlen,sizeof(struct rtattr)));
}

void nlmsg_nested_end(struct nlmsghdr * m, struct nlattr *attr) {
    attr->nla_len =  ((char*)nlmsg_tail(m)) - (char*)attr ;
}

int nlmsg_add_attr(struct nlmsghdr * m, int maxlen, int type, const void * data, int attr_len) {
    struct rtattr *rta = (struct rtattr *)nlmsg_reserve(m,maxlen,RTA_LENGTH(attr_len));
    if (rta==NULL) return -1;
    rta->rta_type = type;
    rta->rta_len = RTA_LENGTH(attr_len);
    memcpy(RTA_DATA(rta), data, attr_len);
    return m->nlmsg_len;
}

bool _process_set_fun(int rt_msg_type, struct nlmsghdr *hdr, void * context) {
    return true;
}

t_std_error nl_do_set_request(nas_nl_sock_TYPES type,struct nlmsghdr *m, void *buff, size_t bufflen) {
    int error = 0;
    int sock = nas_nl_sock_create(type);
    if (sock==-1) return STD_ERR(ROUTE,FAIL,errno);
    do {
        int seq = (int)time(NULL);
        m->nlmsg_seq = seq;
        if (!nl_send_nlmsg(sock,m)) {
            break;
        } else {
            close(sock);
            return cps_api_ret_code_OK;
        }

    } while(0);
    if (sock!=-1) close(sock);
    return STD_ERR(ROUTE,FAIL,error);
}


static int create_intf_socket(void) {
    return nl_sock_create(RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR,NETLINK_ROUTE);
}

static int create_route_socket(void) {
    return nl_sock_create(RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE,NETLINK_ROUTE);
}

static int create_neigh_socket(void) {
    return nl_sock_create(RTMGRP_NEIGH,NETLINK_ROUTE);
}

typedef int (*create_soc_fn)(void);
static create_soc_fn sock_create_functions[] = {
    create_route_socket,
    create_intf_socket,
    create_neigh_socket
};

int nas_nl_sock_create(nas_nl_sock_TYPES type)  {
    if (type >= nas_nl_sock_T_MAX) return -1;
    return sock_create_functions[type]();
}


void nas_os_pack_nl_hdr(struct nlmsghdr *nlh, __u16 msg_type, __u16 nl_flags)
{
    nlh->nlmsg_pid = 0;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_flags = nl_flags ;
    nlh->nlmsg_type = msg_type;
}

void nas_os_pack_if_hdr(struct ifinfomsg *ifmsg, unsigned char ifi_family,
                        unsigned int flags, int if_index)
{
    ifmsg->ifi_family = ifi_family;
    ifmsg->ifi_flags = flags;
    ifmsg->ifi_index = if_index;
}
