/*
 * Copyright 2025 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */

#include "stp_inc.h"

extern UINT8 g_stp_base_mac[L2_ETH_ADD_LEN];


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
