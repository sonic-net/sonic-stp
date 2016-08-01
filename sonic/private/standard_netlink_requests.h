
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */


/*
 * standard_netlink_requests.h
 */

#ifndef STANDARD_NETLINK_REQUESTS_H_
#define STANDARD_NETLINK_REQUESTS_H_

#include "netlink_tools.h"
#include <linux/netlink.h>
#include <string.h>

/**
 * A extremely common function to return all instances of a given type
 * @param sock is the socket on which to send the request
 * @param type is the netlink query type (ie. RTM_GETROUTE)
 * @param family is the query family like AF_INET or AF_PACKET, etc..
 * @param seq is the request id sequence that will be expected in the response
 * @return true of successfully sent the message
 */
static inline bool nl_route_send_get_all(int sock, int type, int family, int seq) {
    struct rtgenmsg rtm;
    memset(&rtm, 0, sizeof(rtm));
    rtm.rtgen_family = family;

    return nl_send_request(sock, type, NLM_F_ROOT| NLM_F_DUMP|NLM_F_REQUEST,
            seq,&rtm,sizeof(rtm));
}

#endif /* STANDARD_NETLINK_REQUESTS_H_ */
