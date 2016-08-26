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
 * nas_os_int_utils.c
 *
 *  Created on: May 19, 2015
 */

#include "nas_os_int_utils.h"

#include "std_error_codes.h"
#include "cps_api_interface_types.h"
#include "event_log.h"
#include "std_mac_utils.h"
#include "dell-interface.h"
#include "dell-base-if.h"
#include "dell-base-if-linux.h"

#include <net/if_arp.h>
#include <linux/if.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

t_std_error nas_os_util_int_mtu_get(const char *name, unsigned int *mtu) {
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR_OK;

    do {
        if (ioctl(sock, SIOCGIFMTU, &ifr) >= 0) {
            *mtu = (ifr.ifr_mtu ) ;
            break;
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-GET",STD_ERR_EXT_PRIV(err));
    } while(0);

    close(sock);
    return err;
}

t_std_error nas_os_util_int_admin_state_get(const char *name, db_interface_state_t *state,
        db_interface_operational_state_t *ostate) {
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR_OK;

    do {
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
            *state = (ifr.ifr_flags & IFF_UP) ? DB_ADMIN_STATE_UP : DB_ADMIN_STATE_DN;
            if (ostate!=NULL) {
                *ostate = (ifr.ifr_flags & IFF_RUNNING) ? DB_OPER_STATE_UP : DB_OPER_STATE_DN;
            }
            break;
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-GET",STD_ERR_EXT_PRIV(err));
    } while(0);

    close(sock);
    return err;
}

t_std_error nas_os_util_int_mac_addr_get(const char *name, hal_mac_addr_t *macAddr) {
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);
    t_std_error err = STD_ERR_OK;

    do {
        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) >= 0) {
            memcpy(*macAddr, ifr.ifr_hwaddr.sa_data,sizeof(*macAddr));
            break;
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-SET",STD_ERR_EXT_PRIV(err));
    } while (0);
    close(sock);
    return err;
}

t_std_error nas_os_util_int_admin_state_set(const char *name, db_interface_state_t state,
        db_interface_operational_state_t ostate) {
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR_OK;

    do {
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
            if (state == DB_ADMIN_STATE_UP) {
                ifr.ifr_flags |= IFF_UP;
            } else {
                ifr.ifr_flags &= ~IFF_UP;
            }
            if (ioctl(sock, SIOCSIFFLAGS, &ifr) >=0) {
                break;
            }
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-SET",STD_ERR_EXT_PRIV(err));
    } while(0);

    close(sock);
    return err;
}

t_std_error nas_os_util_int_mtu_set(const char *name, unsigned int mtu)
{
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR_OK;
    ifr.ifr_mtu = mtu;

    if (ioctl(sock, SIOCSIFMTU, &ifr) < 0) {
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-SET",errno);
    }
    close(sock);
    return err;
}

t_std_error nas_os_util_int_mac_addr_set(const char *name, hal_mac_addr_t *macAddr) {
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);
    t_std_error err = STD_ERR_OK;

    do {

        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        memcpy(ifr.ifr_hwaddr.sa_data, *macAddr, sizeof(*macAddr));
        if (ioctl(sock, SIOCSIFHWADDR, &ifr) >=0 ) {
            break;
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-SET",STD_ERR_EXT_PRIV(err));
    } while (0);
    close(sock);
    return err;
}

t_std_error nas_os_util_int_flags_get(const char *name, unsigned *flags)
{
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR_OK;

    do {
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
            *flags = ifr.ifr_flags;
            break;
        }
        err = STD_ERR(INTERFACE,FAIL,errno);
        EV_LOG_ERRNO(ev_log_t_INTERFACE,3,"DB-LINUX-GET",STD_ERR_EXT_PRIV(err));
    } while(0);

    close(sock);
    return err;
}

t_std_error nas_os_util_int_get_if_details(const char *name, cps_api_object_t obj)
{
    struct ifreq  ifr;
    strncpy(ifr.ifr_ifrn.ifrn_name,name,sizeof(ifr.ifr_ifrn.ifrn_name)-1);
    const int NAS_LINK_MTU_HDR_SIZE = 32;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock==-1) return STD_ERR(INTERFACE,FAIL,errno);

    t_std_error err = STD_ERR(INTERFACE,FAIL,errno);

    cps_api_object_attr_add(obj, IF_INTERFACES_INTERFACE_NAME, name, (strlen(name)+1));

    do {
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
            cps_api_object_attr_add_u32(obj,BASE_IF_LINUX_IF_INTERFACES_INTERFACE_IF_FLAGS, ifr.ifr_flags);
            cps_api_object_attr_add_u32(obj,IF_INTERFACES_INTERFACE_ENABLED,
                (ifr.ifr_flags & IFF_UP) ? true :false);
        } else break;

        ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) >= 0) {
            char buff[40];
            const char *_p = std_mac_to_string((const hal_mac_addr_t *)(ifr.ifr_hwaddr.sa_data),
                                                buff, sizeof(buff));
            cps_api_object_attr_add(obj,DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS,_p,strlen(_p)+1);
        } else break;

        if (ioctl(sock, SIOCGIFMTU, &ifr) >= 0) {
            cps_api_object_attr_add_u32(obj, DELL_IF_IF_INTERFACES_INTERFACE_MTU,
                                        (ifr.ifr_mtu  + NAS_LINK_MTU_HDR_SIZE));
        } else break;

        err = STD_ERR_OK;
    } while(0);

    close(sock);
    return err;
}

