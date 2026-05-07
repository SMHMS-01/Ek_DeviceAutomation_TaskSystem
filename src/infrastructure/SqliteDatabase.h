#pragma once

#include "IDatabase.h"
#include <string>
#include <mutex>
#include <vector>

#ifdef USE_SQLITE3
struct sqlite3;
#endif

namespace device_automation::infrastructure {

/**
 * SqliteDatabase implementation or fallback mock if SQLite is not available.
 */
class SqliteDatabase : public IDatabase
{
public:
    SqliteDatabase() = default;
    ~SqliteDatabase() override;

    bool open(const std::string& path) override;
    void close() override;
    bool execute(const std::string& sql) override;
    QueryResult query(const std::string& sql) override;

private:
    std::mutex mu_;

#ifdef USE_SQLITE3
    sqlite3* db_ = nullptr;
#else
    bool opened_ = false;
    std::string opened_path_;
    std::vector<std::string> executed_sql_;
#endif
};

} // namespace device_automation::infrastructure
