/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * PVST tags a BPDU or not based on the port's tagging mode in the VLAN. A
 * tagged BPDU sent to an untagged-only member port is dropped by the peer,
 * so getting this wrong stops the two bridges converging at all.
 */

#include <gtest/gtest.h>

#include "stpd_driver.h"

namespace {

const uint16_t kVlan = 100;
const uint16_t kInst = 0;

const int kUntagged = 0;
const int kTagged = 1;

class BpduTaggingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        stptest_pvst_enable();
        stptest_pvst_add_empty_vlan(kVlan, kInst);
    }

    static const char *port() { return stptest_port_name(0); }

    // Enrol the port exactly once, with a single consistent tagging mode,
    // so a later re-announcement is the only thing under test.
    static void enrol(int mode)
    {
        stptest_pvst_port_config_tagging(port(), kInst, 1, 0, mode);
    }

    // What would actually go on the wire.
    static bool sendsUntagged()
    {
        return stptest_pvst_port_is_untagged(kVlan, port()) != 0;
    }
};

TEST_F(BpduTaggingTest, UntaggedMemberSendsUntaggedBpdus)
{
    enrol(kUntagged);
    EXPECT_TRUE(sendsUntagged());
}

TEST_F(BpduTaggingTest, TaggedMemberSendsTaggedBpdus)
{
    enrol(kTagged);
    EXPECT_FALSE(sendsUntagged());
}

// The ordinary case: the port is already under STP control when the mode
// arrives, after a membership bounce or an STP enable.
TEST_F(BpduTaggingTest, TaggingIsReconciledWhenAnUntaggedMemberIsReannounced)
{
    enrol(kTagged);
    ASSERT_FALSE(sendsUntagged());

    stptest_pvst_vlan_member_tagging(kVlan, kInst, port(), kUntagged);

    EXPECT_TRUE(sendsUntagged())
        << "port was re-announced as untagged but BPDUs still go out tagged; "
           "the peer will drop them and no election can complete";
}

// The reverse direction has to work too.
TEST_F(BpduTaggingTest, TaggingIsReconciledWhenATaggedMemberIsReannounced)
{
    enrol(kUntagged);
    ASSERT_TRUE(sendsUntagged());

    stptest_pvst_vlan_member_tagging(kVlan, kInst, port(), kTagged);

    EXPECT_FALSE(sendsUntagged())
        << "port was re-announced as tagged but BPDUs still go out untagged";
}

// Re-announcing the same mode must change nothing.
TEST_F(BpduTaggingTest, ReannouncingTheSameModeIsIdempotent)
{
    enrol(kUntagged);
    for (int i = 0; i < 3; i++)
        stptest_pvst_vlan_member_tagging(kVlan, kInst, port(), kUntagged);
    EXPECT_TRUE(sendsUntagged());

    stptest_reset_protocol();
    stptest_pvst_enable();
    stptest_pvst_add_empty_vlan(kVlan, kInst);
    enrol(kTagged);
    for (int i = 0; i < 3; i++)
        stptest_pvst_vlan_member_tagging(kVlan, kInst, port(), kTagged);
    EXPECT_FALSE(sendsUntagged());
}

}  // namespace
