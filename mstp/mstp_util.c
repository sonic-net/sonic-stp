/*
 * Copyright 2025 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

extern UINT8 g_stp_base_mac[L2_ETH_ADD_LEN];

/*****************************************************************************/
/* mstputil_get_common_bridge: returns the common bridge structure for the   */
/* input mstp index                                                          */
/*****************************************************************************/
MSTP_COMMON_BRIDGE *mstputil_get_common_bridge(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE 		*mstp_bridge = NULL;
	MSTP_COMMON_BRIDGE 	*cbridge = NULL;

	mstp_bridge = mstpdata_get_bridge();

	if(!mstp_bridge)
		return NULL;

	if (MSTP_IS_CIST_INDEX(mstp_index))
	{
		MSTP_CIST_BRIDGE *cist_bridge;
		cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
		cbridge =  &(cist_bridge->co);
	}
	else
	{
		MSTP_MSTI_BRIDGE *msti_bridge;
		msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
		if (msti_bridge)
			cbridge = &(msti_bridge->co);
	}

	return cbridge;
}

/*****************************************************************************/
/* mstputil_get_common_port: returns the common port structure for instance  */
/*****************************************************************************/
MSTP_COMMON_PORT *mstputil_get_common_port(MSTP_INDEX mstp_index, MSTP_PORT *mstp_port)
{
	MSTP_COMMON_PORT *cport = NULL;

	if (mstp_port != NULL)
	{
		if (MSTP_IS_CIST_INDEX(mstp_index))
		{
			MSTP_CIST_PORT *cist_port = MSTP_GET_CIST_PORT(mstp_port);
			cport =  &(cist_port->co);
		}
		else
		{
			MSTP_MSTI_PORT *msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
			if (msti_port)
			{
				cport = &(msti_port->co);
			}
		}
	}

	return cport;
}

/*****************************************************************************/
/* mstputil_get_index: returns mstp index associated with the mstid          */
/*****************************************************************************/
MSTP_INDEX mstputil_get_index(MSTP_MSTID mstid)
{
	MSTP_BRIDGE *mstp_bridge;

	if (MSTP_IS_CIST_MSTID(mstid))
	{
		return MSTP_INDEX_CIST;
	}

	if (!MSTP_IS_VALID_MSTID(mstid))
	{
		return MSTP_INDEX_INVALID;
	}

	mstp_bridge = mstpdata_get_bridge();
	if(mstp_bridge != NULL)
	{
		return MSTP_GET_INSTANCE_INDEX(mstp_bridge, mstid);
	}
	else
	{
		return MSTP_INDEX_INVALID;
	}
}

/*****************************************************************************/
/* mstputil_get_mstid: returns mstid associated with the input index         */
/*****************************************************************************/
MSTP_MSTID mstputil_get_mstid(MSTP_INDEX mstp_index)
{
	MSTP_MSTI_BRIDGE *msti_bridge;

	if (MSTP_IS_CIST_INDEX(mstp_index))
		return MSTP_MSTID_CIST;

	msti_bridge = MSTP_GET_MSTI_BRIDGE(mstpdata_get_bridge(), mstp_index);
	
	if (msti_bridge == NULL)
		return MSTP_MSTID_INVALID;

	return msti_bridge->co.bridgeIdentifier.system_id;
}

/*****************************************************************************/
/* mstputil_get_cist_bridge: returns the cist bridge structure               */
/*****************************************************************************/
MSTP_CIST_BRIDGE * mstputil_get_cist_bridge()
{
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    if (mstp_bridge == NULL)
        return NULL;

    return MSTP_GET_CIST_BRIDGE(mstp_bridge);
}

/*****************************************************************************/
/* mstputil_get_msti_bridge: returns the msti bridge structure               */
/*****************************************************************************/
MSTP_MSTI_BRIDGE * mstputil_get_msti_bridge(MSTP_MSTID mstid)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_INDEX mstp_index;
	MSTP_MSTI_BRIDGE *msti_bridge = NULL;

	if (mstp_bridge == NULL)
		return NULL;
	
	mstp_index = mstputil_get_index(mstid);
	if (mstp_index != MSTP_INDEX_INVALID)
	{
		msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
	}
	
	return msti_bridge;
}

/*****************************************************************************/
/* mstputil_get_cist_port: returns the cist port structure                   */
/*****************************************************************************/
MSTP_CIST_PORT * mstputil_get_cist_port(PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return NULL;

	return MSTP_GET_CIST_PORT(mstp_port);
}

/*****************************************************************************/
/* mstputil_get_msti_port: returns the msti port structure for the input     */
/* mstid                                                                     */
/*****************************************************************************/
MSTP_MSTI_PORT * mstputil_get_msti_port(MSTP_MSTID mstid, PORT_ID port_number)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_INDEX mstp_index;

	if (mstp_port == NULL)
		return NULL;

	mstp_index = mstputil_get_index(mstid);
	if (mstp_index == MSTP_INDEX_INVALID)
		return NULL;

	return MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
}

/*****************************************************************************/
/* mstputil_get_default_name: returns the default name for the mst           */
/* configuration id. assumes that input name is at least MSTP_NAME_LENGTH    */
/* bytes in length                                                           */
/*****************************************************************************/
void mstputil_get_default_name(UINT8 *name, UINT32 name_length)
{
	union { UINT8 b[6]; MAC_ADDRESS s; } mac_address;

	if (name == NULL)
		return;

	if (name_length < MSTP_NAME_LENGTH)
		return;

    //populate base mac 
    HOST_TO_NET_MAC(&mac_address.s, &g_stp_base_mac_addr);

    snprintf(name, name_length,
		"%02x-%02x-%02x-%02x-%02x-%02x",
		mac_address.b[0],
		mac_address.b[1],
		mac_address.b[2],
		mac_address.b[3],
		mac_address.b[4],
		mac_address.b[5]
	);
}

/*****************************************************************************/
/* mstputil_bridge_to_string: converts the input bridge identifier to string */
/*****************************************************************************/
UINT8 * mstputil_bridge_to_string(MSTP_BRIDGE_IDENTIFIER *bridge_id, UINT8 *buffer, UINT16 size)
{
	union
	{
		UINT8 b[6];
		MAC_ADDRESS s;
	} mac_address;
    uint16_t tmp;

	HOST_TO_NET_MAC(&(mac_address.s), &(bridge_id->address));
    tmp = (bridge_id->priority << 12) | (bridge_id->system_id);

	snprintf
	(
     buffer,
     size,
     "%04x.%02x%02x.%02x%02x.%02x%02x",
     tmp,
     mac_address.b[0],
		mac_address.b[1],
		mac_address.b[2],
		mac_address.b[3],
		mac_address.b[4],
		mac_address.b[5]
	);

	return buffer;
}

/*****************************************************************************/
/* mstputil_port_to_string: converts the input port identifier to string     */
/*****************************************************************************/
void mstputil_port_to_string(MSTP_PORT_IDENTIFIER *portId, UINT8 *buffer, UINT16 size)
{
	if (portId->number == MSTP_INVALID_PORT)
		snprintf(buffer, size, "N/A");
	else
		snprintf(buffer, size, "%u-%d",
			MSTP_DECODE_PORT_PRIORITY(portId->priority),
			portId->number);
}

/*****************************************************************************/
/* mstputil_compute_message_digest: computes the message digest using md5    */
/* of the vlan to mstid mapping table                                        */
/*****************************************************************************/
void mstputil_compute_message_digest(bool print)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	UINT8 key[16] = MSTP_CONFIG_DIGEST_SIGNATURE_KEY;
	int md_len = 0; 
	unsigned char *md5_ptr = NULL; 

	memset(mstp_bridge->mstConfigId.config_digest,
		0, sizeof(mstp_bridge->mstConfigId.config_digest));

	/* PKCS11 HMAC_CALC function */
	md5_ptr = HMAC(EVP_md5(), key, sizeof(key),
			(unsigned char *) &mstp_bridge->mstid_table, sizeof(mstp_bridge->mstid_table),
			mstp_bridge->mstConfigId.config_digest, &md_len);
	if (md5_ptr == NULL) 
	{ 
		STP_LOG_ERR("MD5 calculation Error in HMAC"); 
		return;   
	} 
	if (md_len != MD5_DIGEST_LENGTH) 
		STP_LOG_ERR("MD5 digest len : %d != 16",md_len); 
	else 
		STP_LOG_DEBUG("MD5 : 0x%s", md5_ptr); 
	
    if (print)
	{
		mstpdebug_print_config_digest(mstp_bridge->mstConfigId.config_digest);
	}
}

/*****************************************************************************/
/* mstputil_computeReselect: computes if port role selection is required     */
/*****************************************************************************/
bool mstputil_computeReselect(MSTP_INDEX mstp_index)
{
	MSTP_COMMON_BRIDGE * cbridge;

	cbridge = mstputil_get_common_bridge(mstp_index);
	if (cbridge)
	{
		return cbridge->reselect;
	}

	return false;
}

/*****************************************************************************/
/* mstputil_setReselectAll: sets reselect for all instances                  */
/*****************************************************************************/
bool mstputil_setReselectAll(PORT_ID port_number)
{
	MSTP_PORT *mstp_port;
	MSTP_INDEX mstp_index;
	
	mstp_port = mstpdata_get_port(port_number);
	if (mstp_port == NULL)
		return false;

	// cist
	mstp_setReselectTree(MSTP_INDEX_CIST);

	// msti
	mstp_index = l2_proto_get_first_instance(&mstp_port->instance_mask);
	while (mstp_index != L2_PROTO_INDEX_INVALID)
	{
		mstp_setReselectTree(mstp_index);
		mstp_index = l2_proto_get_next_instance(&mstp_port->instance_mask, mstp_index);
	}

	return true;
}

/*****************************************************************************/
/* mstputil_set_changedMaster: sets changedMaster on all mst instances       */
/*****************************************************************************/
void mstputil_set_changedMaster(MSTP_PORT *mstp_port)
{
	MSTP_INDEX mstp_index;
	MSTP_MSTI_PORT *msti_port;

	for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
	{
		msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
		if (msti_port)
		{
			msti_port->co.changedMaster = true;
			
			/*802.1Q-2011 - See explanation given in note 2 on page 346. SyncMaster() procedure
			is invoked when the root priority vector for the CIST is recalculated in the 
			updateRolesCIST(), and has a different Regional Root Identifier than that previously 
			selected, and has or had a nonzero CIST External Root Path Cost (See updateRolesTree()).
			SyncMaster() clears the agree, agreed, and synced variables; and sets the sync variable
			as below.*/
			if (mstp_port->infoInternal)
			{
				msti_port->co.agree = false;
				msti_port->co.agreed = false;
				msti_port->co.synced = false;
				msti_port->co.sync = true;
			}

			// Trigerring prs for msti - signaling from
			// CIST to MSTI (see mstp_prs_machine() for the continuation of
			// this).
			mstp_setReselectTree(mstp_index);
		}
	}
}

/*****************************************************************************/
/* mstputil_compare_bridge_id: compares bridge identifiers                   */
/*****************************************************************************/
SORT_RETURN mstputil_compare_bridge_id(MSTP_BRIDGE_IDENTIFIER *id1, MSTP_BRIDGE_IDENTIFIER *id2)
{
	if (id1->priority > id2->priority)
		return GREATER_THAN;

	if (id1->priority < id2->priority)
		return LESS_THAN;

	return stputil_compare_mac(&id1->address, &id2->address);
}

/*****************************************************************************/
/* mstputil_compare_cist_bridge_id: compares bridge identifiers              */
/*****************************************************************************/
SORT_RETURN mstputil_compare_cist_bridge_id(MSTP_BRIDGE_IDENTIFIER *id1, MSTP_BRIDGE_IDENTIFIER *id2)
{
	if (id1->priority > id2->priority)
		return GREATER_THAN;

	if (id1->priority < id2->priority)
		return LESS_THAN;

	if (id1->system_id > id2->system_id)
		return GREATER_THAN;

	if (id1->system_id < id2->system_id)
		return LESS_THAN;

	return stputil_compare_mac(&id1->address, &id2->address);
}

/*****************************************************************************/
/* mstputil_compare_port_id: compares port identifiers                       */
/*****************************************************************************/
SORT_RETURN mstputil_compare_port_id(MSTP_PORT_IDENTIFIER *p1, MSTP_PORT_IDENTIFIER *p2)
{
	if (p1->priority > p2->priority)
		return GREATER_THAN;
	else
	if (p1->priority < p2->priority)
		return LESS_THAN;

	if (p1->number > p2->number)
		return GREATER_THAN;
	else
	if (p1->number < p2->number)
		return LESS_THAN;

	return EQUAL_TO;
}

/*****************************************************************************/
/* mstputil_compare_cist_vectors: compares cist vectors                      */
/*****************************************************************************/
SORT_RETURN mstputil_compare_cist_vectors(MSTP_CIST_VECTOR *vec1, MSTP_CIST_VECTOR *vec2)
{
	SORT_RETURN val;

	// compare vectors
	val = mstputil_compare_cist_bridge_id(&(vec1->root), &(vec2->root));
	if (val != EQUAL_TO)
		return val;

	if (vec1->extPathCost > vec2->extPathCost)
		return GREATER_THAN;
	else
	if (vec1->extPathCost < vec2->extPathCost)
		return LESS_THAN;

	val = mstputil_compare_cist_bridge_id(&(vec1->regionalRoot), &(vec2->regionalRoot));
	if (val != EQUAL_TO)
		return val;

	if (vec1->intPathCost > vec2->intPathCost)
		return GREATER_THAN;
	else
	if (vec1->intPathCost < vec2->intPathCost)
		return LESS_THAN;

	val = mstputil_compare_cist_bridge_id(&(vec1->designatedId), &(vec2->designatedId));
	if (val != EQUAL_TO)
		return val;

	return mstputil_compare_port_id(&(vec1->designatedPort), &(vec2->designatedPort));
}

/*****************************************************************************/
/* mstputil_compare_msti_vectors: compares msti vectors                      */
/*****************************************************************************/
SORT_RETURN mstputil_compare_msti_vectors(MSTP_MSTI_VECTOR *vec1, MSTP_MSTI_VECTOR *vec2)
{
	SORT_RETURN val;

	val = mstputil_compare_bridge_id(&(vec1->regionalRoot), &(vec2->regionalRoot));
	if (val != EQUAL_TO)
		return val;

	if (vec1->intPathCost > vec2->intPathCost)
		return GREATER_THAN;
	else
	if (vec1->intPathCost < vec2->intPathCost)
		return LESS_THAN;

	val = mstputil_compare_bridge_id(&(vec1->designatedId), &(vec2->designatedId));
	if (val != EQUAL_TO)
		return val;

	return mstputil_compare_port_id(&(vec1->designatedPort), &(vec2->designatedPort));
}

/*****************************************************************************/
/* mstputil_are_cist_times_equal: checks if the input times are equal        */
/*****************************************************************************/
bool mstputil_are_cist_times_equal(MSTP_CIST_TIMES *t1, MSTP_CIST_TIMES *t2)
{
	return
	(
		t1->fwdDelay == t2->fwdDelay &&
		t1->maxAge == t2->maxAge &&
		t1->messageAge == t2->messageAge &&
		t1->helloTime == t2->helloTime &&
		t1->remainingHops == t2->remainingHops
	);
}

/*****************************************************************************/
/* mstputil_is_cist_root_bridge: checks if cist bridge is the root bridge    */
/*****************************************************************************/
bool mstputil_is_cist_root_bridge()
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_CIST_BRIDGE *cist_bridge;

	cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
	return (mstputil_compare_cist_bridge_id(&cist_bridge->co.bridgeIdentifier, &cist_bridge->rootPriority.root) == EQUAL_TO);
}

/*****************************************************************************/
/* mstputil_is_msti_root_bridge: checks if msti bridge is the root bridge    */
/*****************************************************************************/
bool mstputil_is_msti_root_bridge(MSTP_INDEX mstp_index)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_MSTI_BRIDGE *msti_bridge;

	msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
	if (msti_bridge)
	{
		return (mstputil_compare_bridge_id(&msti_bridge->co.bridgeIdentifier, &msti_bridge->rootPriority.regionalRoot) == EQUAL_TO);
	}

	return false;
}

/*****************************************************************************/
/* mstputil_compute_cist_designated_priority: computes cist                  */
/* designatedPriority vector for the input port                              */
/*****************************************************************************/
void mstputil_compute_cist_designated_priority(MSTP_CIST_BRIDGE *cist_bridge, MSTP_PORT *mstp_port)
{
	MSTP_CIST_PORT *cist_port = MSTP_GET_CIST_PORT(mstp_port);
	MSTP_CIST_VECTOR *designatedPriority = &cist_port->designatedPriority;
	MSTP_CIST_VECTOR *rootPriority = &cist_bridge->rootPriority;

    designatedPriority->root = rootPriority->root;
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_ROOT_BIT);

    designatedPriority->extPathCost = rootPriority->extPathCost;
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_EX_PATH_COST_BIT);

    designatedPriority->regionalRoot = ((mstp_port->ppmState == MSTP_PPM_SELECTING_STP) ?
            cist_bridge->co.bridgeIdentifier : rootPriority->regionalRoot);
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_REG_ROOT_BIT);

    designatedPriority->intPathCost = rootPriority->intPathCost;
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_IN_PATH_COST_BIT);

    designatedPriority->designatedId = cist_bridge->co.bridgeIdentifier;
    SET_BIT(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_BRIDGE_BIT);

    designatedPriority->designatedPort = cist_port->co.portId;

}

/*****************************************************************************/
/* mstputil_compute_msti_designated_priority: computes msti                  */
/* designatedPriority vector for the input port                              */
/*****************************************************************************/
void mstputil_compute_msti_designated_priority(MSTP_MSTI_BRIDGE *msti_bridge, MSTP_MSTI_PORT *msti_port)
{
	MSTP_MSTI_VECTOR *designatedPriority = &msti_port->designatedPriority;
	MSTP_MSTI_VECTOR *rootPriority = &msti_bridge->rootPriority;

	designatedPriority->regionalRoot = rootPriority->regionalRoot;
    SET_BIT(msti_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_REG_ROOT_BIT);

	designatedPriority->intPathCost = rootPriority->intPathCost;
    SET_BIT(msti_port->modified_fields, MSTP_PORT_MEMBER_IN_PATH_COST_BIT);

	designatedPriority->designatedId = msti_bridge->co.bridgeIdentifier;
	SET_BIT(msti_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_BRIDGE_BIT);

    designatedPriority->designatedPort = msti_port->co.portId;
}

/*****************************************************************************/
/* mstputil_extract_cist_msg_info: gets cist msgTimes and msgPriority from   */
/* received bpdu                                                             */
/*****************************************************************************/
void mstputil_extract_cist_msg_info(PORT_ID port_number, MSTP_BPDU *bpdu)
{
	MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_CIST_PORT *cist_port;

	if (!mstp_port)
	{
		return;
	}

	if (MSTP_DEBUG_BPDU_RX(MSTP_INDEX_CIST, port_number))
	{
		STP_LOG_INFO("[MST %u] Port %d - RX BPDU", MSTP_MSTID_CIST, port_number);
		mstpdebug_display_bpdu(bpdu, true);
	}

	cist_port = MSTP_GET_CIST_PORT(mstp_port);

	cist_port->msgTimes.fwdDelay = (UINT8) bpdu->forward_delay;
	cist_port->msgTimes.helloTime = (UINT8) bpdu->hello_time;
	cist_port->msgTimes.maxAge = (UINT8) bpdu->max_age;
	cist_port->msgTimes.messageAge = (UINT8) bpdu->message_age;

	if (bpdu->protocol_version_id == MSTP_VERSION_ID)
	{
		cist_port->msgTimes.remainingHops = bpdu->cist_remaining_hops;
		cist_port->msgPriority.designatedId = bpdu->cist_bridge;
		cist_port->msgPriority.intPathCost = (mstp_port->rcvdInternal? bpdu->cist_int_path_cost : 0);
	}
	else
	{
		cist_port->msgTimes.remainingHops = mstp_bridge->cist.bridgeTimes.remainingHops;
		cist_port->msgPriority.designatedId = bpdu->cist_regional_root;
		cist_port->msgPriority.intPathCost = 0;
	}

	cist_port->msgPriority.root = bpdu->cist_root;
	cist_port->msgPriority.extPathCost = bpdu->cist_ext_path_cost;
	cist_port->msgPriority.regionalRoot = bpdu->cist_regional_root;
	cist_port->msgPriority.designatedPort.number = bpdu->cist_port.number;
	cist_port->msgPriority.designatedPort.priority = bpdu->cist_port.priority;
}

/*****************************************************************************/
/* mstputil_extract_msti_msg_info: gets msti msgTimes and msgPriority from   */
/* received bpdu                                                             */
/*****************************************************************************/
void mstputil_extract_msti_msg_info(PORT_ID port_number, MSTP_BPDU *bpdu, MSTI_CONFIG_MESSAGE *msg)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_MSTI_PORT *msti_port;
	MSTP_MSTID mstid;
	MSTP_INDEX mstp_index;

	mstid = msg->msti_regional_root.system_id;
	mstp_index = mstputil_get_index(mstid);

	if (mstp_index == MSTP_INDEX_INVALID)
		return;

	msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
	if (msti_port == NULL)
		return;

	if (MSTP_DEBUG_BPDU_RX(mstp_index, port_number))
	{
		STP_LOG_DEBUG("[MST %u] Port %d - received MSTI config message",
			mstputil_get_mstid(mstp_index), port_number);
		mstpdebug_display_msti_bpdu(msg);
	}

	msti_port->msgPriority.regionalRoot = msg->msti_regional_root;
	msti_port->msgPriority.intPathCost = msg->msti_int_path_cost;

	msti_port->msgPriority.designatedId.address = bpdu->cist_bridge.address;
	msti_port->msgPriority.designatedId.priority = msg->msti_bridge_priority;
	msti_port->msgPriority.designatedId.system_id = mstid;

	msti_port->msgPriority.designatedPort.number = bpdu->cist_port.number;
	msti_port->msgPriority.designatedPort.priority = msg->msti_port_priority;

	msti_port->msgTimes.remainingHops = msg->msti_remaining_hops;
}

/*****************************************************************************/
/* mstputil_network_order_bpdu: converts from host to network order          */
/*****************************************************************************/
void mstputil_network_order_bpdu(MSTP_BPDU *bpdu)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    UINT16 i;
    UINT16 num_msti_messages;

    bpdu->protocol_id = htons(bpdu->protocol_id);
    if (bpdu->type == TCN_BPDU_TYPE)
        return;

    HOST_TO_NET_MAC(&bpdu->cist_root.address, &bpdu->cist_root.address);
    *((UINT16 *)&bpdu->cist_root) = htons(*((UINT16 *)&bpdu->cist_root));
    bpdu->cist_ext_path_cost = htonl(bpdu->cist_ext_path_cost);
    HOST_TO_NET_MAC(&bpdu->cist_regional_root.address, &bpdu->cist_regional_root.address);
    *((UINT16 *)&bpdu->cist_regional_root) = htons(*((UINT16 *)&bpdu->cist_regional_root));


    bpdu->message_age = htons(bpdu->message_age);
    bpdu->max_age = htons(bpdu->max_age);
    bpdu->hello_time = htons(bpdu->hello_time);
    bpdu->forward_delay = htons(bpdu->forward_delay);

    if (bpdu->protocol_version_id == RSTP_VERSION_ID || 
            bpdu->protocol_version_id == STP_VERSION_ID)
        return;

    num_msti_messages = MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(bpdu->v3_length);

    // Validate the number of MSTI messages to prevent buffer overflow
    if (num_msti_messages > MSTP_MAX_INSTANCES_PER_REGION) {
        num_msti_messages = MSTP_MAX_INSTANCES_PER_REGION;
    }

    for (i = 0; i < num_msti_messages; i++)
    {
        HOST_TO_NET_MAC(&bpdu->msti_msgs[i].msti_regional_root.address,
            &bpdu->msti_msgs[i].msti_regional_root.address);
        *((UINT16 *)&bpdu->msti_msgs[i].msti_regional_root) = htons(*((UINT16 *)&bpdu->msti_msgs[i].msti_regional_root));

        bpdu->msti_msgs[i].msti_int_path_cost = ntohl(bpdu->msti_msgs[i].msti_int_path_cost);
    }

    bpdu->v3_length = htons(bpdu->v3_length);
    bpdu->mst_config_id.revision_number = htons(bpdu->mst_config_id.revision_number);
    bpdu->cist_int_path_cost = htonl(bpdu->cist_int_path_cost);
    *((UINT16 *) &bpdu->cist_port)    = htons(*((UINT16 *) &bpdu->cist_port));
    HOST_TO_NET_MAC(&bpdu->cist_bridge.address, &bpdu->cist_bridge.address);
    *((UINT16 *)&bpdu->cist_bridge) = htons(*((UINT16 *)&bpdu->cist_bridge));
#endif // !BIG_ENDIAN
}

/*****************************************************************************/
/* mstputil_host_order_bpdu: converts from network to host order             */
/*****************************************************************************/
void mstputil_host_order_bpdu(MSTP_BPDU *bpdu)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    UINT16 i;
    UINT16 num_msti_messages;

    bpdu->protocol_id = ntohs(bpdu->protocol_id);
    if (bpdu->type == TCN_BPDU_TYPE)
        return;

    NET_TO_HOST_MAC(&bpdu->cist_root.address, &bpdu->cist_root.address);
    *((UINT16 *)&bpdu->cist_root) = ntohs(*((UINT16 *)&bpdu->cist_root));
    bpdu->cist_ext_path_cost = ntohl(bpdu->cist_ext_path_cost);
    NET_TO_HOST_MAC(&bpdu->cist_regional_root.address, &bpdu->cist_regional_root.address);
    *((UINT16 *)&bpdu->cist_regional_root) = ntohs(*((UINT16 *)&bpdu->cist_regional_root));

    bpdu->message_age = ntohs(bpdu->message_age) >> 8;
    bpdu->max_age = ntohs(bpdu->max_age) >> 8;
    bpdu->hello_time = ntohs(bpdu->hello_time) >> 8;
    bpdu->forward_delay = ntohs(bpdu->forward_delay) >> 8;

    if (bpdu->protocol_version_id == RSTP_VERSION_ID || 
            bpdu->protocol_version_id == STP_VERSION_ID)
        return;

    bpdu->v3_length = ntohs(bpdu->v3_length);
    num_msti_messages = MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(bpdu->v3_length);

    // Validate the number of MSTI messages to prevent buffer overflow
    if (num_msti_messages > MSTP_MAX_INSTANCES_PER_REGION) {
        num_msti_messages = MSTP_MAX_INSTANCES_PER_REGION;
    }

    for (i = 0; i < num_msti_messages; i++)
    {
        NET_TO_HOST_MAC(&bpdu->msti_msgs[i].msti_regional_root.address,
            &bpdu->msti_msgs[i].msti_regional_root.address);
        *((UINT16 *)&bpdu->msti_msgs[i].msti_regional_root) = ntohs(*((UINT16 *)&bpdu->msti_msgs[i].msti_regional_root));

        bpdu->msti_msgs[i].msti_int_path_cost = ntohl(bpdu->msti_msgs[i].msti_int_path_cost);
    }

    bpdu->mst_config_id.revision_number = ntohs(bpdu->mst_config_id.revision_number);
    bpdu->cist_int_path_cost = ntohl(bpdu->cist_int_path_cost);
    *((UINT16 *) &bpdu->cist_port)    = ntohs(*((UINT16 *) &bpdu->cist_port));
    NET_TO_HOST_MAC(&bpdu->cist_bridge.address, &bpdu->cist_bridge.address);
    *((UINT16 *)&bpdu->cist_bridge) = ntohs(*((UINT16 *)&bpdu->cist_bridge));
#else  //!BIG_ENDIAN
    bpdu->message_age >>= 8;
    bpdu->max_age >>= 8;
    bpdu->hello_time >>= 8;
    bpdu->forward_delay >>= 8;
#endif 
}

/*****************************************************************************/
/* mstputil_update_stats: updates the bpdu transmit and receive statistics   */
/* for the specific port                                                     */
/*****************************************************************************/
void mstputil_update_stats(PORT_ID port_number, MSTP_BPDU *bpdu, bool rx)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	MSTP_BPDU_STATS *stats;

	if (mstp_port == NULL)
        return;

	stats = (rx) ? &(mstp_port->stats.rx) : &(mstp_port->stats.tx);
    stats->update_flag = true;

	switch (bpdu->type)
	{
		case CONFIG_BPDU_TYPE:
			stats->config_bpdu++;
			break;

		case TCN_BPDU_TYPE:
			stats->tcn_bpdu++;
			break;

		case RSTP_BPDU_TYPE:
			if (bpdu->protocol_version_id == RSTP_VERSION_ID)
				stats->rstp_bpdu++;
			else
				stats->mstp_bpdu++;
			break;

		default:
			STP_LOG_ERR("mstputil_update_stats() - unknown bpdu type %u",
				bpdu->type);
			break;
	}
}

/*****************************************************************************/
/* mstputil_validate_bpdu: checks that the received bpdu is valid.           */
/*****************************************************************************/
bool mstputil_validate_bpdu(MSTP_BPDU *bpdu, UINT16 size)
{
	// size indicates the packet length of the bpdu

	if (size < STP_SIZEOF_TCN_BPDU)
	{
		STP_LOG_ERR("invalid pkt len %u",size);
		return false;
	}

	if (bpdu->protocol_id != 0)
	{
		STP_LOG_ERR("protocol id %u is non-zero",bpdu->protocol_id);
		return false;
	}

	// validation of stp packet
	if (bpdu->protocol_version_id == STP_VERSION_ID)
	{
		if ((bpdu->type == TCN_BPDU_TYPE && size >= STP_SIZEOF_TCN_BPDU) ||
			(bpdu->type == CONFIG_BPDU_TYPE && size >= STP_SIZEOF_CONFIG_BPDU))
		{
			return true;
		}

		STP_LOG_ERR("bpdu type=%u, size=%u - mismatch",
			bpdu->type, size);
		return false;
	}

	// validation of rstp packet
	if (bpdu->protocol_version_id == RSTP_VERSION_ID)
	{
		if (bpdu->type == RSTP_BPDU_TYPE && size >= RSTP_SIZEOF_BPDU)
		{
			return true;
		}

		STP_LOG_ERR("bpdu type=%u, size=%u - mismatch",
			bpdu->type, size);
		return false;
	}

	// validation of mstp packet
	if (bpdu->protocol_version_id >= MSTP_VERSION_ID)
	{
		 if (bpdu->type == RSTP_BPDU_TYPE)
		 {
			if ((size >= STP_SIZEOF_CONFIG_BPDU && size < 102) ||
				(bpdu->v1_length != 0) ||
				(bpdu->v3_length % 16 != 0))
			{
				// decode this as an rstp bpdu (modify the version id so that
				// upper layers do not get confused).
				bpdu->protocol_version_id = RSTP_VERSION_ID;
				if (size < RSTP_SIZEOF_BPDU)
				{
					STP_LOG_ERR("mstp bpdu, size %u mismatched",
						size);
					return false;
				}

				return true;
			}

			// Additional validation: check if v3_length would cause buffer overflow
			// when calculating the number of MSTI configuration messages
			UINT16 num_msti_messages = MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(bpdu->v3_length);
			if (num_msti_messages > MSTP_MAX_INSTANCES_PER_REGION) {
				STP_LOG_ERR("mstp bpdu v3_length %u would cause %u MSTI messages, exceeding limit of %u",
					bpdu->v3_length, num_msti_messages, MSTP_MAX_INSTANCES_PER_REGION);
				return false;
			}

			// mstp bpdu
			return true;
		 }
	}

	STP_LOG_ERR("unknown protocol version %u",
		bpdu->protocol_version_id);
	return false;
}

/* FUNCTION
 *		mstputil_set_kernel_bridge_port_state_for_single_vlan()
 *
 * SYNOPSIS
 *      Linux VLAN aware brige model doesn't have a mechanism to update vlan + port state.
 *		As a workaround we update the VLAN port membership for linux kernel bridge for filtering the traffic 
 *		if port is blocking (not forwarding) then VLAN is removed from the port in kernel
 *		when it moves to forwarding VLAN is added back to the port
 */
bool mstputil_set_kernel_bridge_port_state_for_single_vlan(VLAN_ID vlan_id, PORT_ID port_number, enum L2_PORT_STATE state)
{
    int ret;
    VLAN_ID   untag_vlan;
    char cmd_buff[100];

    untag_vlan = mstpdata_get_untag_vlan_for_port(port_number);

    if(state == FORWARDING) 
    {
        snprintf(cmd_buff, 100, "/sbin/bridge vlan add vid %u dev %s %s", vlan_id, 
            stp_intf_get_port_name(port_number), ((vlan_id == untag_vlan)?"untagged":"tagged"));
        ret = system(cmd_buff);
        if(ret == -1)
        {
            STP_LOG_ERR("[Vlan %u] Port %d Error: strerr - %s", vlan_id, port_number, strerror(errno));
            return false;
        }
        STP_LOG_INFO("[Vlan %u] Port %d (%s) Added", vlan_id, port_number, ((vlan_id == untag_vlan)? "untagged": "tagged"));
    }
    else if(state != FORWARDING) 
    {
         snprintf(cmd_buff, 100, "/sbin/bridge vlan del vid %u dev %s %s", vlan_id, 
            stp_intf_get_port_name(port_number), ((vlan_id == untag_vlan)?"untagged":"tagged"));
        ret = system(cmd_buff);
        if(ret == -1)
        {
            STP_LOG_ERR("[Vlan %u] Port %d Error: strerr - %s", vlan_id, port_number, strerror(errno));
            return false;
        }
        STP_LOG_INFO("[Vlan %u] Port %d Removed", vlan_id, port_number);
    }
    else
    {
        return true;//no-op
    }

    return true;
}

bool mstputil_set_kernel_bridge_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE state)
{
    MSTP_COMMON_BRIDGE 	*cbridge = NULL;
    MSTP_BRIDGE *mstp_bridge = mstpdata_get_bridge();

    int         ret;
    BITMAP_T    *mstp_vlanmask;
    VLAN_ID     vlan_id = VLAN_ID_INVALID, untag_vlan;
    VLAN_MASK   vlanmask;
    UINT8       vlanmask_string[500] = {0,};
    char        cmd_buff[100];

    cbridge = mstputil_get_common_bridge(mstp_index);

    if(!cbridge)
        return false;

    mstp_vlanmask = mstpdata_get_vlan_mask_for_port(port_number);
    if(!mstp_vlanmask)
        return false;

    vlan_bmp_init(&vlanmask);

    /* vlanmask has all vlans binded to the port. 
     * We need to find out those vlans which is mapped to the given mstp index*/
    and_masks((BITMAP_T *)&vlanmask , mstp_vlanmask, (BITMAP_T *)&cbridge->vlanmask);

    if (is_mask_clear((BITMAP_T *)&vlanmask))
        return false;

    untag_vlan = mstpdata_get_untag_vlan_for_port(port_number);

    if (IS_MEMBER(mstp_bridge->admin_disable_mask, port_number))
    {
        copy_mask((BITMAP_T *)&vlanmask, mstp_vlanmask);
    }

    if(state == FORWARDING)  
    {
        vlan_id = vlanmask_get_first_vlan(&vlanmask);
        while (vlan_id != VLAN_ID_INVALID)
        {
            snprintf(cmd_buff, 100, "/sbin/bridge vlan add vid %u dev %s %s", vlan_id, 
                stp_intf_get_port_name(port_number), ((vlan_id == untag_vlan)?"untagged":"tagged"));
            ret = system(cmd_buff);
            if(ret == -1)
            {
                STP_LOG_ERR("[Vlan %u] Port %d Error: strerr - %s", vlan_id, port_number, strerror(errno));
                return false;
            }
            vlan_id = vlanmask_get_next_vlan(&vlanmask, vlan_id);
        }
        vlanmask_to_string((BITMAP_T *)&vlanmask, vlanmask_string, sizeof(vlanmask_string));
        STP_LOG_INFO("[Port %d] Vlan %s Added", port_number, vlanmask_string);
    }
    else if(state != FORWARDING)
    {
        vlan_id = vlanmask_get_first_vlan(&vlanmask);
        while (vlan_id != VLAN_ID_INVALID)
        {
            snprintf(cmd_buff, 100, "/sbin/bridge vlan del vid %u dev %s %s", vlan_id, 
            stp_intf_get_port_name(port_number), ((vlan_id == untag_vlan)?"untagged":"tagged"));
            ret = system(cmd_buff);
            if(ret == -1)
            {
                STP_LOG_ERR("[Vlan %u] Port %d Error: strerr - %s", vlan_id, port_number, strerror(errno));
                return false;
            }
            vlan_id = vlanmask_get_next_vlan(&vlanmask, vlan_id);
        }
        vlanmask_to_string((BITMAP_T *)&vlanmask, vlanmask_string, sizeof(vlanmask_string));
        STP_LOG_INFO("[Port %d] Vlan %s Removed", port_number, vlanmask_string);
    }
    else
    {
        return true;//no-op
    }

    return true;
}


/*****************************************************************************/
/* mstputil_set_port_state: sets the port state for all the vlans associated */
/* with the input mst instance to the input state                            */
/*****************************************************************************/
bool mstputil_set_port_state(MSTP_INDEX mstp_index, PORT_ID port_number, enum L2_PORT_STATE state)
{
	MSTP_COMMON_PORT 	*cport = NULL;
	char                *ifname = NULL;
    MSTP_MSTID mst_id = mstputil_get_mstid(mstp_index);
	MSTP_PORT *mstp_port;
    MSTP_MSTI_PORT *msti_port;
    MSTP_CIST_PORT *cist_port;
    PORT_MASK *portmask;
    MSTP_COMMON_BRIDGE *cbridge;

	ifname = stp_intf_get_port_name(port_number);

    mstp_port = mstpdata_get_port(port_number);
    if(!mstp_port)
        return false;

    cport = mstputil_get_common_port(mstp_index, mstp_port);

    if (!cport)
        return false;

    if(cport->state == state)
    {
        /* new vlan mapping should be updated */
        STP_LOG_INFO("[MST %d] %s state %d same", mst_id, ifname, cport->state);
        return false;
    }
    cport->state = state;

    STP_LOG_INFO("[MST %d] %s %s", mst_id, ifname, MSTP_STATE_STRING(state, port_number));

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        cist_port = MSTP_GET_CIST_PORT(mstp_port);
        if(cist_port)
            SET_BIT(cist_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT);

        /* If port is not a member of CIST, we dont need to update the hw port state */
        portmask = mstplib_instance_get_portmask(mstp_index);
        if(!is_member(portmask, port_number))
        {
            STP_LOG_INFO("[MST %d] %s state not send to h/w", mst_id, ifname);
            return false;
        }
    }
    else
    {
        msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
        if (msti_port)
            SET_BIT(msti_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT);
    }
    
    mstputil_set_kernel_bridge_port_state(mstp_index, port_number, state);

	if(ifname)
	{
        stpsync_update_port_state(ifname, mst_id, state);
    }		
	return true;
}

/*****************************************************************************/
/* mstputil_flush: Flushes the FDB for a given port of a specific MST        */
/* insance if the fdb flush is pending after topology change                 */
/*****************************************************************************/
bool mstputil_flush(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    char * ifname = NULL;
    MSTP_MSTID mst_id = mstputil_get_mstid(mstp_index);
    MSTP_COMMON_PORT *cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
    if(cport == NULL)
        return false;
    
    ifname = stp_intf_get_port_name(port_number);
    if(!ifname)
        return false;
    stpsync_flush_instance_port(ifname, mst_id);
    STP_LOG_INFO("[MST %d] %s flush", mst_id, ifname);
  

    return true;
}

/*****************************************************************************/
/* mstptimer_decrement: timer utility to decrement timer if required.        */
/* returns true if the timer was active and expired                          */
/*****************************************************************************/
bool mstptimer_decrement(UINT16 *ptimer)
{
	if (*ptimer != 0)
	{
		(*ptimer)--;
		if (*ptimer == 0)
		{
			return true;
		}
	}
	return false;
}
/*****************************************************************************/
/* mstptimer_update: updates all timers for this mst instance                */
/*****************************************************************************/
static void mstptimer_update(MSTP_INDEX mstp_index, PORT_ID port_number)
{
	MSTP_COMMON_PORT *cport, *msti_cport;
	MSTP_PORT *mstp_port;
	MSTP_MSTI_PORT *msti_port;
	MSTP_MSTID mstp_id=mstputil_get_mstid(mstp_index);
    MSTP_INDEX mst_index;
	
    mstp_port = mstpdata_get_port(port_number);
    if(!mstp_port)
        return;
    
	cport = mstputil_get_common_port(mstp_index, mstpdata_get_port(port_number));
	if (cport == NULL)
		return;

    if ((cport->role == MSTP_ROLE_DESIGNATED ||
        cport->role == MSTP_ROLE_ROOT ||
        cport->role == MSTP_ROLE_MASTER) &&
        (mstptimer_decrement(&cport->fdWhile)))
    {
        mstp_prt_gate(mstp_index, port_number);
    }

    if (cport->role != MSTP_ROLE_ROOT &&
        mstptimer_decrement(&cport->rrWhile))
    {
        STP_LOG_INFO("[MST %u] Port %d rrWhile expired",  mstp_id, port_number);
        mstp_prt_gate(mstp_index, port_number);
    }

    if (cport->role != MSTP_ROLE_BACKUP &&
        mstptimer_decrement(&cport->rbWhile))
    {
        STP_LOG_INFO("[MST %u] Port %d rbWhile expired",  mstp_id, port_number);
        mstp_prt_gate(mstp_index, port_number);		
    }

    if (mstptimer_decrement(&cport->tcWhile))
    {
        STP_LOG_INFO("[MST %u] Port %d tcWhile expired",  mstp_id, port_number);
        mstp_tcm_gate(mstp_index, port_number);
    }

    if (mstptimer_decrement(&cport->rcvdInfoWhile))
    {
        SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_REM_TIME_BIT);
        STP_LOG_INFO("[MST %u Port %d rcvdInfoWhile expired",  mstp_id, port_number);
        mstp_pim_gate(mstp_index, port_number, NULL);
    }
    else
    {
        if(cport->role == MSTP_ROLE_ROOT || cport->role == MSTP_ROLE_ALTERNATE || cport->role == MSTP_ROLE_BACKUP)
        {
            SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_REM_TIME_BIT);
        }
    }
    
    mstp_flush(mstp_index, port_number);
   
}

/*****************************************************************************/
/* mstputil_timer_tick: called every 100 ms handles all instance timers           */
/*****************************************************************************/
void mstputil_timer_tick()
{
	MSTP_INDEX mstp_index;
	PORT_ID port_number;
	MSTP_COMMON_PORT *cport;
	MSTP_BRIDGE *mstp_bridge;
	MSTP_PORT *mstp_port;
	static UINT8 mstp_tick = 0;
    MSTP_MSTID mstp_id;

    mstp_bridge = mstpdata_get_bridge();
    if (mstp_bridge == NULL || !mstp_bridge->active)
        return;

    mstp_tick++;

    if (mstp_tick >= 10)
    {
        // call the timers every one second
        mstp_tick = 0;

        port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
        while (port_number != BAD_PORT_ID)
        {
            mstp_port = mstpdata_get_port(port_number);
            if (mstp_port)
            {
                // cist
                cport = mstputil_get_common_port(MSTP_INDEX_CIST, mstp_port);
                if (cport != NULL)
                {
                    mstptimer_update(MSTP_INDEX_CIST, port_number);
                }

                // msti
                for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
                {
                    if (mstp_bridge->msti[mstp_index] == NULL)
                        continue;
                    cport = mstputil_get_common_port(mstp_index, mstp_port);
                    if (cport != NULL)
                    {
                        mstptimer_update(mstp_index, port_number);
                    }
                }

                if (mstp_port->txCount)
                {
                    mstp_port->txCount--;
                }

                if (mstptimer_decrement(&mstp_port->helloWhen))
                {
                    if (!mstp_bridge->disable_auto_edge_port &&
                            !mstp_port->operEdge &&
                            !mstp_port->stats.rx.config_bpdu &&
                            !mstp_port->stats.rx.tcn_bpdu &&
                            !mstp_port->stats.rx.rstp_bpdu &&
                            !mstp_port->stats.rx.mstp_bpdu &&
                            (mstp_port->stats.tx.rstp_bpdu >= 2 ||
                             mstp_port->stats.tx.config_bpdu >=2 ||
                             mstp_port->stats.tx.mstp_bpdu >= 2))
                    {
                        /*
                         * since no bpdus have been received - set operEdge to true
                         * this will be reset if bpdus are received.
                         */
                        mstp_port->operEdge = true;

                        //Change in operEdge status should trigger PRT and TCM
                        mstp_prt_gate(MSTP_INDEX_CIST, port_number);
                        mstp_tcm_gate(MSTP_INDEX_CIST, port_number);

                        for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_MAX; mstp_index++)
                        {
                            if (mstp_bridge->msti[mstp_index] == NULL)
                                continue;

                            mstp_prt_gate(mstp_index, port_number);
                            mstp_tcm_gate(mstp_index, port_number);
                        }
                    }
                    mstp_ptx_gate(port_number);
                }

                if (mstptimer_decrement(&mstp_port->mdelayWhile))
                {
                    mstp_ppm_gate(port_number);
                }
            } 
            port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
        }

        for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_CIST; mstp_index++)
        {
            if (mstp_index == MSTP_INDEX_CIST)
            {
                mstputil_timer_sync_db(mstp_index);
            }
            else
            {
                if (mstp_bridge->msti[mstp_index] == NULL)
                    continue;
                mstputil_timer_sync_db(mstp_index);
            }
        }

        g_stp_bpdu_sync_tick_id++;
        if(g_stp_bpdu_sync_tick_id >= 10)
        {
            mstputil_timer_sync_bpdu_counters();
            g_stp_bpdu_sync_tick_id = 0;
        }
    }
}

bool mstputil_is_recv_superior_cist_msg(MSTP_CIST_VECTOR *cist_msg_vec, MSTP_CIST_TIMES *cist_msg_times, MSTP_CIST_PORT *cist_port, MSTP_BPDU *bpdu)
{
	SORT_RETURN result;
	bool flag;

	if (cist_port == NULL)
	{
		return false;
	}

	if (bpdu->type == TCN_BPDU_TYPE)
	{
		return false;
	}

	result = mstputil_compare_cist_vectors(cist_msg_vec, &cist_port->portPriority);

	/* case 1 and case 2 */
	if ((bpdu->type == CONFIG_BPDU_TYPE) ||
		(bpdu->type == RSTP_BPDU_TYPE && bpdu->cist_flags.role == MSTP_ROLE_DESIGNATED))
	{
		/* case 1 a */
		if (result == LESS_THAN)
		{
			return true;
		}
		else if (result == EQUAL_TO)
		{
			if(mstputil_compare_cist_bridge_id(&(cist_port->portPriority.designatedId),
				&(cist_port->designatedPriority.designatedId)) == LESS_THAN)
			{

				return true;
			}
		}

		flag = mstputil_are_cist_times_equal(cist_msg_times, &(cist_port->portTimes));

		/* case 1 b */
		// add changes here to match 802.1Q-Rev changes. this will prevent
		// stale information from circulating too long. the primary issue is that
		// if the bridge priority of the designated bridge changes then the change
		// needs to be registered by the mstp instance
		if ((stputil_compare_mac(&(cist_msg_vec->designatedId.address),
				&(cist_port->portPriority.designatedId.address)) == EQUAL_TO) &&
			(cist_msg_vec->designatedPort.number ==
				cist_port->portPriority.designatedPort.number) &&
			(flag == false || result != EQUAL_TO))
		{
			return true;
		}
	}

	return false;
}

bool mstputil_is_recv_superior_msti_msg(MSTP_MSTI_VECTOR *msti_msg_vec, MSTP_MSTI_PORT *msti_port, bool flag)
{
	SORT_RETURN result;

	// compare vectors
	result = mstputil_compare_msti_vectors(msti_msg_vec, &msti_port->portPriority);

	/* case 1 a */
	if (result == LESS_THAN)
	{
		return true;
	}
	else if(result == EQUAL_TO)
	{
                if (mstputil_compare_bridge_id(&(msti_port->portPriority.designatedId), 
			&(msti_port->designatedPriority.designatedId)) == LESS_THAN)
		{
			return true;
		}
	}

	/* case 1 b */
	// MSTP_BUG - see equivalent note in mstp_rcvInfoCist() above.
	if ((stputil_compare_mac(&(msti_msg_vec->designatedId.address),
			&(msti_port->portPriority.designatedId.address)) == EQUAL_TO) &&
		(msti_msg_vec->designatedPort.number ==
			msti_port->portPriority.designatedPort.number) &&
		(flag == false || result != EQUAL_TO))
	{
		return true;
	}

	return false;
}

/*****************************************************************************/
/* mstputil_get_bpdu_size: gets size of the bpdu to transmit                 */
/*****************************************************************************/
static UINT16 mstputil_get_bpdu_size(MSTP_BPDU *bpdu)
{
	UINT16 bpdu_size, max_instances, msti_config_msgsize, v3_length, num_instances;

	if (bpdu->protocol_version_id == STP_VERSION_ID)
	{
		bpdu_size = sizeof(STP_CONFIG_BPDU);
	}
	else
	if (bpdu->protocol_version_id == RSTP_VERSION_ID)
	{
		bpdu_size = sizeof(RSTP_BPDU);
	}
	else
	{
		bpdu_size = sizeof(MSTP_BPDU);
		max_instances = MSTP_MAX_INSTANCES_PER_REGION;
		v3_length = bpdu->v3_length;
		msti_config_msgsize = sizeof(MSTI_CONFIG_MESSAGE);
		num_instances = MSTP_GET_NUM_MSTI_CONFIG_MESSAGES(v3_length);

        // Validate the number of MSTI messages to prevent potential underflow
		if (num_instances > max_instances) {
			num_instances = max_instances;
		}

		bpdu_size -= ((max_instances-num_instances) * msti_config_msgsize);
	}

	return bpdu_size;
}

/*****************************************************************************/
/* mstputil_transmit_bpdu: utility function to transmit bpdu                 */
/*****************************************************************************/
void mstputil_transmit_bpdu(PORT_ID port_number, enum STP_BPDU_TYPE type)
{
	MSTP_PORT *mstp_port = mstpdata_get_port(port_number);
	UINT8 *bpdu;
	UINT16 bpdu_size;
	PORT_ID real_port;
	MSTP_BPDU *mstp_bpdu;
    MAC_ADDRESS smac = {0};
    VLAN_ID vlan_id;

	if (mstp_port == NULL)
		return;

    stp_intf_get_mac(port_number, &smac);
	switch (type)
	{
		case CONFIG_BPDU_TYPE:
		case RSTP_BPDU_TYPE:
			mstp_bpdu = mstpdata_get_bpdu();
			bpdu = (UINT8 *) mstp_bpdu;
			bpdu_size = mstputil_get_bpdu_size(mstp_bpdu);
			COPY_MAC(&mstp_bpdu->mac_header.source_address, &smac);
			mstputil_network_order_bpdu(mstp_bpdu);
			break;

		case TCN_BPDU_TYPE:
			bpdu = (UINT8*) &g_stp_tcn_bpdu;
			bpdu_size = sizeof(STP_TCN_BPDU);
			COPY_MAC(&g_stp_tcn_bpdu.mac_header.source_address, &smac);
			break;

		default:
			STP_LOG_ERR("invalid bpdu type %u",type);
			return;
	}

	mstputil_update_stats(port_number, (MSTP_BPDU *) bpdu, false);

    if(stp_pkt_tx_handler(port_number, 0 , (void*) bpdu, bpdu_size, false) == -1)
    {
        STP_LOG_ERR("[BPDU Vlan %u] Send MSTP BPDU Failed Port %d", vlan_id, port_number);
    }

}

/*****************************************************************************/
/* mstputil_clear_timer: the timer value is set to 0                         */
/*****************************************************************************/
void mstputil_clear_timer(UINT16 *timer)
{
	*timer = 0;
	return;
}

/*****************************************************************************/
/* l2_proto_get_first_instance: returns the first protocol index that is set */
/* in the mask                                                               */
/*****************************************************************************/
UINT16 l2_proto_get_first_instance(L2_PROTO_INSTANCE_MASK *mask)
{
    return l2_proto_get_next_instance(mask, L2_PROTO_INDEX_INVALID);
}

/*****************************************************************************/
/* l2_proto_get_next_instance: utility function that gets the next protocol  */
/* index set in the mask                                                     */
/*****************************************************************************/
UINT16 l2_proto_get_next_instance(L2_PROTO_INSTANCE_MASK *mask, UINT16 index)
{
    UINT16 i, j, init_val, start_index, instance;

    start_index = ((index == L2_PROTO_INDEX_INVALID) ? 0 : (index + 1));

    init_val = start_index >> 5;
    for (i = init_val; i < L2_PROTO_INDEX_MASKS; i++)
    {
        if (mask->m[i] == 0)
            continue;

        j = ((i == init_val) ? (start_index & 0x1f) : 0);

        for (; j < 32; j++)
        {
            instance = ((i << 5) + j);
            if (L2_PROTO_INSTANCE_MASK_ISSET(mask, instance))
            {
                return instance;
            }
        }
    }

    return L2_PROTO_INDEX_INVALID;
}

/*****************************************************************************/
/* l2_proto_mask_is_clear: checks if the protocol mask is clear              */
/*****************************************************************************/
bool l2_proto_mask_is_clear(L2_PROTO_INSTANCE_MASK *mask)
{
    UINT16 i;

    for (i = 0; i < L2_PROTO_INDEX_MASKS; i++)
    {
        if (mask->m[i] != 0)
            return false;
    }

    return true;
}

/*****************************************************************************/
/* l2_proto_mask_to_string: converts the input proto instance mask to a      */
/* string for easier readability                                             */
/*****************************************************************************/
bool l2_proto_mask_to_string(L2_PROTO_INSTANCE_MASK *mask, UINT8 *str, UINT32 maxlen)
{
    UINT16 curr_index, start_index, end_index;
    UINT32 len;
    bool flag;

    if (str == NULL)
        return false;

    *str = '\0';

    len = 0;
    start_index = end_index = L2_PROTO_INDEX_INVALID;

    curr_index = l2_proto_get_first_instance(mask);
    
    while (curr_index != L2_PROTO_INDEX_INVALID)
    {
        flag = true;
        if (start_index == L2_PROTO_INDEX_INVALID)
        {
            end_index = start_index = curr_index;
        }
        else if (curr_index == end_index + 1)
        {
            end_index = curr_index;
        }
        else
        {
            flag = false;
            
            if (len >= maxlen)
                return false;

            if (start_index == end_index)
                len += snprintf(str+len, maxlen-len, "%d ", start_index);
            else
                len += snprintf(str+len, maxlen-len, "%d to %d ", start_index, end_index);

            start_index = end_index = L2_PROTO_INDEX_INVALID;
        }
        if (flag)
            curr_index = l2_proto_get_next_instance(mask, curr_index);
    }

    if (start_index != L2_PROTO_INDEX_INVALID)
    {
        if (len >= maxlen)
            return false;

        if (start_index == end_index)
            len += snprintf(str+len, maxlen-len, "%d ", start_index);
        else
            len += snprintf(str+len, maxlen-len, "%d to %d ", start_index, end_index);
    }
    return true;
}

/*****************************************************************************/
/* mstputil_sync_cist_port: sync cist port info to APP DB                    */
/*****************************************************************************/
void mstputil_sync_cist_port(PORT_ID port_number)
{
    STP_MST_PORT_TABLE mstp_intf = {0};
    char * ifname;
    MSTP_CIST_PORT *cist_port;
    MSTP_PORT *mstp_port;

    mstp_port = mstpdata_get_port(port_number);
    if(!mstp_port)
        return;

    cist_port = mstputil_get_cist_port(port_number);
    if (cist_port == NULL)
        return;

    if(!cist_port->modified_fields && !cist_port->co.modified_fields)
        return;

    ifname = stp_intf_get_port_name(port_number);
    if(ifname == NULL)
        return;

    mstp_intf.mst_id = MSTP_MSTID_CIST;

    memset(mstp_intf.if_name, '\0', sizeof(mstp_intf.if_name));
    strncpy(mstp_intf.if_name, ifname, sizeof(mstp_intf.if_name)-1);

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_PORT_ID_BIT))
    {
        mstp_intf.port_id = port_number;
    }
    else
    {
        mstp_intf.port_id = 0xFFFF;
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_PORT_PRIORITY_BIT))
    {
        mstp_intf.port_priority = cist_port->co.portId.priority << 4;
    }
    else
    {
        mstp_intf.port_priority = -1;
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_ROOT_BIT))
    {
        mstputil_bridge_to_string(&cist_port->designatedPriority.root, mstp_intf.designated_root, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_REG_ROOT_BIT))
    {
        mstputil_bridge_to_string(&cist_port->designatedPriority.regionalRoot, mstp_intf.designated_reg_root, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_EX_PATH_COST_BIT))
    {
        mstp_intf.external_cost = cist_port->designatedPriority.extPathCost;
    }
    else
    {
         mstp_intf.external_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_IN_PATH_COST_BIT))
    {
        mstp_intf.designated_cost = cist_port->designatedPriority.intPathCost;
    }
    else
    {
         mstp_intf.designated_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_BRIDGE_BIT))
    {
        mstputil_bridge_to_string(&cist_port->portPriority.designatedId, mstp_intf.designated_bridge, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT))
    {
        if((mstp_port->restrictedRole) && (cist_port->co.state == BLOCKING) && (cist_port->co.role == MSTP_ROLE_ALTERNATE))
            strcpy(mstp_intf.port_state, "ROOT-INC");
        strncpy(mstp_intf.port_state, MSTP_STATE_STRING(cist_port->co.state, port_number), (sizeof(mstp_intf.port_state)-1));

        if (cist_port->co.state == DISABLED)
        {
            mstp_intf.designated_cost = 0;
            mstp_intf.external_cost = 0;
            strncpy(mstp_intf.designated_bridge, "0000.0000.0000.0000", STP_SYNC_BRIDGE_ID_LEN);
            strncpy(mstp_intf.designated_root, "0000.0000.0000.0000", STP_SYNC_BRIDGE_ID_LEN);
        }
    }
    else
    {
        mstp_intf.port_state[0] = '\0';
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_PORT_PATH_COST_BIT))
    {
        mstp_intf.path_cost = cist_port->co.intPortPathCost;
    }
    else
    {
        mstp_intf.path_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(cist_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_PORT_BIT))
    {
        mstp_intf.designated_port = (cist_port->designatedPriority.designatedPort.priority << 12 | cist_port->designatedPriority.designatedPort.number);
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_ROLE_BIT))
    {
        if (cist_port->co.state == DISABLED)
        {
            mstp_intf.port_role = MSTP_ROLE_DISABLED;
        }
        else
        {
            mstp_intf.port_role = cist_port->co.role;
        }
    }
    else
    {
        mstp_intf.port_role = 0xFF;
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_FWD_TRANSITIONS_BIT))
    {
        mstp_intf.forward_transitions  = cist_port->co.forwardTransitions; 
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_BPDU_SENT_BIT))
    {
        mstp_intf.tx_bpdu = mstp_port->stats.tx.mstp_bpdu + 
            mstp_port->stats.tx.rstp_bpdu +
            mstp_port->stats.tx.config_bpdu + 
            mstp_port->stats.tx.tcn_bpdu;
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_BPDU_RECVD_BIT))
    {
        mstp_intf.rx_bpdu = mstp_port->stats.rx.mstp_bpdu +
            mstp_port->stats.rx.rstp_bpdu +
            mstp_port->stats.rx.config_bpdu +
            mstp_port->stats.rx.tcn_bpdu;
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_CLEAR_STATS_BIT))
    {
        mstp_intf.clear_stats = 1;
    }

    if(IS_BIT_SET(cist_port->co.modified_fields, MSTP_PORT_MEMBER_REM_TIME_BIT))
    {
        mstp_intf.rem_time = cist_port->co.rcvdInfoWhile;
    }
    else
    {
        mstp_intf.rem_time = 0xFF;
    }

    cist_port->co.modified_fields = 0;
    cist_port->modified_fields = 0;

    stpsync_update_mst_port_info(&mstp_intf);
}
/*****************************************************************************/
/* mstputil_sync_msti_port: sync msti port info to APP DB                    */
/*****************************************************************************/
void mstputil_sync_msti_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    STP_MST_PORT_TABLE mstp_intf = {0};
    char * ifname;
    MSTP_PORT *mstp_port;
    MSTP_MSTI_PORT *msti_port;

    mstp_port = mstpdata_get_port(port_number);

    msti_port = MSTP_GET_MSTI_PORT(mstp_port, mstp_index);
    if (msti_port == NULL)
        return;

    if(!msti_port->modified_fields && !msti_port->co.modified_fields)
        return;

    ifname = stp_intf_get_port_name(port_number);
    if(ifname == NULL)
        return;

    mstp_intf.mst_id = mstputil_get_mstid(mstp_index);

    memset(mstp_intf.if_name, '\0', sizeof(mstp_intf.if_name));
    strncpy(mstp_intf.if_name, ifname, sizeof(mstp_intf.if_name)-1);

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_PORT_ID_BIT))
    {
        mstp_intf.port_id = port_number;
    }
    else
    {
        mstp_intf.port_id = 0xFFFF;
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_PORT_PRIORITY_BIT))
    {
        mstp_intf.port_priority = msti_port->co.portId.priority << 4;
    }
    else
    {
        mstp_intf.port_priority = -1;
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_REG_ROOT_BIT))
    {
        mstputil_bridge_to_string(&msti_port->designatedPriority.regionalRoot, mstp_intf.designated_root, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_IN_PATH_COST_BIT))
    {
        mstp_intf.designated_cost = msti_port->designatedPriority.intPathCost;
    }
    else
    { 
        mstp_intf.designated_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_BRIDGE_BIT))
    {
        mstputil_bridge_to_string(&msti_port->portPriority.designatedId, mstp_intf.designated_bridge, STP_SYNC_BRIDGE_ID_LEN);
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_STATE_BIT))
    {
        if((mstp_port->restrictedRole) && (msti_port->co.state == BLOCKING) && (msti_port->co.role == MSTP_ROLE_ALTERNATE))
            strcpy(mstp_intf.port_state, "ROOT-INC");
        else
            strncpy(mstp_intf.port_state, MSTP_STATE_STRING(msti_port->co.state, port_number), (sizeof(mstp_intf.port_state)-1));

        if (msti_port->co.state == DISABLED)
        {
            mstp_intf.designated_cost = 0;
            strncpy(mstp_intf.designated_bridge, "0000.0000.0000.0000", STP_SYNC_BRIDGE_ID_LEN);
            strncpy(mstp_intf.designated_root, "0000.0000.0000.0000", STP_SYNC_BRIDGE_ID_LEN);
            strncpy(mstp_intf.designated_reg_root, "0000.0000.0000.0000", STP_SYNC_BRIDGE_ID_LEN);
        }
    }
    else
    {
        mstp_intf.port_state[0] = '\0';
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_PORT_PATH_COST_BIT))
    {
        mstp_intf.path_cost = msti_port->co.intPortPathCost;
    }
    else
    {
        mstp_intf.path_cost = 0xFFFFFFFF;
    }

    if(IS_BIT_SET(msti_port->modified_fields, MSTP_PORT_MEMBER_DESIGN_PORT_BIT))
    {
        mstp_intf.designated_port = (msti_port->designatedPriority.designatedPort.priority << 12 | msti_port->designatedPriority.designatedPort.number);
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_PORT_ROLE_BIT))
    {
        if (msti_port->co.state == DISABLED)
        {
            mstp_intf.port_role = MSTP_ROLE_DISABLED;
        }
        else
        {
            mstp_intf.port_role = msti_port->co.role;
        }
    }
    else
    {
        mstp_intf.port_role = 0xFF;
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_BPDU_SENT_BIT))
    {
        mstp_intf.tx_bpdu = msti_port->co.stats.tx_bpdu;
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_BPDU_RECVD_BIT))
    {
        mstp_intf.rx_bpdu = msti_port->co.stats.rx_bpdu;
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_CLEAR_STATS_BIT))
    {
        mstp_intf.clear_stats = 1;
    }

    if(IS_BIT_SET(msti_port->co.modified_fields, MSTP_PORT_MEMBER_REM_TIME_BIT))
    {
        mstp_intf.rem_time = msti_port->co.rcvdInfoWhile;
    }

    msti_port->modified_fields = 0;
    msti_port->co.modified_fields = 0;

    stpsync_update_mst_port_info(&mstp_intf);
}

/*****************************************************************************/
/* mstputil_sync_mst_port: sync mst port info to APP DB                      */
/*****************************************************************************/
void mstputil_sync_mst_port(MSTP_INDEX mstp_index, PORT_ID port_number)
{
    if (MSTP_IS_CIST_INDEX(mstp_index))
        mstputil_sync_cist_port(port_number);
    else
        mstputil_sync_msti_port(mstp_index, port_number); 

}

/*****************************************************************************/
/* mstputil_sync_mst_instance: sync mst instance  info to APP DB             */
/*****************************************************************************/
void mstputil_sync_mst_instance(MSTP_INDEX mstp_index)
{
    STP_MST_TABLE stp_mst_table = {0};
	char * ifname = NULL;
    MSTP_CIST_BRIDGE *cist_bridge;
    MSTP_MSTI_BRIDGE *msti_bridge;
    MSTP_BRIDGE         *mstp_bridge = NULL;
    MSTP_COMMON_BRIDGE  *cbridge = NULL;
    PORT_ID port_id;
    int ret = 0;

    mstp_bridge = mstpdata_get_bridge();

    if(!mstp_bridge)
        return ;

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        cist_bridge = MSTP_GET_CIST_BRIDGE(mstp_bridge);
        if(cist_bridge)
        {
            if(!cist_bridge->modified_fields)
                return;

            cbridge = &cist_bridge->co;

            stp_mst_table.mst_id = mstputil_get_mstid(mstp_index);

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_VLAN_MASK_SET))
            {
                mask_to_string2( (BITMAP_T *)&cist_bridge->co.vlanmask, stp_mst_table.vlan_mask, 20000);
            }
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_ID_BIT))
            {
                mstputil_bridge_to_string(&cist_bridge->rootPriority.root, stp_mst_table.root_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT))
            {
                mstputil_bridge_to_string(&cist_bridge->rootPriority.regionalRoot, stp_mst_table.reg_root_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT))
            {
                stp_mst_table.root_path_cost = cist_bridge->rootPriority.extPathCost;
            }
            else
            {
                stp_mst_table.root_path_cost = 0xFFFFFFFF;
            }

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_IN_ROOT_PATH_COST_BIT))
            {
                stp_mst_table.in_root_path_cost = cist_bridge->rootPriority.intPathCost;
            }
            else
            {
                stp_mst_table.in_root_path_cost = 0xFFFFFFFF;
            }

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT))
            {
                mstputil_bridge_to_string(&cist_bridge->co.bridgeIdentifier, stp_mst_table.bridge_id, STP_SYNC_BRIDGE_ID_LEN);
                stp_mst_table.bridge_priority = MSTP_DECODE_PRIORITY(cist_bridge->co.bridgeIdentifier.priority);
            }
            else
            {
                stp_mst_table.bridge_priority = 0xFFFF;
            }           
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_MAX_AGE_BIT))
            {
                stp_mst_table.root_max_age = mstp_bridge->cist.rootTimes.maxAge;
            }
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_HELLO_TIME_BIT))
            {
                stp_mst_table.root_hello_time = mstp_bridge->cist.rootTimes.helloTime;
            }
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_FWD_DELAY_BIT))
            {
                stp_mst_table.root_forward_delay = mstp_bridge->cist.rootTimes.fwdDelay; 
            }
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_TX_HOLD_COUNT_BIT))
            {
                stp_mst_table.tx_hold_count = mstp_bridge->txHoldCount;
            }
            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT))
            {
                stp_mst_table.rem_hops = cist_bridge->rootTimes.remainingHops;
            }
            else
            {
                stp_mst_table.rem_hops = 0xFF;
            }

            if(IS_BIT_SET(cist_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT))
            {
                if (mstplib_is_root_bridge(stp_mst_table.mst_id))
                {
                    strcpy(stp_mst_table.root_port, "Root");
                }
                else
                {
                    snprintf(stp_mst_table.root_port, IFNAMSIZ, "%s", stp_intf_get_port_name(cist_bridge->co.rootPortId));
                }
            }

            cist_bridge->modified_fields = 0;

            stpsync_update_mst_info(&stp_mst_table);
        }
    }
    else
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge)
        {
            if(!msti_bridge->modified_fields)
                return;

            cbridge = &msti_bridge->co;

            stp_mst_table.mst_id = mstputil_get_mstid(mstp_index);

            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_VLAN_MASK_SET))
            {
                mask_to_string2( (BITMAP_T *)&msti_bridge->co.vlanmask, stp_mst_table.vlan_mask, 20000);
            }

            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REG_ROOT_ID_BIT))
            {
                mstputil_bridge_to_string(&msti_bridge->rootPriority.regionalRoot, stp_mst_table.root_bridge_id, STP_SYNC_BRIDGE_ID_LEN);
            }

            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PATH_COST_BIT))
            {
                stp_mst_table.root_path_cost = msti_bridge->rootPriority.intPathCost;
            }
            else
            {
                stp_mst_table.root_path_cost = 0xFFFFFFFF;
            }
            stp_mst_table.in_root_path_cost = 0xFFFFFFFF;

            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_BRIDGE_ID_BIT))
            {
                mstputil_bridge_to_string(&msti_bridge->co.bridgeIdentifier, stp_mst_table.bridge_id, STP_SYNC_BRIDGE_ID_LEN);
                stp_mst_table.bridge_priority = MSTP_DECODE_PRIORITY(msti_bridge->co.bridgeIdentifier.priority);
            }
            else
            {
                stp_mst_table.bridge_priority = 0xFFFF;
            }
            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_REM_HOPS_BIT))
            {
                stp_mst_table.rem_hops = msti_bridge->rootTimes.remainingHops;
            }
            else
            {
                stp_mst_table.rem_hops = 0xFF;
            }
            if(IS_BIT_SET(msti_bridge->modified_fields, MSTP_BRIDGE_DATA_MEMBER_ROOT_PORT_BIT))
            {
                if (mstplib_is_root_bridge(stp_mst_table.mst_id))
                {
                    strcpy(stp_mst_table.root_port, "Root");
                }
                else
                {
                    snprintf(stp_mst_table.root_port, IFNAMSIZ, "%s", 
                        stp_intf_get_port_name(msti_bridge->co.rootPortId));
                }

            }

            msti_bridge->modified_fields = 0;
            stpsync_update_mst_info(&stp_mst_table);
        }

    }
}

/*****************************************************************************/
/* mstputil_is_same_bridge_priority: checks bridge priority is same as the   */
/* given value                                                               */
/*****************************************************************************/
bool mstputil_is_same_bridge_priority(MSTP_BRIDGE_IDENTIFIER *id, UINT16 priority)
{
    return (priority == stputil_get_bridge_priority((BRIDGE_IDENTIFIER *)id));
}

/*****************************************************************************/
/* mstputil_timer_sync_db: Timer handler to sync APP DB                      */
/*****************************************************************************/
void mstputil_timer_sync_db(MSTP_INDEX mstp_index)
{
    PORT_ID port_number;
    MSTP_COMMON_BRIDGE  *cbridge = NULL;
    MSTP_BRIDGE *mstp_bridge;
    PORT_MASK *portmask;

    mstputil_sync_mst_instance(mstp_index);

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        mstp_bridge = mstpdata_get_bridge();
        portmask = mstp_bridge->control_mask;
    }
    else
    {
        cbridge = mstputil_get_common_bridge(mstp_index);
        if (!cbridge)
            return;
        portmask = cbridge->portmask;
    }
    for(port_number = port_mask_get_first_port(portmask);
            port_number != BAD_PORT_ID;
            port_number = port_mask_get_next_port(portmask, port_number))
    {
        mstputil_sync_mst_port(mstp_index, port_number);
    }
}

/*****************************************************************************/
/* mstputil_sync_bpdu_counters: Sync BPDU counters to  APP DB                */
/*****************************************************************************/
void mstputil_sync_bpdu_counters(MSTP_INDEX mstp_index, PORT_ID port_number, MSTP_COMMON_PORT *cport)
{
    SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_BPDU_SENT_BIT);
    SET_BIT(cport->modified_fields, MSTP_PORT_MEMBER_BPDU_RECVD_BIT);

    mstputil_sync_mst_port(mstp_index, port_number);
}

/*********************************************************************************/
/* mstputil_timer_sync_bpdu_counters:Timer handler to sync bodu counters to APP DB*/
/**********************************************************************************/
void mstputil_timer_sync_bpdu_counters()
{
	PORT_ID port_number;
    MSTP_BRIDGE *mstp_bridge;
    MSTP_PORT *mstp_port;
    MSTP_INDEX  mstp_index;
    MSTP_COMMON_PORT *cport;

    mstp_bridge = mstpdata_get_bridge();

    port_number = port_mask_get_first_port(mstp_bridge->enable_mask);
    while (port_number != BAD_PORT_ID)
    {
        mstp_port = mstpdata_get_port(port_number);
        if (mstp_port)
        {
            for (mstp_index = MSTP_INDEX_MIN; mstp_index <= MSTP_INDEX_CIST; mstp_index++)
            {
                if (mstp_index != MSTP_INDEX_CIST)
                {
                    if (mstp_bridge->msti[mstp_index] == NULL)
                        continue;
                }

                cport = mstputil_get_common_port(mstp_index, mstp_port);

                if (!cport)
                {
                    continue;
                }

                if(mstp_index == MSTP_INDEX_CIST && 
                        (mstp_port->stats.rx.update_flag|| mstp_port->stats.tx.update_flag))
                {
                    mstputil_sync_bpdu_counters(mstp_index, port_number, cport);
                    mstp_port->stats.tx.update_flag = false;
                    mstp_port->stats.rx.update_flag = false;
                }
                else if(cport->stats.update_flag)
                {
                    mstputil_sync_bpdu_counters(mstp_index, port_number, cport);
                    cport->stats.update_flag = false;
                }

            }
        }
        port_number = port_mask_get_next_port(mstp_bridge->enable_mask, port_number);
    }
}

/*********************************************************************************/
/* mstputil_is_member_port_of_instance:Checks port is a member of instance       */
/**********************************************************************************/
bool mstputil_is_member_port_of_instance(MSTP_INDEX mstp_index, PORT_ID port_id)
{
    MSTP_MSTI_BRIDGE *msti_bridge;
    PORT_MASK *port_mask = NULL;
    MSTP_BRIDGE  *mstp_bridge = NULL;

    mstp_bridge = mstpdata_get_bridge();

    if (MSTP_IS_CIST_INDEX(mstp_index))
    {
        port_mask = mstp_bridge->control_mask;
    }
    else
    {
        msti_bridge = MSTP_GET_MSTI_BRIDGE(mstp_bridge, mstp_index);
        if (msti_bridge)
            port_mask = msti_bridge->co.portmask;
    }

    if(port_mask && is_member(port_mask, port_id))
        return true;
    
    return false;
}
/**********************************************************************/
/* mstputil_update_mst_stats:  Update MSTP rx/tx BPDU stats           */
/**********************************************************************/
void mstputil_update_mst_stats(PORT_ID port_number, MSTP_MSTI_PORT *msti_port, bool rx)
{
    MSTP_PORT *mstp_port = mstpdata_get_port(port_number);

    if (mstp_port == NULL)
        return;

    msti_port->co.stats.update_flag = true;
    if(rx)
        msti_port->co.stats.rx_bpdu++;
    else
        msti_port->co.stats.tx_bpdu++;
}
