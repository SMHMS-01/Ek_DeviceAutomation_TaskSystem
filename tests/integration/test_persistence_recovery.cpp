#include "application/AuditService.h"
#include "application/WorkflowManager.h"
#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/SimpleScheduler.h"

#include <cassert>
#include <cstdio>
#include <iostream>

using namespace device_automation::application;
using namespace device_automation::domain;
using namespace device_automation::infrastructure;
using namespace device_automation::scheduler;

int main()
{
    const char* db_path = "/tmp/device_automation_persistence_recovery.sqlite";
    std::remove(db_path);

    SqliteDatabase db;
    assert(db.open(db_path));

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

    const bool ok = manager.submit_and_run(graph, [](Task&) {
        return TaskExecutionResult{TaskExecutionStatus::Success, "persisted"};
    });
    assert(ok);

    AuditService audit(db);
    assert(audit.replay_task_state(prepare_id.to_string()) == "Completed");
    assert(audit.replay_task_state(analyze_id.to_string()) == "Completed");
    assert(audit.events_for_task(prepare_id.to_string()).size() == 3);
    assert(audit.events_for_workflow(graph.id.to_string()).size() == 6);

    const auto interrupted_task = TaskId::generate().to_string();
    const auto interrupted_workflow = WorkflowId::generate().to_string();
    db.execute("INSERT INTO tasks(id, workflow_id, name, state, priority, retry_count, error_message) "
               "VALUES('" +
               interrupted_task + "', '" + interrupted_workflow +
               "', 'interrupted task', 'Running', 2, 0, '')");

    const int recovered = audit.recover_running_tasks();
    assert(recovered == 1);

    auto recovered_rows = db.query("SELECT state FROM tasks WHERE id = '" + interrupted_task + "'");
    assert(recovered_rows.size() == 1);
    assert(recovered_rows.front().at("state") == "Paused");
    assert(audit.replay_task_state(interrupted_task) == "Paused");

    auto migrations = db.query("SELECT version FROM schema_migrations WHERE version = 1");
    assert(migrations.size() == 1);

    assert(db.execute("CREATE TABLE IF NOT EXISTS tx_probe (id TEXT PRIMARY KEY) STRICT"));
    assert(db.begin_transaction());
    assert(db.execute("INSERT INTO tx_probe(id) VALUES('rolled_back')"));
    assert(db.rollback_transaction());
    auto rolled_back = db.query("SELECT id FROM tx_probe WHERE id = 'rolled_back'");
    assert(rolled_back.empty());

    assert(db.begin_transaction());
    assert(db.execute("INSERT INTO tx_probe(id) VALUES('committed')"));
    assert(db.commit_transaction());
    auto committed = db.query("SELECT id FROM tx_probe WHERE id = 'committed'");
    assert(committed.size() == 1);

    db.close();
    std::cout << "persistence recovery passed\n";
    return 0;
}
