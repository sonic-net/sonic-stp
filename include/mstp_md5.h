/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * MD5 (RFC 1321) for the MST configuration digest. The digest is a protocol
 * constant that every bridge in a region must compute byte for byte the same
 * way, so it is built here rather than through a crypto library whose
 * provider configuration may refuse MD5.
 */

#ifndef __MSTP_MD5_H__
#define __MSTP_MD5_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSTP_MD5_BLOCK_SIZE  64
#define MSTP_MD5_DIGEST_SIZE 16

typedef struct
{
	uint32_t state[4];
	uint64_t length;	/* bytes consumed so far */
	uint8_t  buffer[MSTP_MD5_BLOCK_SIZE];
} MSTP_MD5_CTX;

void mstp_md5_init(MSTP_MD5_CTX *ctx);
void mstp_md5_update(MSTP_MD5_CTX *ctx, const uint8_t *data, size_t len);
void mstp_md5_final(MSTP_MD5_CTX *ctx, uint8_t out[MSTP_MD5_DIGEST_SIZE]);

/* Digest of one buffer. */
void mstp_md5(const uint8_t *data, size_t len, uint8_t out[MSTP_MD5_DIGEST_SIZE]);

/* HMAC-MD5 (RFC 2104) with a key of at most one block. */
void mstp_hmac_md5(const uint8_t *key, size_t key_len,
		   const uint8_t *data, size_t data_len,
		   uint8_t out[MSTP_MD5_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __MSTP_MD5_H__ */
