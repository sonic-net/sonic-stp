/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The default path cost of a port comes from its speed. Breakout sub-ports
 * run at speeds the old fixed table did not list, and a cost of 0 makes
 * every such port look free to the spanning tree.
 */

#include <gtest/gtest.h>

#include "stpd_driver.h"

namespace {

const int kExtend = 1;
const int kLegacy = 0;

struct SpeedCost
{
    unsigned int speed_mbps;
    unsigned int cost;
};

// 802.1t table 17-3: the values the fixed table already gave.
TEST(PathCostTest, ExtendModeKeepsTheStandardTableValues)
{
    const SpeedCost table[] = {
        {10, 2000000},      {100, 200000},   {1000, 20000},
        {10000, 2000},      {25000, 800},    {40000, 500},
        {100000, 200},      {400000, 50},    {800000, 25},
    };

    for (const SpeedCost &sc : table)
        EXPECT_EQ(stptest_path_cost(sc.speed_mbps, kExtend), sc.cost)
            << "speed " << sc.speed_mbps;
}

// Breakout sub-ports: 2x20G, 8x50G, 2x200G and 4x200G, plus 2.5G and 5G,
// and the 1.6T port of the NH-4220 (12.5 rounds down to 12).
TEST(PathCostTest, ExtendModeDerivesBreakoutSpeedsFromTheFormula)
{
    const SpeedCost table[] = {
        {2500, 8000}, {5000, 4000}, {20000, 1000},
        {50000, 400}, {200000, 100}, {1000000, 20}, {1600000, 12},
    };

    for (const SpeedCost &sc : table)
        EXPECT_EQ(stptest_path_cost(sc.speed_mbps, kExtend), sc.cost)
            << "speed " << sc.speed_mbps;
}

// The formula is clamped to the 802.1t range at both ends.
TEST(PathCostTest, ExtendModeClampsToTheStandardRange)
{
    EXPECT_EQ(stptest_path_cost(1, kExtend), 20000000u);
    EXPECT_EQ(stptest_path_cost(100000000, kExtend), 1u);
}

// 802.1D-1998 table 8-5, with in-between speeds taking the next lower row.
TEST(PathCostTest, LegacyModeUsesTheOldTableWithRanges)
{
    const SpeedCost table[] = {
        {4, 250},       {10, 100},     {100, 19},      {1000, 4},
        {2500, 4},      {10000, 2},    {20000, 2},     {25000, 1},
        {50000, 1},     {200000, 1},   {800000, 1},    {1600000, 1},
    };

    for (const SpeedCost &sc : table)
        EXPECT_EQ(stptest_path_cost(sc.speed_mbps, kLegacy), sc.cost)
            << "speed " << sc.speed_mbps;
}

// No speed is the one case that still has no cost.
TEST(PathCostTest, UnknownSpeedStillReportsZero)
{
    EXPECT_EQ(stptest_path_cost(0, kExtend), 0u);
    EXPECT_EQ(stptest_path_cost(0, kLegacy), 0u);
}

} // namespace
