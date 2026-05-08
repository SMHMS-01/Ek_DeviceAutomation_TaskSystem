#include "Task.h"

#include <utility>

namespace device_automation::domain
{

Task make_atomic_task(std::string name, Priority priority)
{
    Task task;
    task.id = TaskId::generate();
    task.name = std::move(name);
    task.kind = TaskKind::Atomic;
    task.priority = priority;
    task.state = TaskState::Pending;
    task.created_at = Timestamp::now();
    task.last_checkpoint.task_id = task.id;
    return task;
}

} // namespace device_automation::domain
