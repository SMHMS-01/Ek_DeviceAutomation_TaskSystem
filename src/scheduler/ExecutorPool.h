#pragma once

#include "IExecutor.h"
#include "SchedulingPolicy.h"

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace device_automation::scheduler
{

struct ExecutorRunResult
{
    std::string executor_id;
    device_automation::domain::TaskId task_id;
    TaskExecutionResult result;
};

class ExecutorPool
{
public:
    void add_executor(std::string executor_id, std::shared_ptr<IExecutor> executor);
    void add_executor(std::string executor_id, std::shared_ptr<IExecutor> executor,
                      ExecutorCapabilities capabilities);

    std::future<ExecutorRunResult> submit(device_automation::domain::Task task,
                                          SchedulingPolicy &policy);

    std::vector<ExecutorSnapshot> snapshots() const;
    std::size_t size() const;

private:
    struct ExecutorSlot
    {
        std::string id;
        std::shared_ptr<IExecutor> executor;
        ExecutorCapabilities capabilities;
        std::shared_ptr<std::atomic<std::size_t>> active_tasks;
    };

    ExecutorSlot select_slot(const device_automation::domain::Task &task, SchedulingPolicy &policy);

    mutable std::mutex mu_;
    std::vector<ExecutorSlot> executors_;
};

} // namespace device_automation::scheduler
