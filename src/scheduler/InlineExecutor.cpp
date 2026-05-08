#include "InlineExecutor.h"

#include "domain/Types.h"

namespace device_automation::scheduler
{

TaskExecutionResult InlineExecutor::execute(device_automation::domain::Task &task)
{
    task.last_checkpoint.stage_name = task.name;
    task.last_checkpoint.saved_at = device_automation::domain::Timestamp::now();
    return TaskExecutionResult{TaskExecutionStatus::Success, "inline executor completed"};
}

} // namespace device_automation::scheduler
