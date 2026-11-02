/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * An instance publishes its whole state once, then only what changes. The
 * pending bitmap that makes the first publish complete has to drain for
 * every kind of instance, or the row is rewritten every second for the life
 * of the daemon.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;

// 100ms a tick. Long enough for the CIST to leave discarding on every
// port with the default forward delay.
const unsigned int kConvergeTicks = 700;
const unsigned int kSteadyTicks = 300;

class MstpInitialSyncTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

    // The CIST keeps no VLAN of its own here: the one VLAN maps to the MSTI.
    static void configureMstp()
    {
        const uint16_t vlans[] = {kVlan};

        stptest_mstp_enable();
        stptest_mstp_global_config();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
        stptest_mstp_vlan_ports(kVlan, 1);
        stptest_mstp_add_instance(kMstId, vlans, 1);
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_inst_port_config(stptest_port_name(i), kMstId);
    }
};

// Both rows are written while the bridge comes up.
TEST_F(MstpInitialSyncTest, EveryInstancePublishesWhileConverging)
{
    configureMstp();
    stptest_tick(kConvergeTicks);

    const std::set<std::string> &rows = db().keys(stptest::kStpMstInstTable);
    EXPECT_TRUE(rows.count("0")) << "no CIST row";
    EXPECT_TRUE(rows.count("1")) << "no MSTI row";
}

// Once converged, neither the MSTI nor a CIST without VLANs of its own has
// anything left to publish, so the rows must go quiet. The regression this
// guards: fields an MSTI never writes were counted as still unpublished,
// and the MSTI row was rewritten every second forever.
TEST_F(MstpInitialSyncTest, ConvergedInstancesStopRewritingTheirRows)
{
    configureMstp();
    stptest_tick(kConvergeTicks);

    db().resetWrites();
    stptest_tick(kSteadyTicks);

    EXPECT_EQ(db().writes(stptest::kStpMstInstTable), 0u)
        << "instance rows were rewritten " << db().writes(stptest::kStpMstInstTable)
        << " times over " << (kSteadyTicks / 10) << "s with nothing changing";
}

} // namespace
