#include "application/DeviceStateReconciler.h"
#include "domain/Task.h"
#include "infrastructure/MockDevice.h"

#include <iostream>
#include <stdexcept>

using namespace device_automation::application;
using namespace device_automation::domain;
using namespace device_automation::infrastructure;

namespace
{

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Task make_task_in_state(TaskState state)
{
    auto task = make_atomic_task("reconcile target", Priority::Normal);
    task.state = state;
    return task;
}

} // namespace

int main()
{
    DeviceStateReconciler reconciler;
    MockDevice device;

    auto running = make_task_in_state(TaskState::Running);
    device.set_status("Running");
    auto consistent = reconciler.reconcile(running, device);
    require(consistent.outcome == ReconcileOutcome::Consistent,
            "running fsm and running device must be consistent");

    auto running_but_completed = make_task_in_state(TaskState::Running);
    device.set_status("Completed");
    auto ahead = reconciler.reconcile(running_but_completed, device);
    require(ahead.outcome == ReconcileOutcome::DeviceAhead,
            "completed device with running fsm must be device-ahead");

    auto completed_but_idle = make_task_in_state(TaskState::Completed);
    device.set_status("Idle");
    auto behind = reconciler.reconcile(completed_but_idle, device);
    require(behind.outcome == ReconcileOutcome::DeviceBehind,
            "completed fsm with idle device must be device-behind");

    auto waiting = make_task_in_state(TaskState::WaitingForHuman);
    device.set_status("garbled-state");
    auto unknown = reconciler.reconcile(waiting, device);
    require(unknown.outcome == ReconcileOutcome::Unknown,
            "unrecognized device state must require human review");

    std::cout << "device state reconciler passed\n";
    return 0;
}
