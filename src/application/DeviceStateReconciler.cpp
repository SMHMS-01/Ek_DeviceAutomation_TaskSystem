#include "DeviceStateReconciler.h"

#include <algorithm>
#include <cctype>

namespace device_automation::application
{

namespace
{

std::string normalized(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool is_waiting_state(device_automation::domain::TaskState state)
{
    using device_automation::domain::TaskState;
    return state == TaskState::Pending || state == TaskState::Ready || state == TaskState::Paused ||
           state == TaskState::WaitingForHuman;
}

} // namespace

ReconcileResult
DeviceStateReconciler::reconcile(const device_automation::domain::Task &task,
                                 device_automation::infrastructure::IDevice &device) const
{
    ReconcileResult result;
    result.fsm_state = task.state;

    try
    {
        result.raw_device_status = device.status();
        result.physical_state = parse_device_state(result.raw_device_status);
    }
    catch (...)
    {
        result.outcome = ReconcileOutcome::Unknown;
        result.message = "device status query failed";
        return result;
    }

    using device_automation::domain::TaskState;
    if (result.physical_state == PhysicalDeviceState::Unknown)
    {
        result.outcome = ReconcileOutcome::Unknown;
        result.message = "device state is unknown";
        return result;
    }

    if ((task.state == TaskState::Completed &&
         result.physical_state == PhysicalDeviceState::Completed) ||
        (task.state == TaskState::Running &&
         result.physical_state == PhysicalDeviceState::Running) ||
        (task.state == TaskState::Failed && result.physical_state == PhysicalDeviceState::Failed) ||
        (is_waiting_state(task.state) && result.physical_state == PhysicalDeviceState::Idle))
    {
        result.outcome = ReconcileOutcome::Consistent;
        result.message = "fsm and device state are consistent";
        return result;
    }

    if (result.physical_state == PhysicalDeviceState::Completed &&
        task.state != TaskState::Completed)
    {
        result.outcome = ReconcileOutcome::DeviceAhead;
        result.message = "device completed before fsm";
        return result;
    }

    if (task.state == TaskState::Completed &&
        result.physical_state != PhysicalDeviceState::Completed)
    {
        result.outcome = ReconcileOutcome::DeviceBehind;
        result.message = "fsm completed before device";
        return result;
    }

    if (task.state == TaskState::Running && result.physical_state == PhysicalDeviceState::Idle)
    {
        result.outcome = ReconcileOutcome::DeviceBehind;
        result.message = "fsm running but device is idle";
        return result;
    }

    result.outcome = ReconcileOutcome::Unknown;
    result.message = "state mismatch requires human review";
    return result;
}

const char *DeviceStateReconciler::outcome_to_string(ReconcileOutcome outcome)
{
    switch (outcome)
    {
    case ReconcileOutcome::Consistent:
        return "Consistent";
    case ReconcileOutcome::DeviceAhead:
        return "DeviceAhead";
    case ReconcileOutcome::DeviceBehind:
        return "DeviceBehind";
    case ReconcileOutcome::Unknown:
        return "Unknown";
    default:
        return "Unknown";
    }
}

const char *DeviceStateReconciler::physical_state_to_string(PhysicalDeviceState state)
{
    switch (state)
    {
    case PhysicalDeviceState::Idle:
        return "Idle";
    case PhysicalDeviceState::Running:
        return "Running";
    case PhysicalDeviceState::Completed:
        return "Completed";
    case PhysicalDeviceState::Failed:
        return "Failed";
    case PhysicalDeviceState::Unknown:
        return "Unknown";
    default:
        return "Unknown";
    }
}

PhysicalDeviceState DeviceStateReconciler::parse_device_state(const std::string &raw_status)
{
    const auto state = normalized(raw_status);
    if (state == "idle" || state == "pending" || state == "ready" || state == "paused")
    {
        return PhysicalDeviceState::Idle;
    }
    if (state == "running" || state == "busy")
    {
        return PhysicalDeviceState::Running;
    }
    if (state == "completed" || state == "complete" || state == "done")
    {
        return PhysicalDeviceState::Completed;
    }
    if (state == "failed" || state == "error")
    {
        return PhysicalDeviceState::Failed;
    }
    return PhysicalDeviceState::Unknown;
}

} // namespace device_automation::application
