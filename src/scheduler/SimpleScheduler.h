#pragma once

#include "domain/TaskGraph.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/IDatabase.h"

#include <functional>
#include <string>
#include <vector>

namespace device_automation::scheduler {

enum class TaskExecutionStatus { Success, RetryableFailure, PermanentFailure };

struct TaskExecutionResult
{
    TaskExecutionStatus status = TaskExecutionStatus::Success;
    std::string message;
};

using TaskHandler = std::function<TaskExecutionResult(device_automation::domain::Task&)>;

struct AuditRecord
{
    std::string task_id;
    std::string event_type;
    std::string before_state;
    std::string after_state;
    std::string reason;
};

class SimpleScheduler
{
public:
    SimpleScheduler(device_automation::infrastructure::IDatabase& db,
                    device_automation::infrastructure::EventBus& event_bus);

    void initialize_storage();
    bool run(device_automation::domain::TaskGraph& graph, TaskHandler handler);
    const std::vector<AuditRecord>& audit_records() const { return audit_records_; }

private:
    void transition(device_automation::domain::Task& task,
                    device_automation::domain::TaskState next,
                    const std::string& reason);
    void record_audit(const device_automation::domain::Task& task,
                      device_automation::domain::TaskState before,
                      device_automation::domain::TaskState after,
                      const std::string& reason);

    device_automation::infrastructure::IDatabase& db_;
    device_automation::infrastructure::EventBus& event_bus_;
    std::vector<AuditRecord> audit_records_;
};

} // namespace device_automation::scheduler
