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

#ifndef __STP_DBSYNC__
#define __STP_DBSYNC__

#ifdef __cplusplus
extern "C" {
#endif

#define IS_BIT_SET(field, bit)   ((field) & (1 << (bit)))
#define SET_BIT(field, bit)      ((field) |= (1 << (bit)))
#define SET_ALL_BITS(field)      ((field) = 0xFFFFFFFF)
#define RESET_BIT(field, bit)    ((field) &= ((unsigned int) ~(1 << (bit))))

#define STP_SYNC_BRIDGE_ID_LEN          (20)
#define STP_SYNC_PORT_IDENTIFIER_LEN    (16)
#define STP_SYNC_PORT_STATE_LEN         (15)

typedef struct
{
	uint16_t			vlan_id; 
	char				bridge_id[STP_SYNC_BRIDGE_ID_LEN];
	uint8_t				max_age;
	uint8_t				hello_time;
	uint8_t				forward_delay;
	uint8_t			    hold_time;
	
	uint32_t			topology_change_time;	// time of last tc event
	uint32_t			topology_change_count;

	char				root_bridge_id[STP_SYNC_BRIDGE_ID_LEN];
	uint32_t			root_path_cost;
	char				desig_bridge_id[STP_SYNC_BRIDGE_ID_LEN];
	char                root_port[IFNAMSIZ];
	uint8_t				root_max_age;
	uint8_t				root_hello_time;
	uint8_t				root_forward_delay;
	uint16_t			stp_instance; 
    uint32_t            modified_fields;
} STP_VLAN_TABLE;

typedef struct
{
	char 			if_name[IFNAMSIZ];
	uint16_t        port_id;
	uint8_t         port_priority;
	uint16_t		vlan_id; 
	uint32_t	    path_cost;
    char            port_state[STP_SYNC_PORT_STATE_LEN];
	uint32_t		designated_cost;
	char      		designated_root[STP_SYNC_BRIDGE_ID_LEN];
    char     		designated_bridge[STP_SYNC_BRIDGE_ID_LEN];
    char			designated_port;

	uint32_t		forward_transitions;
	uint32_t		tx_config_bpdu;
	uint32_t		rx_config_bpdu;
	uint32_t		tx_tcn_bpdu;
	uint32_t		rx_tcn_bpdu;
	uint32_t        root_protect_timer;

    uint8_t         clear_stats;
    uint32_t        modified_fields;
} STP_VLAN_PORT_TABLE;

extern void stpsync_add_vlan_to_instance(uint16_t vlan_id, uint16_t instance);
extern void stpsync_del_vlan_from_instance(uint16_t vlan_id, uint16_t instance);
extern void stpsync_update_stp_class(STP_VLAN_TABLE * stp_vlan);
extern void stpsync_del_stp_class(uint16_t vlan_id);
extern void stpsync_update_port_class(STP_VLAN_PORT_TABLE * stp_vlan_intf);
extern void stpsync_del_port_class(char * if_name, uint16_t vlan_id);
extern void stpsync_update_port_state(char * ifName, uint16_t instance, uint8_t state);
extern void stpsync_del_port_state(char * ifName, uint16_t instance);
extern void stpsync_update_vlan_port_state(char * ifName, uint16_t vlan_id, uint8_t state);
extern void stpsync_del_vlan_port_state(char * ifName, uint16_t vlan_id);
extern void stpsync_update_fastage_state(uint16_t vlan_id, bool add);
extern uint32_t stpsync_get_port_speed(char * ifName);
extern void stpsync_update_port_admin_state(char * ifName, bool up, bool physical);
extern void stpsync_update_bpdu_guard_shutdown(char * ifName, bool enabled);
extern void stpsync_del_stp_port(char * ifName);
extern void stpsync_update_port_fast(char * ifName, bool enabled);
extern void stpsync_clear_appdb_stp_tables(void);
#ifdef __cplusplus
}/* extern "C" */
#endif

#endif
