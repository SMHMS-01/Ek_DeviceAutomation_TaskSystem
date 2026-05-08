#include "SimpleScheduler.h"

#include "domain/TaskStateMachine.h"
#include "infrastructure/SchemaMigration.h"

#include <sstream>

namespace device_automation::scheduler
{

using device_automation::domain::Task;
using device_automation::domain::TaskState;
using device_automation::domain::TaskStateMachine;
using device_automation::domain::Timestamp;

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

SimpleScheduler::SimpleScheduler(device_automation::infrastructure::IDatabase &db,
                                 device_automation::infrastructure::EventBus &event_bus)
    : db_(db), event_bus_(event_bus)
{
}

void SimpleScheduler::initialize_storage()
{
    device_automation::infrastructure::MigrationRunner runner(db_);
    runner.apply({device_automation::infrastructure::SchemaMigration{
        1,
        "core_tasks_and_audit",
        {"CREATE TABLE IF NOT EXISTS tasks ("
         "id TEXT PRIMARY KEY, workflow_id TEXT NOT NULL, name TEXT, state TEXT NOT NULL, "
         "priority INTEGER, retry_count INTEGER DEFAULT 0, error_message TEXT)",
         "CREATE TABLE IF NOT EXISTS audit_events ("
         "id TEXT PRIMARY KEY, type TEXT NOT NULL, task_id TEXT, workflow_id TEXT, actor TEXT, "
         "before_state TEXT, after_state TEXT, reason TEXT, occurred_at INTEGER NOT NULL) "
         "STRICT"}}});
}

bool SimpleScheduler::run(device_automation::domain::TaskGraph &graph, TaskHandler handler)
{
    graph.state = device_automation::domain::GraphState::Running;
    while (!graph.all_completed())
    {
        auto ready = graph.ready_tasks();
        if (ready.empty())
        {
            graph.state = device_automation::domain::GraphState::Failed;
            return false;
        }

        for (const auto &id : ready)
        {
            auto &task = graph.task(id);
            transition(task, TaskState::Ready, "dependencies met");
            transition(task, TaskState::Running, "executor assigned");
            task.started_at = Timestamp::now();

            auto result = handler(task);
            if (result.status == TaskExecutionStatus::Success)
            {
                task.finished_at = Timestamp::now();
                transition(task, TaskState::Completed,
                           result.message.empty() ? "done" : result.message);
                continue;
            }

            task.error_message = result.message;
            transition(task, TaskState::Failed, result.message);
            ++task.attempts;
            if (result.status == TaskExecutionStatus::RetryableFailure &&
                task.retry_policy.auto_retry && task.attempts < task.retry_policy.max_attempts)
            {
                transition(task, TaskState::Pending, "auto retry");
            }
            else
            {
                transition(task, TaskState::WaitingForHuman,
                           "retry exhausted or permanent failure");
                graph.state = device_automation::domain::GraphState::Failed;
                return false;
            }
        }
    }
    graph.state = device_automation::domain::GraphState::Completed;
    return true;
}

void SimpleScheduler::transition(Task &task, TaskState next, const std::string &reason)
{
    TaskStateMachine validator(task.state);
    if (!validator.can_transition_to(next))
    {
        throw device_automation::domain::InvalidStateTransitionException(task.state, next, reason);
    }

    auto before = task.state;
    task.state = next;
    record_audit(task, before, next, reason);

    std::ostringstream sql;
    sql << "INSERT INTO tasks(id, workflow_id, name, state, priority, retry_count, error_message) "
        << "VALUES(" << quote(task.id.to_string()) << ", " << quote(task.workflow_id.to_string())
        << ", " << quote(task.name) << ", " << quote(TaskStateMachine::state_to_string(task.state))
        << ", " << static_cast<int>(task.priority) << ", " << task.attempts << ", "
        << quote(task.error_message) << ") "
        << "ON CONFLICT(id) DO UPDATE SET state=excluded.state, retry_count=excluded.retry_count, "
        << "error_message=excluded.error_message";
    db_.execute(sql.str());
}

void SimpleScheduler::record_audit(const Task &task, TaskState before, TaskState after,
                                   const std::string &reason)
{
    AuditRecord record{task.id.to_string(), "TaskStateChanged",
                       TaskStateMachine::state_to_string(before),
                       TaskStateMachine::state_to_string(after), reason};
    audit_records_.push_back(record);
    event_bus_.publish("task.state_changed", record.task_id + ":" + record.after_state);

    std::ostringstream sql;
    sql << "INSERT INTO audit_events(id, type, task_id, workflow_id, actor, before_state, "
           "after_state, "
           "reason, occurred_at) VALUES("
        << quote(device_automation::domain::EventId::generate().to_string()) << ", "
        << quote(record.event_type) << ", " << quote(record.task_id) << ", "
        << quote(task.workflow_id.to_string()) << ", 'system', " << quote(record.before_state)
        << ", " << quote(record.after_state) << ", " << quote(record.reason) << ", "
        << Timestamp::now().millis() << ")";
    db_.execute(sql.str());
}

} // namespace device_automation::scheduler
