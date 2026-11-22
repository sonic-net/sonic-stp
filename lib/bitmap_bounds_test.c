/* Copyright 2026 SONiC contributors. Licensed under the Apache License, Version 2.0. */

#include <assert.h>

#include "bitmap.h"

int main(void)
{
    BITMAP_T *bmp = NULL;

    assert(bmp_alloc(&bmp, 32) == 0);
    assert(BMP_IS_BIT_POS_VALID(bmp, 31));
    assert(!BMP_IS_BIT_POS_VALID(bmp, 32));
    bmp_set(bmp, 31);
    assert(bmp_isset(bmp, 31));
    assert(!bmp_isset(bmp, 32));
    bmp_set(bmp, 32);
    bmp_reset(bmp, 32);
    assert(bmp_isset(bmp, 31));
    bmp_free(bmp);

    assert(bmp_alloc(&bmp, 33) == 0);
    assert(BMP_IS_BIT_POS_VALID(bmp, 32));
    assert(!BMP_IS_BIT_POS_VALID(bmp, 33));
    bmp_set(bmp, 32);
    assert(bmp_isset(bmp, 32));
    assert(!bmp_isset(bmp, 33));
    bmp_free(bmp);

    return 0;
}
