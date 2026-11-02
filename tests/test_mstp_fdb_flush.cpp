/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * MSTP asks orchagent to flush the FDB through STP_INST_PORT_FLUSH_TABLE.
 * That row belongs to a topology change. A converged bridge with a stable
 * topology must stop writing it, because every row is a hardware FDB flush
 * and a steady stream of them keeps the switch in permanent flood mode.
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

class MstpFdbFlushTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

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

// A converged bridge that nothing has happened to must not flush at all.
// The regression this guards: the per-second timer called mstp_flush()
// unconditionally, so every port emitted a flush every second forever and
// no MAC could survive long enough to be used.
TEST_F(MstpFdbFlushTest, ConvergedBridgeStopsFlushingTheFdb)
{
    configureMstp();
    stptest_tick(kConvergeTicks);

    db().resetWrites();
    stptest_tick(kSteadyTicks);

    EXPECT_EQ(db().writes(stptest::kStpInstPortFlushTable), 0u)
        << "a stable topology asked orchagent to flush the FDB "
        << db().writes(stptest::kStpInstPortFlushTable) << " times over "
        << (kSteadyTicks / 10) << "s; each one wipes the hardware FDB, so "
           "learning never sticks and the switch floods everything";
}

// Ports keep being driven, so the silence above is not the daemon having
// stopped running its per-port timers.
TEST_F(MstpFdbFlushTest, ConvergedBridgeStillPublishesPortState)
{
    configureMstp();
    stptest_tick(kConvergeTicks);

    db().resetWrites();
    stptest_tick(kSteadyTicks);

    EXPECT_GT(db().writes(stptest::kStpMstPortTable), 0u)
        << "no MST port rows were written at all, so the flush count above "
           "proves nothing";
}

} // namespace
