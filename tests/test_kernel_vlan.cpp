/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * stpd blocks a port in the kernel bridge by deleting the VLAN from it and
 * unblocks it by adding the VLAN back. The add has to recreate the member
 * the way vlanmgrd made it: an untagged member carries the PVID, a tagged
 * one does not. These check the command text, since the fake kernel runs
 * nothing.
 */

#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "stpd_driver.h"

namespace {

const uint16_t kVlan = 100;
const uint16_t kInst = 0;
const int kUntagged = 0;
const int kTagged = 1;

// 100ms a tick. Two forward delays and change, so a port with no portfast
// reaches FORWARDING on a bridge that hears no BPDUs.
const unsigned int kPvstForwardTicks = 400;
// The CIST leaves discarding on every port with the default forward delay;
// same figure the MSTP flush tests use.
const unsigned int kMstpForwardTicks = 700;

bool endsWith(const std::string &s, const std::string &tail)
{
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

std::string addFor(const char *port)
{
    std::string needle = std::string("vlan add vid 100 dev ") + port + " ";
    return stptest_last_kernel_command_containing(needle.c_str());
}

class KernelVlanTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        stptest_init();
        stptest_reset_protocol();
        stptest_reset_kernel_commands();
    }

    static const char *port0() { return stptest_port_name(0); }
    static const char *port1() { return stptest_port_name(1); }
};

TEST_F(KernelVlanTest, PvstReaddsAnUntaggedMemberWithItsPvid)
{
    stptest_pvst_enable();
    stptest_pvst_add_empty_vlan(kVlan, kInst);
    stptest_pvst_port_config_tagging(port0(), kInst, 1, 0, kUntagged);
    stptest_pvst_port_config_tagging(port1(), kInst, 1, 0, kTagged);
    stptest_pvst_vlan_member_tagging(kVlan, kInst, port0(), kUntagged);
    stptest_pvst_vlan_member_tagging(kVlan, kInst, port1(), kTagged);

    stptest_tick(kPvstForwardTicks);

    ASSERT_EQ(stptest_pvst_port_state(kVlan, port0()), 4) << "port never reached FORWARDING";
    EXPECT_TRUE(endsWith(addFor(port0()), " pvid untagged"))
        << "untagged member re-added as: " << addFor(port0());
    EXPECT_TRUE(endsWith(addFor(port1()), " tagged"))
        << "tagged member re-added as: " << addFor(port1());
    EXPECT_EQ(addFor(port1()).find("pvid"), std::string::npos)
        << "a tagged member must not take the PVID: " << addFor(port1());
}

class MstpKernelVlanTest : public KernelVlanTest
{
protected:
    void configureMstp()
    {
        stptest_mstp_enable();
        stptest_mstp_global_config();
        for (uint32_t i = 0; i < STPTEST_NUM_PORTS; i++)
            stptest_mstp_port_config(stptest_port_name(i), 1);
        stptest_mstp_vlan_ports(kVlan, 1);
        stptest_tick(kMstpForwardTicks);
    }

    // vlanmgrd re-announces a member when its tagging mode changes, and the
    // daemon then re-adds the VLAN in the kernel for a forwarding port.
    void reannounce(const char *port, int mode)
    {
        stptest_reset_kernel_commands();
        stptest_mstp_vlan_port(kVlan, port, mode, 1);
        stptest_tick(10);
    }
};

TEST_F(MstpKernelVlanTest, AnUntaggedMemberKeepsItsPvidWhenReannounced)
{
    configureMstp();
    EXPECT_TRUE(endsWith(addFor(port0()), " pvid untagged"))
        << "untagged member added as: " << addFor(port0());

    reannounce(port0(), kUntagged);

    EXPECT_TRUE(endsWith(addFor(port0()), " pvid untagged"))
        << "untagged member re-added as: " << addFor(port0());
}

// The member was untagged, then the operator made it tagged. The daemon
// must not hand the PVID back on the re-add.
TEST_F(MstpKernelVlanTest, AMemberMadeTaggedIsReaddedTagged)
{
    configureMstp();

    reannounce(port0(), kTagged);

    const std::string cmd = addFor(port0());
    EXPECT_TRUE(endsWith(cmd, " tagged")) << "re-added as: " << cmd;
    EXPECT_EQ(cmd.find("pvid"), std::string::npos)
        << "the stale untagged record handed the PVID to a tagged member: " << cmd;
}

} // namespace
