
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
 * nas_nlmsg_object_utils.h
 *
 *  Created on: Mar 30, 2015
 */

#ifndef CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_OBJECT_UTILS_H_
#define CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_OBJECT_UTILS_H_

#include "nas_nlmsg.h"
#include "std_ip_utils.h"
#include "cps_api_object.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int nas_nl_add_attr_ip(struct nlmsghdr * m, int maxlen, int type, cps_api_object_attr_t attr) {
    hal_ip_addr_t * _ip = (hal_ip_addr_t *)cps_api_object_attr_data_bin(attr);
    return nlmsg_add_attr(m,maxlen,type,
        _ip->af_index==AF_INET ?
             (void*)&_ip->u.v4_addr: &_ip->u.v6_addr,
        _ip->af_index==AF_INET ?
                sizeof(_ip->u.v4_addr) : sizeof(_ip->u.v6_addr));
}

static inline int nas_nl_add_attr_int(struct nlmsghdr * m, int maxlen, int type, cps_api_object_attr_t attr) {
    int32_t _int = cps_api_object_attr_data_u32(attr);
    return nlmsg_add_attr(m,maxlen,type,&_int,sizeof(_int));
}

void rta_add_mac( struct nlattr* rtatp,cps_api_object_t obj, uint32_t attr );

void rta_add_mask( int family, uint_t prefix_len, cps_api_object_t obj, uint32_t attr);

void rta_add_e_ip( struct nlattr* rtatp,int family, cps_api_object_t obj,
        cps_api_attr_id_t *attr, size_t attr_id_len);

inline static void rta_add_ip( struct nlattr* rtatp,int family,
        cps_api_object_t obj, cps_api_attr_id_t attr) {
    rta_add_e_ip( rtatp,family,obj,&attr,1);
}

unsigned int rta_add_name( struct nlattr* rtatp,cps_api_object_t obj, uint32_t attr);

#ifdef __cplusplus
}
#endif

#endif /* CPS_API_LINUX_INC_PRIVATE_NAS_NLMSG_OBJECT_UTILS_H_ */
