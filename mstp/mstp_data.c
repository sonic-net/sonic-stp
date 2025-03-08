/*
 * Copyright 2024 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

/*****************************************************************************/
/* mstp global information                                                   */
/*****************************************************************************/

MSTP_GLOBAL         g_mstp_global;
MSTP_VLAN_PORT_DB   g_mstp_vlan_port_db[MAX_VLAN_ID];
MSTP_PORT_VLAN_DB  **g_mstp_port_vlan_db;
 
/*****************************************************************************/
/* mstpdata_get_port: returns the data structure associated with the input   */
/* port number                                                               */
/*****************************************************************************/
MSTP_PORT * mstpdata_get_port(PORT_ID port_number)
{
	if (port_number < g_max_stp_port)
	{
		return g_mstp_global.port_arr[port_number];
	}
	return NULL;
}

/*****************************************************************************/
/* mstpdata_get_bpdu: returns the global bpdu structure                      */
/*****************************************************************************/
MSTP_BPDU *mstpdata_get_bpdu()
{
	return &(g_mstp_global.bpdu);
}

/*****************************************************************************/
/* mstpdata_get_bridge: returns the global bridge data structure             */
/*****************************************************************************/
MSTP_BRIDGE *mstpdata_get_bridge()
{
	return (g_mstp_global.bridge);
}

/*****************************************************************************/
/* mstpdata_alloc_msti_bridge: allocates msti bridge data structure          */
/* associated with input mstp index                                          */
/*****************************************************************************/
static MSTP_MSTI_BRIDGE * mstpdata_alloc_msti_bridge(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE *mstp_bridge;
	MSTP_MSTI_BRIDGE *msti_bridge;

	mstp_bridge = mstpdata_get_bridge();

	if (mstp_bridge == NULL)
	{
		STP_LOG_ERR("mstp_bridge is null");
		return NULL;
	}

	if (!MSTP_IS_VALID_MSTI_INDEX(mstp_index))
	{
		STP_LOG_ERR("[MST Index %d] mstp_index invalid", mstp_index);
		return NULL;
	}

	msti_bridge = mstp_bridge->msti[mstp_index];
	if (msti_bridge == NULL)
	{
        STP_LOG_DEBUG("[MST Index %d] Memory allocated",mstp_index);	
		msti_bridge = (MSTP_MSTI_BRIDGE *) calloc(1,sizeof(MSTP_MSTI_BRIDGE));
		mstp_bridge->msti[mstp_index] = msti_bridge;
	}
	else
	{
        STP_LOG_ERR("[MST Index %d] Failed to allocate memory for mstp index", mstp_index);	
	}

	return msti_bridge;
}

/*****************************************************************************/
/* mstpdata_free_msti_bridge: free msti bridge data structure associated     */
/* with input mstp index                                                     */
/*****************************************************************************/
static void mstpdata_free_msti_bridge(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE *mstp_bridge;
	MSTP_MSTI_BRIDGE *msti_bridge;

	mstp_bridge = mstpdata_get_bridge();
	if (mstp_bridge == NULL)
	{
		STP_LOG_ERR("mstp_bridge is null");
		return;
	}

	if (!MSTP_IS_VALID_MSTI_INDEX(mstp_index))
	{
        STP_LOG_ERR("[MST Index %d] mstp_index invalid", mstp_index);
		return;
	}

	msti_bridge = mstp_bridge->msti[mstp_index];
	if (msti_bridge == NULL)
	{
        STP_LOG_DEBUG("[MST Index %d] Cannot deallocate, msti_bridge not present", mstp_index);	
		return;
	}

    STP_LOG_DEBUG("[MST Index %d] Deallocated msti_bridge", mstp_index);	

	free(msti_bridge);
	mstp_bridge->msti[mstp_index] = NULL;
}

/*****************************************************************************/
/* mstpdata_alloc_msti_port: allocates msti port data structure associated   */
/* with input mstp index and port number                                     */
/*****************************************************************************/
MSTP_MSTI_PORT * mstpdata_alloc_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;

	mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("[MST Index %d] Port %d mstp_port is null", mstp_index, port_number);
        return NULL;
    }

    if (!MSTP_IS_VALID_MSTI_INDEX(mstp_index))
    {
        STP_LOG_ERR("[MST Index %d] Port %d mstp_index invalid", mstp_index, port_number);
        return NULL;
    }

    msti_port = mstp_port->msti[mstp_index];
    if (msti_port == NULL)
    {
        msti_port = (MSTP_MSTI_PORT *) calloc(1,sizeof(MSTP_MSTI_PORT));
        if (msti_port != NULL)
        {
            mstp_port->msti[mstp_index] = msti_port;
            L2_PROTO_INSTANCE_MASK_SET(&(mstp_port->instance_mask), mstp_index);
            STP_LOG_DEBUG("[MST Index %d] Port %d msti_port allocated", mstp_index, port_number);
        }
        else
        {
            STP_LOG_ERR("[MST Index %d] Port %d Failed to allocate memory", mstp_index, port_number);
        }
    }
    else
    {
        STP_LOG_DEBUG("[MST Index %d] Port %d msti_port exists", mstp_index, port_number);
    }

	return msti_port;
}

/*****************************************************************************/
/* mstpdata_free_msti_port: frees the msti port structure associated with    */
/* the input port and mstp_index                                             */
/*****************************************************************************/
void mstpdata_free_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;

	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
	{
        STP_LOG_ERR("[MST Index %d] Port %d mstp_port is null", mstp_index, port_number);
		return;
	}

	if (!MSTP_IS_VALID_MSTI_INDEX(mstp_index))
	{
        STP_LOG_ERR("[MST Index %d] Port %d mstp_index invalid", mstp_index, port_number);
		return;
	}

	msti_port = mstp_port->msti[mstp_index];
	if (msti_port == NULL)
	{

		STP_LOG_ERR("[MST Index %d] Port %d msti_port null", mstp_index, port_number);
		return;
	}

    L2_PROTO_INSTANCE_MASK_CLR(&(mstp_port->instance_mask), mstp_index);

    STP_LOG_DEBUG("[MST Index %d] Port %d msti_port deallocated", mstp_index, port_number);

    free(msti_port);
    mstp_port->msti[mstp_index] = NULL;
}

/*****************************************************************************/
/* mstpdata_alloc_mstp_bridge: allocates the mstp bridge structure           */
/*****************************************************************************/
MSTP_BRIDGE * mstpdata_alloc_mstp_bridge()
{
    int i = 0;
    int ret = 0;
    MSTP_GLOBAL *mstp_global = &g_mstp_global;

	if (mstp_global->bridge == NULL)
    {
        mstp_global->bridge = (MSTP_BRIDGE *) calloc(1,sizeof(MSTP_BRIDGE));
        if(!mstp_global->bridge)
        {
            STP_LOG_CRITICAL("Calloc failed");
            return NULL;
        }
    }

    ret = bmp_alloc(&mstp_global->bridge->control_mask, g_max_stp_port);
    ret |= bmp_alloc(&mstp_global->bridge->enable_mask, g_max_stp_port);
    ret |= bmp_alloc(&mstp_global->bridge->admin_disable_mask, g_max_stp_port);
    ret |= bmp_alloc(&mstp_global->bridge->admin_pt2pt_mask, g_max_stp_port);
    ret |= bmp_alloc(&mstp_global->bridge->admin_edge_mask, g_max_stp_port);

    for (i = 0; i<MSTP_MAX_INSTANCES_PER_REGION+1; i++)
    {
        ret |= bmp_alloc(&mstp_global->bridge->config_mask[i], g_max_stp_port);
    }

    if (ret != 0)
    {
        if(mstp_global->bridge->control_mask)
        {
            bmp_free(mstp_global->bridge->control_mask);
        }
        if(mstp_global->bridge->enable_mask)
        {
            bmp_free(mstp_global->bridge->enable_mask);
        }
        if(mstp_global->bridge->admin_disable_mask)
        {
            bmp_free(mstp_global->bridge->admin_disable_mask);
        }
        if(mstp_global->bridge->admin_pt2pt_mask)
        {
            bmp_free(mstp_global->bridge->admin_pt2pt_mask);
        }
         if(mstp_global->bridge->admin_edge_mask)
        {
            bmp_free(mstp_global->bridge->admin_edge_mask);
        }
        for (i = 0; i<MSTP_MAX_INSTANCES_PER_REGION+1; i++)
        {
            if(mstp_global->bridge->config_mask[i])
            {
                bmp_free(mstp_global->bridge->config_mask[i]);
            }
        }

        STP_LOG_CRITICAL("BMP alloc failed");
        if(mstp_global->bridge)
        {
            free(mstp_global->bridge);
        }
        return NULL;
    }

	return mstp_global->bridge;
}

/*****************************************************************************/
/* mstpdata_free_mstp_bridge: frees the mstp bridge structure                */
/*****************************************************************************/
void mstpdata_free_mstp_bridge()
{
    MSTP_INDEX mstp_index;
    MSTP_GLOBAL *mstp_global = &g_mstp_global;
    int i = 0;

    if (mstp_global->bridge == NULL)
    {
        return;
    }

    for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
    {
        if (mstp_global->bridge->msti[mstp_index] != NULL)
        {
            STP_LOG_DEBUG("[MST index %u] structure was not freed",  mstp_index);
            mstpdata_free_index(mstp_index);
        }
    }

    if(mstp_global->bridge->control_mask)
    {
        if(mstp_global->bridge->control_mask)
        {
            bmp_free(mstp_global->bridge->control_mask);
        }
        if(mstp_global->bridge->enable_mask)
        {
            bmp_free(mstp_global->bridge->enable_mask);
        }
        if(mstp_global->bridge->admin_disable_mask)
        {
            bmp_free(mstp_global->bridge->admin_disable_mask);
        }
        if(mstp_global->bridge->admin_pt2pt_mask)
        {
            bmp_free(mstp_global->bridge->admin_pt2pt_mask);
        }
         if(mstp_global->bridge->admin_edge_mask)
        {
            bmp_free(mstp_global->bridge->admin_edge_mask);
        }
        for (i = 0; i<MSTP_MAX_INSTANCES_PER_REGION+1; i++)
        {
            if(mstp_global->bridge->config_mask[i])
            {
                bmp_free(mstp_global->bridge->config_mask[i]);
            }
        }
    }
    STP_LOG_DEBUG("mstp_bridge deallocated");

    free(mstp_global->bridge);
    mstp_global->bridge = NULL;
}

/*****************************************************************************/
/* mstpdata_alloc_mstp_port: allocates the mstp port structure for the input */
/* port number                                                               */
/*****************************************************************************/
MSTP_PORT *mstpdata_alloc_mstp_port(PORT_ID port_number)
{
    MSTP_GLOBAL *mstp_global = &g_mstp_global;

	if (port_number > g_max_stp_port)
	{
		STP_LOG_ERR("Port %d invalid", port_number);
		return NULL;
	}

	if (mstp_global->port_arr[port_number] == NULL)
	{
        mstp_global->port_arr[port_number] = (MSTP_PORT *) calloc(1,sizeof(MSTP_PORT));
        STP_LOG_DEBUG("Port %d mstp_port allocated", port_number);
    }
    return mstp_global->port_arr[port_number];
}

/*****************************************************************************/
/* mstpdata_free_port: frees the mstp port structure for the input port      */
/*****************************************************************************/
void mstpdata_free_port(PORT_ID port_number)
{
    MSTP_INDEX mstp_index;
    MSTP_PORT *mstp_port;

    mstp_port = mstpdata_get_port(port_number);
    if (mstp_port == NULL)
    {
        STP_LOG_ERR("%d mstp_port is null", port_number);
        return;
    }

    for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
    {
        if (mstp_port->msti[mstp_index] != NULL)
        {
            STP_LOG_DEBUG("[MST Index %d] %d Structure was not freed",
                    mstp_index, port_number);
            mstpdata_free_msti_port(mstp_index, port_number);
        }
    }

    STP_LOG_DEBUG("[MST] %d mstp_port deallocated", port_number);

    free(mstp_port);
    g_mstp_global.port_arr[port_number] = NULL;
}

/*****************************************************************************/
/* mstpdata_get_new_index: returns an unused mstp index                      */
/*****************************************************************************/
MSTP_INDEX mstpdata_get_new_index()
{
	MSTP_INDEX mstp_index;
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
    MSTP_GLOBAL *mstp_global = &g_mstp_global;

    STP_LOG_DEBUG("Remaining instances %d", mstp_global->free_instances);	

	if (mstp_global->free_instances > 0)
	{
		for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
		{
			if (mstp_bridge->msti[mstp_index] != NULL)
 				continue;

			if (mstpdata_alloc_msti_bridge(mstp_index) == NULL)
				return MSTP_INDEX_INVALID; // unable to allocate memory

			mstp_global->free_instances--;
			
            STP_LOG_DEBUG("[MST Index %d] New mstp index allocated, remaining %d", 
                    mstp_index, mstp_global->free_instances);	

			return mstp_index;
		}
	}

	return MSTP_INDEX_INVALID;
}

/*****************************************************************************/
/* mstpdata_free_index: frees an instance index                              */
/*****************************************************************************/
bool mstpdata_free_index(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_MSTI_BRIDGE *msti_bridge;

	msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
	if (msti_bridge)
	{
		mstpdata_free_msti_bridge(mstp_index);
		g_mstp_global.free_instances++;
        STP_LOG_DEBUG("[MST Index %d] Deallocated index", mstp_index);
	}

	return true;
}

/*****************************************************************************/
/* mstpdata_reset: clears the mstp global data structure                     */
/*****************************************************************************/
void mstpdata_reset()
{
	memset(&g_mstp_global, 0, sizeof(g_mstp_global));
}

/*****************************************************************************/
/* mstpdata_port_vlan_db_init: Init port to vlan mapping DB                  */
/*****************************************************************************/
bool mstpdata_port_vlan_db_init()
{
    PORT_ID port_number;
    int ret = 0;

    g_mstp_port_vlan_db = calloc(1, (sizeof(MSTP_PORT_VLAN_DB *) * g_max_stp_port));
    if (!g_mstp_port_vlan_db)
    {
        STP_LOG_CRITICAL("BMP alloc failed");
        return false;
    }

    for(port_number = 0; port_number  < g_max_stp_port ; port_number++)
    {
        g_mstp_port_vlan_db[port_number] = (MSTP_PORT_VLAN_DB *) calloc(1,sizeof(MSTP_PORT_VLAN_DB));
        if(!g_mstp_port_vlan_db[port_number])
        {
            STP_LOG_CRITICAL("Calloc failed");
            return false;
        }

        ret = bmp_alloc(&g_mstp_port_vlan_db[port_number]->vlan_mask, MAX_VLAN_ID);
        if (ret != 0)
        {
            STP_LOG_CRITICAL("BMP alloc failed");
            return false;
        }
        g_mstp_port_vlan_db[port_number]->untag_vlan = VLAN_ID_INVALID;
    }
    return true;
}
/*****************************************************************************/
/* mstpdata_vlan_port_db_init: Init vlan to port mapping DB                  */
/*****************************************************************************/
void mstpdata_vlan_port_db_init()
{
    UINT16 vlan_id;
    int ret = 0;

    for(vlan_id = 1; vlan_id < MAX_VLAN_ID; vlan_id++)
    {
        ret = bmp_alloc(&g_mstp_vlan_port_db[vlan_id].port_mask, g_max_stp_port);
        if (ret != 0)
        {
            STP_LOG_CRITICAL("BMP alloc failed");
            return;
        }
    }
    return;
}
/*****************************************************************************/
/* mstpdata_update_port_mask_for_vlan: Update ports to the VLAN DB           */
/*****************************************************************************/
void mstpdata_update_port_mask_for_vlan(VLAN_ID vlan_id, PORT_MASK *port_mask, UINT8 add)
{
    int ret = 0;

    if(!g_mstp_vlan_port_db[vlan_id].port_mask)
    {
        STP_LOG_ERR("[Vlan %d] port mask is null", vlan_id);
        return;
    }

    if(add)
        or_masks(g_mstp_vlan_port_db[vlan_id].port_mask, g_mstp_vlan_port_db[vlan_id].port_mask, port_mask);
    else
        and_not_masks(g_mstp_vlan_port_db[vlan_id].port_mask, g_mstp_vlan_port_db[vlan_id].port_mask, port_mask);
}

/*****************************************************************************/
/* mstpdata_update_port_vlan: Update vlan to the PORT DB                     */
/*****************************************************************************/
void mstpdata_update_port_vlan(PORT_ID port_id, VLAN_ID vlan_id, UINT8 add)
{
    int ret = 0;

    if(!g_mstp_port_vlan_db[port_id]->vlan_mask)
    {
        STP_LOG_ERR("[Port %d] vlan mask null", port_id);
        return;
    }

    if(add)
    {
        set_mask_bit(g_mstp_port_vlan_db[port_id]->vlan_mask, vlan_id);
    }
    else
    {
        clear_mask_bit(g_mstp_port_vlan_db[port_id]->vlan_mask, vlan_id);

        if(g_mstp_port_vlan_db[port_id]->untag_vlan == vlan_id)
            g_mstp_port_vlan_db[port_id]->untag_vlan = VLAN_ID_INVALID;

    }
}

/*****************************************************************************/
/* mstpdata_get_port_mask_for_vlan: Get portmask from vlan                   */
/*****************************************************************************/
PORT_MASK* mstpdata_get_port_mask_for_vlan(VLAN_ID vlan_id)
{
    if(vlan_id >= MAX_VLAN_ID)
         return NULL;

    if(!g_mstp_vlan_port_db[vlan_id].port_mask)
        return NULL;

    if(is_mask_clear(g_mstp_vlan_port_db[vlan_id].port_mask))
         return NULL;

    return g_mstp_vlan_port_db[vlan_id].port_mask;
}
/*****************************************************************************/
/* mstpdata_get_vlan_mask_for_port: Get vlanmask from port                   */
/*****************************************************************************/
BITMAP_T* mstpdata_get_vlan_mask_for_port(PORT_ID port_id)
{
    if(!g_mstp_port_vlan_db[port_id])
        return NULL;

    if(!g_mstp_port_vlan_db[port_id]->vlan_mask)
        return NULL;

    if(is_mask_clear(g_mstp_port_vlan_db[port_id]->vlan_mask))
        return NULL;

    return (g_mstp_port_vlan_db[port_id]->vlan_mask);
}
/*****************************************************************************/
/* mstpdata_update_untag_vlan_for_port: Update untag vlan for the port       */
/*****************************************************************************/
void mstpdata_update_untag_vlan_for_port(PORT_ID port_id, VLAN_ID vlan_id)
{
    if(!g_mstp_port_vlan_db[port_id])
    {
        STP_LOG_ERR("[%d] Port DB doesnt exist", port_id);
        return;
    }
    g_mstp_port_vlan_db[port_id]->untag_vlan = vlan_id;
}

/*****************************************************************************/
/* mstpdata_get_untag_vlan_for_port: Get untag vlan for the port             */
/*****************************************************************************/
VLAN_ID mstpdata_get_untag_vlan_for_port(PORT_ID port_id)
{
    if(!g_mstp_port_vlan_db[port_id])
        return VLAN_ID_INVALID;

    return(g_mstp_port_vlan_db[port_id]->untag_vlan);
}
/*****************************************************************************/
/* mstpdata_is_vlan_present: Check vlan is configured in the system          */
/*****************************************************************************/
bool mstpdata_is_vlan_present(VLAN_ID vlan_id)
{
    if(vlan_id >= MAX_VLAN_ID)
        return false;
   
    if(!g_mstp_vlan_port_db[vlan_id].port_mask)
         return false;

    if(!is_mask_clear(g_mstp_vlan_port_db[vlan_id].port_mask))
        return true;

    return false;
}
/*****************************************************************************/
/* mstpdata_reset_vlan_port_db: Reset vlan-port DB                           */
/*****************************************************************************/
void mstpdata_reset_vlan_port_db()
{
    VLAN_ID vlan_id;

    for(vlan_id = 1; vlan_id < MAX_VLAN_ID; vlan_id++)
    {
        if(g_mstp_vlan_port_db[vlan_id].port_mask)
            clear_mask(g_mstp_vlan_port_db[vlan_id].port_mask);
    }
}
/*****************************************************************************/
/* mstpdata_reset_port_vlan_db: Reset port-vlan DB                           */
/*****************************************************************************/
void mstpdata_reset_port_vlan_db()
{
    PORT_ID port_number;

    for(port_number = 0; port_number  < g_max_stp_port ; port_number++)
    {
        if(g_mstp_port_vlan_db[port_number])
        {
            clear_mask(g_mstp_port_vlan_db[port_number]->vlan_mask);
            g_mstp_port_vlan_db[port_number]->untag_vlan = VLAN_ID_INVALID;
        }
    }
}
/*****************************************************************************/
/* mstpdata_update_vlanport_db_on_po_delete: Handle PO de                    */
/*****************************************************************************/
void mstpdata_update_vlanport_db_on_po_delete(PORT_ID port_id)
{
    BITMAP_T *vlanmask;
    VLAN_ID vlan_id;

    vlanmask = mstpdata_get_vlan_mask_for_port(port_id);

    if(vlanmask)
    {
        vlan_id = vlanmask_get_first_vlan(vlanmask);
        while (vlan_id != VLAN_ID_INVALID)
        {
            if(g_mstp_vlan_port_db[vlan_id].port_mask)
                clear_mask_bit(g_mstp_vlan_port_db[vlan_id].port_mask, port_id);
            vlan_id = vlanmask_get_next_vlan(vlanmask, vlan_id);
        }
        clear_mask(g_mstp_port_vlan_db[port_id]->vlan_mask);
    }
    g_mstp_port_vlan_db[port_id]->untag_vlan = VLAN_ID_INVALID;
}