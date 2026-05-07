#pragma once

#include "domain/TaskGraph.h"
#include "scheduler/SimpleScheduler.h"

namespace device_automation::application {

class WorkflowManager
{
public:
    explicit WorkflowManager(device_automation::scheduler::SimpleScheduler& scheduler);

    bool submit_and_run(device_automation::domain::TaskGraph& graph,
                        device_automation::scheduler::TaskHandler handler);

private:
    device_automation::scheduler::SimpleScheduler& scheduler_;
};

} // namespace device_automation::application
