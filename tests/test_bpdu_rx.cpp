/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Received BPDUs drive the protocol, and until now nothing could hand one to
 * the daemon without a testbed. These feed frames straight into the receive
 * path and read the daemon's own view of the tree back.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kInst = 0;
// Priority 0 in the top nibble; the VLAN goes in the low twelve bits.
const uint16_t kSuperiorPriority = 0x0000;
const uint16_t kDefaultPriority = 0x8000;
const uint8_t kLowMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t kHighMac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xfe};
// Ten ticks are a second of protocol time.
const unsigned int kSecond = 10;

class BpduRxTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
        stptest_pvst_enable();
        stptest_pvst_add_vlan(kVlan, kInst);
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_pvst_port_config_unguarded(stptest_port_name(i), kInst, 1);
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_port_up(stptest_port_name(i));
        // Two forward delays take every port to FORWARDING.
        stptest_tick(31 * kSecond);
        ASSERT_TRUE(stptest_pvst_is_root(kVlan)) << "a lone bridge starts as its own root";
        ASSERT_EQ(stptest_pvst_port_state(kVlan, port0()), 4) << "port 0 did not reach FORWARDING";
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }
    static const char *port0() { return stptest_port_name(0); }
    static const char *port1() { return stptest_port_name(1); }

    // A neighbour keeps talking: one BPDU per hello for a few seconds.
    static void hello(const char *port, uint16_t priority, const uint8_t mac[6], unsigned int seconds)
    {
        for (unsigned int s = 0; s < seconds; s++) {
            stptest_pvst_rx_config_bpdu(port, kVlan, priority, mac, 0, 0);
            stptest_tick(2 * kSecond);
        }
    }
};

TEST_F(BpduRxTest, ASuperiorBpduMakesItsPortTheRootPort)
{
    hello(port0(), kSuperiorPriority, kLowMac, 3);

    EXPECT_FALSE(stptest_pvst_is_root(kVlan))
        << "the bridge stayed root against a better one; the daemon dropped "
        << stptest_pvst_drop_count() << " frames";
    EXPECT_TRUE(stptest_pvst_root_port_is(kVlan, port0()));
}

// The same BPDU on a port with BPDU guard is dropped, not acted on.
TEST_F(BpduRxTest, AGuardedPortDropsTheBpdu)
{
    stptest_pvst_port_config(port0(), kInst, 1, 0);
    unsigned int before = stptest_pvst_drop_count();

    hello(port0(), kSuperiorPriority, kLowMac, 1);

    EXPECT_TRUE(stptest_pvst_is_root(kVlan));
    EXPECT_GT(stptest_pvst_drop_count(), before);
}

TEST_F(BpduRxTest, AnInferiorBpduLeavesTheBridgeRoot)
{
    hello(port0(), kDefaultPriority, kHighMac, 3);

    EXPECT_TRUE(stptest_pvst_is_root(kVlan));
}

// A TCN arriving on a designated port of the root must start a topology
// change and the fast ageing the switch needs to relearn its addresses.
TEST_F(BpduRxTest, ATcnOnTheRootStartsFastAgeing)
{
    stptest_pvst_rx_tcn_bpdu(port1(), kVlan);
    stptest_tick(kSecond);

    EXPECT_TRUE(stptest_pvst_topology_change(kVlan));
    EXPECT_EQ(db().keys(stptest::kStpFastAgeingFlushTable).count("Vlan100"), 1u)
        << "no fast-ageing request was published for the VLAN";
}

// The link of a forwarding port going down is a topology change of the
// root's own: it must start fast ageing without any BPDU at all.
TEST_F(BpduRxTest, ALinkDownOnAForwardingPortStartsFastAgeing)
{
    stptest_port_down(port1());
    stptest_tick(kSecond);

    EXPECT_EQ(stptest_pvst_port_state(kVlan, port1()), 0) << "port 1 is not DISABLED after link down";
    EXPECT_TRUE(stptest_pvst_topology_change(kVlan));
    EXPECT_EQ(db().keys(stptest::kStpFastAgeingFlushTable).count("Vlan100"), 1u);
}

}  // namespace
