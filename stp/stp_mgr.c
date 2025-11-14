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

#include "stp_inc.h"
#include "stp_main.h"

MAC_ADDRESS g_stp_base_mac_addr;

char msgtype_str[][64] = {
    "STP_INVALID_MSG",
    "STP_INIT_READY",
    "STP_BRIDGE_CONFIG",
    "STP_VLAN_CONFIG",
    "STP_VLAN_PORT_CONFIG",
    "STP_PORT_CONFIG",
    "STP_VLAN_MEM_CONFIG",
    "STP_STPCTL_MSG",
    "STP_MST_GLOBAL_CONFIG",
    "STP_MST_INST_CONFIG",
    "STP_MST_VLAN_PORT_LIST_CONFIG",
    "STP_MST_INST_PORT_CONFIG",
    "STP_MAX_MSG"
};

void stpmgr_libevent_destroy(struct event *ev)
{
    g_stpd_stats_libev_no_of_sockets--;
    event_del(ev);
}

struct event *stpmgr_libevent_create(struct event_base *base, 
        evutil_socket_t sock,
        short flags,
        void *cb_fn,
        void *arg, 
        const struct timeval *tv)
{
    struct event *ev = 0;
    int prio;

    g_stpd_stats_libev_no_of_sockets++;

    if (-1 == sock) //100ms timer
        prio = STP_LIBEV_HIGH_PRI_Q;
    else
    {
        prio = STP_LIBEV_LOW_PRI_Q;
        evutil_make_socket_nonblocking(sock);
    }

    ev = event_new(base, sock, flags, cb_fn, arg);
    if (ev)
    {
        if(-1 == event_priority_set(ev, prio))
        {
            STP_LOG_ERR("event_priority_set failed");
            return NULL;
        }

        if (-1 != event_add(ev, tv))
        {
            STP_LOG_DEBUG("Event Added : ev-%p, arg : %s", ev, (char *)arg);
            STP_LOG_DEBUG("base : %p, sock : %d, flags : %x, cb_fn : %p", base, sock, flags, cb_fn);
            if (tv)
                STP_LOG_DEBUG("tv.sec : %u, tv.usec : %u", tv->tv_sec, tv->tv_usec);

            return ev;
        }
    }
    return NULL;
}



/* FUNCTION
 *		stpmgr_init()
 *
 * SYNOPSIS
 *	    initializes stp global data	
 */
void stpmgr_init(UINT16 max_stp_instances)
{
    if(max_stp_instances == 0)
        sys_assert(0);

    if (stpdata_init_global_structures(max_stp_instances) == false)
    {
        STP_LOG_CRITICAL("error - STP global structures initialization failed");
        sys_assert(0);
    }

    STP_LOG_INFO("init done, max stp instances %d", max_stp_instances);
}

/* FUNCTION
 *		stpmgr_initialize_stp_class()
 *
 * SYNOPSIS
 *		initialize an inactive stp class.
 */
void stpmgr_initialize_stp_class(STP_CLASS *stp_class, VLAN_ID vlan_id)
{
	STP_INDEX stp_index;

	stp_index = GET_STP_INDEX(stp_class);

	stp_class->vlan_id = vlan_id;
	
	stputil_set_bridge_priority(&stp_class->bridge_info.bridge_id, STP_DFLT_PRIORITY, vlan_id);
	NET_TO_HOST_MAC(&stp_class->bridge_info.bridge_id.address, &g_stp_base_mac_addr);

	stp_class->bridge_info.bridge_max_age = STP_DFLT_MAX_AGE;
	stp_class->bridge_info.bridge_hello_time = STP_DFLT_HELLO_TIME;
	stp_class->bridge_info.bridge_forward_delay = STP_DFLT_FORWARD_DELAY;
	stp_class->bridge_info.hold_time = STP_DFLT_HOLD_TIME;

	stp_class->bridge_info.root_id = stp_class->bridge_info.bridge_id;
	stp_class->bridge_info.root_path_cost = 0;
	stp_class->bridge_info.root_port = STP_INVALID_PORT;

	stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
	stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
	stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;
    SET_ALL_BITS(stp_class->bridge_info.modified_fields); 
    SET_ALL_BITS(stp_class->modified_fields);
}

/* FUNCTION
 *		stpmgr_initialize_control_port()
 *
 * SYNOPSIS
 *		initialize the ports in the input control mask. filters out those
 *		ports that are already part of the control mask.
 */
void stpmgr_initialize_control_port(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	memset(stp_port_class, 0, sizeof(STP_PORT_CLASS));

	// initialize non-zero values
	stp_port_class->port_id.number = port_number;
	stp_port_class->port_id.priority = stp_intf_get_port_priority(port_number); 
	stp_port_class->path_cost = stp_intf_get_path_cost(port_number);
	stp_port_class->change_detection_enabled = true;
	stp_port_class->auto_config = true;
}

/* FUNCTION
 *		stpmgr_activate_stp_class()
 *
 * SYNOPSIS
 *		takes the stp class from stp config state to the stp class active
 *		state.
 */
void stpmgr_activate_stp_class(STP_CLASS *stp_class)
{
	stp_class->state = STP_CLASS_ACTIVE;
	
	stp_class->bridge_info.topology_change_detected = false;
	stp_class->bridge_info.topology_change = false;

	stptimer_stop(&stp_class->tcn_timer);
	stptimer_stop(&stp_class->topology_change_timer);

	port_state_selection(stp_class);
	config_bpdu_generation(stp_class);
	stptimer_start(&stp_class->hello_timer, 0);
}

/* FUNCTION
 *		stpmgr_deactivate_stp_class()
 *
 * SYNOPSIS
 *		moves the stp class from active to config state.
 */
void stpmgr_deactivate_stp_class(STP_CLASS *stp_class)
{
	if (stp_class->state == STP_CLASS_CONFIG)
		return;

	stp_class->state = STP_CLASS_CONFIG;

	stptimer_stop(&stp_class->tcn_timer);
	stptimer_stop(&stp_class->topology_change_timer);
	stptimer_stop(&stp_class->hello_timer);

	if (stp_class->bridge_info.topology_change)
	{
		stp_class->bridge_info.topology_change = false;
		stputil_set_vlan_topo_change(stp_class);
	}

	// reset root specific information
	stp_class->bridge_info.root_id = stp_class->bridge_info.bridge_id;
	stp_class->bridge_info.root_path_cost = 0;
	stp_class->bridge_info.root_port = STP_INVALID_PORT;
	
	stpmgr_set_bridge_params(stp_class);
}

/* 8.8.1 */
void stpmgr_initialize_port(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
    STP_LOG_DEBUG("vlan %d port %d", stp_class->vlan_id, port_number);

	become_designated_port(stp_class, port_number);

    stp_port_class->state = BLOCKING;
    stputil_set_port_state(stp_class, stp_port_class);

	stp_port_class->topology_change_acknowledge = false;
	stp_port_class->config_pending = false;
	stp_port_class->change_detection_enabled = true;
	stp_port_class->self_loop = false;

	stptimer_stop(&stp_port_class->message_age_timer);
	stptimer_stop(&stp_port_class->forward_delay_timer);
	stptimer_stop(&stp_port_class->hold_timer);
}

/* 8.8.2 */
void stpmgr_enable_port(STP_CLASS *stp_class, PORT_ID port_number)
{	
	STP_PORT_CLASS *stp_port_class;
	bool result = false;
	
	if (is_member(stp_class->enable_mask, port_number))
		return;

	set_mask_bit(stp_class->enable_mask, port_number);

	stpmgr_initialize_port(stp_class, port_number);

	port_state_selection(stp_class);
}

/* 8.8.3 */
void stpmgr_disable_port(STP_CLASS *stp_class, PORT_ID port_number)
{
	bool root;
	STP_PORT_CLASS *stp_port_class;

	if (!is_member(stp_class->enable_mask, port_number))
		return;

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	/* this can happen if a module has been deleted, stp can remove the
	 * data structure before vlan has a chance to cleanup. adding a check
	 * to only this routine.
	 */
	if (stp_port_class == NULL)
		return;

	root = root_bridge(stp_class);
	become_designated_port(stp_class, port_number);

	stp_port_class->state = DISABLED;

	/* do not send a message to VPORT manager to set the state to
	 * disabled - this call is being initiated from there
	 */

	stp_port_class->topology_change_acknowledge = false;
	stp_port_class->config_pending = false;
	stp_port_class->change_detection_enabled = true;
	stp_port_class->self_loop = false;
    stp_port_class->bpdu_guard_active = false;

	stptimer_stop(&stp_port_class->message_age_timer);
	stptimer_stop(&stp_port_class->forward_delay_timer);

	if (stp_port_class->root_protect_timer.active == true)
	{
		stp_port_class->root_protect_timer.active = false;
		stptimer_stop(&stp_port_class->root_protect_timer);
	}

	clear_mask_bit(stp_class->enable_mask, port_number);
	configuration_update(stp_class);
	port_state_selection(stp_class);

	if (root_bridge(stp_class) && !root)
	{
		stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
		stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
		stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;

		topology_change_detection(stp_class);
		stptimer_stop(&stp_class->tcn_timer);
		config_bpdu_generation(stp_class);
		stptimer_start(&stp_class->hello_timer, 0);
	}
}

/* 8.8.4 */
void stpmgr_set_bridge_priority(STP_CLASS *stp_class, BRIDGE_IDENTIFIER *bridge_id)
{
	bool root;
	PORT_ID port_number;
	STP_PORT_CLASS *stp_port_class;

	root = root_bridge(stp_class);

	port_number = port_mask_get_first_port(stp_class->enable_mask);
	while (port_number != BAD_PORT_ID)
	{
		stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
		if (designated_port(stp_class, port_number))
		{
			stp_port_class->designated_bridge = *bridge_id;
            SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_BRIDGE_BIT);
		}

		port_number = port_mask_get_next_port(stp_class->enable_mask, port_number);
	}

	stp_class->bridge_info.bridge_id = *bridge_id;
	
	configuration_update(stp_class);	
	port_state_selection(stp_class);

	if (root_bridge(stp_class))
	{
		if (!root)
		{
			stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
			stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
			stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;

			topology_change_detection(stp_class);
			stptimer_stop(&stp_class->tcn_timer);
			config_bpdu_generation(stp_class);
			stptimer_start(&stp_class->hello_timer, 0);
		}
	}
}

/* 8.8.5 */
void stpmgr_set_port_priority(STP_CLASS *stp_class, PORT_ID port_number, UINT16 priority)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	if (designated_port(stp_class, port_number))
	{
		stp_port_class->designated_port.priority = priority >> 4;
	}

	stp_port_class->port_id.priority = priority >> 4;
    SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT);

	if (stputil_compare_bridge_id(&stp_class->bridge_info.bridge_id, &stp_port_class->designated_bridge) == EQUAL_TO &&
		stputil_compare_port_id(&stp_port_class->port_id, &stp_port_class->designated_port) == LESS_THAN)
	{
		become_designated_port (stp_class, port_number);
		port_state_selection (stp_class);
    
        SET_BIT(stp_port_class->modified_fields, STP_PORT_CLASS_MEMBER_DESIGN_PORT_BIT);
	}
}

/* 8.8.6 */
void stpmgr_set_path_cost(STP_CLASS *stp_class, PORT_ID port_number, bool auto_config, UINT32 path_cost)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);

	stp_port_class->path_cost = path_cost;
	stp_port_class->auto_config = auto_config;

	configuration_update(stp_class);
	port_state_selection(stp_class);
    
}

/* 8.8.7 */
void stpmgr_enable_change_detection(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	stp_port_class->change_detection_enabled = true;
}

/* 8.8.8 */
void stpmgr_disable_change_detection(STP_CLASS *stp_class, PORT_ID port_number)
{
	STP_PORT_CLASS *stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	stp_port_class->change_detection_enabled = false;
}

/* FUNCTION
 *		stpmgr_set_bridge_params()
 *
 * SYNOPSIS
 *		used when user configures an stp bridge parameter to propagate the
 *		values if this is the root bridge.
 */
void stpmgr_set_bridge_params(STP_CLASS *stp_class)
{
	if (root_bridge(stp_class))
	{
		stp_class->bridge_info.max_age = stp_class->bridge_info.bridge_max_age;
		stp_class->bridge_info.hello_time = stp_class->bridge_info.bridge_hello_time;
		stp_class->bridge_info.forward_delay = stp_class->bridge_info.bridge_forward_delay;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT);
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT);
	}
}

/* FUNCTION
 *		stpmgr_config_bridge_priority()
 *
 * SYNOPSIS
 *		sets the bridge priority.
 */
bool stpmgr_config_bridge_priority(STP_INDEX stp_index, UINT16 priority)
{
	STP_CLASS *stp_class;
	BRIDGE_IDENTIFIER bridge_id;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	bridge_id = stp_class->bridge_info.bridge_id;

    if (stputil_get_bridge_priority(&bridge_id) != priority)
    {
	    stputil_set_bridge_priority(&bridge_id, priority, stp_class->vlan_id);

	    if (stp_class->state == STP_CLASS_ACTIVE)
	    {
		    stpmgr_set_bridge_priority(stp_class, &bridge_id);
            /* Sync to APP DB */
            SET_ALL_BITS(stp_class->bridge_info.modified_fields); 
            SET_ALL_BITS(stp_class->modified_fields);
	    }
	    else
	    {
		    stp_class->bridge_info.bridge_id = bridge_id;
		    stp_class->bridge_info.root_id = bridge_id;
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT);
            SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT);
	    }
    }

	return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_max_age()
 *
 * SYNOPSIS
 *		sets the bridge max-age.
 */
bool stpmgr_config_bridge_max_age(STP_INDEX stp_index, UINT16 max_age)
{
	STP_CLASS *stp_class;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
    
    if (max_age && stp_class->bridge_info.bridge_max_age != max_age)
    {
	    stp_class->bridge_info.bridge_max_age = (UINT8) max_age;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_MAX_AGE_BIT);
	    stpmgr_set_bridge_params(stp_class);
    }
	return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_hello_time()
 *
 * SYNOPSIS
 *		sets the bridge hello-time.
 */
bool stpmgr_config_bridge_hello_time(STP_INDEX stp_index, UINT16 hello_time)
{
	STP_CLASS *stp_class;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);

    if (hello_time && stp_class->bridge_info.bridge_hello_time != hello_time)
    {
	    stp_class->bridge_info.bridge_hello_time = (UINT8) hello_time;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_HELLO_TIME_BIT);
	    stpmgr_set_bridge_params(stp_class);
    }
	return true;
}

/* FUNCTION
 *		stpmgr_config_bridge_forward_delay()
 *
 * SYNOPSIS
 *		sets the bridge forward-delay.
 */
bool stpmgr_config_bridge_forward_delay(STP_INDEX stp_index, UINT16 fwd_delay)
{
	STP_CLASS *stp_class;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);

    if (fwd_delay && stp_class->bridge_info.bridge_forward_delay != fwd_delay)
    {
	    stp_class->bridge_info.bridge_forward_delay = (UINT8) fwd_delay;
        SET_BIT(stp_class->bridge_info.modified_fields, STP_BRIDGE_DATA_MEMBER_BRIDGE_FWD_DELAY_BIT);
	    stpmgr_set_bridge_params(stp_class);
    }

	return true;
}

/* FUNCTION
 *		stpmgr_config_port_priority()
 *
 * SYNOPSIS
 *		sets the port priority.
 */
bool stpmgr_config_port_priority(STP_INDEX stp_index, PORT_ID port_number, UINT16 priority, bool is_global)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	if (!is_member(stp_class->control_mask, port_number))
		return false;

	stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
    if (is_global)
    {
        /* If per vlan attributes are set, ignore global attributes */
        if (IS_STP_PER_VLAN_FLAG_SET(stp_port, STP_CLASS_PORT_PRI_FLAG))
            return true;
    }
    else
    {
        if (priority == stp_intf_get_port_priority(port_number))
            CLR_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PORT_PRI_FLAG);
        else
            SET_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PORT_PRI_FLAG);
    }

	if (stp_class->state == STP_CLASS_ACTIVE)
	{
		stpmgr_set_port_priority(stp_class, port_number, priority);
	}
	else
	{
		stp_port->port_id.priority = priority >> 4;
	}
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PORT_PRIORITY_BIT);
	return true;
}

/* FUNCTION
 *		stpmgr_config_port_path_cost()
 *
 * SYNOPSIS
 *		sets the ports path cost.
 */
bool stpmgr_config_port_path_cost(STP_INDEX stp_index, PORT_ID port_number, bool auto_config, 
                                UINT32 path_cost, bool is_global)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;
    UINT32 def_path_cost;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	if (!is_member(stp_class->control_mask, port_number))
		return false;

	stp_port = GET_STP_PORT_CLASS(stp_class, port_number);

    def_path_cost = stp_intf_get_path_cost(port_number);
    if (is_global)
    {
        /* If per vlan attributes are set, ignore global attributes */
        if (IS_STP_PER_VLAN_FLAG_SET(stp_port, STP_CLASS_PATH_COST_FLAG))
            return true;
    }
    else
    {
        if (path_cost == def_path_cost)
            CLR_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PATH_COST_FLAG);
        else
            SET_STP_PER_VLAN_FLAG(stp_port, STP_CLASS_PATH_COST_FLAG);
    }

	if (auto_config)
	{
		path_cost = def_path_cost;
	}

	if (stp_class->state == STP_CLASS_ACTIVE)
	{
		stpmgr_set_path_cost(stp_class, port_number, auto_config, path_cost);
	}
	else
	{
		stp_port->path_cost = path_cost;
		stp_port->auto_config = auto_config;
	}
    SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_MEMBER_PATH_COST_BIT);

	return true;
}

/*****************************************************************************/
/* stpmgr_clear_port_statistics: clears the bpdu statistics associated with  */
/* input port.                                                               */
/*****************************************************************************/
static void stpmgr_clear_port_statistics(STP_CLASS *stp_class, PORT_ID port_number)
{    
    STP_PORT_CLASS *stp_port = NULL;

    if (port_number == BAD_PORT_ID)
    {
        /* Clear stats for all interface */
        port_number = port_mask_get_first_port(stp_class->control_mask);
        while (port_number != BAD_PORT_ID)
        {
            stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
            if (stp_port != NULL)
            {
                stp_port->rx_config_bpdu =
                stp_port->rx_tcn_bpdu =
                stp_port->tx_config_bpdu =
                stp_port->tx_tcn_bpdu = 0;
            }
            SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT);
            stputil_sync_port_counters(stp_class, stp_port);
            port_number = port_mask_get_next_port(stp_class->control_mask, port_number);
        }
    }
    else
    {
        stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
        if (stp_port != NULL)
        {
            stp_port->rx_config_bpdu =
            stp_port->rx_tcn_bpdu =
            stp_port->tx_config_bpdu =
            stp_port->tx_tcn_bpdu = 0;
            SET_BIT(stp_port->modified_fields, STP_PORT_CLASS_CLEAR_STATS_BIT);
            stputil_sync_port_counters(stp_class, stp_port);
        }
    }
}

/*****************************************************************************/
/* stpmgr_clear_statistics: clears the bpdu statistics associated with all   */
/* the ports in input mask.                                                  */
/*****************************************************************************/
void stpmgr_clear_statistics(VLAN_ID vlan_id, PORT_ID port_number)
{
    STP_INDEX stp_index;
    STP_CLASS *stp_class;

    if (vlan_id == VLAN_ID_INVALID)
    {
        for (stp_index = 0; stp_index < g_stp_instances; stp_index++)
        {
            stp_class = GET_STP_CLASS(stp_index);
            if (stp_class->state == STP_CLASS_FREE)
                continue;
            stpmgr_clear_port_statistics(stp_class, port_number);
        }
    }
    else
    {
        if (stputil_get_index_from_vlan(vlan_id, &stp_index) == true)
        {
            stp_class = GET_STP_CLASS(stp_index);
            if (stp_class->state != STP_CLASS_FREE)
                stpmgr_clear_port_statistics(stp_class, port_number);
        }
    }
}

/* FUNCTION
 *		stpmgr_release_index()
 *
 * SYNOPSIS
 *		releases the stp index
 */
bool stpmgr_release_index(STP_INDEX stp_index)
{
	STP_CLASS *stp_class;
	PORT_ID port_number;

	if (stp_index == STP_INDEX_INVALID)
		return false;

	stp_class = GET_STP_CLASS(stp_index);
	if (stp_class->state == STP_CLASS_FREE)
		return true; // already released

	clear_mask(stp_class->enable_mask);
	stpmgr_deactivate_stp_class(stp_class);

	port_number = port_mask_get_first_port(stp_class->control_mask);
	while (port_number != BAD_PORT_ID)
	{
		stpmgr_delete_control_port(stp_index, port_number, true);
		port_number = port_mask_get_next_port(stp_class->control_mask, port_number);
	}
    
    stpsync_del_vlan_from_instance(stp_class->vlan_id, stp_index);
    stpsync_del_stp_class(stp_class->vlan_id);

	stpdata_class_free(stp_index);

	return true;
}

/* FUNCTION
 *		stpmgr_add_control_port()
 *
 * SYNOPSIS
 *		adds the port mask to the list of stp ports controlled by the
 *		stp instance
 */
bool stpmgr_add_control_port(STP_INDEX stp_index, PORT_ID port_number, uint8_t mode)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port_class;

    STP_LOG_DEBUG("add_control_port inst %d port %d", stp_index, port_number);
	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	if (stp_class->state == STP_CLASS_FREE)
	{
		return false;
	}

	// filter out ports that are already part of the control mask
	if (is_member(stp_class->control_mask, port_number))
		return true;

	set_mask_bit(stp_class->control_mask, port_number);

    if (mode == 0) // UnTagged mode
    	set_mask_bit(stp_class->untag_mask, port_number);

	stpmgr_initialize_control_port(stp_class, port_number);

	stp_port_class = GET_STP_PORT_CLASS(stp_class, port_number);
	if (stp_intf_is_port_up(port_number))
	{
		stpmgr_add_enable_port(stp_index, port_number);
	}
    else
    {
        stputil_set_port_state(stp_class, stp_port_class);
    }
	
    if(stp_port_class)
    {
        SET_ALL_BITS(stp_port_class->modified_fields);
    }

	return true;
}

/* FUNCTION
 *		stpmgr_delete_control_port()
 *
 * SYNOPSIS
 *		removes the port mask from the list of stp ports controlled by the
 *		stp instance
 */
bool stpmgr_delete_control_port(STP_INDEX stp_index, PORT_ID port_number, bool del_stp_port)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;
	char * if_name;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	if (stp_class->state == STP_CLASS_FREE)
	{
		return false;
	}

	if (!is_member(stp_class->control_mask, port_number))
	{
		return false;
	}

	stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
	/* Reset state to forwarding before deletion */
	stp_port->state = FORWARDING;
    stputil_set_kernel_bridge_port_state(stp_class, stp_port);
    if(!del_stp_port)
        stpsync_update_port_state(GET_STP_PORT_IFNAME(stp_port), GET_STP_INDEX(stp_class), stp_port->state);

	stpmgr_delete_enable_port(stp_index, port_number);

	if_name = stp_intf_get_port_name(port_number);
	if(if_name)
    {
        /* When STP is disabled on port, we still need the STP port to be active in Orchagent and SAI
         * so that port state is in forwarding, deletion will result in removal in SAI which in turn will
         * set the STP state to disabled
         * */
        if(del_stp_port)
    	    stpsync_del_port_state(if_name, stp_index);
        stpsync_del_port_class(if_name, stp_class->vlan_id);
    }

	clear_mask_bit(stp_class->control_mask, port_number);
    clear_mask_bit(stp_class->untag_mask, port_number);

	return true;
}

/* FUNCTION
 *		stpmgr_add_enable_port()
 *
 * SYNOPSIS
 *		adds the port to the list of enabled stp ports
 */
bool stpmgr_add_enable_port(STP_INDEX stp_index, PORT_ID port_number)
{
	STP_CLASS *stp_class;

	if (stp_index == STP_INDEX_INVALID)
    {
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
    }

	stp_class = GET_STP_CLASS(stp_index);
	if (is_member(stp_class->enable_mask, port_number))
		return true;

	// check if the port is a member of the control mask before enabling
	if (!is_member(stp_class->control_mask, port_number))
	{
		STP_LOG_ERR("port %d not part of control mask (stp_index %u)",
			port_number, stp_index);
		return false;
	}

	if (stp_class->state == STP_CLASS_CONFIG)
	{
		stpmgr_activate_stp_class(stp_class);
	}

	stpmgr_enable_port(stp_class, port_number);
    
	return true;
}

/* FUNCTION
 *		stpmgr_delete_enable_port()
 *
 * SYNOPSIS
 *		removes the port from the list of enabled stp ports
 */
bool stpmgr_delete_enable_port(STP_INDEX stp_index, PORT_ID port_number)
{
	STP_CLASS *stp_class;

	if (stp_index == STP_INDEX_INVALID)
	{
        STP_LOG_ERR("invalid stp index %d", stp_index);
		return false;
	}

	stp_class = GET_STP_CLASS(stp_index);
	if (!is_member(stp_class->enable_mask, port_number))
		return true;

	stpmgr_disable_port(stp_class, port_number);
	if (is_mask_clear(stp_class->enable_mask))
	{
		stpmgr_deactivate_stp_class(stp_class);
	}
	return true;
}

/* FUNCTION
 *		stpmgr_update_stats()
 *
 * SYNOPSIS
 *		updates stp port statistics
 */
static void stpmgr_update_stats(STP_INDEX stp_index, PORT_ID port_number, void *buffer, bool pvst)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;
	STP_CONFIG_BPDU *bpdu;
	UINT8 *typestring = NULL;

	stp_class = GET_STP_CLASS(stp_index);
	stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
	bpdu = (STP_CONFIG_BPDU *) buffer;

	switch (bpdu->type)
	{
		case RSTP_BPDU_TYPE:
		case CONFIG_BPDU_TYPE:
			stp_port->rx_config_bpdu++;
			break;

		case TCN_BPDU_TYPE:
			stp_port->rx_tcn_bpdu++;
			break;

		default:
			stp_port->rx_drop_bpdu++;
			STP_LOG_ERR("error - stpmgr_update_stats() - unknown bpdu type %u", bpdu->type);
			return;
	}
}

/* FUNCTION
 *		stpmgr_process_pvst_bpdu()
 *
 * SYNOPSIS
 *		validates the pvst bpdu and passes to the stp bpdu handler process
 */
void stpmgr_process_pvst_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer)
{
	STP_CONFIG_BPDU *bpdu;
	STP_CLASS *stp_class;

	stp_class = GET_STP_CLASS(stp_index);
	if (!is_member(stp_class->enable_mask, port_number))
	{
		if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
		{
			STP_PKTLOG("Dropping PVST BPDU, Port:%d not in Vlan:%d enable mask", port_number, stp_class->vlan_id);
		}
		stp_class->rx_drop_bpdu++;
		return;
	}

	// hack the pointer to make it appear as a config bpdu so that the
	// rest of the routines can be called without any problems.
	bpdu = (STP_CONFIG_BPDU *) (((UINT8 *)buffer) + 5);

	stputil_decode_bpdu(bpdu);
	stpmgr_update_stats(stp_index, port_number, bpdu, true /* pvst */);
	stputil_process_bpdu(stp_index, port_number, (void *) bpdu);
}

/* FUNCTION
 *		stpmgr_process_stp_bpdu()
 *
 * SYNOPSIS
 *		passes it to the protocol bpdu handler based on the type of
 *		bpdu received. note that the bpdu should have been validated
 *		before this function is called.
 */
void stpmgr_process_stp_bpdu(STP_INDEX stp_index, PORT_ID port_number, void *buffer)
{
	STP_CONFIG_BPDU *bpdu = (STP_CONFIG_BPDU *) buffer;
	STP_CLASS *stp_class = GET_STP_CLASS(stp_index);
	
	if (!is_member(stp_class->enable_mask, port_number))
	{
		if (STP_DEBUG_BPDU_RX(stp_class->vlan_id, port_number))
		{
			STP_PKTLOG("Dropping BPDU, Port:%d not in Vlan:%d enable mask", port_number, stp_class->vlan_id);
		}
		return;
	}

	stputil_decode_bpdu(bpdu);
	stpmgr_update_stats(stp_index, port_number, bpdu, false /* pvst */);
	stputil_process_bpdu(stp_index, port_number, buffer);
}


/* FUNCTION
 *		stpmgr_config_fastuplink()
 *
 * SYNOPSIS
 *		enables/disables fast uplink for all the ports in the port mask
 */
void stpmgr_config_fastuplink(PORT_ID port_number, bool enable)
{
	if (enable)
    {
		if(STP_IS_FASTUPLINK_CONFIGURED(port_number))
		    return;

        set_mask_bit(g_fastuplink_mask, port_number);
    }
	else
    {
		if(!STP_IS_FASTUPLINK_CONFIGURED(port_number))
		    return;

        clear_mask_bit(g_fastuplink_mask, port_number);
    }
}

bool stpmgr_is_bpdu_rcvd_on_bpdu_guard_enabled_port(VLAN_ID vlan_id, PORT_ID port_id)
{
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;
    STP_INDEX		stp_index = STP_INDEX_INVALID;

    if (!STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
        return false;

    stputil_get_index_from_vlan(vlan_id, &stp_index);
    if (stp_index == STP_INDEX_INVALID)
        return false;

    stp_class = GET_STP_CLASS(stp_index);
    stp_port = GET_STP_PORT_CLASS(stp_class, port_id);

    if (stp_port == NULL)
        return false;

    if(!stp_port->bpdu_guard_active)
    {
        stp_port->bpdu_guard_active = true;
        return true;
    }
    return false;
}

/* FUNCTION
 *		stpmgr_protect_process()
 *
 * SYNOPSIS
 *		If "spanning protect" is configured, take the proper action.
 *
 * RETURN VALUE
 *		true if it is "spanning protect"; false otherwise.
 */
bool stpmgr_protect_process(PORT_ID rx_port, uint16_t vlan_id)
{
	if (!STP_IS_PROTECT_CONFIGURED(rx_port) && !STP_IS_PROTECT_DO_DISABLE_CONFIGURED(rx_port))
		return (false);

	if (STP_IS_PROTECT_DO_DISABLE_CONFIGURED(rx_port)) 
	{
        //If already disabled
		if(STP_IS_PROTECT_DO_DISABLED(rx_port))
		    return (true);

		// Update protect_disabled_mask
        set_mask_bit(stp_global.protect_disabled_mask, rx_port);

		// log message
        if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
        {
            STP_SYSLOG("STP: BPDU received, interface %s disabled due to BPDU guard trigger", stp_intf_get_port_name(rx_port));
        }
        else
        {
            STP_SYSLOG("STP: BPDU(%u) received, interface %s disabled due to BPDU guard trigger", vlan_id, stp_intf_get_port_name(rx_port));
        }
		// Disable rx_port
        stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(rx_port), true);
	    stpsync_update_port_admin_state(stp_intf_get_port_name(rx_port), false, STP_IS_ETH_PORT_ID(rx_port));
	}
    else
    {
        if(stpmgr_is_bpdu_rcvd_on_bpdu_guard_enabled_port(vlan_id, rx_port) ||
                mstpmgr_is_bpdu_rcvd_on_bpdu_guard_enabled_port(rx_port))
        {
            if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
                STP_SYSLOG("STP: Warning - BPDU received on BPDU guard enabled interface %s", stp_intf_get_port_name(rx_port));
            else
                STP_SYSLOG("STP: Warning - BPDU(VLAN %u) received on BPDU guard enabled interface %s", vlan_id, stp_intf_get_port_name(rx_port));
        }
    }

	return (true);
}

/* FUNCTION
 *		stpmgr_config_fastspan()
 *
 * SYNOPSIS
 *		enables/disables stp port fast.
 *
 * NOTES
 *		
 */
static bool stpmgr_config_fastspan(PORT_ID port_id, bool enable)
{
	bool ret = true;

	if (enable)
	{
		if(is_member(g_fastspan_config_mask, port_id))
		    return ret;
        set_mask_bit(g_fastspan_config_mask, port_id);
        set_mask_bit(g_fastspan_mask, port_id);
        stpsync_update_port_fast(stp_intf_get_port_name(port_id), true);
	}
	else
	{
		if(!is_member(g_fastspan_config_mask, port_id))
		    return ret;
        clear_mask_bit(g_fastspan_config_mask, port_id);
        clear_mask_bit(g_fastspan_mask, port_id);
        stpsync_update_port_fast(stp_intf_get_port_name(port_id), false);
	}
	return ret;
}

void stpmgr_reset_bpdu_guard_active(PORT_ID port_id)
{
	STP_INDEX index;
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;

	for (index = 0; index < g_stp_instances; index++)
	{
		stp_class = GET_STP_CLASS(index);
		if ((stp_class->state == STP_CLASS_FREE) ||
			(!is_member(stp_class->control_mask, port_id)))
		{
			continue;
		}

		stp_port = GET_STP_PORT_CLASS(stp_class, port_id);

        if(stp_port && stp_port->bpdu_guard_active)
	        stp_port->bpdu_guard_active = false;
    }
}

/* FUNCTION
 *		stpmgr_config_protect()
 *
 * SYNOPSIS
 *		enables/disables stp bpdu protection on the input mask.
 *
 * NOTES
 *		The protection mask is (protect_mask || protect_do_disable_mask).
 */
bool stpmgr_config_protect(PORT_ID port_id, bool enable, bool do_disable)
{
	bool ret = true;

	if (enable)
	{
		if (do_disable)
            set_mask_bit(stp_global.protect_do_disable_mask, port_id);
        else
            clear_mask_bit(stp_global.protect_do_disable_mask, port_id);

        set_mask_bit(stp_global.protect_mask, port_id);
	}
	else
	{
        clear_mask_bit(stp_global.protect_do_disable_mask, port_id);
        
        if(STP_IS_PROTECT_DO_DISABLED(port_id))
        {
            clear_mask_bit(stp_global.protect_disabled_mask, port_id);
            stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(port_id), false);
        }

        clear_mask_bit(stp_global.protect_mask, port_id);

        if (STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
            stpmgr_reset_bpdu_guard_active(port_id);
        else if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
            mstpmgr_reset_bpdu_guard_active(port_id);
	}

	return ret;
}

/*****************************************************************************/
/* stpmgr_config_root_protect: enables/disables root-guard feature on the    */
/* input mask.                                                               */
/*****************************************************************************/
bool stpmgr_config_root_protect(PORT_ID port_id, bool enable)
{
	if (enable)
        set_mask_bit(stp_global.root_protect_mask, port_id);
	else
        clear_mask_bit(stp_global.root_protect_mask, port_id);

	return true;
}

/*****************************************************************************/
/* stpmgr_config_root_protect_timeout: configures the timeout in seconds for */
/* which the violated stp port is kept in blocking state.                    */
/*****************************************************************************/
static bool stpmgr_config_root_protect_timeout(UINT timeout)
{
	// sanity check (should never happen)
	if (timeout < STP_MIN_ROOT_PROTECT_TIMEOUT ||
		timeout > STP_MAX_ROOT_PROTECT_TIMEOUT)
	{
		STP_LOG_ERR("input timeout %u not in range", timeout);
		return false;
	}

	stp_global.root_protect_timeout = timeout;
	return true;
}


/* FUNCTION
 *		stpmgr_set_extend_mode()
 *
 * SYNOPSIS
 *		sets the bridge to operate in extend mode (non-legacy mode). in this
 *		mode, bridge instances will operate in the 802.1d-2004 mode rather than
 *		802.1d-1998 mode.
 */
void stpmgr_set_extend_mode(bool enable)
{
	if (enable == g_stpd_extend_mode)
		return;

	g_stpd_extend_mode = enable;
}

/* FUNCTION
 *		stpmgr_port_event()
 *
 * SYNOPSIS
 *		port event handler. propagates port events across all stp and
 *		rstp_instances.
 */
void stpmgr_port_event(PORT_ID port_number, bool up)
{
	STP_INDEX index;
	STP_CLASS *stp_class;
	STP_PORT_CLASS *stp_port;
	UINT32 path_cost;
	bool (*func)(STP_INDEX, PORT_ID);

    STP_LOG_INFO("%d interface event: %s", port_number, (up ? "UP" : "DOWN"));
	// reset auto-config variables.
	if (!up)
	{
		if (!is_member(g_fastspan_mask, port_number) &&
			is_member(g_fastspan_config_mask, port_number))
		{
			stputil_update_mask(g_fastspan_mask, port_number, true);
            stpsync_update_port_fast(stp_intf_get_port_name(port_number), true);
		}
	}

	if(up)
	{
		if(STP_IS_PROTECT_DO_DISABLED(port_number))
        {
            clear_mask_bit(stp_global.protect_disabled_mask, port_number);
            stpsync_update_bpdu_guard_shutdown(stp_intf_get_port_name(port_number), false);
        }
	}

	if (g_stp_active_instances == 0)
		return;

	func = (up) ? stpmgr_add_enable_port : stpmgr_delete_enable_port;
	path_cost = stputil_get_default_path_cost(port_number, g_stpd_extend_mode);
	for (index = 0; index < g_stp_instances; index++)
	{
		stp_class = GET_STP_CLASS(index);
		if ((stp_class->state == STP_CLASS_FREE) ||
			(!is_member(stp_class->control_mask, port_number)))
		{
			continue;
		}

		// to accomodate auto-negotiated port speed, reset path-cost
		stp_port = GET_STP_PORT_CLASS(stp_class, port_number);
		if (stp_port->auto_config)
		{
			stp_port->path_cost = path_cost;
		}
		(*func) (index, port_number);
        SET_ALL_BITS(stp_port->modified_fields);
	}
}


void stpmgr_rx_stp_bpdu(uint16_t vlan_id, uint32_t port_id, char *pkt)
{
    STP_INDEX 				stp_index = STP_INDEX_INVALID;
    STP_CONFIG_BPDU			*bpdu = NULL;
    bool 				    flag = true;

    // check for stp protect configuration.
    if (stpmgr_protect_process(port_id, vlan_id))
    {
        return;
    }

    bpdu = (STP_CONFIG_BPDU *) pkt;

    // validate bpdu
    if (!stputil_validate_bpdu(bpdu))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Invalid STP BPDU received on Vlan:%d Port:%d - dropping",
                    vlan_id, port_id);
        }
        stp_global.stp_drop_count++;
        return;
    }

    // untagged ieee 802.1d bpdus processing
    if (stputil_is_port_untag(vlan_id, port_id))
    {
        vlan_id = 1;

        // 3 - if STP untagged BPDU is received and STP or RSTP is enabled on native vlan, 
        // STP BPDU gets processed by STP or RSTP respectively,else will be processed by MSTP
        if (stputil_is_protocol_enabled(L2_PVSTP) && (bpdu->protocol_version_id == STP_VERSION_ID))
        {
            flag = stputil_get_index_from_vlan(vlan_id, &stp_index);
        }
    }
    else // Tagged BPDU Processing
    {	
        if (stputil_is_protocol_enabled(L2_PVSTP))
            flag = stputil_get_index_from_vlan(vlan_id, &stp_index);
    }

    // rstp/stp processing
    if (!flag)
    {
        if (bpdu->protocol_version_id == STP_VERSION_ID) 
        {
            if (bpdu->type == TCN_BPDU_TYPE)
                stp_global.tcn_drop_count++;
            else if (bpdu->type == CONFIG_BPDU_TYPE)
                stp_global.stp_drop_count++;
        }

        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu received on vlan %u, port %d", vlan_id, port_id);

        return;
    }

    if (stp_index != STP_INDEX_INVALID)
    {
        // ieee 802.1d 9.3.4 validation of bpdus.
        if (bpdu->type != TCN_BPDU_TYPE &&
                ntohs(bpdu->message_age) >= ntohs(bpdu->max_age))
        {
            STP_LOG_INFO("Invalid BPDU (message age %u exceeds max age %u)",
                    ntohs(bpdu->message_age), ntohs(bpdu->max_age));
        }
        else
        {
            stpmgr_process_stp_bpdu(stp_index, port_id, bpdu);
        }
    }
    else
    {
        if (bpdu->protocol_version_id == STP_VERSION_ID) 
        {
            if (bpdu->type == TCN_BPDU_TYPE)
                stp_global.tcn_drop_count++;
            else if (bpdu->type == CONFIG_BPDU_TYPE)
                stp_global.stp_drop_count++;
        }

        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu - stp not configured Vlan:%d Port:%d", vlan_id, port_id);
    }

}

void stpmgr_rx_pvst_bpdu(uint16_t vlan_id, uint32_t port_id, void *pkt)
{
    STP_INDEX 				stp_index = STP_INDEX_INVALID;
    PVST_CONFIG_BPDU		*bpdu = NULL;

    // check for stp protect configuration.
    if (stpmgr_protect_process(port_id, vlan_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Dropping pvst bpdu on port:%d with stp protect enabled for Vlan:%d",
                    port_id, vlan_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    // validate pvst bpdu
    bpdu = (PVST_CONFIG_BPDU *) (((UINT8*) pkt));
    if (!stputil_validate_pvst_bpdu(bpdu))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Invalid PVST BPDU received Vlan:%d Port:%d - dropping",
                    vlan_id, port_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    // drop pvst-bpdus associated with vlan 1. wait for untagged ieee bpdu
    if((vlan_id == 1) && stputil_is_port_untag(vlan_id, port_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
        {
            STP_PKTLOG("Dropping PVST BPDU for VLAN:%d Port:%d",vlan_id, port_id);
        }
        stp_global.pvst_drop_count++;
        return;
    }

    stputil_get_index_from_vlan(vlan_id, &stp_index);

    if (stp_index != STP_INDEX_INVALID)
    {
        // ieee 802.1d 9.3.4 validation of bpdus.
        if (bpdu->type != TCN_BPDU_TYPE &&
                ntohs(bpdu->message_age) >= ntohs(bpdu->max_age))
        {
            STP_LOG_INFO("Invalid BPDU (message age %u exceeds max age %u) vlan %u port %u",
                    ntohs(bpdu->message_age), ntohs(bpdu->max_age), vlan_id, port_id);
            stp_global.pvst_drop_count++;
        }
        else
        {
            stpmgr_process_pvst_bpdu(stp_index, port_id, bpdu);
        }
    }
    else
    {
        stp_global.pvst_drop_count++;
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("dropping bpdu - stp/rstp not configured vlan %u port %u", vlan_id, port_id);
    }
}

void stpmgr_process_rx_bpdu(uint16_t vlan_id, uint32_t port_id, unsigned char *pkt)
{
    // sanity checks
    if (!IS_VALID_VLAN(vlan_id))
    {
        if (STP_DEBUG_BPDU_RX(vlan_id, port_id))
            STP_PKTLOG("Rx: INVALID VLAN-%u on Port-%u", vlan_id, port_id);
        return;
    }

    //check DA mac
    //PVST := 01 00 0c cc cc cd
    //STP  := 01 80 c2 00 00 00
    if (pkt[1] == 128) //pkt[1] == 0x80
        stpmgr_rx_stp_bpdu(vlan_id, port_id, pkt);
    else
        stpmgr_rx_pvst_bpdu(vlan_id, port_id, pkt);
}

void stpmgr_100ms_timer(evutil_socket_t fd, short what, void *arg)
{
    const char *data = (char *)arg;
    g_stpd_stats_libev_timer++;
    stptimer_tick();
}

static void stpmgr_process_bridge_config_msg(void *msg)
{
    STP_BRIDGE_CONFIG_MSG *pmsg = (STP_BRIDGE_CONFIG_MSG *)msg;
    int i;
    STP_CLASS *stp_class;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    STP_LOG_INFO("opcode : %d, stp_mode:%d, rg_timeout:%d, mac : %x%x:%x%x:%x%x", 
            pmsg->opcode, pmsg->stp_mode, pmsg->rootguard_timeout, pmsg->base_mac_addr[0], 
            pmsg->base_mac_addr[1], pmsg->base_mac_addr[2], pmsg->base_mac_addr[3], 
            pmsg->base_mac_addr[4], pmsg->base_mac_addr[5]);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stp_global.enable = true;
        stp_global.proto_mode = pmsg->stp_mode;
        
        stpmgr_config_root_protect_timeout(pmsg->rootguard_timeout);

        memcpy((char *)&g_stp_base_mac_addr._ulong, pmsg->base_mac_addr, sizeof(g_stp_base_mac_addr._ulong));
        memcpy((char *)&g_stp_base_mac_addr._ushort, 
                (char *)(pmsg->base_mac_addr + 4),
                sizeof(g_stp_base_mac_addr._ushort));
    }
    else if (pmsg->opcode == STP_DEL_COMMAND)
    {
        stp_global.enable = false;
        //Release all index

        for (i = 0; i < g_stp_instances; i++)
        {
            stp_class = GET_STP_CLASS(i);
            if (stp_class->state == STP_CLASS_FREE)
                continue;

            stpmgr_release_index(i);
        }

        clear_mask(g_stp_enable_mask);
        stp_intf_reset_port_params();
    }
}

static bool stpmgr_vlan_stp_enable(STP_VLAN_CONFIG_MSG *pmsg)
{
    PORT_ATTR *attr;
    int port_count;
    uint32_t port_id;

    STP_LOG_DEBUG("newInst:%d inst_id:%d", pmsg->newInstance, pmsg->inst_id);

    if (pmsg->newInstance)
    {
        stpdata_init_class(pmsg->inst_id, pmsg->vlan_id);

        stpsync_add_vlan_to_instance(pmsg->vlan_id, pmsg->inst_id);

        attr = pmsg->port_list;
        for (port_count = 0; port_count < pmsg->count; port_count++)
        {
            STP_LOG_INFO("Intf:%s Enab:%d Mode:%d", attr[port_count].intf_name, attr[port_count].enabled, attr[port_count].mode);
            port_id = stp_intf_get_port_id_by_name(attr[port_count].intf_name);

            if(port_id == BAD_PORT_ID)
                continue;

            if (attr[port_count].enabled)
            {
                stpmgr_add_control_port(pmsg->inst_id, port_id, attr[port_count].mode); // Sets control_mask
            }
            else
            {
                /* STP not enabled on this interface. Make it FORWARDING */
                stpsync_update_port_state(attr[port_count].intf_name, pmsg->inst_id, FORWARDING);
            }
        }
    }

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stpmgr_config_bridge_forward_delay(pmsg->inst_id, pmsg->forward_delay);
        stpmgr_config_bridge_hello_time(pmsg->inst_id, pmsg->hello_time);
        stpmgr_config_bridge_max_age(pmsg->inst_id, pmsg->max_age);
        stpmgr_config_bridge_priority(pmsg->inst_id, pmsg->priority);
    }
    return true;
}

static bool stpmgr_vlan_stp_disable(STP_VLAN_CONFIG_MSG *pmsg)
{
    stpmgr_release_index(pmsg->inst_id);
    return true;
}

static void stpmgr_process_vlan_config_msg(void *msg)
{
    STP_VLAN_CONFIG_MSG *pmsg = (STP_VLAN_CONFIG_MSG *)msg;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }
    
    if(pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, NewInst:%d, vlan:%d, Inst:%d fwd_del:%d, hello:%d, max_age:%d, pri:%d, count:%d", 
            pmsg->opcode, pmsg->newInstance, pmsg->vlan_id, pmsg->inst_id, pmsg->forward_delay, 
            pmsg->hello_time, pmsg->max_age, pmsg->priority, pmsg->count);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        stpmgr_vlan_stp_enable(pmsg);
    }
    else if (pmsg->opcode == STP_DEL_COMMAND)
    {
        stpmgr_vlan_stp_disable(pmsg);
    }
    else
        STP_LOG_ERR("invalid opcode %d", pmsg->opcode);
}

static void stpmgr_send_reply(struct sockaddr_un addr, void *msg, int len)
{
    int rc;

    STP_LOG_INFO("sending msg to %s", addr.sun_path);
    rc = sendto(g_stpd_ipc_handle, msg, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1)
        STP_LOG_ERR("reply send error %s", strerror(errno));
    else
        STP_LOG_DEBUG("reply sent");
}

static void stpmgr_process_vlan_intf_config_msg(void *msg)
{
    STP_VLAN_PORT_CONFIG_MSG *pmsg = (STP_VLAN_PORT_CONFIG_MSG *)msg;
    uint32_t port_id;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    if(pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, vlan_id:%d intf:%s, inst_id:%d, cost:%d, pri:%d", 
            pmsg->opcode, pmsg->vlan_id, pmsg->intf_name, pmsg->inst_id, pmsg->path_cost, pmsg->priority);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if(port_id == BAD_PORT_ID)
        return;

    if (pmsg->priority != -1)
        stpmgr_config_port_priority(pmsg->inst_id, port_id, pmsg->priority, false);
    if (pmsg->path_cost)
        stpmgr_config_port_path_cost(pmsg->inst_id, port_id, false, pmsg->path_cost, false);
}


static void stpmgr_process_intf_config_msg(void *msg)
{
    STP_PORT_CONFIG_MSG *pmsg = (STP_PORT_CONFIG_MSG *)msg;
    uint32_t port_id;
    int inst_count;
    VLAN_ATTR *attr;
    UINT32 path_cost;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }
    
    STP_LOG_INFO("op:%d, intf:%s, enable:%d, root_grd:%d, bpdu_grd:%d , do_dis:%d, cost:%d, pri:%d, portfast:%d, uplink_fast:%d, count:%d", 
            pmsg->opcode, pmsg->intf_name, pmsg->enabled, pmsg->root_guard, pmsg->bpdu_guard, 
            pmsg->bpdu_guard_do_disable, pmsg->path_cost, pmsg->priority, 
            pmsg->portfast, pmsg->uplink_fast, pmsg->count);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if(port_id == BAD_PORT_ID)
    {
        if(!STP_IS_PO_PORT(pmsg->intf_name))
            return;
        
        port_id = stp_intf_handle_po_preconfig(pmsg->intf_name);
        if(port_id == BAD_PORT_ID)
            return;
    }

    stputil_set_global_enable_mask(port_id, pmsg->enabled);

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        if (pmsg->priority != -1)
            stp_intf_set_port_priority(port_id, pmsg->priority);
        if (pmsg->path_cost)
            stp_intf_set_path_cost(port_id, pmsg->path_cost, false);

        attr = pmsg->vlan_list;
        for (inst_count = 0; inst_count < pmsg->count; inst_count++)
        {
            if(attr[inst_count].inst_id > g_stp_instances)
            {
                STP_LOG_ERR("invalid instance id %d", attr[inst_count].inst_id);
                continue;
            }

            STP_LOG_DEBUG("%d", attr[inst_count].inst_id);

            if (pmsg->enabled)
            {
                stpmgr_add_control_port(attr[inst_count].inst_id, port_id, attr[inst_count].mode);
            
                if (pmsg->priority != -1)
                    stpmgr_config_port_priority(attr[inst_count].inst_id, port_id, pmsg->priority, true);
                if (pmsg->path_cost)
                    stpmgr_config_port_path_cost(attr[inst_count].inst_id, port_id, false, pmsg->path_cost, true);
            }
            else
            {
                stpmgr_delete_control_port(attr[inst_count].inst_id, port_id, false);
            }
        }
            
        if (pmsg->enabled)
        {
            stpmgr_config_root_protect(port_id, pmsg->root_guard);
            stpmgr_config_protect(port_id, pmsg->bpdu_guard, pmsg->bpdu_guard_do_disable);
    
            stpmgr_config_fastspan(port_id, pmsg->portfast);
            stpmgr_config_fastuplink(port_id, pmsg->uplink_fast);
        }
    }
    else
    {
        /* Interface is deleted either due to non-L2 and PO delete */
        stp_intf_set_port_priority(port_id, STP_DFLT_PORT_PRIORITY);
        path_cost = stputil_get_default_path_cost(port_id, g_stpd_extend_mode);
        stp_intf_set_path_cost(port_id, path_cost, true);
    }

    if (pmsg->opcode == STP_DEL_COMMAND || !pmsg->enabled)
    {
        stpmgr_config_root_protect(port_id, false);
        stpmgr_config_protect(port_id, false, false);

        stpmgr_config_fastspan(port_id, true);
        stpmgr_config_fastuplink(port_id, false);

        stpsync_del_stp_port(pmsg->intf_name);
    }
}

static void stpmgr_process_vlan_mem_config_msg(void *msg)
{
    STP_VLAN_MEM_CONFIG_MSG *pmsg = (STP_VLAN_MEM_CONFIG_MSG *)msg;
    uint32_t port_id;
    STP_CLASS * stp_class;
	STP_PORT_CLASS *stp_port_class;

    if (!pmsg)
    {
        STP_LOG_ERR("rcvd NULL msg");
        return;
    }

    if(pmsg->inst_id > g_stp_instances)
    {
        STP_LOG_ERR("invalid inst_id:%d", pmsg->inst_id);
        return;
    }

    STP_LOG_INFO("op:%d, vlan:%d, inst_id:%d, intf:%s, mode:%d, cost:%d, pri:%d enabled:%d", 
            pmsg->opcode, pmsg->vlan_id, pmsg->inst_id, pmsg->intf_name, pmsg->mode, 
            pmsg->path_cost, pmsg->priority, pmsg->enabled);

    port_id = stp_intf_get_port_id_by_name(pmsg->intf_name);
    if(port_id == BAD_PORT_ID)
        return;

    if (pmsg->opcode == STP_SET_COMMAND)
    {
        if (pmsg->enabled)
        {
            stpmgr_add_control_port(pmsg->inst_id, port_id, pmsg->mode);
        }
        else
        {
            /* STP not enabled on this interface. Make it FORWARDING */
            stpsync_update_port_state(pmsg->intf_name, pmsg->inst_id, FORWARDING);
        }

        if (pmsg->priority != -1)
            stpmgr_config_port_priority(pmsg->inst_id, port_id, pmsg->priority, true);
        if (pmsg->path_cost)
            stpmgr_config_port_path_cost(pmsg->inst_id, port_id, false, pmsg->path_cost, true);
    }
    else
    {
        /* This is a case where vlan is deleted from port, so we shouldn't add vid from linux bridge port this happens 
         * as before deletion we set port to forwarding state (which adds vid to linux bridge port)
         * Setting kernel state to forward ensures we skip deleting the vid from the linux bridge port
         * */
        
        stp_class = GET_STP_CLASS(pmsg->inst_id);
        if (is_member(stp_class->control_mask, port_id))
        {
            stp_port_class = GET_STP_PORT_CLASS(stp_class, port_id);
            stp_port_class->kernel_state = STP_KERNEL_STATE_FORWARD;

            stpmgr_delete_control_port(pmsg->inst_id, port_id, true);
        }
        else
        {
        	stpsync_del_port_state(pmsg->intf_name, pmsg->inst_id);
        }
    }
}

static void stpmgr_process_ipc_msg(STP_IPC_MSG *msg, int len, struct sockaddr_un client_addr)
{
    int ret;
    STP_LOG_INFO("rcvd %s msg type", msgtype_str[msg->msg_type]);

    /* Temp code until warm boot is handled */
    if(msg->msg_type != STP_INIT_READY && msg->msg_type != STP_STPCTL_MSG)
    {
        if(g_max_stp_port == 0)
        {
            STP_LOG_ERR("max port invalid ignore msg type %s", msgtype_str[msg->msg_type]);
            return;
        }
    }

    switch(msg->msg_type)
    {
        case STP_INIT_READY:
        {
            STP_INIT_READY_MSG *pmsg = (STP_INIT_READY_MSG *)msg->data;
            /* All ports are initialized in the system. Now build IF DB in STP */
            ret = stp_intf_event_mgr_init();
            if(ret == -1)
                return;

            /* Do other protocol related inits */
            stpmgr_init(pmsg->max_stp_instances);
            if (!mstpmgr_init())
            {
                STP_LOG_ERR("MSTP global structures initialization failed\n");
                return;
            }
            break;
        }
        case STP_BRIDGE_CONFIG:
        {
            stpmgr_process_bridge_config_msg(msg->data);
            /*if(msg->proto_mode == L2_PVSTP)
            {
                stpmgr_process_bridge_config_msg(msg->data);
            }
            else if(msg->proto_mode == L2_MSTP)
            {
                mstpmgr_process_bridge_config_msg(msg->data);
            }*/
            break;
        }
        case STP_VLAN_CONFIG:
        {
            stpmgr_process_vlan_config_msg(msg->data);
            break;
        }
        case STP_VLAN_PORT_CONFIG:
        {
            stpmgr_process_vlan_intf_config_msg(msg->data);
            break;
        }
        case STP_PORT_CONFIG:
        {
            stpmgr_process_intf_config_msg(msg->data);
            /*if(msg->proto_mode == L2_PVSTP)
            {
                stpmgr_process_intf_config_msg(msg->data);
            }
            else if(msg->proto_mode == L2_MSTP)
            {
                mstpmgr_process_intf_config_msg(msg->data);
            }*/
            break;
        }
        case STP_VLAN_MEM_CONFIG:
        {
            stpmgr_process_vlan_mem_config_msg(msg->data);
            break;
        }
        case STP_STPCTL_MSG:
        {
            STP_LOG_INFO("Server received from %s", client_addr.sun_path);
            if (STP_IS_PROTOCOL_ENABLED(L2_PVSTP))
            {
                stpdbg_process_ctl_msg(msg->data);
            }
            else if (STP_IS_PROTOCOL_ENABLED(L2_MSTP))
            {
                mstpdbg_process_ctl_msg(msg->data);
            }
            stpmgr_send_reply(client_addr, (void *)msg, len);
            break;
        }
        case STP_MST_GLOBAL_CONFIG:
        {
            mstpmgr_process_mstp_global_config_msg(msg->data);
            break;
        }
        case STP_MST_INST_CONFIG:
        {
            mstpmgr_process_inst_vlan_config_msg(msg->data);
            break;
        }
        case STP_MST_VLAN_PORT_LIST_CONFIG:
        {
            mstpmgr_process_vlan_mem_config_msg(msg->data);
            break;
        }
        case STP_MST_INST_PORT_CONFIG:
        {
            mstpmgr_process_inst_port_config_msg(msg->data);
            break;
        }

        default:
            break;
    }
}

/* Process all messages from clients (STPMGRd) */
void stpmgr_recv_client_msg(evutil_socket_t fd, short what, void *arg)
{
    char buffer[4096];
    int len;
    struct sockaddr_un client_sock;

    g_stpd_stats_libev_ipc++;

    len = sizeof(struct sockaddr_un);
    len = recvfrom(fd, buffer, 4096, 0, (struct sockaddr *) &client_sock, &len);
    if (len == -1)
    {
        STP_LOG_ERR("recv  message error %s", strerror(errno));
    }
    else
    {
        stpmgr_process_ipc_msg((STP_IPC_MSG *)buffer, len, client_sock);
    }
}
