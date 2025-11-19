/*
 * Copyright 2019 Broadcom. The term "Broadcom" refers to Broadcom Inc. and/or
 * its subsidiaries.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bitmap.h"

/*
 * STP indexing starts from 0 onwards
 * return :
 * >=0 on finding a valid bit position
 * -1  if there are no bits set
 */
int glibc_util_find_first_set_bit (uint32_t bmp)
{
    int ret = ffsl(bmp);
    if (ret == 0)
        return -1;

    return (ret - 1);
}

bool bmp_is_mask_equal(BITMAP_T *bmp1, BITMAP_T *bmp2)
{
    uint16_t i = 0;
    if(bmp1->size != bmp2->size)
        return false;

    for (;i<bmp1->size;i++)
        if (bmp1->arr[i] != bmp2->arr[i])
            return false;

    return true;
}

void bmp_copy_mask(BITMAP_T *dst, BITMAP_T *src)
{
    uint16_t i = 0;
    uint16_t size = 0;
    size = (dst->size < src->size) ? dst->size : src->size;

    for (;i<size;i++)
        dst->arr[i] = src->arr[i];
}

void bmp_not_mask(BITMAP_T *dst, BITMAP_T *src)
{
    uint16_t i=0;
    uint16_t size = 0;
    size = (dst->size < src->size) ? dst->size : src->size;

    for (;i<size;i++)
        dst->arr[i] = ~(src->arr[i]);
}

void bmp_and_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2)
{
    uint16_t i=0;
    uint16_t size = 0;
    size = (bmp1->size < bmp2->size) ? bmp1->size : bmp2->size;
    size = (tgt->size < size) ? tgt->size : size;

    for (;i<size;i++)
        tgt->arr[i] = (bmp1->arr[i] & bmp2->arr[i]);
}

void bmp_and_not_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2)
{
    uint16_t i=0;
    uint16_t size = 0;
    size = (bmp1->size < bmp2->size) ? bmp1->size : bmp2->size;
    size = (tgt->size < size) ? tgt->size : size;

    for (;i<size;i++)
        tgt->arr[i] = (bmp1->arr[i] & (~(bmp2->arr[i])));
}

void bmp_or_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2)
{
    uint16_t i=0;
    uint16_t size = 0;
    size = (bmp1->size < bmp2->size) ? bmp1->size : bmp2->size;
    size = (tgt->size < size) ? tgt->size : size;

    for (;i<size;i++)
        tgt->arr[i] = (bmp1->arr[i] | bmp2->arr[i]);
}

void bmp_xor_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2)
{
    uint16_t i=0;
    uint16_t size = 0;
    size = (bmp1->size < bmp2->size) ? bmp1->size : bmp2->size;
    size = (tgt->size < size) ? tgt->size : size;

    for (;i<size;i++)
        tgt->arr[i] = (bmp1->arr[i] ^ bmp2->arr[i]);
}

BMP_ID bmp_find_first_set_bit_after_offset(BITMAP_T *bmp, uint16_t offset)
{
    offset += 1;

    BMP_ID bmp_id           = BMP_INVALID_ID;
    uint16_t offset_index   = offset >> BMP_MASK_LEN;
    uint16_t offset_pos     = offset & BMP_MASK;

    unsigned int offset_bmp = bmp->arr[offset_index];
    int16_t first_set_bit   = BMP_INVALID_ID;
    //mask-out0(set 0) to all bits before offset
    offset_bmp = offset_bmp & (-1U << offset_pos);

    do {
        first_set_bit = glibc_util_find_first_set_bit(offset_bmp);
        if (first_set_bit != BMP_INVALID_ID)
        {
            bmp_id = (offset_index * BMP_MASK_BITS) + first_set_bit;
            return bmp_id;
        }
        offset_index++;
        offset_bmp = bmp->arr[offset_index];
    }while(offset_index < bmp->size);

    return BMP_INVALID_ID;
}


BMP_ID bmp_find_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset)
{
    offset += 1;

    BMP_ID bmp_id           = BMP_INVALID_ID;
    uint16_t offset_index   = offset >> BMP_MASK_LEN;
    uint16_t offset_pos     = offset & BMP_MASK;

    unsigned int offset_bmp = bmp->arr[offset_index];
    int16_t first_set_bit   = BMP_INVALID_ID;

    //mask-out1(set 1) to all bits before offset
    offset_bmp = offset_bmp | (~(-1U << offset_pos));

    do {
        first_set_bit = glibc_util_find_first_set_bit(~offset_bmp);
        if (first_set_bit != BMP_INVALID_ID)
        {
            bmp_id = (offset_index * BMP_MASK_BITS) + first_set_bit;
            return bmp_id;
        }
        offset_index++;
        if(offset_index < bmp->size)
            offset_bmp = bmp->arr[offset_index];
    }while(offset_index < bmp->size);

    return BMP_INVALID_ID;
}

BMP_ID bmp_set_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset)
{
    BMP_ID bmp_id = bmp_find_first_unset_bit_after_offset(bmp, offset);

    if (bmp_id != BMP_INVALID_ID)
        BMP_SET(bmp, bmp_id);

    return bmp_id;
}

BMP_ID bmp_find_first_unset_bit(BITMAP_T *bmp)
{
    uint8_t i = 0;
    int8_t first_set_bit = BMP_INVALID_ID;
    BMP_ID bmp_id = BMP_INVALID_ID; 
    do {
        first_set_bit = glibc_util_find_first_set_bit(~(bmp->arr[i]));
        if (first_set_bit != BMP_INVALID_ID)
        {
            bmp_id = (i * BMP_MASK_BITS) + first_set_bit;
            return bmp_id;
        }
        i++;
    }while(i < bmp->size);

    return BMP_INVALID_ID;
}

BMP_ID bmp_set_first_unset_bit(BITMAP_T *bmp)
{
    BMP_ID bmp_id = bmp_find_first_unset_bit(bmp);

    if (bmp_id != BMP_INVALID_ID)
        BMP_SET(bmp, bmp_id);

    return bmp_id;
}

BMP_ID bmp_get_next_set_bit(BITMAP_T *bmp, BMP_ID bmp_id)
{
    return bmp_find_first_set_bit_after_offset(bmp, bmp_id);
}

BMP_ID bmp_get_first_set_bit(BITMAP_T *bmp)
{
    return bmp_get_next_set_bit(bmp, BMP_INVALID_ID);
}



bool bmp_isset_any(BITMAP_T *bmp)
{
    uint16_t i;
    for (i=0;i<bmp->size;i++)
    {
        if (bmp->arr[i])
            return true;
    }
    return false;
}

bool bmp_isset(BITMAP_T *bmp, uint16_t bit)
{
    if (!bmp)
    {
        APP_LOG_ERR("Invalid bmp_ptr");
        return false;
    }
    if (BMP_ISSET(bmp, bit))
        return true;

    return false;
}

void bmp_set(BITMAP_T *bmp, uint16_t bit)
{
    if (!bmp)
    {
        APP_LOG_ERR("Invalid bmp_ptr");
        return;
    }

    if (!BMP_IS_BIT_POS_VALID(bmp, bit))
    {
        APP_LOG_ERR("Invalid Key : %hu",bit);
        return;
    }
    
    BMP_SET(bmp, bit);
}

void bmp_set_all(BITMAP_T *bmp)
{
    uint8_t i = 0;
    for(i=0; i<bmp->size; i++)
        bmp->arr[i] = -1;
    return;
}

void bmp_reset(BITMAP_T *bmp, uint16_t bit)
{
    if (!bmp)
    {
        APP_LOG_ERR("Invalid bmp_ptr");
        return;
    }

    if (!BMP_IS_BIT_POS_VALID(bmp, bit))
    {
        APP_LOG_ERR("Invalid Key : %hu",bit);
        return;
    }
    
    BMP_RESET(bmp, bit);
}

void bmp_reset_all(BITMAP_T *bmp)
{
    memset(bmp->arr, 0, (sizeof(uint32_t) * bmp->size));
    return;
}

void bmp_print_all(BITMAP_T *bmp)
{
    int i=0;

    APP_LOG_INFO("nbits: %d size : %d=>", bmp->nbits, bmp->size);
    for (i=0; i<bmp->size; i++)
    {
        APP_LOG_INFO("%04hx %04hx", (uint16_t)((bmp->arr[i] & BMP_FIRST16_MASK)>>16), (uint16_t)((bmp->arr[i] & BMP_SECOND16_MASK)));
    }
        APP_LOG_INFO("");
}

void bmp_free(BITMAP_T *bmp)
{
    free(bmp);
}

int8_t bmp_alloc(BITMAP_T **bmp, uint16_t nbits)
{
    uint16_t arr_size = 0;

    if (nbits == 0)
    {
        APP_LOG_ERR("Invalid params : nbits-%u", nbits);
        return -1;
    }

    arr_size = (nbits + (BMP_MASK_BITS - 1))/BMP_MASK_BITS;

    *bmp = calloc(1, sizeof(BITMAP_T) + (arr_size * sizeof(unsigned int)));
    if (*bmp == NULL)
    {
        APP_LOG_CRITICAL("calloc Failed");
        return -1;
    }

    (*bmp)->size = arr_size;
    (*bmp)->nbits = nbits;
    APP_LOG_DEBUG("created BITMAP of size %d for %u bits", (*bmp)->size, (*bmp)->nbits);

    return 0;
}

unsigned int bmp_count_set_bits(BITMAP_T *bmp)
{
    uint16_t i;
    unsigned int count = 0;
    unsigned int temp = 0;
    for (i=0;i<bmp->size;i++)
    {
        if (!bmp->arr[i])
            continue;
        temp = bmp->arr[i];
        while (temp)
        {
            count++;
            temp = temp & (temp - 1);
        }
    }
    return count;
}

void static_bmp_init(STATIC_BITMAP_T *bmp)
{
    bmp->nbits = STATIC_BMP_NBITS;
    bmp->size = STATIC_BMP_SZ;
    memset(&bmp->arr, 0, STATIC_BMP_SZ * sizeof(unsigned int));
    return;
}
