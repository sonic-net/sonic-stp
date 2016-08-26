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
 * filename: nas_os_vlan.c
 */

#include "event_log.h"
#include "nas_os_vlan.h"
#include "nas_os_vlan_utils.h"
#include "nas_os_interface.h"
#include "cps_api_object_key.h"
#include "cps_api_object_attr.h"
#include "std_mac_utils.h"
#include "dell-base-if-vlan.h"
#include "dell-base-if.h"
#include "dell-interface.h"
#include "netlink_tools.h"
#include "nas_nlmsg.h"
#include "ds_api_linux_interface.h"
#include "nas_os_int_utils.h"

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>

#define NL_MSG_BUFFER_LEN 4096

t_std_error nas_os_add_vlan(cps_api_object_t obj, hal_ifindex_t *br_index)
{
    char buff[NL_MSG_BUFFER_LEN];
    hal_ifindex_t if_index = 0;
    const char *info_kind = "bridge";

    memset(buff,0,NL_MSG_BUFFER_LEN);

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    cps_api_object_attr_t vlan_name_attr = cps_api_get_key_data(obj, IF_INTERFACES_INTERFACE_NAME);
    if(vlan_name_attr == CPS_API_ATTR_NULL) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Missing Vlan name for adding to kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    const char *br_name = cps_api_object_attr_data_bin(vlan_name_attr);

    /*
     * If Bridge already exist in the kernel then do not create one,
     * get the ifindex from kernel and return it
     */
    if((if_index = cps_api_interface_name_to_if_index(br_name)) != 0){
        *br_index = if_index;
        return STD_ERR_OK;
    }

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "ADD Bridge name %s ",
           br_name);
    nas_os_pack_nl_hdr(nlh, RTM_NEWLINK, (NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL));

    /* @TODO : Setting promisc mode to lift IP packets for now.
     *         Revisit when we get more details..
     */
    nas_os_pack_if_hdr(ifmsg, AF_BRIDGE, (IFF_BROADCAST | IFF_PROMISC), if_index);

    //Add the interface name
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_IFNAME, br_name, (strlen(br_name)+1));

    //Add MAC if already sent
    cps_api_object_attr_t mac_attr = cps_api_object_attr_get(obj,DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS);

    if(mac_attr !=NULL) {
        hal_mac_addr_t mac_addr;
        void *addr = cps_api_object_attr_data_bin(mac_attr);
        if (std_string_to_mac(&mac_addr, (const char *)addr, sizeof(mac_addr))) {
            nlmsg_add_attr(nlh, sizeof(buff), IFLA_ADDRESS, &mac_addr , sizeof(hal_mac_addr_t));
            EV_LOG(INFO, NAS_OS, ev_log_s_MAJOR, "NAS-OS", "Setting mac address %s in vlan interface %s ",
                    (const char *)addr, br_name);
        }
    }

    //Add the info_kind to indicate bridge
    struct nlattr *attr_nh = nlmsg_nested_start(nlh, sizeof(buff));
    attr_nh->nla_len = 0;
    attr_nh->nla_type = IFLA_LINKINFO;
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_INFO_KIND, info_kind, (strlen(info_kind)+1));
    nlmsg_nested_end(nlh,attr_nh);

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh,buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure adding Vlan %s to kernel",
               br_name);
        return (STD_ERR(NAS_OS,FAIL, 0));
    }
    //return the kernel index to caller, used by caller in case of ADD VLAN
    if((*br_index = cps_api_interface_name_to_if_index(br_name)) == 0) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Error finding the ifindex of bridge");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_del_vlan(cps_api_object_t obj)
{
    hal_ifindex_t if_index;
    const char *br_name = NULL;
    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "DEL Vlan");

    cps_api_object_attr_t vlan_name = cps_api_get_key_data(obj, IF_INTERFACES_INTERFACE_NAME);
    cps_api_object_attr_t vlan_if = cps_api_object_attr_get(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);

    if((vlan_if == NULL) && (vlan_name == NULL)) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Key parameters missing for vlan deletion in kernel");
                return (STD_ERR(NAS_OS,FAIL, 0));
    }

    if (vlan_if != NULL)  {
        if_index = (hal_ifindex_t)cps_api_object_attr_data_u32(vlan_if);
    } else {
        br_name = cps_api_object_attr_data_bin(vlan_name);
        if((if_index = cps_api_interface_name_to_if_index(br_name)) == 0) {
            EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Error finding the ifindex of vlan interface");
            return (STD_ERR(NAS_OS,FAIL, 0));
        }
    }
    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "DEL Bridge name %s index %d", br_name, if_index);

    return nas_os_del_interface(if_index);
}

static t_std_error nas_os_add_vlan_in_br(int vlan_id, int vlan_index, int if_index, int br_index)
{
    char buff[NL_MSG_BUFFER_LEN];
    memset(buff,0,sizeof(buff));

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    //nlmsg_len is updated in reserve api's above ..
    nas_os_pack_nl_hdr(nlh, RTM_SETLINK, (NLM_F_REQUEST | NLM_F_ACK));

    nas_os_pack_if_hdr(ifmsg, AF_UNSPEC, 0, vlan_index);

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Vlan I/F index %d Bridge %d port %d vlan %d",
           ifmsg->ifi_index, br_index, if_index, vlan_id);

    nlmsg_add_attr(nlh,sizeof(buff),IFLA_MASTER, &br_index, sizeof(int));
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_LINK, &if_index, sizeof(int));

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh,buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure adding port to bridge in kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_t_port_to_vlan(int vlan_id, const char *vlan_name, int port_index, int br_index, int *vlan_index)
{
    char buff[NL_MSG_BUFFER_LEN];

    memset(buff,0,sizeof(buff));

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    nas_os_pack_nl_hdr(nlh, RTM_NEWLINK, (NLM_F_REQUEST | NLM_F_ACK | NLM_F_CREATE | NLM_F_EXCL));

    db_interface_state_t astate;
    db_interface_operational_state_t ostate;

    unsigned int flags = (IFF_BROADCAST | IFF_MULTICAST) ;

    char if_name[HAL_IF_NAME_SZ+1];

    cps_api_interface_if_index_to_name(port_index, if_name, sizeof(if_name));

    if (nas_os_util_int_admin_state_get(if_name,&astate,&ostate)!=STD_ERR_OK)
        return (STD_ERR(NAS_OS,FAIL, 0));;

    /* For tagged interface, kernel does not inherit the port's admin status
     * during creation. But any update that happens on the port later
     * is inherited. Update the tagged interface admin to port's admin status */
    if(astate == DB_ADMIN_STATE_UP) {
        flags |= IFF_UP;
    }

    nas_os_pack_if_hdr(ifmsg, AF_UNSPEC, flags, 0);

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Add tagged Vlan Name %s Vlan Id %d for port %d",
           vlan_name, vlan_id, port_index);

    nlmsg_add_attr(nlh,sizeof(buff), IFLA_IFNAME, vlan_name, (strlen(vlan_name)+1));
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_LINK, &port_index, sizeof(int));

    /* VLAN info is set of nested attributes
     * IFLA_LINK_INFO(IFLA_INFO_KIND, IFLA_INFO_DATA(IFLA_VLAN_ID))*/
    struct nlattr *attr_nh = nlmsg_nested_start(nlh, sizeof(buff));
    attr_nh->nla_len = 0;
    attr_nh->nla_type = IFLA_LINKINFO;

    const char *info_kind = "vlan";
    nlmsg_add_attr(nlh,sizeof(buff),IFLA_INFO_KIND, info_kind, (strlen(info_kind)+1));

    if(vlan_id != 0) {
        struct nlattr *attr_nh_data = nlmsg_nested_start(nlh, sizeof(buff));
        attr_nh_data->nla_len = 0;
        attr_nh_data->nla_type = IFLA_INFO_DATA;

        nlmsg_add_attr(nlh,sizeof(buff),IFLA_VLAN_ID, &vlan_id, sizeof(int));

        nlmsg_nested_end(nlh,attr_nh_data);
    }
    //End of IFLA_LINK_INFO
    nlmsg_nested_end(nlh,attr_nh);

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh,buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure adding vlan interface in kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    if((*vlan_index = cps_api_interface_name_to_if_index(vlan_name)) == 0) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Error finding the ifindex of vlan interface");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }
    //Add the interface in the bridge now
    return (nas_os_add_vlan_in_br(vlan_id, *vlan_index, port_index, br_index));

}

t_std_error nas_os_ut_port_to_vlan(hal_ifindex_t port_index, hal_ifindex_t br_index)
{
    return (nas_os_add_vlan_in_br(0, port_index, port_index, br_index));
}

t_std_error nas_os_add_port_to_vlan(cps_api_object_t obj, hal_ifindex_t *vlan_index)
{
    char buff[NL_MSG_BUFFER_LEN];
    memset(buff,0,sizeof(buff));

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "ADD Port to Vlan");

    cps_api_object_attr_t vlan_index_attr = cps_api_object_attr_get(obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
    cps_api_object_attr_t vlan_id_attr = cps_api_object_attr_get(obj, BASE_IF_VLAN_IF_INTERFACES_INTERFACE_ID);
    cps_api_object_attr_t vlan_t_port_attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_TAGGED_PORTS);
    cps_api_object_attr_t vlan_ut_port_attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS);

    if((vlan_index_attr == NULL) ||
       (vlan_id_attr == NULL) ||
       ((vlan_t_port_attr == NULL) &&
       (vlan_ut_port_attr == NULL))) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Key parameters missing for vlan addition in kernel");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    int vlan_id = (int)cps_api_object_attr_data_u32(vlan_id_attr);
    hal_ifindex_t br_index = (int)cps_api_object_attr_data_u32(vlan_index_attr);
    hal_ifindex_t if_index = 0;
    // for now adding only one port at a time
    if (vlan_t_port_attr != NULL) {
        if_index = (hal_ifindex_t)cps_api_object_attr_data_u32(vlan_t_port_attr);
    }
    else {
        if_index = (hal_ifindex_t)cps_api_object_attr_data_u32(vlan_ut_port_attr);
        //set the vlan id to 0 for untagged port as kernel does not use it.
        vlan_id = 0;
    }

    /* construct the vlan interface name, if port is e00-1 and vlan_id is 100
     * then interface is e00-1.100 */

    //get port name first
    char if_name[HAL_IF_NAME_SZ+1], vlan_name[HAL_IF_NAME_SZ+1];
    if(cps_api_interface_if_index_to_name(if_index, if_name, sizeof(if_name)) == NULL) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure getting interface name for %d", if_index);
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    nas_os_get_vlan_if_name(if_name, sizeof(if_name), vlan_id, vlan_name);

    // @TODO check for admin status and bring it up after adding port to vlan.

    if(vlan_t_port_attr) {
        return nas_os_t_port_to_vlan(vlan_id, vlan_name, if_index, br_index, vlan_index);
    }
    //if not tagged then add untagged port
    return nas_os_ut_port_to_vlan(if_index, br_index);
}

static t_std_error nas_os_del_vlan_from_br(hal_ifindex_t vlan_if_index)
{
    char buff[NL_MSG_BUFFER_LEN];
    memset(buff,0,sizeof(buff));

    struct nlmsghdr *nlh = (struct nlmsghdr *) nlmsg_reserve((struct nlmsghdr *)buff,sizeof(buff),sizeof(struct nlmsghdr));
    struct ifinfomsg *ifmsg = (struct ifinfomsg *) nlmsg_reserve(nlh,sizeof(buff),sizeof(struct ifinfomsg));

    if(vlan_if_index == 0) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Invalid vlan interface index for deletion");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    nas_os_pack_nl_hdr(nlh, RTM_SETLINK, (NLM_F_REQUEST | NLM_F_ACK));

    nas_os_pack_if_hdr(ifmsg, AF_UNSPEC, 0, vlan_if_index);

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Del i/f %d from bridge",
            vlan_if_index);
    //when deleting from bridge set the master index to zero
    int master_index = 0;

    nlmsg_add_attr(nlh, sizeof(buff),IFLA_MASTER, &master_index, sizeof(int));

    if(nl_do_set_request(nas_nl_sock_T_INT,nlh,buff,sizeof(buff)) != STD_ERR_OK) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure deleting port from bridge in kernel");
           return (STD_ERR(NAS_OS,FAIL, 0));
    }

    return STD_ERR_OK;
}

t_std_error nas_os_del_port_from_vlan(cps_api_object_t obj)
{
    char if_name[HAL_IF_NAME_SZ+1] = "", vlan_name[HAL_IF_NAME_SZ+1];

    cps_api_object_attr_t vlan_id_attr = cps_api_object_attr_get(obj, BASE_IF_VLAN_IF_INTERFACES_INTERFACE_ID);
    cps_api_object_attr_t vlan_t_port_attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_TAGGED_PORTS);
    cps_api_object_attr_t vlan_ut_port_attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_UNTAGGED_PORTS);

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Del port from Vlan");

    if((vlan_id_attr == NULL) ||
      ((vlan_t_port_attr == NULL) &&
      (vlan_ut_port_attr == NULL))) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Parameters missing for deletion of vlan");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    int vlan_id = (int)cps_api_object_attr_data_u32(vlan_id_attr);
    hal_ifindex_t port_index = 0;
    // for now deleting only one port at a time
    if(vlan_t_port_attr != NULL) {
        port_index = (int)cps_api_object_attr_data_u32(vlan_t_port_attr);

        if(cps_api_interface_if_index_to_name(port_index, if_name, sizeof(if_name)) == NULL) {
            EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure getting interface name for %d", port_index);
            return (STD_ERR(NAS_OS,FAIL, 0));
        }

        nas_os_get_vlan_if_name(if_name, sizeof(if_name), vlan_id, vlan_name);

        int vlan_if_index = cps_api_interface_name_to_if_index(vlan_name);
        if (vlan_if_index == 0) {
            EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "No interface exist for %s \n", vlan_name);
            return (STD_ERR(NAS_OS,FAIL, 0));
        }
        if(nas_os_del_vlan_from_br(vlan_if_index) != STD_ERR_OK)
            return (STD_ERR(NAS_OS,FAIL, 0));
                //Call the interface delete
        return nas_os_del_interface(vlan_if_index);
    }
    else {
        port_index = (int)cps_api_object_attr_data_u32(vlan_ut_port_attr);
        return nas_os_del_vlan_from_br(port_index);
    }

}


t_std_error nas_os_del_vlan_interface(cps_api_object_t obj)
{
    char if_name[HAL_IF_NAME_SZ+1] = "", vlan_name[HAL_IF_NAME_SZ+1] = "";

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Del Vlan Interface ");

    cps_api_object_attr_t port_attr = cps_api_object_attr_get(obj, DELL_IF_IF_INTERFACES_INTERFACE_TAGGED_PORTS);
    cps_api_object_attr_t vlan_id_attr = cps_api_object_attr_get(obj, BASE_IF_VLAN_IF_INTERFACES_INTERFACE_ID);

    if((port_attr == NULL) ||
       (vlan_id_attr == NULL)){
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Missing Vlan interface parameters for deletion");
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    hal_ifindex_t port_index = (int)cps_api_object_attr_data_u32(port_attr);
    hal_vlan_id_t vlan_id = (hal_vlan_id_t)cps_api_object_attr_data_u32(vlan_id_attr);

    EV_LOG(INFO, NAS_OS, ev_log_s_MINOR, "NAS-OS", "Vlan i/f %d, ID %d for deletion", port_index, vlan_id);

    if(cps_api_interface_if_index_to_name(port_index, if_name, sizeof(if_name)) == NULL) {
        EV_LOG(ERR, NAS_OS, ev_log_s_CRITICAL, "NAS-OS", "Failure getting interface name for %d", port_index);
        return (STD_ERR(NAS_OS,FAIL, 0));
    }

    nas_os_get_vlan_if_name(if_name, sizeof(if_name), vlan_id, vlan_name);

    int vlan_if_index = cps_api_interface_name_to_if_index(vlan_name);

    //Call the interface delete
    return nas_os_del_interface(vlan_if_index);
}



