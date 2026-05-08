#include "SqliteDatabase.h"
#include "Logger.h"

#include <iostream>
#include <mutex>

#ifdef USE_SQLITE3
#include <sqlite3.h>
#endif

namespace device_automation::infrastructure {

namespace {

#ifdef USE_SQLITE3
bool execute_sql(sqlite3* db, const std::string& sql)
{
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string e = err ? err : "unknown";
        sqlite3_free(err);
        Logger::log(Logger::Level::Error, "sqlite3_exec failed: " + e);
        return false;
    }
    return true;
}
#endif

} // namespace

SqliteDatabase::~SqliteDatabase()
{
    close();
}

bool SqliteDatabase::open(const std::string& path)
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    int rc = sqlite3_open_v2(path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        Logger::log(Logger::Level::Error, "sqlite3_open_v2 failed: " + std::string(sqlite3_errstr(rc)));
        return false;
    }
    Logger::log(Logger::Level::Info, "Opened SQLite database: " + path);
    return true;
#else
    // Fallback: simple in-memory mock
    opened_ = true;
    opened_path_ = path;
    Logger::log(Logger::Level::Info, "Sqlite3 not available; using mock DB for: " + path);
    return true;
#endif
}

void SqliteDatabase::close()
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    Logger::log(Logger::Level::Info, "Closed SQLite database");
#else
    opened_ = false;
    opened_path_.clear();
    Logger::log(Logger::Level::Info, "Closed mock DB");
#endif
}

bool SqliteDatabase::execute(const std::string& sql)
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    if (!db_) {
        Logger::log(Logger::Level::Error, "execute() called on unopened DB");
        return false;
    }
    return execute_sql(db_, sql);
#else
    if (!opened_) {
        Logger::log(Logger::Level::Error, "execute() called on unopened mock DB");
        return false;
    }
    // Record the executed SQL for testing/inspection
    executed_sql_.push_back(sql);
    return true;
#endif
}

QueryResult SqliteDatabase::query(const std::string& sql)
{
    std::lock_guard<std::mutex> lk(mu_);
    QueryResult rows;
#ifdef USE_SQLITE3
    if (!db_) {
        Logger::log(Logger::Level::Error, "query() called on unopened DB");
        return rows;
    }
    char* err = nullptr;
    auto callback = [](void* data, int column_count, char** values, char** names) -> int {
        auto* result = static_cast<QueryResult*>(data);
        DatabaseRow row;
        for (int i = 0; i < column_count; ++i) {
            row[names[i] ? names[i] : ""] = values[i] ? values[i] : "";
        }
        result->push_back(std::move(row));
        return 0;
    };
    int rc = sqlite3_exec(db_, sql.c_str(), callback, &rows, &err);
    if (rc != SQLITE_OK) {
        std::string e = err ? err : "unknown";
        sqlite3_free(err);
        Logger::log(Logger::Level::Error, "sqlite3_query failed: " + e);
    }
#else
    if (!opened_) {
        Logger::log(Logger::Level::Error, "query() called on unopened mock DB");
        return rows;
    }
    (void)sql;
#endif
    return rows;
}

bool SqliteDatabase::begin_transaction()
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    if (!db_) {
        Logger::log(Logger::Level::Error, "begin_transaction() called on unopened DB");
        return false;
    }
    return execute_sql(db_, "BEGIN IMMEDIATE");
#else
    if (!opened_ || in_transaction_) {
        return false;
    }
    in_transaction_ = true;
    executed_sql_.push_back("BEGIN IMMEDIATE");
    return true;
#endif
}

bool SqliteDatabase::commit_transaction()
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    if (!db_) {
        Logger::log(Logger::Level::Error, "commit_transaction() called on unopened DB");
        return false;
    }
    return execute_sql(db_, "COMMIT");
#else
    if (!opened_ || !in_transaction_) {
        return false;
    }
    in_transaction_ = false;
    executed_sql_.push_back("COMMIT");
    return true;
#endif
}

bool SqliteDatabase::rollback_transaction()
{
    std::lock_guard<std::mutex> lk(mu_);
#ifdef USE_SQLITE3
    if (!db_) {
        Logger::log(Logger::Level::Error, "rollback_transaction() called on unopened DB");
        return false;
    }
    return execute_sql(db_, "ROLLBACK");
#else
    if (!opened_ || !in_transaction_) {
        return false;
    }
    in_transaction_ = false;
    executed_sql_.push_back("ROLLBACK");
    return true;
#endif
}

} // namespace device_automation::infrastructure
