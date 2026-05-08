#pragma once

#include "SimpleScheduler.h"
#include "domain/Task.h"

namespace device_automation::scheduler
{

class IExecutor
{
public:
    virtual ~IExecutor() = default;

    virtual TaskExecutionResult execute(device_automation::domain::Task &task) = 0;
};

} // namespace device_automation::scheduler
