
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
 * filename: nl_api.h
 */

#ifndef __NL_API_H
#define __NL_API_H


#include "ds_common_types.h"
#include "cps_api_interface_types.h"
#include "cps_api_operation.h"
#include "std_type_defs.h"
#include "std_error_codes.h"

#include <stddef.h>
#include <linux/rtnetlink.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This define was selected to give about 10m buffer space on each netlink socket
 * created for kernel communication.  If the socket buffer is low for any reason
 * the kernel will discard events.  Missing discarded events is something
 * that we don't expect and would have undesirable consequences
 */
#define NL_SOCKET_BUFFER_LEN (10*1024*1024)

typedef enum  {
    nas_nl_sock_T_ROUTE=0,
    nas_nl_sock_T_INT=1,
    nas_nl_sock_T_NEI=2,
    nas_nl_sock_T_MAX
}nas_nl_sock_TYPES;


int nas_nl_sock_create(nas_nl_sock_TYPES type) ;

void os_send_refresh(nas_nl_sock_TYPES type);



typedef bool (*fun_process_nl_message) (int rt_msg_type, struct nlmsghdr *hdr, void * context);

bool netlink_tools_process_socket(int sock, fun_process_nl_message handlers,
        void * context, char * scratch_buff, size_t scratch_buff_len,
        const int * seq, int *error_code);

bool nl_send_request(int sock, int type, int flags, int seq, void * req, size_t len );

bool nl_send_nlmsg(int sock, struct nlmsghdr *m);

t_std_error nl_do_set_request(nas_nl_sock_TYPES type,struct nlmsghdr *m, void *buff, size_t bufflen);

void nas_os_pack_nl_hdr(struct nlmsghdr *nlh, __u16 msg_type, __u16 nl_flags);

void nas_os_pack_if_hdr(struct ifinfomsg *ifmsg, unsigned char ifi_family,
                        unsigned int flags, int if_index);

#ifdef __cplusplus
}
#endif

#endif
