#include "application/WorkflowManager.h"
#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/SimpleScheduler.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace device_automation::domain;
using namespace device_automation::infrastructure;
using namespace device_automation::scheduler;
using namespace device_automation::application;

int main()
{
    const char* db_path = "/tmp/device_automation_acceptance.sqlite";
    std::remove(db_path);

    SqliteDatabase db;
    assert(db.open(db_path));

    EventBus bus;
    std::vector<std::string> events;
    bus.subscribe("task.state_changed", [&events](const std::string& payload) {
        events.push_back(payload);
    });

    TaskGraph graph;
    graph.name = "acceptance-sample-workflow";
    auto load = make_atomic_task("load sample", Priority::High);
    auto measure = make_atomic_task("measure sample", Priority::Normal);
    auto archive = make_atomic_task("archive result", Priority::Low);
    const auto load_id = load.id;
    const auto measure_id = measure.id;
    const auto archive_id = archive.id;

    graph.add_task(load);
    graph.add_task(measure);
    graph.add_task(archive);
    graph.add_dependency(load_id, measure_id);
    graph.add_dependency(measure_id, archive_id);

    SimpleScheduler scheduler(db, bus);
    WorkflowManager manager(scheduler);
    const bool ok = manager.submit_and_run(graph, [](Task& task) {
        task.last_checkpoint.stage_name = task.name;
        task.last_checkpoint.saved_at = Timestamp::now();
        return TaskExecutionResult{TaskExecutionStatus::Success, "accepted"};
    });

    assert(ok);
    assert(graph.state == GraphState::Completed);
    assert(graph.task(load_id).state == TaskState::Completed);
    assert(graph.task(measure_id).state == TaskState::Completed);
    assert(graph.task(archive_id).state == TaskState::Completed);
    assert(scheduler.audit_records().size() == 9);
    assert(events.size() == scheduler.audit_records().size());

    db.close();
    std::cout << "acceptance workflow passed with " << scheduler.audit_records().size()
              << " audited state transitions\n";
    return 0;
}
