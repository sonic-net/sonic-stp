/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Configuration can reach the daemon before the feature is enabled, because
 * stpmgrd replays CONFIG_DB on connect and the rows drain in table order.
 * The same configuration must produce the same bridge either way.
 */

#include <gtest/gtest.h>

#include <cstdio>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;
const uint16_t kVlans[] = {kVlan};

class ConfigBeforeEnableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

    static void ports()
    {
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
    }
    static void members() { stptest_mstp_vlan_ports(kVlan, 1); }
    static void instance() { stptest_mstp_add_instance(kMstId, kVlans, 1); }
    static void enable()
    {
        stptest_mstp_enable();
        stptest_mstp_global_config();
    }

    // Port rows the daemon published for one instance.
    static size_t portRows(const char *prefix)
    {
        size_t n = 0;
        for (const auto &key : db().keys(stptest::kStpMstPortTable))
            if (key.rfind(prefix, 0) == 0)
                n++;
        return n;
    }
};

// Enable first: the ordering the daemon is usually driven in.
TEST_F(ConfigBeforeEnableTest, EnableFirstEnrolsEveryPortInTheCist)
{
    enable();
    ports();
    members();
    instance();
    stptest_tick(700);

    EXPECT_EQ(portRows("0:"), STPTEST_NUM_PORTS);
    EXPECT_EQ(portRows("1:"), STPTEST_NUM_PORTS);
}

// Same configuration, applied before the feature is enabled. Every port must
// still reach the CIST: a bridge with no CIST ports runs no common tree.
TEST_F(ConfigBeforeEnableTest, ConfigBeforeEnableStillEnrolsEveryPortInTheCist)
{
    ports();
    members();
    instance();
    enable();
    stptest_tick(700);

    EXPECT_EQ(portRows("0:"), STPTEST_NUM_PORTS)
        << "no port enrolled in the CIST when the configuration arrived "
           "before the feature was enabled";
    EXPECT_EQ(portRows("1:"), STPTEST_NUM_PORTS);
}

// Narrowing which part of the configuration causes it: ports and VLAN
// members before enable are fine, so the instance row is the trigger.
TEST_F(ConfigBeforeEnableTest, PortsAndMembersBeforeEnableAreFine)
{
    ports();
    members();
    enable();
    instance();
    stptest_tick(700);

    EXPECT_EQ(portRows("0:"), STPTEST_NUM_PORTS);
    EXPECT_EQ(portRows("1:"), STPTEST_NUM_PORTS);
}

}  // namespace
