#include "ExecutorPool.h"

#include <stdexcept>
#include <utility>

namespace device_automation::scheduler
{

void ExecutorPool::add_executor(std::string executor_id, std::shared_ptr<IExecutor> executor)
{
    ExecutorCapabilities capabilities;
    add_executor(std::move(executor_id), std::move(executor), capabilities);
}

void ExecutorPool::add_executor(std::string executor_id, std::shared_ptr<IExecutor> executor,
                                ExecutorCapabilities capabilities)
{
    if (!executor)
    {
        throw std::invalid_argument("executor must not be null");
    }

    std::lock_guard<std::mutex> lock(mu_);
    executors_.push_back(ExecutorSlot{std::move(executor_id), std::move(executor), capabilities,
                                      std::make_shared<std::atomic<std::size_t>>(0)});
}

std::future<ExecutorRunResult> ExecutorPool::submit(device_automation::domain::Task task,
                                                    SchedulingPolicy &policy)
{
    auto slot = select_slot(task, policy);

    try
    {
        return std::async(std::launch::async,
                          [slot, task = std::move(task)]() mutable
                          {
                              ExecutorRunResult run_result;
                              run_result.executor_id = slot.id;
                              run_result.task_id = task.id;
                              try
                              {
                                  run_result.result = slot.executor->execute(task);
                              }
                              catch (const std::exception &error)
                              {
                                  run_result.result = TaskExecutionResult{
                                      TaskExecutionStatus::PermanentFailure, error.what()};
                              }
                              catch (...)
                              {
                                  run_result.result =
                                      TaskExecutionResult{TaskExecutionStatus::PermanentFailure,
                                                          "unknown executor error"};
                              }
                              slot.active_tasks->fetch_sub(1);
                              return run_result;
                          });
    }
    catch (...)
    {
        slot.active_tasks->fetch_sub(1);
        throw;
    }
}

std::vector<ExecutorSnapshot> ExecutorPool::snapshots() const
{
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<ExecutorSnapshot> snapshots;
    snapshots.reserve(executors_.size());
    for (const auto &executor : executors_)
    {
        snapshots.push_back(
            ExecutorSnapshot{executor.id, executor.active_tasks->load(), executor.capabilities});
    }
    return snapshots;
}

std::size_t ExecutorPool::size() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return executors_.size();
}

ExecutorPool::ExecutorSlot ExecutorPool::select_slot(const device_automation::domain::Task &task,
                                                     SchedulingPolicy &policy)
{
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<ExecutorSnapshot> snapshots;
    snapshots.reserve(executors_.size());
    for (const auto &executor : executors_)
    {
        snapshots.push_back(
            ExecutorSnapshot{executor.id, executor.active_tasks->load(), executor.capabilities});
    }

    auto selected_id = policy.select_executor(task, snapshots);
    if (!selected_id)
    {
        throw std::runtime_error("no executor available");
    }

    for (const auto &executor : executors_)
    {
        if (executor.id == *selected_id)
        {
            executor.active_tasks->fetch_add(1);
            return executor;
        }
    }
    throw std::runtime_error("scheduling policy selected an unknown executor");
}

} // namespace device_automation::scheduler
