/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The bitmap walks back every port and VLAN mask in the daemon. A walk that
 * finds nothing, and a walk that runs off the end of a scan, must stay
 * inside arr[].
 */

#include <gtest/gtest.h>

extern "C" {
#include "bitmap.h"
}

namespace {

const uint16_t kBits = 64;   // two 32-bit words

class BitmapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bmp = nullptr;
        ASSERT_EQ(bmp_alloc(&bmp, kBits), 0);
        ASSERT_NE(bmp, nullptr);
    }
    void TearDown() override
    {
        if (bmp)
            bmp_free(bmp);
    }

    BITMAP_T *bmp;
};

// An all-zero bitmap: the walk reaches the end without a hit.
TEST_F(BitmapTest, FirstSetBitOnAnEmptyBitmapIsInvalid)
{
    EXPECT_EQ(bmp_get_first_set_bit(bmp), BMP_INVALID_ID);
}

// Asking for the bit after the last one is how every scan ends.
TEST_F(BitmapTest, NextSetBitAfterTheLastBitIsInvalid)
{
    BMP_SET(bmp, kBits - 1);

    EXPECT_EQ(bmp_get_next_set_bit(bmp, kBits - 1), BMP_INVALID_ID);
}

TEST_F(BitmapTest, NextUnsetBitAfterTheLastBitIsInvalid)
{
    EXPECT_EQ(bmp_find_first_unset_bit_after_offset(bmp, kBits - 1), BMP_INVALID_ID);
}

// A full scan must still visit every bit and then stop.
TEST_F(BitmapTest, WalkVisitsEverySetBitAndStops)
{
    const BMP_ID expected[] = {0, 31, 32, 63};
    for (BMP_ID id : expected)
        BMP_SET(bmp, id);

    std::vector<BMP_ID> seen;
    for (BMP_ID id = bmp_get_first_set_bit(bmp);
         id != BMP_INVALID_ID;
         id = bmp_get_next_set_bit(bmp, id))
    {
        seen.push_back(id);
        ASSERT_LE(seen.size(), sizeof(expected) / sizeof(expected[0]))
            << "the walk did not terminate";
    }

    ASSERT_EQ(seen.size(), sizeof(expected) / sizeof(expected[0]));
    for (size_t i = 0; i < seen.size(); i++)
        EXPECT_EQ(seen[i], expected[i]);
}

// The guard must not hide a bit in the last word.
TEST_F(BitmapTest, TheLastBitIsStillFound)
{
    BMP_SET(bmp, kBits - 1);
    EXPECT_EQ(bmp_get_first_set_bit(bmp), kBits - 1);
}

TEST_F(BitmapTest, FirstUnsetBitIsStillFound)
{
    BMP_SET(bmp, 0);

    EXPECT_EQ(bmp_find_first_unset_bit_after_offset(bmp, 0), 1);
}

}  // namespace
