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

#ifndef __STPSYNC__
#define __STPSYNC__

#include <string>
#include "dbconnector.h"
#include "producerstatetable.h"
#include "stp_dbsync.h"

namespace swss {

    class StpSync {
        public:
            StpSync(DBConnector *db, DBConnector *cfgDb);
            void addVlanToInstance(uint16_t vlan_id, uint16_t instance);
            void delVlanFromInstance(uint16_t vlan_id, uint16_t instance);
            void updateStpVlanInfo(STP_VLAN_TABLE * stp_vlan);
            void delStpVlanInfo(uint16_t vlan_id);
            void updateStpVlanInterfaceInfo(STP_VLAN_PORT_TABLE * stp_vlan_intf);
            void delStpVlanInterfaceInfo(char * if_name, uint16_t vlan_id);
            void updateStpPortState(char * ifName, uint16_t instance, uint8_t state);
            void delStpPortState(char * ifName, uint16_t instance);
            void updateStpVlanPortState(char * ifName, uint16_t vlan_id, uint8_t state);
            void delStpVlanPortState(char * ifName, uint16_t vlan_id);
            void updateStpVlanFastage(uint16_t vlan_id, bool add);
            void updatePortAdminState(char * if_name, bool up, bool physical);
            uint32_t getPortSpeed(char * if_name);
            void updateBpduGuardShutdown(char * ifName, bool disabled);
            void delStpInterface(char * ifName);
            void updatePortFast(char * ifName, bool disabled);
            void clearAllStpAppDbTables(void);
            void flushStpInstancePort(char *if_name, uint16_t instance);
            void updateStpMstInfo(STP_MST_TABLE * stp_mst);
            void delStpMstInfo(uint16_t mst_id);
            void updateStpMstInterfaceInfo(STP_MST_PORT_TABLE * stp_mst_intf);
            void delStpMstInterfaceInfo(char * if_name, uint16_t mst_id);
			void updateBoundaryPort(char *if_name, bool enabled, char *proto);

        protected:
        private:
            ProducerStateTable m_stpVlanTable;
            ProducerStateTable m_stpVlanPortTable;
            ProducerStateTable m_stpVlanInstanceTable;
            ProducerStateTable m_stpPortTable;
            ProducerStateTable m_stpPortStateTable; 
            ProducerStateTable m_stpMstTable;
            ProducerStateTable m_stpMstPortTable;
            ProducerStateTable m_stpFastAgeFlushTable;
            ProducerStateTable m_stpInstancePortFlushTable; 
			Table m_appPortTable;
			Table m_cfgPortTable;
			Table m_cfgLagTable;
    };

}

#endif

