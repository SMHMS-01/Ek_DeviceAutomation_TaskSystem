#include "application/AuditService.h"
#include "application/InterventionService.h"
#include "domain/Task.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/SqliteDatabase.h"
#include "scheduler/SimpleScheduler.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

Task make_task_in_state(TaskState state)
{
    auto task = make_atomic_task("intervention target", Priority::High);
    task.workflow_id = WorkflowId::generate();
    task.state = state;
    return task;
}

bool has_event_type(const std::vector<AuditEntry> &events, const std::string &event_type)
{
    for (const auto &event : events)
    {
        if (event.type == event_type)
        {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    const char *db_path = "/tmp/device_automation_intervention.sqlite";
    std::remove(db_path);

    SqliteDatabase db;
    require(db.open(db_path), "database must open");

    EventBus bus;
    std::vector<std::string> state_events;
    std::vector<std::string> intervention_events;
    bus.subscribe("task.state_changed",
                  [&state_events](const std::string &payload) { state_events.push_back(payload); });
    bus.subscribe("task.intervention", [&intervention_events](const std::string &payload)
                  { intervention_events.push_back(payload); });

    SimpleScheduler scheduler(db, bus);
    scheduler.initialize_storage();

    InterventionService intervention(db, bus);
    AuditService audit(db);

    auto running = make_task_in_state(TaskState::Running);
    auto pause_result = intervention.pause(running, "operator-a", "inspect sample holder");
    require(pause_result.accepted, "running task must be pausable");
    require(running.state == TaskState::Paused, "pause must update task state");
    require(audit.replay_task_state(running.id.to_string()) == "Paused",
            "pause must persist and audit state");

    auto resume_result = intervention.resume(running, "operator-a", "holder confirmed");
    require(resume_result.accepted, "paused task must be resumable");
    require(running.state == TaskState::Running, "resume must update task state");
    require(audit.replay_task_state(running.id.to_string()) == "Running",
            "resume must persist and audit state");

    auto failed = make_task_in_state(TaskState::Failed);
    failed.error_message = "transient transport error";
    auto retry_result = intervention.retry(failed, "operator-b", "replace cable and retry");
    require(retry_result.accepted, "failed task must be retryable by human");
    require(failed.state == TaskState::Pending, "retry must move failed task to pending");
    require(failed.error_message.empty(), "retry must clear previous error message");
    require(audit.replay_task_state(failed.id.to_string()) == "Pending",
            "retry must persist and audit state");

    auto waiting = make_task_in_state(TaskState::WaitingForHuman);
    auto force_result =
        intervention.force_complete(waiting, "operator-c", "physical result verified");
    require(force_result.accepted, "waiting task must support forced completion");
    require(waiting.state == TaskState::Completed, "force complete must complete task");

    auto pending = make_task_in_state(TaskState::Pending);
    auto cancel_result = intervention.cancel(pending, "operator-d", "sample withdrawn");
    require(cancel_result.accepted, "pending task must be cancellable");
    require(pending.state == TaskState::Cancelled, "cancel must cancel task");

    auto invalid = intervention.pause(pending, "operator-d", "already cancelled");
    require(!invalid.accepted, "terminal task intervention must be rejected");

    auto missing_reason_task = make_task_in_state(TaskState::Pending);
    auto missing_reason = intervention.cancel(missing_reason_task, "operator-e", "");
    require(!missing_reason.accepted, "human intervention reason must be mandatory");

    const auto running_events = audit.events_for_task(running.id.to_string());
    require(has_event_type(running_events, "HumanPaused"), "pause audit event must be present");
    require(has_event_type(running_events, "HumanResumed"), "resume audit event must be present");
    require(intervention_events.size() == 5, "accepted interventions must publish events");
    require(state_events.size() == 5, "accepted interventions must publish state changes");

    db.close();
    std::cout << "intervention service passed\n";
    return 0;
}
