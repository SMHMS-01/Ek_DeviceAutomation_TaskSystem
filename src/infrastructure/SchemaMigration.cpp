#include "SchemaMigration.h"

#include "domain/Types.h"

namespace device_automation::infrastructure
{

namespace
{

std::string quote(const std::string &value)
{
    std::string out = "'";
    for (char ch : value)
    {
        if (ch == '\'')
        {
            out += "''";
        }
        else
        {
            out += ch;
        }
    }
    out += "'";
    return out;
}

} // namespace

MigrationRunner::MigrationRunner(IDatabase &db) : db_(db)
{
}

bool MigrationRunner::apply(const std::vector<SchemaMigration> &migrations)
{
    if (!ensure_migration_table())
    {
        return false;
    }

    for (const auto &migration : migrations)
    {
        if (migration.version <= 0 || migration.name.empty())
        {
            return false;
        }
        if (is_applied(migration.version))
        {
            continue;
        }
        if (!apply_one(migration))
        {
            return false;
        }
    }
    return true;
}

bool MigrationRunner::ensure_migration_table()
{
    return db_.execute("CREATE TABLE IF NOT EXISTS schema_migrations ("
                       "version INTEGER PRIMARY KEY, name TEXT NOT NULL, "
                       "applied_at INTEGER NOT NULL) STRICT");
}

bool MigrationRunner::is_applied(int version)
{
    auto rows = db_.query("SELECT version FROM schema_migrations WHERE version = " +
                          std::to_string(version));
    return !rows.empty();
}

bool MigrationRunner::apply_one(const SchemaMigration &migration)
{
    if (!db_.begin_transaction())
    {
        return false;
    }

    for (const auto &statement : migration.statements)
    {
        if (!db_.execute(statement))
        {
            db_.rollback_transaction();
            return false;
        }
    }

    const auto applied_at = device_automation::domain::Timestamp::now().millis();
    const bool recorded =
        db_.execute("INSERT INTO schema_migrations(version, name, applied_at) "
                    "VALUES(" +
                    std::to_string(migration.version) + ", " + quote(migration.name) + ", " +
                    std::to_string(applied_at) + ")");
    if (!recorded)
    {
        db_.rollback_transaction();
        return false;
    }

    return db_.commit_transaction();
}

} // namespace device_automation::infrastructure
