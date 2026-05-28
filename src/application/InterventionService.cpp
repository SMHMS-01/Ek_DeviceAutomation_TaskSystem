#include "InterventionService.h"

#include "domain/TaskStateMachine.h"

#include <algorithm>
#include <sstream>
#include <utility>

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

bool has_permission(const InterventionRequest &request, InterventionPermission permission)
{
    return std::find(request.permissions.begin(), request.permissions.end(), permission) !=
           request.permissions.end();
}

InterventionRequest trusted_request(std::string actor, std::string reason,
                                    InterventionPermission permission)
{
    InterventionRequest request;
    request.actor = std::move(actor);
    request.reason = std::move(reason);
    request.confirmed = true;
    request.permissions.push_back(permission);
    return request;
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
    return pause(task, trusted_request(actor, reason, InterventionPermission::TaskControl));
}

InterventionResult InterventionService::resume(device_automation::domain::Task &task,
                                               const std::string &actor, const std::string &reason)
{
    return resume(task, trusted_request(actor, reason, InterventionPermission::TaskControl));
}

InterventionResult InterventionService::cancel(device_automation::domain::Task &task,
                                               const std::string &actor, const std::string &reason)
{
    return cancel(task, trusted_request(actor, reason, InterventionPermission::Cancel));
}

InterventionResult InterventionService::retry(device_automation::domain::Task &task,
                                              const std::string &actor, const std::string &reason)
{
    return retry(task, trusted_request(actor, reason, InterventionPermission::Retry));
}

InterventionResult InterventionService::force_complete(device_automation::domain::Task &task,
                                                       const std::string &actor,
                                                       const std::string &reason)
{
    return force_complete(task,
                          trusted_request(actor, reason, InterventionPermission::ForceComplete));
}

InterventionResult InterventionService::rollback(device_automation::domain::Task &task,
                                                 const std::string &actor,
                                                 const std::string &reason)
{
    return rollback(task, trusted_request(actor, reason, InterventionPermission::Rollback));
}

InterventionResult InterventionService::pause(device_automation::domain::Task &task,
                                              const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::Paused, "HumanPaused",
                 InterventionPermission::TaskControl, false, request);
}

InterventionResult InterventionService::resume(device_automation::domain::Task &task,
                                               const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::Running, "HumanResumed",
                 InterventionPermission::TaskControl, false, request);
}

InterventionResult InterventionService::cancel(device_automation::domain::Task &task,
                                               const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::Cancelled, "HumanCancelled",
                 InterventionPermission::Cancel, true, request);
}

InterventionResult InterventionService::retry(device_automation::domain::Task &task,
                                              const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::Pending, "HumanRetried",
                 InterventionPermission::Retry, false, request);
}

InterventionResult InterventionService::force_complete(device_automation::domain::Task &task,
                                                       const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::Completed, "HumanForcedComplete",
                 InterventionPermission::ForceComplete, true, request);
}

InterventionResult InterventionService::rollback(device_automation::domain::Task &task,
                                                 const InterventionRequest &request)
{
    return apply(task, device_automation::domain::TaskState::RollingBack, "HumanRollbackStarted",
                 InterventionPermission::Rollback, true, request);
}

InterventionResult InterventionService::apply(device_automation::domain::Task &task,
                                              device_automation::domain::TaskState next_state,
                                              const std::string &event_type,
                                              InterventionPermission permission,
                                              bool confirmation_required,
                                              const InterventionRequest &request)
{
    const auto before_state = task.state;
    const auto before_task = task;
    InterventionResult result{false, "", before_state, next_state};
    if (request.actor.empty())
    {
        result.message = "actor is required";
        return result;
    }
    if (request.reason.empty())
    {
        result.message = "reason is required";
        return result;
    }
    if (!has_permission(request, permission))
    {
        result.message = "permission denied";
        return result;
    }
    if (confirmation_required && !request.confirmed)
    {
        result.message = "confirmation is required";
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
        task.last_checkpoint.saved_by = request.actor;
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
    else if (event_type == "HumanRollbackStarted")
    {
        task.last_checkpoint.task_id = task.id;
        task.last_checkpoint.saved_at = device_automation::domain::Timestamp::now();
        task.last_checkpoint.saved_by = request.actor;
    }

    task.state = next_state;
    if (!db_.begin_transaction())
    {
        task = before_task;
        result.message = "database transaction begin failed";
        return result;
    }

    if (!persist_task(task) ||
        !record_audit(task, before_state, next_state, event_type, request.actor, request.reason))
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
