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

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "applog.h"

#define BMP_INVALID_ID -1

//Bitmap will be offset of 32bits and accessed using uint32_t
#define BMP_MASK_BITS 32
#define BMP_MASK_LEN 5
#define BMP_MASK 0x1f

#define BMP_FIRST16_MASK  0xffff0000
#define BMP_SECOND16_MASK 0x0000ffff

#define BMP_IS_BIT_POS_VALID(_bmp, _bit) ((_bit >= 0) && (_bit <= _bmp->nbits))

#define BMP_GET_ARR_ID(_p)  (_p/BMP_MASK_BITS)
#define BMP_GET_ARR_POS(_p) ((_p) & BMP_MASK) 

#define BMP_SET(_bmp,_k)   (_bmp->arr[BMP_GET_ARR_ID(_k)] |= (1U<<(BMP_GET_ARR_POS(_k))))
#define BMP_RESET(_bmp,_k) (_bmp->arr[BMP_GET_ARR_ID(_k)] &= ~(1U<<(BMP_GET_ARR_POS(_k))))
#define BMP_ISSET(_bmp,_k) (_bmp->arr[BMP_GET_ARR_ID(_k)] & (1U<<(BMP_GET_ARR_POS(_k))))

typedef struct BITMAP_S{
    //nbits: //range :(0-65535), bits : 16
    uint16_t nbits;

    //Auto derived from nbits
    //
    //size : min number of "unsigned int" required to accomodate all the bits
    //range : 1-2047
    uint16_t size; 
    unsigned int *arr;
}BITMAP_T;

typedef int32_t BMP_ID;

#define STATIC_BMP_NBITS  4096
#define STATIC_BMP_SZ     ((STATIC_BMP_NBITS + (BMP_MASK_BITS -1))/BMP_MASK_BITS)

typedef struct STATIC_BITMAP_S{
    uint16_t nbits;
    uint16_t size;
    //until this point members should be in sync with BITMAP_T
    //Coz, internally we typecast STATIC_BITMAP_T to BITMAP_T
    unsigned int arr[STATIC_BMP_SZ];
}STATIC_BITMAP_T;

//
//For static allocation ,
// ex:
// BITMAP_T vlan_bmp;
// BMP_INIT_STATIC(vlan_bmp, 4096)
//
#define BMP_GET_ARR_SIZE_FROM_BITS(nbits) ((nbits + (BMP_MASK_BITS - 1))/BMP_MASK_BITS)
#define BMP_INIT_STATIC(bmp, nbits) \
do {\
    unsigned int bmp_arr[BMP_GET_ARR_SIZE_FROM_BITS(nbits)];\
    bmp.nbits  = nbits;\
    bmp.size   = BMP_GET_ARR_SIZE_FROM_BITS(nbits);\
    bmp.arr    = &bmp_arr[0];\
}while(0);

bool bmp_is_mask_equal(BITMAP_T *bmp1, BITMAP_T *bmp2);
void bmp_copy_mask(BITMAP_T *dst, BITMAP_T *src);
void bmp_not_mask(BITMAP_T *dst, BITMAP_T *src);
void bmp_and_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);
void bmp_and_not_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);
void bmp_or_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);
void bmp_xor_masks(BITMAP_T *tgt, BITMAP_T *bmp1, BITMAP_T *bmp2);
bool bmp_isset_any(BITMAP_T *bmp);
bool bmp_isset(BITMAP_T *bmp, uint16_t bit);
void bmp_set(BITMAP_T *bmp, uint16_t bit);
void bmp_set_all(BITMAP_T *bmp);
void bmp_reset(BITMAP_T *bmp, uint16_t bit);
void bmp_reset_all(BITMAP_T *bmp);
void bmp_print_all(BITMAP_T *bmp);
int8_t bmp_init(BITMAP_T *bmp);
int8_t bmp_alloc(BITMAP_T **bmp, uint16_t nbits);
void bmp_free(BITMAP_T *bmp);

BMP_ID bmp_find_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset);
BMP_ID bmp_set_first_unset_bit_after_offset(BITMAP_T *bmp, uint16_t offset);
BMP_ID bmp_find_first_unset_bit(BITMAP_T *bmp);
BMP_ID bmp_set_first_unset_bit(BITMAP_T *bmp);
BMP_ID bmp_get_next_set_bit(BITMAP_T *bmp, BMP_ID id);
BMP_ID bmp_get_first_set_bit(BITMAP_T *bmp);
unsigned int bmp_count_set_bits(BITMAP_T *bmp);
BITMAP_T *static_mask_init(STATIC_BITMAP_T *bmp);
void static_bmp_init(STATIC_BITMAP_T *vbmp);

#endif //__BITMAP_H__
