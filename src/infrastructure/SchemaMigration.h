#pragma once

#include "IDatabase.h"

#include <string>
#include <vector>

namespace device_automation::infrastructure {

struct SchemaMigration
{
    int version = 0;
    std::string name;
    std::vector<std::string> statements;
};

class MigrationRunner
{
public:
    explicit MigrationRunner(IDatabase& db);

    bool apply(const std::vector<SchemaMigration>& migrations);

private:
    bool ensure_migration_table();
    bool is_applied(int version);
    bool apply_one(const SchemaMigration& migration);

    IDatabase& db_;
};

} // namespace device_automation::infrastructure
