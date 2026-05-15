#include "DeviceExecutor.h"

#include "domain/Types.h"

#include <stdexcept>
#include <utility>

namespace device_automation::scheduler
{

DeviceExecutor::DeviceExecutor(device_automation::domain::DeviceId device_id,
                               std::shared_ptr<device_automation::infrastructure::IDevice> device)
    : device_id_(std::move(device_id)), device_(std::move(device))
{
    if (!device_)
    {
        throw std::invalid_argument("device must not be null");
    }
}

TaskExecutionResult DeviceExecutor::execute(device_automation::domain::Task &task)
{
    std::lock_guard<std::mutex> lock(mu_);
    if (!connected_)
    {
        connected_ = device_->connect();
        if (!connected_)
        {
            return TaskExecutionResult{TaskExecutionStatus::RetryableFailure,
                                       "device connection failed"};
        }
    }

    const auto command = command_for(task);
    if (!device_->send(command))
    {
        return TaskExecutionResult{TaskExecutionStatus::RetryableFailure, "device send failed"};
    }

    task.last_checkpoint.task_id = task.id;
    task.last_checkpoint.stage_name = "device:" + device_id_.to_string();
    task.last_checkpoint.context["command"] = command;
    task.last_checkpoint.context["response"] = device_->receive();
    task.last_checkpoint.saved_at = device_automation::domain::Timestamp::now();
    return TaskExecutionResult{TaskExecutionStatus::Success, "device executor completed"};
}

const device_automation::domain::DeviceId &DeviceExecutor::device_id() const
{
    return device_id_;
}

std::string DeviceExecutor::command_for(const device_automation::domain::Task &task) const
{
    auto command = task.params.find("command");
    if (command != task.params.end())
    {
        return command->second;
    }
    return task.name;
}

} // namespace device_automation::scheduler
