/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Every IPC handler trusts the element count embedded in the payload, so a
 * frame that claims more elements than it carries is walked off the end of
 * the 4KB receive buffer. These drive malformed frames at the daemon and
 * assert it rejects them, stays up, and still accepts real traffic
 * afterwards. The frames are built in the C driver, so the daemon's
 * headers are never compiled as C++.
 */

#include <gtest/gtest.h>

#include "fake_appdb.h"

#include "stpd_driver.h"

using stptest::FakeAppDb;

namespace {

const uint16_t kVlan = 100;
const uint16_t kMstId = 1;
const uint16_t kBogusMstId = 2;

class IpcBoundsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        FakeAppDb::instance().reset();
    }

    static FakeAppDb &db() { return FakeAppDb::instance(); }

    // A working MSTP bridge, so "still alive" means something afterwards.
    static void configureMstp()
    {
        const uint16_t vlans[] = {kVlan};

        stptest_mstp_enable();
        stptest_mstp_global_config();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
        stptest_mstp_vlan_ports(kVlan, 1);
        stptest_mstp_add_instance(kMstId, vlans, 1);
    }

    static void configurePvst()
    {
        stptest_pvst_enable();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_pvst_port_config(stptest_port_name(i), 1, 1, 0);
    }
};

// The headline case: vlan_count says 4094, the frame carries one VLAN.
// Unbounded, the handler walks about 8KB past a 4KB stack buffer.
TEST_F(IpcBoundsTest, MstInstanceVlanCountBeyondThePayloadIsRejected)
{
    configureMstp();
    const size_t before = db().size(stptest::kStpMstInstTable);

    stptest_send_mst_instance_bogus_vlan_count(kBogusMstId, 4094);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a malformed instance message took the daemon down";
    EXPECT_EQ(db().size(stptest::kStpMstInstTable), before)
        << "the malformed instance message was acted on";
}

// Same shape on the port list of a VLAN-member message.
TEST_F(IpcBoundsTest, MstVlanPortCountBeyondThePayloadIsRejected)
{
    configureMstp();

    stptest_send_mst_vlan_ports_bogus_port_count(kVlan, 2048);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a malformed vlan-member message took the daemon down";
}

// The PVST side of the same defect: STP_VLAN_CONFIG carries a port list.
TEST_F(IpcBoundsTest, PvstVlanPortCountBeyondThePayloadIsRejected)
{
    configurePvst();

    stptest_send_pvst_vlan_bogus_port_count(kVlan, 1000);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a malformed vlan config message took the daemon down";
}

// A count read as negative must not become a huge unsigned bound.
TEST_F(IpcBoundsTest, NegativeElementCountIsRejected)
{
    configurePvst();

    stptest_send_pvst_vlan_bogus_port_count(kVlan, -1);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a negative element count took the daemon down";
}

// An inflated msg_len must not be believed over the bytes that arrived.
TEST_F(IpcBoundsTest, DeclaredLengthLongerThanTheFrameIsRejected)
{
    configureMstp();
    const size_t before = db().size(stptest::kStpMstInstTable);

    stptest_send_mst_instance_inflated_msg_len(kBogusMstId, 64);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "an inflated msg_len took the daemon down";
    EXPECT_EQ(db().size(stptest::kStpMstInstTable), before)
        << "a frame whose msg_len exceeded the bytes received was acted on";
}

// A payload too short for its own fixed part.
TEST_F(IpcBoundsTest, PayloadShorterThanTheFixedPartIsRejected)
{
    configureMstp();

    stptest_send_mst_instance_truncated();

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a truncated instance message took the daemon down";
}

// Shorter than the IPC header itself, so msg_type cannot even be read.
TEST_F(IpcBoundsTest, FrameShorterThanTheHeaderIsRejected)
{
    configureMstp();

    stptest_send_short_frame(3);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "a runt frame took the daemon down";
}

// msg_type indexes a name table before anything else looks at it, so an
// out-of-range value is an out-of-bounds read on the very first use.
TEST_F(IpcBoundsTest, UnknownMessageTypeIsRejected)
{
    configureMstp();

    stptest_send_unknown_msg_type(9999);
    stptest_send_unknown_msg_type(-1);

    EXPECT_TRUE(stptest_protocol_enabled())
        << "an out-of-range msg_type took the daemon down";
}

// The gate must not reject valid frames.
// A large message is still a well-formed one. An instance mapping 3000
// VLANs is about 6 KB on the wire; clipped to a page it would fail the
// element count and vanish without a trace.
TEST_F(IpcBoundsTest, ALargeInstanceMessageArrivesWhole)
{
    const uint16_t kVlanCount = 3000;

    configureMstp();
    stptest_mstp_add_instance_range(kBogusMstId, 1000, kVlanCount);

    EXPECT_EQ(stptest_mstp_instance_vlan_count(kBogusMstId), kVlanCount)
        << "the instance did not get every VLAN of a message larger than "
           "the old 4 KB receive buffer";
}

TEST_F(IpcBoundsTest, WellFormedTrafficStillReachesTheDaemon)
{
    configureMstp();
    for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
        stptest_mstp_inst_port_config(stptest_port_name(i), kMstId);

    stptest_tick(700);

    EXPECT_TRUE(stptest_protocol_enabled());
    EXPECT_FALSE(db().empty(stptest::kStpMstInstTable))
        << "the bounds check rejected a valid instance message";
}

}  // namespace
