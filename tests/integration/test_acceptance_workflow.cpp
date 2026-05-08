#include "application/WorkflowManager.h"
#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/SimpleScheduler.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace device_automation::domain;
using namespace device_automation::infrastructure;
using namespace device_automation::scheduler;
using namespace device_automation::application;

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
    const char *db_path = "/tmp/device_automation_acceptance.sqlite";
    std::remove(db_path);

    SqliteDatabase db;
    require(db.open(db_path), "database must open");

    EventBus bus;
    std::vector<std::string> events;
    bus.subscribe("task.state_changed",
                  [&events](const std::string &payload) { events.push_back(payload); });

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
    const bool ok = manager.submit_and_run(
        graph,
        [](Task &task)
        {
            task.last_checkpoint.stage_name = task.name;
            task.last_checkpoint.saved_at = Timestamp::now();
            return TaskExecutionResult{TaskExecutionStatus::Success, "accepted"};
        });

    require(ok, "workflow must complete");
    require(graph.state == GraphState::Completed, "graph must be completed");
    require(graph.task(load_id).state == TaskState::Completed, "load task must be completed");
    require(graph.task(measure_id).state == TaskState::Completed, "measure task must be completed");
    require(graph.task(archive_id).state == TaskState::Completed, "archive task must be completed");
    require(scheduler.audit_records().size() == 9, "workflow must emit nine audits");
    require(events.size() == scheduler.audit_records().size(), "event count must match audits");

    db.close();
    std::cout << "acceptance workflow passed with " << scheduler.audit_records().size()
              << " audited state transitions\n";
    return 0;
}
