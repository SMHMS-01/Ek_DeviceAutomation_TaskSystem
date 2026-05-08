#pragma once

#include "Priority.h"
#include "TaskState.h"
#include "Types.h"

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace device_automation::domain
{

struct RetryPolicy
{
    enum class Backoff
    {
        Fixed,
        Linear,
        Exponential
    };

    int max_attempts = 3;
    bool auto_retry = true;
    Backoff backoff = Backoff::Exponential;
    std::chrono::milliseconds base_delay{500};
    std::chrono::milliseconds max_delay{30000};
};

struct CheckPoint
{
    TaskId task_id;
    int stage_index = 0;
    std::string stage_name;
    std::map<std::string, std::string> context;
    Timestamp saved_at;
    std::string saved_by = "system";
};

struct Task
{
    TaskId id;
    std::string name;
    TaskKind kind = TaskKind::Atomic;
    Priority priority = Priority::Normal;
    std::vector<TaskId> dependencies;
    std::vector<TaskId> dependents;
    std::map<std::string, std::string> params;
    DeviceId target_device;
    TaskState state = TaskState::Pending;
    RetryPolicy retry_policy;
    CheckPoint last_checkpoint;
    Timestamp created_at;
    Timestamp scheduled_at;
    Timestamp started_at;
    Timestamp finished_at;
    std::string created_by = "system";
    WorkflowId workflow_id;
    int version = 1;
    int attempts = 0;
    std::string error_message;
};

Task make_atomic_task(std::string name, Priority priority = Priority::Normal);

} // namespace device_automation::domain
