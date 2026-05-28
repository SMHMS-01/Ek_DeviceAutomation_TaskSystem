#pragma once

#include "domain/Task.h"
#include "infrastructure/IDevice.h"

#include <string>

namespace device_automation::application
{

enum class PhysicalDeviceState
{
    Idle,
    Running,
    Completed,
    Failed,
    Unknown
};

enum class ReconcileOutcome
{
    Consistent,
    DeviceAhead,
    DeviceBehind,
    Unknown
};

struct ReconcileResult
{
    ReconcileOutcome outcome = ReconcileOutcome::Unknown;
    PhysicalDeviceState physical_state = PhysicalDeviceState::Unknown;
    device_automation::domain::TaskState fsm_state = device_automation::domain::TaskState::Pending;
    std::string raw_device_status;
    std::string message;
};

class DeviceStateReconciler
{
public:
    ReconcileResult reconcile(const device_automation::domain::Task &task,
                              device_automation::infrastructure::IDevice &device) const;

    static const char *outcome_to_string(ReconcileOutcome outcome);
    static const char *physical_state_to_string(PhysicalDeviceState state);

private:
    static PhysicalDeviceState parse_device_state(const std::string &raw_status);
};

} // namespace device_automation::application
