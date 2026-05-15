#include "SchedulingPolicy.h"

#include <algorithm>

namespace device_automation::scheduler
{

namespace
{

std::optional<std::string> least_loaded(const std::vector<ExecutorSnapshot> &executors)
{
    if (executors.empty())
    {
        return std::nullopt;
    }
    auto selected = std::min_element(executors.begin(), executors.end(),
                                     [](const auto &lhs, const auto &rhs)
                                     {
                                         if (lhs.active_tasks == rhs.active_tasks)
                                         {
                                             return lhs.executor_id < rhs.executor_id;
                                         }
                                         return lhs.active_tasks < rhs.active_tasks;
                                     });
    return selected->executor_id;
}

} // namespace

std::optional<std::string>
RoundRobinPolicy::select_executor(const device_automation::domain::Task &,
                                  const std::vector<ExecutorSnapshot> &executors)
{
    if (executors.empty())
    {
        return std::nullopt;
    }
    const auto index = next_ % executors.size();
    ++next_;
    return executors[index].executor_id;
}

std::optional<std::string>
PriorityFirstPolicy::select_executor(const device_automation::domain::Task &,
                                     const std::vector<ExecutorSnapshot> &executors)
{
    return least_loaded(executors);
}

std::optional<std::string>
DeviceAffinityPolicy::select_executor(const device_automation::domain::Task &task,
                                      const std::vector<ExecutorSnapshot> &executors)
{
    if (!task.target_device.is_empty())
    {
        std::vector<ExecutorSnapshot> matching_executors;
        for (const auto &executor : executors)
        {
            if (executor.capabilities.device_bound &&
                executor.capabilities.device_id == task.target_device)
            {
                matching_executors.push_back(executor);
            }
        }
        if (!matching_executors.empty())
        {
            return least_loaded(matching_executors);
        }
    }
    return least_loaded(executors);
}

} // namespace device_automation::scheduler
