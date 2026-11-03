/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unmapping the last VLAN from an instance reaches the daemon as an instance
 * row with no VLANs. That row must take the instance down, not be ignored.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;
const uint16_t kVlans[] = {kVlan};
const uint16_t kNoVlans[] = {0};

class MstUnmapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
        stptest_mstp_enable();
        stptest_mstp_global_config();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
        stptest_mstp_vlan_ports(kVlan, 1);
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

    static size_t portRows(const char *prefix)
    {
        size_t n = 0;
        for (const auto &key : db().keys(stptest::kStpMstPortTable))
            if (key.rfind(prefix, 0) == 0)
                n++;
        return n;
    }

    static bool instancePublished(uint16_t mst_id)
    {
        return db().keys(stptest::kStpMstInstTable).count(std::to_string(mst_id)) == 1;
    }
};

// The row the CLI writes after `vlan del` of the last VLAN: the instance keeps
// its priority and carries no VLANs. The daemon must stop running it.
TEST_F(MstUnmapTest, UnmappingTheLastVlanTakesTheInstanceDown)
{
    stptest_mstp_add_instance(kMstId, kVlans, 1);
    stptest_tick(700);
    ASSERT_TRUE(instancePublished(kMstId));
    ASSERT_EQ(portRows("1:"), STPTEST_NUM_PORTS);

    stptest_mstp_add_instance(kMstId, kNoVlans, 0);
    stptest_tick(700);

    EXPECT_FALSE(instancePublished(kMstId))
        << "instance 1 is still published after its last VLAN was unmapped";
    EXPECT_EQ(portRows("1:"), 0u);
    EXPECT_EQ(portRows("0:"), STPTEST_NUM_PORTS) << "the VLAN did not return to the CIST";
}

// A priority set before any VLAN is mapped must still create nothing.
TEST_F(MstUnmapTest, PriorityBeforeAnyVlanCreatesNoInstance)
{
    stptest_mstp_add_instance(kMstId, kNoVlans, 0);
    stptest_tick(700);

    EXPECT_FALSE(instancePublished(kMstId));
    EXPECT_EQ(portRows("1:"), 0u);
}

}  // namespace
