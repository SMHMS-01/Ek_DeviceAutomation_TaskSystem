#include "application/AuditService.h"
#include "application/WorkflowManager.h"
#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SchemaMigration.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/SimpleScheduler.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>

using namespace device_automation::application;
using namespace device_automation::domain;
using namespace device_automation::infrastructure;
using namespace device_automation::scheduler;

namespace
{

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

} // namespace

int main()
{
    const char *db_path = "/tmp/device_automation_persistence_recovery.sqlite";
    std::remove(db_path);

    SqliteDatabase db;
    require(db.open(db_path), "database must open");

    MigrationRunner migration_runner(db);
    require(migration_runner.apply({SchemaMigration{
                100,
                "probe_success",
                {"CREATE TABLE IF NOT EXISTS migration_probe (id TEXT PRIMARY KEY) STRICT"}}}),
            "first migration apply must succeed");
    require(migration_runner.apply({SchemaMigration{
                100,
                "probe_success",
                {"CREATE TABLE IF NOT EXISTS migration_probe (id TEXT PRIMARY KEY) STRICT"}}}),
            "second migration apply must be idempotent");
    auto probe_migrations = db.query("SELECT version FROM schema_migrations WHERE version = 100");
    require(probe_migrations.size() == 1, "migration must be recorded once");

    const bool failed_migration = migration_runner.apply(
        {SchemaMigration{101,
                         "probe_failure",
                         {"CREATE TABLE migration_failure_probe (id TEXT PRIMARY KEY) STRICT",
                          "THIS IS NOT VALID SQL"}}});
    require(!failed_migration, "invalid migration must fail");
    require(db.query("SELECT version FROM schema_migrations WHERE version = 101").empty(),
            "failed migration must not be recorded");
    require(db.query("SELECT name FROM sqlite_master WHERE type = 'table' AND "
                     "name = 'migration_failure_probe'")
                .empty(),
            "failed migration must roll back DDL");

    EventBus bus;
    SimpleScheduler scheduler(db, bus);
    WorkflowManager manager(scheduler);

    TaskGraph graph;
    auto prepare = make_atomic_task("prepare recovery sample", Priority::High);
    auto analyze = make_atomic_task("analyze recovery sample", Priority::Normal);
    const auto prepare_id = prepare.id;
    const auto analyze_id = analyze.id;
    graph.add_task(prepare);
    graph.add_task(analyze);
    graph.add_dependency(prepare_id, analyze_id);

    const bool ok = manager.submit_and_run(
        graph,
        [](Task &) {
            return TaskExecutionResult{TaskExecutionStatus::Success, "persisted"};
        });
    require(ok, "workflow must complete");

    AuditService audit(db);
    require(audit.replay_task_state(prepare_id.to_string()) == "Completed",
            "prepare state must replay to completed");
    require(audit.replay_task_state(analyze_id.to_string()) == "Completed",
            "analyze state must replay to completed");
    require(audit.events_for_task(prepare_id.to_string()).size() == 3,
            "prepare task must have three audit events");
    require(audit.events_for_workflow(graph.id.to_string()).size() == 6,
            "workflow must have six audit events");

    const auto interrupted_task = TaskId::generate().to_string();
    const auto interrupted_workflow = WorkflowId::generate().to_string();
    db.execute(
        "INSERT INTO tasks(id, workflow_id, name, state, priority, retry_count, error_message) "
        "VALUES('" +
        interrupted_task + "', '" + interrupted_workflow +
        "', 'interrupted task', 'Running', 2, 0, '')");

    const int recovered = audit.recover_running_tasks();
    require(recovered == 1, "one interrupted task must be recovered");

    auto recovered_rows = db.query("SELECT state FROM tasks WHERE id = '" + interrupted_task + "'");
    require(recovered_rows.size() == 1, "recovered task row must exist");
    require(recovered_rows.front().at("state") == "Paused", "recovered task must be paused");
    require(audit.replay_task_state(interrupted_task) == "Paused",
            "recovered state must replay to paused");

    auto migrations = db.query("SELECT version FROM schema_migrations WHERE version = 1");
    require(migrations.size() == 1, "core migration must be present");

    require(db.execute("CREATE TABLE IF NOT EXISTS tx_probe (id TEXT PRIMARY KEY) STRICT"),
            "transaction probe table must be created");
    require(db.begin_transaction(), "rollback transaction must begin");
    require(db.execute("INSERT INTO tx_probe(id) VALUES('rolled_back')"),
            "rollback probe insert must succeed");
    require(db.rollback_transaction(), "rollback transaction must roll back");
    auto rolled_back = db.query("SELECT id FROM tx_probe WHERE id = 'rolled_back'");
    require(rolled_back.empty(), "rolled back row must not exist");

    require(db.begin_transaction(), "commit transaction must begin");
    require(db.execute("INSERT INTO tx_probe(id) VALUES('committed')"),
            "commit probe insert must succeed");
    require(db.commit_transaction(), "commit transaction must commit");
    auto committed = db.query("SELECT id FROM tx_probe WHERE id = 'committed'");
    require(committed.size() == 1, "committed row must exist");

    db.close();
    std::cout << "persistence recovery passed\n";
    return 0;
}
