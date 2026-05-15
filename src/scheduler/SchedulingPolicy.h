#pragma once

#include "domain/Task.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace device_automation::scheduler
{

struct ExecutorCapabilities
{
    bool device_bound = false;
    device_automation::domain::DeviceId device_id;
};

struct ExecutorSnapshot
{
    std::string executor_id;
    std::size_t active_tasks = 0;
    ExecutorCapabilities capabilities;
};

class SchedulingPolicy
{
public:
    virtual ~SchedulingPolicy() = default;

    virtual std::optional<std::string>
    select_executor(const device_automation::domain::Task &task,
                    const std::vector<ExecutorSnapshot> &executors) = 0;
};

class RoundRobinPolicy : public SchedulingPolicy
{
public:
    std::optional<std::string>
    select_executor(const device_automation::domain::Task &task,
                    const std::vector<ExecutorSnapshot> &executors) override;

private:
    std::size_t next_ = 0;
};

class PriorityFirstPolicy : public SchedulingPolicy
{
public:
    std::optional<std::string>
    select_executor(const device_automation::domain::Task &task,
                    const std::vector<ExecutorSnapshot> &executors) override;
};

class DeviceAffinityPolicy : public SchedulingPolicy
{
public:
    std::optional<std::string>
    select_executor(const device_automation::domain::Task &task,
                    const std::vector<ExecutorSnapshot> &executors) override;
};

} // namespace device_automation::scheduler
