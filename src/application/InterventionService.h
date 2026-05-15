#pragma once

#include "domain/Task.h"
#include "infrastructure/EventBus.h"
#include "infrastructure/IDatabase.h"

#include <string>

namespace device_automation::application
{

struct InterventionResult
{
    bool accepted = false;
    std::string message;
    device_automation::domain::TaskState before_state =
        device_automation::domain::TaskState::Pending;
    device_automation::domain::TaskState after_state =
        device_automation::domain::TaskState::Pending;
};

class InterventionService
{
public:
    InterventionService(device_automation::infrastructure::IDatabase &db,
                        device_automation::infrastructure::EventBus &event_bus);

    InterventionResult pause(device_automation::domain::Task &task, const std::string &actor,
                             const std::string &reason);
    InterventionResult resume(device_automation::domain::Task &task, const std::string &actor,
                              const std::string &reason);
    InterventionResult cancel(device_automation::domain::Task &task, const std::string &actor,
                              const std::string &reason);
    InterventionResult retry(device_automation::domain::Task &task, const std::string &actor,
                             const std::string &reason);
    InterventionResult force_complete(device_automation::domain::Task &task,
                                      const std::string &actor, const std::string &reason);

private:
    InterventionResult apply(device_automation::domain::Task &task,
                             device_automation::domain::TaskState next_state,
                             const std::string &event_type, const std::string &actor,
                             const std::string &reason);
    bool persist_task(const device_automation::domain::Task &task);
    bool record_audit(const device_automation::domain::Task &task,
                      device_automation::domain::TaskState before_state,
                      device_automation::domain::TaskState after_state,
                      const std::string &event_type, const std::string &actor,
                      const std::string &reason);

    device_automation::infrastructure::IDatabase &db_;
    device_automation::infrastructure::EventBus &event_bus_;
};

} // namespace device_automation::application
