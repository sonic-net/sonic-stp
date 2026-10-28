/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * A global STP disable, and the mode switch built out of one, must leave
 * no rows behind in APPL_DB.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlanA = 100;
const uint16_t kVlanB = 200;
const uint16_t kInstA = 0;
const uint16_t kInstB = 1;
const uint16_t kMstId = 1;

// APPL_DB rows are written by the periodic sync, not by config handling.
// PVST spreads instances over 5 ticks, MSTP syncs once per 10.
const unsigned int kSyncTicks = 30;

class BridgeDisableTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

    static std::string dump()
    {
        return "APPL_DB contents:\n" + db().dump();
    }

    // Rows the daemon is responsible for removing.
    static size_t totalRows()
    {
        size_t n = 0;
        const std::vector<std::string> &tables = FakeAppDb::ownedTables();
        for (size_t i = 0; i < tables.size(); i++)
            n += db().size(tables[i]);
        return n;
    }

    // PVST on two VLANs across every port.
    static void configurePvst()
    {
        stptest_pvst_enable();
        stptest_pvst_add_vlan(kVlanA, kInstA);
        stptest_pvst_add_vlan(kVlanB, kInstB);
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
        {
            // portfast off, so the port lands in STP_PORT_TABLE.
            stptest_pvst_port_config(stptest_port_name(i), kInstA, 1, 0);
            stptest_pvst_vlan_member(kVlanA, kInstA, stptest_port_name(i));
            stptest_pvst_vlan_member(kVlanB, kInstB, stptest_port_name(i));
        }
        stptest_tick(kSyncTicks);
    }

    // MSTP with one MSTI holding two VLANs across every port.
    static void configureMstp()
    {
        const uint16_t vlans[] = {kVlanA, kVlanB};

        stptest_mstp_enable();
        stptest_mstp_global_config();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
        stptest_mstp_vlan_ports(kVlanA, 1);
        stptest_mstp_vlan_ports(kVlanB, 1);
        stptest_mstp_add_instance(kMstId, vlans, 2);
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_inst_port_config(stptest_port_name(i), kMstId);
        stptest_tick(kSyncTicks);
    }
};

// Without this, the teardown tests could pass for the wrong reason.
TEST_F(BridgeDisableTest, PvstConfigPopulatesAppDb)
{
    configurePvst();

    EXPECT_FALSE(db().empty(stptest::kStpVlanTable)) << dump();
    EXPECT_FALSE(db().empty(stptest::kStpVlanPortTable)) << dump();
    EXPECT_FALSE(db().empty(stptest::kStpPortTable)) << dump();
    EXPECT_FALSE(db().empty(stptest::kStpVlanInstanceTable)) << dump();
}

TEST_F(BridgeDisableTest, MstpConfigPopulatesAppDb)
{
    configureMstp();

    EXPECT_FALSE(db().empty(stptest::kStpMstInstTable)) << dump();
    EXPECT_FALSE(db().empty(stptest::kStpMstPortTable)) << dump();
    EXPECT_FALSE(db().empty(stptest::kStpPortTable)) << dump();
}

// STP_DEL_COMMAND on the PVST bridge drains every protocol table.
TEST_F(BridgeDisableTest, PvstDisableClearsAllAppDbTables)
{
    configurePvst();
    ASSERT_GT(totalRows(), 0u) << dump();

    stptest_pvst_disable();

    EXPECT_FALSE(stptest_protocol_enabled());
    const std::vector<std::string> &tables = FakeAppDb::ownedTables();
    for (size_t i = 0; i < tables.size(); i++)
        EXPECT_TRUE(db().empty(tables[i]))
            << tables[i] << " still holds " << db().size(tables[i]) << " row(s)\n" << dump();
}

// Same for MSTP, whose teardown is a separate function.
TEST_F(BridgeDisableTest, MstpDisableClearsAllAppDbTables)
{
    configureMstp();
    ASSERT_GT(totalRows(), 0u) << dump();

    stptest_mstp_disable();

    EXPECT_FALSE(stptest_protocol_enabled());
    const std::vector<std::string> &tables = FakeAppDb::ownedTables();
    for (size_t i = 0; i < tables.size(); i++)
        EXPECT_TRUE(db().empty(tables[i]))
            << tables[i] << " still holds " << db().size(tables[i]) << " row(s)\n" << dump();
}

// STP_PORT_TABLE used to be drained only by the per-port CONFIG_DB DELs
// the CLI no longer sends. Called out so a regression names itself.
TEST_F(BridgeDisableTest, PvstDisableClearsStpPortTable)
{
    configurePvst();
    ASSERT_FALSE(db().empty(stptest::kStpPortTable)) << dump();

    stptest_pvst_disable();

    EXPECT_TRUE(db().empty(stptest::kStpPortTable)) << dump();
}

TEST_F(BridgeDisableTest, MstpDisableClearsStpPortTable)
{
    configureMstp();
    ASSERT_FALSE(db().empty(stptest::kStpPortTable)) << dump();

    stptest_mstp_disable();

    EXPECT_TRUE(db().empty(stptest::kStpPortTable)) << dump();
}

// No row written by the old mode may outlive a mode switch.
TEST_F(BridgeDisableTest, PvstToMstpSwitchLeavesNoPvstRows)
{
    configurePvst();
    ASSERT_FALSE(db().empty(stptest::kStpVlanTable)) << dump();
    ASSERT_FALSE(db().empty(stptest::kStpVlanPortTable)) << dump();

    stptest_pvst_disable();
    configureMstp();

    // PVST-only tables must stay empty across the switch.
    EXPECT_TRUE(db().empty(stptest::kStpVlanTable)) << dump();
    EXPECT_TRUE(db().empty(stptest::kStpVlanPortTable)) << dump();

    stptest_mstp_disable();
    const std::vector<std::string> &tables = FakeAppDb::ownedTables();
    for (size_t i = 0; i < tables.size(); i++)
        EXPECT_TRUE(db().empty(tables[i]))
            << tables[i] << " still holds " << db().size(tables[i]) << " row(s)\n" << dump();
}

TEST_F(BridgeDisableTest, MstpToPvstSwitchLeavesNoMstpRows)
{
    configureMstp();
    ASSERT_FALSE(db().empty(stptest::kStpMstInstTable)) << dump();
    ASSERT_FALSE(db().empty(stptest::kStpMstPortTable)) << dump();

    stptest_mstp_disable();
    configurePvst();

    // MSTP-only tables must stay empty across the switch.
    EXPECT_TRUE(db().empty(stptest::kStpMstInstTable)) << dump();
    EXPECT_TRUE(db().empty(stptest::kStpMstPortTable)) << dump();

    stptest_pvst_disable();
    const std::vector<std::string> &tables = FakeAppDb::ownedTables();
    for (size_t i = 0; i < tables.size(); i++)
        EXPECT_TRUE(db().empty(tables[i]))
            << tables[i] << " still holds " << db().size(tables[i]) << " row(s)\n" << dump();
}

// A disable with nothing configured must be a no-op, not a crash.
TEST_F(BridgeDisableTest, DisableWithoutConfigIsHarmless)
{
    stptest_pvst_disable();
    stptest_mstp_disable();

    EXPECT_EQ(0u, totalRows()) << dump();
}

// Back-to-back disables stay idempotent.
TEST_F(BridgeDisableTest, RepeatedPvstDisableIsIdempotent)
{
    configurePvst();
    stptest_pvst_disable();
    stptest_pvst_disable();

    EXPECT_EQ(0u, totalRows()) << dump();
}

} // namespace
