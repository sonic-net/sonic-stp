/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * stpmgrd sends one instance record per STP_MST_INST_CONFIG message, with no
 * count in front of it. These check that the priority in that record is the
 * one the daemon applies.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;
const uint16_t kVlans[] = {kVlan};
const int kDefaultPriority = 32768;

class MstInstanceConfigTest : public ::testing::Test
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
};

TEST_F(MstInstanceConfigTest, ACistPriorityReachesTheBridge)
{
    stptest_mstp_instance_msg(0, 1, 4096, nullptr, 0);

    EXPECT_EQ(stptest_mstp_bridge_priority(0), 4096);
}

// A DEL carries priority 0. The CIST goes back to the default instead.
TEST_F(MstInstanceConfigTest, ACistDeleteRestoresTheDefaultPriority)
{
    stptest_mstp_instance_msg(0, 1, 4096, nullptr, 0);
    ASSERT_EQ(stptest_mstp_bridge_priority(0), 4096);

    stptest_mstp_instance_msg(0, 0, 0, nullptr, 0);

    EXPECT_EQ(stptest_mstp_bridge_priority(0), kDefaultPriority);
}

TEST_F(MstInstanceConfigTest, AnInstancePriorityReachesTheInstance)
{
    stptest_mstp_instance_msg(kMstId, 1, 8192, kVlans, 1);

    EXPECT_EQ(stptest_mstp_instance_vlan_count(kMstId), 1u);
    EXPECT_EQ(stptest_mstp_bridge_priority(kMstId), 8192);
}

}  // namespace
