#pragma once

#include <map>
#include <string>
#include <vector>

namespace device_automation::infrastructure {

using DatabaseRow = std::map<std::string, std::string>;
using QueryResult = std::vector<DatabaseRow>;

/**
 * Minimal database interface for scaffolding.
 */
class IDatabase
{
public:
    virtual ~IDatabase() = default;

    // Open database connection (path may be sqlite file)
    virtual bool open(const std::string& path) = 0;

    // Close connection
    virtual void close() = 0;

    // Execute simple SQL statement (no result set support in scaffold)
    virtual bool execute(const std::string& sql) = 0;

    // Execute a SELECT-like query and return stringified rows.
    virtual QueryResult query(const std::string& sql) = 0;
};

} // namespace device_automation::infrastructure
