/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * MD5 as specified in RFC 1321, and HMAC-MD5 as specified in RFC 2104,
 * for the MST configuration digest (IEEE 802.1Q 13.7).
 */

#include <string.h>

#include "mstp_md5.h"

/* Per-step rotation amounts, four per round. */
static const uint8_t md5_shift[64] = {
	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
	5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
	4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
	6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
};

/* floor(2^32 * abs(sin(i + 1))) for i in 0..63 */
static const uint32_t md5_k[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

static uint32_t rotl32(uint32_t x, unsigned n)
{
	return (x << n) | (x >> (32 - n));
}

static uint32_t load_le32(const uint8_t *p)
{
	return (uint32_t) p[0] | ((uint32_t) p[1] << 8) |
	       ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}

static void store_le32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t) v;
	p[1] = (uint8_t) (v >> 8);
	p[2] = (uint8_t) (v >> 16);
	p[3] = (uint8_t) (v >> 24);
}

/* One 64-byte block through the four rounds. */
static void md5_transform(uint32_t state[4], const uint8_t block[MSTP_MD5_BLOCK_SIZE])
{
	uint32_t m[16];
	uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
	unsigned i;

	for (i = 0; i < 16; i++)
		m[i] = load_le32(block + 4 * i);

	for (i = 0; i < 64; i++)
	{
		uint32_t f;
		unsigned g;

		if (i < 16)
		{
			f = (b & c) | (~b & d);
			g = i;
		}
		else if (i < 32)
		{
			f = (d & b) | (~d & c);
			g = (5 * i + 1) & 15;
		}
		else if (i < 48)
		{
			f = b ^ c ^ d;
			g = (3 * i + 5) & 15;
		}
		else
		{
			f = c ^ (b | ~d);
			g = (7 * i) & 15;
		}

		f = f + a + md5_k[i] + m[g];
		a = d;
		d = c;
		c = b;
		b = b + rotl32(f, md5_shift[i]);
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}

void mstp_md5_init(MSTP_MD5_CTX *ctx)
{
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
	ctx->length = 0;
}

void mstp_md5_update(MSTP_MD5_CTX *ctx, const uint8_t *data, size_t len)
{
	size_t have = (size_t) (ctx->length % MSTP_MD5_BLOCK_SIZE);

	ctx->length += len;

	if (have)
	{
		size_t need = MSTP_MD5_BLOCK_SIZE - have;

		if (len < need)
		{
			memcpy(ctx->buffer + have, data, len);
			return;
		}
		memcpy(ctx->buffer + have, data, need);
		md5_transform(ctx->state, ctx->buffer);
		data += need;
		len -= need;
	}

	while (len >= MSTP_MD5_BLOCK_SIZE)
	{
		md5_transform(ctx->state, data);
		data += MSTP_MD5_BLOCK_SIZE;
		len -= MSTP_MD5_BLOCK_SIZE;
	}

	if (len)
		memcpy(ctx->buffer, data, len);
}

void mstp_md5_final(MSTP_MD5_CTX *ctx, uint8_t out[MSTP_MD5_DIGEST_SIZE])
{
	static const uint8_t pad[MSTP_MD5_BLOCK_SIZE] = { 0x80 };
	uint64_t bits = ctx->length * 8;
	size_t have = (size_t) (ctx->length % MSTP_MD5_BLOCK_SIZE);
	size_t padlen = (have < 56) ? (56 - have) : (120 - have);
	uint8_t length_le[8];
	unsigned i;

	/* One 0x80, zeros to 56 mod 64, then the bit length little-endian. */
	mstp_md5_update(ctx, pad, padlen);
	for (i = 0; i < 8; i++)
		length_le[i] = (uint8_t) (bits >> (8 * i));
	mstp_md5_update(ctx, length_le, sizeof(length_le));

	for (i = 0; i < 4; i++)
		store_le32(out + 4 * i, ctx->state[i]);

	memset(ctx, 0, sizeof(*ctx));
}

void mstp_md5(const uint8_t *data, size_t len, uint8_t out[MSTP_MD5_DIGEST_SIZE])
{
	MSTP_MD5_CTX ctx;

	mstp_md5_init(&ctx);
	mstp_md5_update(&ctx, data, len);
	mstp_md5_final(&ctx, out);
}

void mstp_hmac_md5(const uint8_t *key, size_t key_len,
		   const uint8_t *data, size_t data_len,
		   uint8_t out[MSTP_MD5_DIGEST_SIZE])
{
	uint8_t k_ipad[MSTP_MD5_BLOCK_SIZE];
	uint8_t k_opad[MSTP_MD5_BLOCK_SIZE];
	uint8_t inner[MSTP_MD5_DIGEST_SIZE];
	MSTP_MD5_CTX ctx;
	size_t i;

	/* A key longer than one block is replaced by its digest (RFC 2104). */
	if (key_len > MSTP_MD5_BLOCK_SIZE)
	{
		mstp_md5(key, key_len, inner);
		key = inner;
		key_len = MSTP_MD5_DIGEST_SIZE;
	}

	memset(k_ipad, 0x36, sizeof(k_ipad));
	memset(k_opad, 0x5c, sizeof(k_opad));
	for (i = 0; i < key_len; i++)
	{
		k_ipad[i] ^= key[i];
		k_opad[i] ^= key[i];
	}

	mstp_md5_init(&ctx);
	mstp_md5_update(&ctx, k_ipad, sizeof(k_ipad));
	mstp_md5_update(&ctx, data, data_len);
	mstp_md5_final(&ctx, inner);

	mstp_md5_init(&ctx);
	mstp_md5_update(&ctx, k_opad, sizeof(k_opad));
	mstp_md5_update(&ctx, inner, sizeof(inner));
	mstp_md5_final(&ctx, out);
}
