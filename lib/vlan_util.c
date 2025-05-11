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
#include "l2.h"

VLAN_ID vlanmask_get_first_vlan(void *bmp)
{
    BMP_ID i = bmp_get_first_set_bit((BITMAP_T *)bmp); 
    if(i == BMP_INVALID_ID)
        return VLAN_ID_INVALID;

    return i;
}

VLAN_ID vlanmask_get_next_vlan(void *bmp,VLAN_ID vlan) 
{
    BMP_ID i =  bmp_get_next_set_bit((BITMAP_T *)bmp, (BMP_ID)vlan);
    
    if(i == BMP_INVALID_ID)
        return VLAN_ID_INVALID;

    return i;
}

int vlanmask_to_string(BITMAP_T *mask, uint8_t *str, uint32_t maxlen)
{
    uint32_t i;
    uint32_t len;
    VLAN_ID start_vlan, end_vlan;

    if (str == NULL)
        return false;

    *str = '\0';
    len = 0;
    start_vlan = end_vlan = VLAN_ID_INVALID;

    for (i = MIN_VLAN_ID; i <= MAX_VLAN_ID; i++)
    {
        if (VLANMASK_ISSET(mask, i))
        {
            if (start_vlan == VLAN_ID_INVALID)
            {
                start_vlan = i;
            }
            end_vlan = i;
            continue;
        }

        if (start_vlan != VLAN_ID_INVALID)
        {
            if (len >= maxlen)
                return false;

            if (start_vlan == end_vlan)
                len += snprintf((char *)str+len, maxlen-len, "%d ", start_vlan);
            else
                len += snprintf((char *)str+len, maxlen-len, "%d to %d ", start_vlan, end_vlan);

            start_vlan = end_vlan = VLAN_ID_INVALID;
        }
    }

    if (start_vlan != VLAN_ID_INVALID)
    {
        if (len >= maxlen)
            return false;

        if (start_vlan == end_vlan)
            len += snprintf((char *)str+len, maxlen-len, "%d ", start_vlan);
        else
            len += snprintf((char *)str+len, maxlen-len, "%d to %d ", start_vlan, end_vlan);
    }

    return len;
}
