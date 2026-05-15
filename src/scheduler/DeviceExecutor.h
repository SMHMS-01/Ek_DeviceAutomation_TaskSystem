#pragma once

#include "IExecutor.h"
#include "infrastructure/IDevice.h"

#include <memory>
#include <mutex>
#include <string>

namespace device_automation::scheduler
{

class DeviceExecutor : public IExecutor
{
public:
    DeviceExecutor(device_automation::domain::DeviceId device_id,
                   std::shared_ptr<device_automation::infrastructure::IDevice> device);

    TaskExecutionResult execute(device_automation::domain::Task &task) override;
    const device_automation::domain::DeviceId &device_id() const;

private:
    std::string command_for(const device_automation::domain::Task &task) const;

    device_automation::domain::DeviceId device_id_;
    std::shared_ptr<device_automation::infrastructure::IDevice> device_;
    std::mutex mu_;
    bool connected_ = false;
};

} // namespace device_automation::scheduler
