/*
 * Copyright 2026 Nexthop Systems Inc. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * In-memory stand-in for stpsync/stp_sync.cpp. The daemon reaches APPL_DB
 * only through the stpsync_* API in include/stp_dbsync.h, so linking this
 * instead gives the real protocol code a Redis-free set of tables.
 */

#ifndef __FAKE_APPDB_H__
#define __FAKE_APPDB_H__

#include <map>
#include <set>
#include <string>
#include <vector>

namespace stptest {

// Table names as spelled in sonic-swss-common common/schema.h.
extern const char *const kStpVlanTable;
extern const char *const kStpVlanPortTable;
extern const char *const kStpVlanInstanceTable;
extern const char *const kStpPortTable;
extern const char *const kStpPortStateTable;
extern const char *const kStpMstInstTable;
extern const char *const kStpMstPortTable;
extern const char *const kStpFastAgeingFlushTable;
extern const char *const kStpInstPortFlushTable;

class FakeAppDb
{
public:
    static FakeAppDb &instance();

    // Called by the stpsync_* shims.
    void set(const std::string &table, const std::string &key);
    void del(const std::string &table, const std::string &key);
    void clearTable(const std::string &table);

    // Test side.
    void reset();
    const std::set<std::string> &keys(const std::string &table) const;
    size_t size(const std::string &table) const;
    bool empty(const std::string &table) const;
    std::string dump() const;

    // Every table the daemon writes into.
    static const std::vector<std::string> &allTables();

    // Tables the daemon is responsible for emptying on disable.
    static const std::vector<std::string> &ownedTables();

    // What StpSync::clearAllStpAppDbTables() sweeps. The rest is drained
    // by the per-row deletes on the DEL paths.
    static const std::vector<std::string> &sweptTables();

private:
    FakeAppDb() = default;
    std::map<std::string, std::set<std::string> > m_tables;
};

} // namespace stptest

#endif // __FAKE_APPDB_H__
