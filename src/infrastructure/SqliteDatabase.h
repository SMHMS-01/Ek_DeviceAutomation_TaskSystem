#pragma once

#include "IDatabase.h"
#include <string>

namespace device_automation::infrastructure {

/**
 * Minimal SqliteDatabase stub for scaffolding. Real implementation will
 * depend on sqlite3 and provide proper result handling and transactions.
 */
class SqliteDatabase : public IDatabase
{
public:
    SqliteDatabase() = default;
    ~SqliteDatabase() override = default;

    bool open(const std::string& path) override;
    void close() override;
    bool execute(const std::string& sql) override;
};

} // namespace device_automation::infrastructure
