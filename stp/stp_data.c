/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/* global variables --------------------------------------------------------- */

uint8_t g_dbg_lvl;
STP_GLOBAL			stp_global;
uint32_t g_max_stp_port;
uint16_t g_stp_bmp_po_offset;

int8_t stpdata_init_global_port_mask()
{
    int8_t ret = 0;
    ret |= bmp_alloc(&g_stp_enable_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_stp_enable_config_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_fastspan_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_fastspan_config_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_fastuplink_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_stp_protect_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_stp_protect_do_disable_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_stp_root_protect_mask, g_max_stp_port);
    ret |= bmp_alloc(&g_stp_protect_disabled_mask, g_max_stp_port);

    /* By default fast span is enabled on all ports */
    PORT_MASK_SET_ALL(g_fastspan_mask);
    PORT_MASK_SET_ALL(g_fastspan_config_mask);

    return ret;
}

int8_t stpdata_init_stp_class_port_mask(STP_INDEX stp_index)
{
    int8_t ret = 0;
    STP_CLASS *stp_class;

    stp_class = GET_STP_CLASS(stp_index);
    ret |= bmp_alloc(&stp_class->enable_mask, g_max_stp_port);
    ret |= bmp_alloc(&stp_class->control_mask, g_max_stp_port);
    ret |= bmp_alloc(&stp_class->untag_mask, g_max_stp_port);

    return ret;
}

/* FUNCTION
 *		stpdata_init_global_structures()
 *
 * SYNOPSIS
 *		initializes the stp_global data structure. Called at system startup.
 */
bool stpdata_init_global_structures(UINT16 max_instances)
{
	UINT32 mem_size;
    int i;

	memset((UINT8 *) &stp_global, 0, sizeof(STP_GLOBAL));

    if (stpdata_init_global_port_mask() == -1)
    {
        STP_LOG_ERR("stpdata_init_global_port_mask Failed");
        return false;
    }

	g_stp_instances = max_instances;

	mem_size = g_stp_instances * sizeof(STP_CLASS);
	g_stp_class_array = (STP_CLASS *) calloc(1, mem_size);
	if (g_stp_class_array == NULL)
	{
		STP_LOG_CRITICAL("Memory allocation %d bytes failed", mem_size);
        return false;
	}

	memset(g_stp_class_array, 0, mem_size);

    if (stpdata_malloc_port_structures() == false)
    {
        free(g_stp_class_array);
        return false;
    }

    for (i = 0; i < g_stp_instances; i++)
    {
        if(stpdata_init_stp_class_port_mask(i) == -1)
        {
            STP_LOG_ERR("stpdata_init_stp_class_port_mask Failed");
            free(g_stp_class_array);
            free(g_stp_port_array);
            g_stp_instances = 0;
            g_stp_class_array = 0;
            return false;
        }
    }

	stpdata_init_bpdu_structures();

	/* set debug structures to default values */
	if (-1 == stpdata_init_debug_structures())
    {
        STP_LOG_ERR("stpdata_init_debug_structures Failed");
        return false;
    }

	/* set root-protect timeout to its default value */
	stp_global.root_protect_timeout = STP_DFLT_ROOT_PROTECT_TIMEOUT;

	/*
	 * fast span is enabled by default
	 */
	stp_global.fast_span = true;

	return true;
}

/* FUNCTION
 *		stpdata_malloc_port_structures()
 *
 * SYNOPSIS
 *		allocates the port data structures. called at init
 */
bool stpdata_malloc_port_structures()
{
	UINT32 num_ports, mem_size;

	if (g_stp_port_array == NULL)
	{
		mem_size = sizeof(STP_PORT_CLASS) * g_max_stp_port * g_stp_instances;
		g_stp_port_array = (STP_PORT_CLASS *) calloc(1, mem_size);
		if (g_stp_port_array == NULL)
		{
			STP_LOG_CRITICAL("Memory allocation %d bytes failed for port_class", mem_size);
			return false;
		}
		memset(g_stp_port_array, 0, mem_size);
		return true;
	}

	return false;
}

/* FUNCTION
 *		stpdata_free_port_structures()
 *
 * SYNOPSIS
 *		de-allocates the port data structures.
 */
void stpdata_free_port_structures()
{
	if (g_stp_port_array != NULL)
	{
		free((void *) g_stp_port_array);
		g_stp_port_array = NULL;
	}
}

/* FUNCTION
 *		stpdata_init_class()
 *
 * SYNOPSIS
 *		initializes STP_CLASS 
 */
int stpdata_init_class(STP_INDEX stp_index, VLAN_ID vlan_id)
{
	STP_CLASS *stp_class;

    if (stp_index != STP_INDEX_INVALID)
    {
        stp_class = GET_STP_CLASS(stp_index);
        
        if (stp_class->state != STP_CLASS_FREE)
        {
            STP_LOG_ERR("stpclass not free inst %d vlan %d", stp_index, vlan_id);
            return -1;
        }
    
        stp_class->state = STP_CLASS_CONFIG;
        g_stp_active_instances++;
        stpmgr_initialize_stp_class(stp_class, vlan_id);
    }

    return 0;
}

/* FUNCTION
 *		stpdata_class_free()
 *
 * SYNOPSIS
 *		de-allocates an STP instance
 */
void stpdata_class_free(STP_INDEX stp_index)
{
	STP_CLASS *stp_class;

	stp_class = GET_STP_CLASS(stp_index);
    stp_class->vlan_id = 0;
    stp_class->fast_aging = 0;
	stp_class->state = STP_CLASS_FREE;
    memset(&stp_class->bridge_info, 0, sizeof(BRIDGE_DATA));
    stop_timer(&stp_class->hello_timer);
    stop_timer(&stp_class->tcn_timer);
    stop_timer(&stp_class->topology_change_timer);
    stp_class->last_expiry_time = 0;
    stp_class->last_bpdu_rx_time = 0;
    stp_class->modified_fields = 0;

	g_stp_active_instances--;
}

/* FUNCTION
 *		stpdata_init_bpdu_structures()
 *
 * SYNOPSIS
 *		initialize the configuration bpdu, tcn bpdu for IEEE 802.1D
 *		also initialize the pvst bpdus
 */
void stpdata_init_bpdu_structures()
{
	/* configuration bpdu */
    HOST_TO_NET_MAC(&g_stp_config_bpdu.mac_header.destination_address,&bridge_group_address);
	g_stp_config_bpdu.mac_header.length = htons(STP_SIZEOF_CONFIG_BPDU + sizeof(LLC_HEADER));
	g_stp_config_bpdu.llc_header.destination_address_DSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
	g_stp_config_bpdu.llc_header.source_address_SSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
	g_stp_config_bpdu.llc_header.llc_frame_type = (enum LLC_FRAME_TYPE) UNNUMBERED_INFORMATION;
	g_stp_config_bpdu.type = (enum STP_BPDU_TYPE) CONFIG_BPDU_TYPE;
	g_stp_config_bpdu.protocol_version_id = STP_VERSION_ID;

	/* tcn bpdu */
	HOST_TO_NET_MAC(&g_stp_tcn_bpdu.mac_header.destination_address, &bridge_group_address);
	g_stp_tcn_bpdu.mac_header.length = htons(STP_SIZEOF_TCN_BPDU + sizeof(LLC_HEADER));
	g_stp_tcn_bpdu.llc_header.destination_address_DSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
	g_stp_tcn_bpdu.llc_header.source_address_SSAP = LSAP_BRIDGE_SPANNING_TREE_PROTOCOL;
	g_stp_tcn_bpdu.llc_header.llc_frame_type = UNNUMBERED_INFORMATION;
	g_stp_tcn_bpdu.type = (enum STP_BPDU_TYPE) TCN_BPDU_TYPE;
	g_stp_tcn_bpdu.protocol_version_id = STP_VERSION_ID;

	/* pvst configuration bpdu */
	HOST_TO_NET_MAC(&g_stp_pvst_config_bpdu.mac_header.destination_address, &pvst_bridge_group_address);
	g_stp_pvst_config_bpdu.mac_header.length = htons(50);
	g_stp_pvst_config_bpdu.snap_header.destination_address_DSAP = LSAP_SNAP_LLC;
	g_stp_pvst_config_bpdu.snap_header.source_address_SSAP = LSAP_SNAP_LLC;
	g_stp_pvst_config_bpdu.snap_header.llc_frame_type = UNNUMBERED_INFORMATION;
	g_stp_pvst_config_bpdu.snap_header.protocol_id_filler[0] = 0x00;
	g_stp_pvst_config_bpdu.snap_header.protocol_id_filler[1] = 0x00;
	g_stp_pvst_config_bpdu.snap_header.protocol_id_filler[2] = 0x0c;
	g_stp_pvst_config_bpdu.snap_header.protocol_id = htons(SNAP_CISCO_PVST_ID);
	g_stp_pvst_config_bpdu.tag_length = htons(2); // pvst+ specific field
	g_stp_pvst_config_bpdu.type = (enum STP_BPDU_TYPE) CONFIG_BPDU_TYPE;
	g_stp_pvst_config_bpdu.protocol_version_id = STP_VERSION_ID;

	/* pvst tcn bpdu */
	HOST_TO_NET_MAC(&g_stp_pvst_tcn_bpdu.mac_header.destination_address, &pvst_bridge_group_address);
	g_stp_pvst_tcn_bpdu.mac_header.length = htons(50);
	g_stp_pvst_tcn_bpdu.snap_header.destination_address_DSAP = LSAP_SNAP_LLC;
	g_stp_pvst_tcn_bpdu.snap_header.source_address_SSAP = LSAP_SNAP_LLC;
	g_stp_pvst_tcn_bpdu.snap_header.llc_frame_type = UNNUMBERED_INFORMATION;
	g_stp_pvst_tcn_bpdu.snap_header.protocol_id_filler[0] = 0x00;
	g_stp_pvst_tcn_bpdu.snap_header.protocol_id_filler[1] = 0x00;
	g_stp_pvst_tcn_bpdu.snap_header.protocol_id_filler[2] = 0x0c;
	g_stp_pvst_tcn_bpdu.snap_header.protocol_id = htons(SNAP_CISCO_PVST_ID);
	g_stp_pvst_tcn_bpdu.type = (enum STP_BPDU_TYPE) TCN_BPDU_TYPE;
	g_stp_pvst_tcn_bpdu.protocol_version_id = STP_VERSION_ID;
}

/* FUNCTION
 *		stpdata_init_debug_structures()
 *
 * SYNOPSIS
 *		resets the debug structure contents for STP and RSTP.
 */
int stpdata_init_debug_structures(void)
{
    int ret = 0;
    memset((UINT8 *) &(debugGlobal.stp), 0, sizeof(DEBUG_STP));

    debugGlobal.stp.event = 0;
    debugGlobal.stp.bpdu_rx = 0;
    debugGlobal.stp.bpdu_tx = 0;
    debugGlobal.stp.all_ports = 1;
    debugGlobal.stp.all_vlans = 1;


    ret |= bmp_alloc(&debugGlobal.stp.vlan_mask, MAX_VLAN_ID);
    ret |= bmp_alloc(&debugGlobal.stp.port_mask, g_max_stp_port);
    if (ret != 0)
    {
        STP_LOG_ERR("bmp_alloc Failed");
        return -1;
    }
    return 0;
}

/* FUNCTION
 *		stpdata_get_port_class()
 *
 * SYNOPSIS
 *		get the port class structure associated with stp_class and port_number
 */
STP_PORT_CLASS* stpdata_get_port_class(STP_CLASS *stp_class, PORT_ID port_number)
{
	UINT16 stp_index;

	stp_index = GET_STP_INDEX(stp_class);

	if (g_stp_port_array == NULL)
	{
		STP_LOG_ERR("error - port array null inst:%d port:%d", stp_index, port_number);
		return NULL;
	}

	return &g_stp_port_array[(stp_index + (port_number * g_stp_instances))];
}
