#pragma once

#include "IExecutor.h"

namespace device_automation::scheduler
{

class InlineExecutor : public IExecutor
{
public:
    TaskExecutionResult execute(device_automation::domain::Task &task) override;
};

} // namespace device_automation::scheduler
