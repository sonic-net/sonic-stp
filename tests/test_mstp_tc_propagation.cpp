/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * A topology change received on one port has to reach every other port
 * that leads to another bridge: each of them starts tcWhile and asks
 * orchagent to flush its FDB, which is what stops the bridge forwarding to
 * addresses that just moved. Two kinds of port keep their entries: the one
 * the change arrived on, because that side is the one still valid, and edge
 * ports, whose hosts did not move. A port that has never received a BPDU is
 * an edge port here, because auto edge is on by default.
 */

#include <gtest/gtest.h>

#include <string>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;

const uint16_t kSuperiorPriority = 0;
const uint8_t kSuperiorMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint16_t kInferiorPriority = 0xf000;
const uint8_t kInferiorMac[6] = {0x02, 0xff, 0xff, 0xff, 0xff, 0xff};

// 100ms a tick. Long enough for the CIST to leave discarding on every
// port with the default forward delay.
const unsigned int kConvergeTicks = 700;
const unsigned int kReactTicks = 30;

class MstpTcPropagationTest : public ::testing::Test
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

    // A quiet bridge behind the port: one BPDU it loses the election with,
    // which is enough to make the port a non-edge port.
    static void bridgeBehind(uint32_t idx)
    {
        stptest_mstp_rx_cist_bpdu(stptest_port_name(idx), kInferiorPriority, kInferiorMac, 0, 0);
    }

    static void forgetFlushes()
    {
        db().clearTable(stptest::kStpInstPortFlushTable);
        db().resetWrites();
    }

    // The CIST flush row for a port, as the fake APPL_DB keys it.
    static bool flushed(uint32_t idx)
    {
        const std::set<std::string> &rows = db().keys(stptest::kStpInstPortFlushTable);
        return rows.find(std::string("0:") + stptest_port_name(idx)) != rows.end();
    }
};

// Ports 1 and 2 lead to bridges, port 3 has never heard a BPDU. A superior
// root appears on port 0 announcing a topology change: port 0 becomes the
// root port, ports 1 and 2 flush, and neither port 0 nor the edge port does.
TEST_F(MstpTcPropagationTest, ReceivedTcFlushesEveryNonEdgePort)
{
    configureMstp();
    stptest_tick(kConvergeTicks);
    bridgeBehind(1);
    bridgeBehind(2);
    stptest_tick(kReactTicks);

    ASSERT_FALSE(stptest_mstp_port_is_edge(stptest_port_name(1)));
    ASSERT_FALSE(stptest_mstp_port_is_edge(stptest_port_name(2)));
    ASSERT_TRUE(stptest_mstp_port_is_edge(stptest_port_name(3)))
        << "a port that never received a BPDU is expected to be an edge port; "
           "if auto edge is no longer the default, the edge expectations "
           "below need a configured edge port instead";

    forgetFlushes();
    stptest_mstp_rx_cist_bpdu(stptest_port_name(0), kSuperiorPriority, kSuperiorMac, 1, 1);
    stptest_tick(kReactTicks);

    ASSERT_TRUE(stptest_mstp_cist_root_port_is(stptest_port_name(0)))
        << "the superior BPDU was not accepted, so nothing below is about "
           "topology change propagation";

    EXPECT_TRUE(flushed(1)) << "Ethernet1 kept its FDB after a topology change "
                               "arrived on Ethernet0; stale entries there send "
                               "traffic the wrong way until they age out";
    EXPECT_TRUE(flushed(2)) << "Ethernet2 kept its FDB after a topology change "
                               "arrived on Ethernet0";
    EXPECT_FALSE(flushed(0)) << "the port the change arrived on was flushed "
                                "too; its entries are the ones still valid";
    EXPECT_FALSE(flushed(3)) << "an edge port was flushed; its hosts did not "
                                "move and 802.1Q keeps them out of the change";
}

// The control: the root repeating its BPDU without the flag changes nothing
// and flushes nothing. A flush on every BPDU from the root would pass the
// test above for the wrong reason.
TEST_F(MstpTcPropagationTest, ABpduWithoutTcFromTheRootDoesNotFlush)
{
    configureMstp();
    stptest_tick(kConvergeTicks);
    bridgeBehind(1);
    stptest_mstp_rx_cist_bpdu(stptest_port_name(0), kSuperiorPriority, kSuperiorMac, 1, 0);
    stptest_tick(kReactTicks);
    ASSERT_TRUE(stptest_mstp_cist_root_port_is(stptest_port_name(0)));

    forgetFlushes();
    stptest_mstp_rx_cist_bpdu(stptest_port_name(0), kSuperiorPriority, kSuperiorMac, 1, 0);
    stptest_tick(kReactTicks);

    EXPECT_FALSE(flushed(1)) << "Ethernet1 flushed on a BPDU that carried no "
                                "topology change and changed nothing";
    EXPECT_FALSE(flushed(0));
}

} // namespace
