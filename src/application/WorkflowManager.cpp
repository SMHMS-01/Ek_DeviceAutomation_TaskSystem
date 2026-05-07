#include "WorkflowManager.h"

#include <utility>

namespace device_automation::application {

WorkflowManager::WorkflowManager(device_automation::scheduler::SimpleScheduler& scheduler)
    : scheduler_(scheduler)
{
}

bool WorkflowManager::submit_and_run(device_automation::domain::TaskGraph& graph,
                                     device_automation::scheduler::TaskHandler handler)
{
    scheduler_.initialize_storage();
    graph.state = device_automation::domain::GraphState::Submitted;
    return scheduler_.run(graph, std::move(handler));
}

} // namespace device_automation::application
