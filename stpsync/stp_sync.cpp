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
#include <string.h>
#include <errno.h>
#include <system_error>
#include <sys/socket.h>
#include <linux/if.h>
#include <chrono>
#include "logger.h"
#include "dbconnector.h"
#include "producerstatetable.h"
#include "stp_sync.h"
#include "stp_dbsync.h"
#include <algorithm> 

using namespace std;
using namespace swss;


StpSync::StpSync(DBConnector *db, DBConnector *cfgDb) :
    m_stpVlanTable(db, APP_STP_VLAN_TABLE_NAME),
    m_stpVlanPortTable(db, APP_STP_VLAN_PORT_TABLE_NAME),
    m_stpVlanInstanceTable(db, APP_STP_VLAN_INSTANCE_TABLE_NAME),
    m_stpPortTable(db, APP_STP_PORT_TABLE_NAME),
    m_stpPortStateTable(db, APP_STP_PORT_STATE_TABLE_NAME),
    m_appVlanMemberTable(db, APP_VLAN_MEMBER_TABLE_NAME),
    m_stpFastAgeFlushTable(db, APP_STP_FASTAGEING_FLUSH_TABLE_NAME),
    m_appPortTable(db, APP_PORT_TABLE_NAME),
    m_cfgPortTable(cfgDb, CFG_PORT_TABLE_NAME),
    m_cfgLagTable(cfgDb, CFG_LAG_TABLE_NAME)
{
    SWSS_LOG_NOTICE("STP: sync object");
}

DBConnector db(APPL_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
DBConnector cfgDb(CONFIG_DB, DBConnector::DEFAULT_UNIXSOCKET, 0);
StpSync sync(&db, &cfgDb);

extern "C" {

    void stpsync_add_vlan_to_instance(uint16_t vlan_id, uint16_t instance)
    {
        sync.addVlanToInstance(vlan_id, instance);
    }
    
    void stpsync_del_vlan_from_instance(uint16_t vlan_id, uint16_t instance)
    {
        sync.delVlanFromInstance(vlan_id, instance);
    }
    
    void stpsync_update_stp_class(STP_VLAN_TABLE * stp_vlan)
    {
        sync.updateStpVlanInfo(stp_vlan);
    }
    
    void stpsync_del_stp_class(uint16_t vlan_id)
    {
        sync.delStpVlanInfo(vlan_id);
    }
    
    void stpsync_update_port_class(STP_VLAN_PORT_TABLE * stp_vlan_intf)
    {
        sync.updateStpVlanInterfaceInfo(stp_vlan_intf);
    }

    void stpsync_del_port_class(char * if_name, uint16_t vlan_id)
    {
        sync.delStpVlanInterfaceInfo(if_name, vlan_id);
    }

    void stpsync_update_port_state(char * ifName, uint16_t instance, uint8_t state)
    {
        sync.updateStpPortState(ifName, instance, state);
    }
    
    void stpsync_del_port_state(char * ifName, uint16_t instance)
    {
        sync.delStpPortState(ifName, instance);
    }
   
#if 0
    void stpsync_update_vlan_port_state(char * ifName, uint16_t vlan_id, uint8_t state)
    {
        sync.updateStpVlanPortState(ifName, vlan_id, state);
    }
    
    void stpsync_del_vlan_port_state(char * ifName, uint16_t vlan_id)
    {
        sync.delStpVlanPortState(ifName, vlan_id);
    }
#endif
    void stpsync_update_fastage_state(uint16_t vlan_id, bool add)
    {
        sync.updateStpVlanFastage(vlan_id, add);
    }

    void stpsync_update_port_admin_state(char * ifName, bool up, bool physical)
    {
        sync.updatePortAdminState(ifName, up, physical);
    }
    
    uint32_t stpsync_get_port_speed(char * ifName)
    {
        return sync.getPortSpeed(ifName);
    }
    
    void stpsync_update_bpdu_guard_shutdown(char * ifName, bool enabled)
    {
        sync.updateBpduGuardShutdown(ifName, enabled);
    }
    
    void stpsync_update_port_fast(char * ifName, bool enabled)
    {
        sync.updatePortFast(ifName, enabled);
    }
    
    void stpsync_del_stp_port(char * ifName)
    {
        sync.delStpInterface(ifName);
    }

    void stpsync_clear_appdb_stp_tables(void)
    {
        sync.clearAllStpAppDbTables();
    }
}


void StpSync::addVlanToInstance(uint16_t vlan_id, uint16_t instance)
{
    std::vector<FieldValueTuple> fvVector;
    string vlan;

    vlan = VLAN_PREFIX + to_string(vlan_id);

    FieldValueTuple o("stp_instance", to_string(instance));
    fvVector.push_back(o);

    m_stpVlanInstanceTable.set(vlan, fvVector);

    SWSS_LOG_NOTICE("Add %s to STP instance:%d", vlan.c_str(), instance);
}

void StpSync::delVlanFromInstance(uint16_t vlan_id, uint16_t instance)
{
    string vlan;

    vlan = VLAN_PREFIX + to_string(vlan_id);
    m_stpVlanInstanceTable.del(vlan);

    SWSS_LOG_NOTICE("Delete %s from STP instance:%d", vlan.c_str(), instance);
}

void StpSync::updateStpVlanInfo(STP_VLAN_TABLE * stp_vlan)
{
    string vlan;
    std::vector<FieldValueTuple> fvVector;

    vlan = VLAN_PREFIX + to_string(stp_vlan->vlan_id);

    if(stp_vlan->bridge_id[0] != '\0')
    {
        FieldValueTuple br("bridge_id", stp_vlan->bridge_id);
        fvVector.push_back(br);
    }
    
    if(stp_vlan->max_age != 0)
    {
        FieldValueTuple ma("max_age", to_string(stp_vlan->max_age));
        fvVector.push_back(ma);
    }

    if(stp_vlan->hello_time != 0)
    {
        FieldValueTuple ht("hello_time", to_string(stp_vlan->hello_time));
        fvVector.push_back(ht);
    }
    
    if(stp_vlan->forward_delay != 0)
    {
        FieldValueTuple fd("forward_delay", to_string(stp_vlan->forward_delay));
        fvVector.push_back(fd);
    }
    
    if(stp_vlan->hold_time != 0)
    {
        FieldValueTuple hdt("hold_time", to_string(stp_vlan->hold_time));
        fvVector.push_back(hdt);
    }
    
    if(stp_vlan->topology_change_time != 0)
    {
        FieldValueTuple ltc("last_topology_change", to_string(stp_vlan->topology_change_time));
        fvVector.push_back(ltc);
    }
    
    if(stp_vlan->topology_change_count != 0)
    {
        FieldValueTuple tcc("topology_change_count", to_string(stp_vlan->topology_change_count));
        fvVector.push_back(tcc);
    }
    
    if(stp_vlan->root_bridge_id[0] != '\0')
    {
        FieldValueTuple rbr("root_bridge_id", stp_vlan->root_bridge_id);
        fvVector.push_back(rbr);
    }
    
    if(stp_vlan->root_path_cost != 0xFFFFFFFF)
    {
        FieldValueTuple rpc("root_path_cost", to_string(stp_vlan->root_path_cost));
        fvVector.push_back(rpc);
    }
    
    if(stp_vlan->desig_bridge_id[0] != '\0')
    {
        FieldValueTuple dbr("desig_bridge_id", stp_vlan->desig_bridge_id);
        fvVector.push_back(dbr);
    }

    if(stp_vlan->root_port[0] != '\0')
    {
        FieldValueTuple rp("root_port", stp_vlan->root_port);
        fvVector.push_back(rp);
    }

    if(stp_vlan->root_max_age != 0)
    {
        FieldValueTuple rma("root_max_age", to_string(stp_vlan->root_max_age));
        fvVector.push_back(rma);
    }

    if(stp_vlan->root_hello_time != 0)
    {
        FieldValueTuple rht("root_hello_time", to_string(stp_vlan->root_hello_time));
        fvVector.push_back(rht);
    }
    
    if(stp_vlan->root_forward_delay != 0)
    {
        FieldValueTuple rfd("root_forward_delay", to_string(stp_vlan->root_forward_delay));
        fvVector.push_back(rfd);
    }
    
    FieldValueTuple rsi("stp_instance", to_string(stp_vlan->stp_instance));
    fvVector.push_back(rsi);

    m_stpVlanTable.set(vlan, fvVector);

    SWSS_LOG_DEBUG("Update STP_VLAN_TABLE for %s", vlan.c_str());

}

void StpSync::delStpVlanInfo(uint16_t vlan_id)
{
    string vlan;

    vlan = VLAN_PREFIX + to_string(vlan_id);

    m_stpVlanTable.del(vlan);
    SWSS_LOG_NOTICE("Delete STP_VLAN_TABLE for %s", vlan.c_str());
}


void StpSync::updateStpVlanInterfaceInfo(STP_VLAN_PORT_TABLE * stp_vlan_intf)
{
    std::string ifName(stp_vlan_intf->if_name);
    std::vector<FieldValueTuple> fvVector;
    string vlan, key;

    if(stp_vlan_intf->port_id != 0xFFFF)
    {
        FieldValueTuple pi("port_num", to_string(stp_vlan_intf->port_id));
        fvVector.push_back(pi);
        
    }

    if(stp_vlan_intf->port_priority != 0xFF)
    {
        FieldValueTuple pp("priority", to_string(stp_vlan_intf->port_priority << 4));
        fvVector.push_back(pp);
    }

    if(stp_vlan_intf->path_cost != 0xFFFFFFFF)
    {
        FieldValueTuple pc("path_cost", to_string(stp_vlan_intf->path_cost));
        fvVector.push_back(pc);
    }
    
    if(stp_vlan_intf->port_state[0] != '\0')
    {
        FieldValueTuple ps("port_state", stp_vlan_intf->port_state);
        fvVector.push_back(ps);
    }
    
    if(stp_vlan_intf->designated_cost != 0xFFFFFFFF)
    {
        FieldValueTuple dc("desig_cost", to_string(stp_vlan_intf->designated_cost));
        fvVector.push_back(dc);
    }

    if(stp_vlan_intf->designated_root[0] != '\0')
    {
        FieldValueTuple dr("desig_root", stp_vlan_intf->designated_root);
        fvVector.push_back(dr);
    }
    
    if(stp_vlan_intf->designated_bridge[0] != '\0')
    {
        FieldValueTuple db("desig_bridge", stp_vlan_intf->designated_bridge);
        fvVector.push_back(db);
    }
    
    if(stp_vlan_intf->designated_port != 0)
    {
        FieldValueTuple dp("desig_port", to_string(stp_vlan_intf->designated_port));
        fvVector.push_back(dp);
    }

    if(stp_vlan_intf->forward_transitions != 0)
    {
        FieldValueTuple ft("fwd_transitions", to_string(stp_vlan_intf->forward_transitions));
        fvVector.push_back(ft);
    }

    if((stp_vlan_intf->tx_config_bpdu != 0) || stp_vlan_intf->clear_stats)
    {
        FieldValueTuple tcb("bpdu_sent", to_string(stp_vlan_intf->tx_config_bpdu));
        fvVector.push_back(tcb);
    }
    
    if((stp_vlan_intf->rx_config_bpdu != 0) || stp_vlan_intf->clear_stats)
    {
        FieldValueTuple rcb("bpdu_received", to_string(stp_vlan_intf->rx_config_bpdu));
        fvVector.push_back(rcb);
    }

    if((stp_vlan_intf->tx_tcn_bpdu != 0) || stp_vlan_intf->clear_stats)
    {
        FieldValueTuple ttb("tc_sent", to_string(stp_vlan_intf->tx_tcn_bpdu));
        fvVector.push_back(ttb);
    }
    
    if((stp_vlan_intf->rx_tcn_bpdu != 0) || stp_vlan_intf->clear_stats)
    {
        FieldValueTuple rtb("tc_received", to_string(stp_vlan_intf->rx_tcn_bpdu));
        fvVector.push_back(rtb);
    }
    
    if(stp_vlan_intf->root_protect_timer != 0xFFFFFFFF)
    {
        FieldValueTuple rpt("root_guard_timer", to_string(stp_vlan_intf->root_protect_timer));
        fvVector.push_back(rpt);
    }

    vlan = VLAN_PREFIX + to_string(stp_vlan_intf->vlan_id);
    key = vlan + ":" + ifName;
    
    m_stpVlanPortTable.set(key, fvVector);

    SWSS_LOG_DEBUG("Update STP_VLAN_PORT_TABLE for %s intf %s", vlan.c_str(), ifName.c_str());

}

void StpSync::delStpVlanInterfaceInfo(char * if_name, uint16_t vlan_id)
{
    std::string ifName(if_name);
    string vlan, key;
    
    vlan = VLAN_PREFIX + to_string(vlan_id);
    key = vlan + ":" + ifName;
    
    m_stpVlanPortTable.del(key);

    SWSS_LOG_NOTICE("Delete STP_VLAN_PORT_TABLE for %s intf %s", vlan.c_str(), ifName.c_str());

}

void StpSync::updateStpPortState(char * if_name, uint16_t instance, uint8_t state)
{
    std::string ifName(if_name);
    std::vector<FieldValueTuple> fvVector;
    std::string key;

    key = ifName + ':' + to_string(instance);

    FieldValueTuple a("state", to_string(state));
    fvVector.push_back(a);
    m_stpPortStateTable.set(key, fvVector);

    SWSS_LOG_NOTICE("Update STP port:%s instance:%d state:%d", ifName.c_str(), instance, state);
}

void StpSync::delStpPortState(char * if_name, uint16_t instance)
{
    std::string ifName(if_name);
    std::string key;

    key = ifName + ':' + to_string(instance);
    m_stpPortStateTable.del(key);
    
    SWSS_LOG_NOTICE("Delete STP port:%s instance:%d", ifName.c_str(), instance);
}

#if 0
void StpSync::updateStpVlanPortState(char * if_name, uint16_t vlan_id, uint8_t state)
{
    std::string ifName(if_name);
    std::vector<FieldValueTuple> fvVector;
    std::string key = ifName;
    string vlan;

    vlan = VLAN_PREFIX + to_string(vlan_id);
    key = vlan + ':' + ifName;

    FieldValueTuple o("stp_state", to_string(state));
    
    m_appVlanMemberTable.set(key, fvVector);
    
    SWSS_LOG_NOTICE(" Update STP VLAN %s port %s state %d", vlan.c_str(), if_name, state);
}

void StpSync::delStpVlanPortState(char * if_name, uint16_t vlan_id)
{
    std::string ifName(if_name);
    std::string key = ifName;
    string vlan;

    vlan = VLAN_PREFIX + to_string(vlan_id);
    key = vlan + ':' + ifName;
    m_appVlanMemberTable.del(key);
    
    SWSS_LOG_NOTICE(" Delete STP VLAN %s port %s", vlan.c_str(), if_name);
}
#endif
void StpSync::updateStpVlanFastage(uint16_t vlan_id, bool add)
{
    string vlan;
    std::vector<FieldValueTuple> fvVector;

    vlan = VLAN_PREFIX + to_string(vlan_id);

    if(add)
    {
        FieldValueTuple o("state", "true");
        fvVector.push_back(o);

        m_stpFastAgeFlushTable.set(vlan, fvVector);
    }
    else
    {
        m_stpFastAgeFlushTable.del(vlan);
    }

    SWSS_LOG_NOTICE(" %s VLAN %s fastage", add?"Update":"Delete", vlan.c_str());
}

void StpSync::updatePortAdminState(char * if_name, bool up, bool physical)
{
    std::vector<FieldValueTuple> fvVector;
    std::string ifName(if_name);
    std::string key = ifName;

    FieldValueTuple fv("admin_status", (up ? "up" : "down"));
    fvVector.push_back(fv);

    if(physical)
        m_cfgPortTable.set(key, fvVector);
    else
        m_cfgLagTable.set(key, fvVector);

    SWSS_LOG_NOTICE("STP %s %s port %s", if_name, up?"enable":"disable", physical?"physical":"LAG");
}

uint32_t StpSync::getPortSpeed(char * if_name)
{
    vector<FieldValueTuple> fvs;
    uint32_t speed = 0;
    string port_speed;

    m_appPortTable.get(if_name, fvs);
    auto it = find_if(fvs.begin(), fvs.end(), [](const FieldValueTuple &fv) {
            return fv.first == "speed";
            });

    if (it != fvs.end())
    {
        port_speed = it->second;
        speed = std::atoi(port_speed.c_str());
    }
    SWSS_LOG_NOTICE("STP port %s speed %d", if_name, speed);
    return speed;
}

void StpSync::updateBpduGuardShutdown(char * if_name, bool enabled)
{
    std::vector<FieldValueTuple> fvVector;
    std::string ifName(if_name);
    std::string key = ifName;

    FieldValueTuple fv("bpdu_guard_shutdown", (enabled ? "yes" : "no"));
    fvVector.push_back(fv);

    m_stpPortTable.set(key, fvVector);

    SWSS_LOG_NOTICE("STP %s bpdu guard %s", if_name, enabled ? "yes" : "no");
}

void StpSync::delStpInterface(char * if_name)
{
    std::string ifName(if_name);
    std::string key = ifName;

    m_stpPortTable.del(key);

    SWSS_LOG_NOTICE("STP interface %s delete", if_name);
}

void StpSync::updatePortFast(char * if_name, bool enabled)
{
    std::vector<FieldValueTuple> fvVector;
    std::string ifName(if_name);
    std::string key = ifName;

    FieldValueTuple fs("port_fast", (enabled ? "yes" : "no"));
    fvVector.push_back(fs);

    m_stpPortTable.set(key, fvVector);

    SWSS_LOG_NOTICE("STP %s port fast %s", if_name, enabled ? "yes" : "no");
}

void StpSync::clearAllStpAppDbTables(void)
{
    m_stpVlanTable.clear();
    m_stpVlanPortTable.clear();
    //m_stpVlanInstanceTable.clear();
    m_stpPortTable.clear();
    //m_stpPortStateTable.clear();
    //m_appVlanMemberTable.clear();
    m_stpFastAgeFlushTable.clear();
    SWSS_LOG_NOTICE("STP clear all APP DB STP tables");
}
