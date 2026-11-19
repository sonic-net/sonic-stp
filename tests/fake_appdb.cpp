/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implements include/stp_dbsync.h against an in-memory table model. Each
 * function writes the same key its StpSync counterpart does.
 */

#include "fake_appdb.h"

#include <sstream>

extern "C" {
#include <net/if.h>
#include <stdint.h>
#include "stp_dbsync.h"
}

namespace stptest {

const char *const kStpVlanTable = "STP_VLAN_TABLE";
const char *const kStpVlanPortTable = "STP_VLAN_PORT_TABLE";
const char *const kStpVlanInstanceTable = "STP_VLAN_INSTANCE_TABLE";
const char *const kStpPortTable = "STP_PORT_TABLE";
const char *const kStpPortStateTable = "STP_PORT_STATE_TABLE";
const char *const kStpMstInstTable = "STP_MST_INST_TABLE";
const char *const kStpMstPortTable = "STP_MST_PORT_TABLE";
const char *const kStpFastAgeingFlushTable = "STP_FASTAGEING_FLUSH_TABLE";
const char *const kStpInstPortFlushTable = "STP_INST_PORT_FLUSH_TABLE";

FakeAppDb &FakeAppDb::instance()
{
    static FakeAppDb db;
    return db;
}

void FakeAppDb::set(const std::string &table, const std::string &key)
{
    m_tables[table].insert(key);
}

void FakeAppDb::del(const std::string &table, const std::string &key)
{
    std::map<std::string, std::set<std::string> >::iterator it = m_tables.find(table);
    if (it != m_tables.end())
        it->second.erase(key);
}

void FakeAppDb::clearTable(const std::string &table)
{
    m_tables[table].clear();
}

void FakeAppDb::reset()
{
    m_tables.clear();
}

const std::set<std::string> &FakeAppDb::keys(const std::string &table) const
{
    static const std::set<std::string> kEmpty;
    std::map<std::string, std::set<std::string> >::const_iterator it = m_tables.find(table);
    return (it == m_tables.end()) ? kEmpty : it->second;
}

size_t FakeAppDb::size(const std::string &table) const
{
    return keys(table).size();
}

bool FakeAppDb::empty(const std::string &table) const
{
    return keys(table).empty();
}

std::string FakeAppDb::dump() const
{
    std::ostringstream os;
    for (std::map<std::string, std::set<std::string> >::const_iterator t = m_tables.begin();
         t != m_tables.end(); ++t)
    {
        if (t->second.empty())
            continue;
        os << "  " << t->first << " (" << t->second.size() << "):";
        for (std::set<std::string>::const_iterator k = t->second.begin(); k != t->second.end(); ++k)
            os << " " << *k;
        os << "\n";
    }
    if (os.str().empty())
        return "  <all tables empty>\n";
    return os.str();
}

const std::vector<std::string> &FakeAppDb::allTables()
{
    static std::vector<std::string> tables;
    if (tables.empty())
    {
        tables.push_back(kStpVlanTable);
        tables.push_back(kStpVlanPortTable);
        tables.push_back(kStpVlanInstanceTable);
        tables.push_back(kStpPortTable);
        tables.push_back(kStpPortStateTable);
        tables.push_back(kStpMstInstTable);
        tables.push_back(kStpMstPortTable);
        tables.push_back(kStpFastAgeingFlushTable);
        tables.push_back(kStpInstPortFlushTable);
    }
    return tables;
}

const std::vector<std::string> &FakeAppDb::ownedTables()
{
    // STP_INST_PORT_FLUSH_TABLE is left out: stpd only writes it, and
    // orchagent pops the row away. There is no delete to be missing.
    static std::vector<std::string> tables;
    if (tables.empty())
    {
        const std::vector<std::string> &all = allTables();
        for (size_t i = 0; i < all.size(); i++)
        {
            if (all[i] != kStpInstPortFlushTable)
                tables.push_back(all[i]);
        }
    }
    return tables;
}

const std::vector<std::string> &FakeAppDb::sweptTables()
{
    // Keep in lockstep with StpSync::clearAllStpAppDbTables(), including
    // the tables it deliberately leaves alone.
    static std::vector<std::string> tables;
    if (tables.empty())
    {
        tables.push_back(kStpVlanTable);
        tables.push_back(kStpVlanPortTable);
        tables.push_back(kStpPortTable);
        tables.push_back(kStpMstInstTable);
        tables.push_back(kStpMstPortTable);
        tables.push_back(kStpFastAgeingFlushTable);
    }
    return tables;
}

namespace {

std::string vlanKey(uint16_t vlan_id)
{
    std::ostringstream os;
    os << "Vlan" << vlan_id;
    return os.str();
}

std::string num(unsigned int v)
{
    std::ostringstream os;
    os << v;
    return os.str();
}

FakeAppDb &db()
{
    return FakeAppDb::instance();
}

} // namespace
} // namespace stptest

using stptest::db;
using stptest::num;
using stptest::vlanKey;

extern "C" {

void stpsync_add_vlan_to_instance(uint16_t vlan_id, uint16_t instance)
{
    (void)instance;
    db().set(stptest::kStpVlanInstanceTable, vlanKey(vlan_id));
}

void stpsync_del_vlan_from_instance(uint16_t vlan_id, uint16_t instance)
{
    (void)instance;
    db().del(stptest::kStpVlanInstanceTable, vlanKey(vlan_id));
}

void stpsync_update_stp_class(STP_VLAN_TABLE *stp_vlan)
{
    db().set(stptest::kStpVlanTable, vlanKey(stp_vlan->vlan_id));
}

void stpsync_del_stp_class(uint16_t vlan_id)
{
    db().del(stptest::kStpVlanTable, vlanKey(vlan_id));
}

void stpsync_update_port_class(STP_VLAN_PORT_TABLE *stp_vlan_intf)
{
    db().set(stptest::kStpVlanPortTable,
             vlanKey(stp_vlan_intf->vlan_id) + ":" + stp_vlan_intf->if_name);
}

void stpsync_del_port_class(char *if_name, uint16_t vlan_id)
{
    db().del(stptest::kStpVlanPortTable, vlanKey(vlan_id) + ":" + if_name);
}

void stpsync_update_port_state(char *ifName, uint16_t instance, uint8_t state)
{
    (void)state;
    db().set(stptest::kStpPortStateTable, std::string(ifName) + ":" + num(instance));
}

void stpsync_del_port_state(char *ifName, uint16_t instance)
{
    db().del(stptest::kStpPortStateTable, std::string(ifName) + ":" + num(instance));
}

void stpsync_update_vlan_port_state(char *ifName, uint16_t vlan_id, uint8_t state)
{
    (void)state;
    db().set(stptest::kStpVlanPortTable, vlanKey(vlan_id) + ":" + ifName);
}

void stpsync_del_vlan_port_state(char *ifName, uint16_t vlan_id)
{
    db().del(stptest::kStpVlanPortTable, vlanKey(vlan_id) + ":" + ifName);
}

void stpsync_update_fastage_state(uint16_t vlan_id, bool add)
{
    if (add)
        db().set(stptest::kStpFastAgeingFlushTable, vlanKey(vlan_id));
    else
        db().del(stptest::kStpFastAgeingFlushTable, vlanKey(vlan_id));
}

uint32_t stpsync_get_port_speed(char *ifName)
{
    (void)ifName;
    return 100000; // 100G, so path cost comes out sane.
}

void stpsync_update_bpdu_guard_shutdown(char *ifName, bool enabled)
{
    (void)enabled;
    db().set(stptest::kStpPortTable, ifName);
}

/* The admin state lives in CONFIG_DB, which the tests do not model. */
void stpsync_update_port_admin_state(char *ifName, bool up, bool physical)
{
    (void)ifName;
    (void)up;
    (void)physical;
}

void stpsync_del_stp_port(char *ifName)
{
    db().del(stptest::kStpPortTable, ifName);
}

void stpsync_update_port_fast(char *ifName, bool enabled)
{
    (void)enabled;
    db().set(stptest::kStpPortTable, ifName);
}

void stpsync_clear_appdb_stp_tables(void)
{
    const std::vector<std::string> &swept = stptest::FakeAppDb::sweptTables();
    for (size_t i = 0; i < swept.size(); i++)
        db().clearTable(swept[i]);
}

void stpsync_flush_instance_port(char *ifName, uint16_t instance)
{
    db().set(stptest::kStpInstPortFlushTable, num(instance) + ":" + ifName);
}

void stpsync_update_mst_info(STP_MST_TABLE *stp_mst)
{
    db().set(stptest::kStpMstInstTable, num(stp_mst->mst_id));
}

void stpsync_del_mst_info(uint16_t mst_id)
{
    db().del(stptest::kStpMstInstTable, num(mst_id));
}

void stpsync_update_mst_port_info(STP_MST_PORT_TABLE *stp_mst_intf)
{
    db().set(stptest::kStpMstPortTable,
             num(stp_mst_intf->mst_id) + ":" + stp_mst_intf->if_name);
}

void stpsync_del_mst_port_info(char *if_name, uint16_t mst_id)
{
    db().del(stptest::kStpMstPortTable, num(mst_id) + ":" + if_name);
}

void stpsync_update_boundary_port(char *ifName, bool enabled, char *proto)
{
    (void)enabled;
    (void)proto;
    db().set(stptest::kStpPortTable, ifName);
}

} // extern "C"
