#include "InterventionService.h"

#include "domain/TaskStateMachine.h"

#include <sstream>

namespace device_automation::application
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

InterventionService::InterventionService(device_automation::infrastructure::IDatabase &db,
                                         device_automation::infrastructure::EventBus &event_bus)
    : db_(db), event_bus_(event_bus)
{
}

InterventionResult InterventionService::pause(device_automation::domain::Task &task,
                                              const std::string &actor, const std::string &reason)
{
    return apply(task, device_automation::domain::TaskState::Paused, "HumanPaused", actor, reason);
}

InterventionResult InterventionService::resume(device_automation::domain::Task &task,
                                               const std::string &actor, const std::string &reason)
{
    return apply(task, device_automation::domain::TaskState::Running, "HumanResumed", actor,
                 reason);
}

InterventionResult InterventionService::cancel(device_automation::domain::Task &task,
                                               const std::string &actor, const std::string &reason)
{
    return apply(task, device_automation::domain::TaskState::Cancelled, "HumanCancelled", actor,
                 reason);
}

InterventionResult InterventionService::retry(device_automation::domain::Task &task,
                                              const std::string &actor, const std::string &reason)
{
    return apply(task, device_automation::domain::TaskState::Pending, "HumanRetried", actor,
                 reason);
}

InterventionResult InterventionService::force_complete(device_automation::domain::Task &task,
                                                       const std::string &actor,
                                                       const std::string &reason)
{
    return apply(task, device_automation::domain::TaskState::Completed, "HumanForcedComplete",
                 actor, reason);
}

InterventionResult InterventionService::apply(device_automation::domain::Task &task,
                                              device_automation::domain::TaskState next_state,
                                              const std::string &event_type,
                                              const std::string &actor, const std::string &reason)
{
    const auto before_state = task.state;
    const auto before_task = task;
    InterventionResult result{false, "", before_state, next_state};
    if (actor.empty())
    {
        result.message = "actor is required";
        return result;
    }
    if (reason.empty())
    {
        result.message = "reason is required";
        return result;
    }

    device_automation::domain::TaskStateMachine validator(before_state);
    if (!validator.can_transition_to(next_state))
    {
        result.message =
            std::string("invalid intervention transition: ") +
            device_automation::domain::TaskStateMachine::state_to_string(before_state) + " -> " +
            device_automation::domain::TaskStateMachine::state_to_string(next_state);
        return result;
    }

    if (event_type == "HumanPaused")
    {
        task.last_checkpoint.task_id = task.id;
        task.last_checkpoint.saved_at = device_automation::domain::Timestamp::now();
        task.last_checkpoint.saved_by = actor;
    }
    else if (event_type == "HumanResumed")
    {
        task.started_at = device_automation::domain::Timestamp::now();
    }
    else if (event_type == "HumanCancelled" || event_type == "HumanForcedComplete")
    {
        task.finished_at = device_automation::domain::Timestamp::now();
    }
    else if (event_type == "HumanRetried")
    {
        task.error_message.clear();
        task.scheduled_at = device_automation::domain::Timestamp::now();
    }

    task.state = next_state;
    if (!db_.begin_transaction())
    {
        task = before_task;
        result.message = "database transaction begin failed";
        return result;
    }

    if (!persist_task(task) ||
        !record_audit(task, before_state, next_state, event_type, actor, reason))
    {
        db_.rollback_transaction();
        task = before_task;
        result.message = "database persistence failed";
        return result;
    }

    if (!db_.commit_transaction())
    {
        db_.rollback_transaction();
        task = before_task;
        result.message = "database transaction commit failed";
        return result;
    }

    event_bus_.publish(
        "task.state_changed",
        task.id.to_string() + ":" +
            device_automation::domain::TaskStateMachine::state_to_string(next_state));
    event_bus_.publish("task.intervention", task.id.to_string() + ":" + event_type);

    result.accepted = true;
    result.message = "accepted";
    return result;
}

bool InterventionService::persist_task(const device_automation::domain::Task &task)
{
    std::ostringstream sql;
    sql << "INSERT INTO tasks(id, workflow_id, name, state, priority, retry_count, error_message) "
        << "VALUES(" << quote(task.id.to_string()) << ", " << quote(task.workflow_id.to_string())
        << ", " << quote(task.name) << ", "
        << quote(device_automation::domain::TaskStateMachine::state_to_string(task.state)) << ", "
        << static_cast<int>(task.priority) << ", " << task.attempts << ", "
        << quote(task.error_message) << ") "
        << "ON CONFLICT(id) DO UPDATE SET state=excluded.state, retry_count=excluded.retry_count, "
        << "error_message=excluded.error_message";
    return db_.execute(sql.str());
}

bool InterventionService::record_audit(const device_automation::domain::Task &task,
                                       device_automation::domain::TaskState before_state,
                                       device_automation::domain::TaskState after_state,
                                       const std::string &event_type, const std::string &actor,
                                       const std::string &reason)
{
    std::ostringstream sql;
    sql << "INSERT INTO audit_events(id, type, task_id, workflow_id, actor, before_state, "
           "after_state, reason, occurred_at) VALUES("
        << quote(device_automation::domain::EventId::generate().to_string()) << ", "
        << quote(event_type) << ", " << quote(task.id.to_string()) << ", "
        << quote(task.workflow_id.to_string()) << ", " << quote(actor) << ", "
        << quote(device_automation::domain::TaskStateMachine::state_to_string(before_state)) << ", "
        << quote(device_automation::domain::TaskStateMachine::state_to_string(after_state)) << ", "
        << quote(reason) << ", " << device_automation::domain::Timestamp::now().millis() << ")";
    return db_.execute(sql.str());
}

} // namespace device_automation::application
