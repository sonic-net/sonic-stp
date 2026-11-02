/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The MST configuration digest is HMAC-MD5 over the VLAN-to-instance table
 * with a fixed key (802.1Q 13.7). Two bridges share a region only when
 * their digests match, so the bytes have to be the standard ones whatever
 * crypto provider the image ships.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>

#include "mstp_md5.h"
#include "stpd_driver.h"

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;

// HMAC-MD5 with the 802.1Q key over an all-CIST table, the value every
// vendor advertises for the default mapping.
const char kDefaultDigest[] = "ac36177f50283cd4b83821d8ab26de62";
// The same with VLAN 100 moved to instance 1 (table entry 100 = 0x0001).
const char kVlan100InInstance1Digest[] = "342cd2b5032c494578d4d70aab2782d5";

std::string hex(const uint8_t *bytes, size_t len)
{
    static const char digits[] = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < len; i++)
    {
        s += digits[bytes[i] >> 4];
        s += digits[bytes[i] & 0xf];
    }
    return s;
}

std::string md5_hex(const std::string &msg)
{
    uint8_t d[MSTP_MD5_DIGEST_SIZE];
    mstp_md5(reinterpret_cast<const uint8_t *>(msg.data()), msg.size(), d);
    return hex(d, sizeof(d));
}

// The daemon's own MD5 against the RFC 1321 test suite.
TEST(MstpMd5, Rfc1321Vectors)
{
    EXPECT_EQ("d41d8cd98f00b204e9800998ecf8427e", md5_hex(""));
    EXPECT_EQ("0cc175b9c0f1b6a831c399e269772661", md5_hex("a"));
    EXPECT_EQ("900150983cd24fb0d6963f7d28e17f72", md5_hex("abc"));
    EXPECT_EQ("f96b697d7cb7938d525a2f31aaf161d0", md5_hex("message digest"));
    EXPECT_EQ("c3fcd3d76192e4007dfb496cca67e13b", md5_hex("abcdefghijklmnopqrstuvwxyz"));
    EXPECT_EQ("d174ab98d277d9f5a5611c2c9f419d9f",
              md5_hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));
    EXPECT_EQ("57edf4a22be3c955ac49da2e2107b67a",
              md5_hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890"));
}

// Feeding the input in pieces that straddle the 64-byte block boundary gives
// the same digest as one call.
TEST(MstpMd5, SplitUpdatesMatchOneCall)
{
    const std::string msg(200, 'x');
    uint8_t whole[MSTP_MD5_DIGEST_SIZE], pieces[MSTP_MD5_DIGEST_SIZE];
    const uint8_t *p = reinterpret_cast<const uint8_t *>(msg.data());

    mstp_md5(p, msg.size(), whole);

    MSTP_MD5_CTX ctx;
    mstp_md5_init(&ctx);
    mstp_md5_update(&ctx, p, 1);
    mstp_md5_update(&ctx, p + 1, 62);
    mstp_md5_update(&ctx, p + 63, 2);
    mstp_md5_update(&ctx, p + 65, 135);
    mstp_md5_final(&ctx, pieces);

    EXPECT_EQ(hex(whole, sizeof(whole)), hex(pieces, sizeof(pieces)));
}

// HMAC-MD5 against RFC 2202 test cases 1 and 2.
TEST(MstpMd5, Rfc2202HmacVectors)
{
    uint8_t d[MSTP_MD5_DIGEST_SIZE];
    uint8_t key1[16];
    memset(key1, 0x0b, sizeof(key1));
    const std::string data1 = "Hi There";
    mstp_hmac_md5(key1, sizeof(key1),
                  reinterpret_cast<const uint8_t *>(data1.data()), data1.size(), d);
    EXPECT_EQ("9294727a3638bb1c13f48ef8158bfc9d", hex(d, sizeof(d)));

    const std::string key2 = "Jefe";
    const std::string data2 = "what do ya want for nothing?";
    mstp_hmac_md5(reinterpret_cast<const uint8_t *>(key2.data()), key2.size(),
                  reinterpret_cast<const uint8_t *>(data2.data()), data2.size(), d);
    EXPECT_EQ("750c783e6ab0b503eaa86e310a5db738", hex(d, sizeof(d)));
}

class MstpConfigDigestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        stptest_mstp_enable();
        stptest_mstp_global_config();
    }

    static std::string digest()
    {
        uint8_t d[16];
        stptest_mstp_config_digest(d);
        return hex(d, sizeof(d));
    }
};

TEST_F(MstpConfigDigestTest, DefaultMappingGivesTheStandardDigest)
{
    EXPECT_EQ(kDefaultDigest, digest());
}

TEST_F(MstpConfigDigestTest, MappingAVlanChangesTheDigest)
{
    const uint16_t vlans[] = {kVlan};

    stptest_mstp_add_instance(kMstId, vlans, 1);
    EXPECT_EQ(kVlan100InInstance1Digest, digest());
}

}  // namespace
