
/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */



/*
 * filename: nas_linux_stg_unittest.cpp
 *
 */

#include "nas_linux_l2.h"
#include "cps_api_object.h"
#include "dell-base-stg.h"

#include <gtest/gtest.h>
#include <iostream>


bool nas_linux_stg_set(){

    std::cout<<"Please Enter Interface Index and its stp state to be changed"<<std::endl;
    int ifindex;
    unsigned int  state;
    std::cin>>ifindex;
    std::cin>>state;
    cps_api_object_t obj = cps_api_object_create();
    cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_INTF_IF_INDEX_IFINDEX,ifindex);
    cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_INTF_STATE,state);


    if(nl_int_update_stp_state(obj) != STD_ERR_OK){
        std::cout<<"Interface Index invalid"<<std::endl;
        return false;
    }
    return true;
}

bool nas_linux_vlan_stg_set(){

    std::cout<<"Please Enter Interface Index, VLAN ID and its stp state to be changed"<<std::endl;
    int ifindex;
    unsigned int  state;
    int vlan;
    std::cin>>ifindex;
    std::cin>>vlan;
    std::cin>>state;

    cps_api_object_t obj = cps_api_object_create();
    cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_INTF_IF_INDEX_IFINDEX,ifindex);
    cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_INTF_STATE,state);
    cps_api_object_attr_add_u32(obj,BASE_STG_ENTRY_VLAN,vlan);

    if(nl_int_update_stp_state(obj) != STD_ERR_OK){
        std::cout<<"Interface Index invalid"<<std::endl;
        return false;
    }
    return true;
}


TEST(nas_linux_stg_test, update_stp_state) {
    ASSERT_TRUE(nas_linux_stg_set());
    ASSERT_TRUE(nas_linux_vlan_stg_set());
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


