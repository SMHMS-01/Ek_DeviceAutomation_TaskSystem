#include "domain/Task.h"
#include "infrastructure/MockDevice.h"
#include "scheduler/DeviceExecutor.h"
#include "scheduler/ExecutorPool.h"
#include "scheduler/InlineExecutor.h"
#include "scheduler/SchedulingPolicy.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace device_automation::domain;
using namespace device_automation::infrastructure;
using namespace device_automation::scheduler;

namespace
{

void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool contains(const std::vector<std::string> &values, const std::string &target)
{
    return std::find(values.begin(), values.end(), target) != values.end();
}

} // namespace

int main()
{
    ExecutorPool pool;
    pool.add_executor("inline-a", std::make_shared<InlineExecutor>());
    pool.add_executor("inline-b", std::make_shared<InlineExecutor>());

    RoundRobinPolicy round_robin;
    std::vector<std::future<ExecutorRunResult>> futures;
    for (int i = 0; i < 6; ++i)
    {
        futures.push_back(
            pool.submit(make_atomic_task("cpu task " + std::to_string(i)), round_robin));
    }

    bool used_a = false;
    bool used_b = false;
    for (auto &future : futures)
    {
        auto result = future.get();
        require(result.result.status == TaskExecutionStatus::Success,
                "inline executor task must succeed");
        used_a = used_a || result.executor_id == "inline-a";
        used_b = used_b || result.executor_id == "inline-b";
    }
    require(used_a && used_b, "round-robin policy must use both inline executors");

    auto snapshots = pool.snapshots();
    for (const auto &snapshot : snapshots)
    {
        require(snapshot.active_tasks == 0, "executor activity counters must drain to zero");
    }

    auto device = std::make_shared<MockDevice>();
    auto device_id = DeviceId("device-A");
    ExecutorPool device_pool;
    ExecutorCapabilities device_capabilities;
    device_capabilities.device_bound = true;
    device_capabilities.device_id = device_id;
    device_pool.add_executor("device-A-executor",
                             std::make_shared<DeviceExecutor>(device_id, device),
                             device_capabilities);

    DeviceAffinityPolicy affinity;
    auto spin = make_atomic_task("spin sample", Priority::High);
    spin.target_device = device_id;
    spin.params["command"] = "spin";
    auto cool = make_atomic_task("cool sample", Priority::Normal);
    cool.target_device = device_id;
    cool.params["command"] = "cool";

    auto spin_result = device_pool.submit(spin, affinity).get();
    auto cool_result = device_pool.submit(cool, affinity).get();

    require(spin_result.executor_id == "device-A-executor",
            "device affinity must choose the device-bound executor");
    require(cool_result.executor_id == "device-A-executor",
            "device affinity must keep matching device tasks on the same executor");
    require(spin_result.result.status == TaskExecutionStatus::Success,
            "first device task must succeed");
    require(cool_result.result.status == TaskExecutionStatus::Success,
            "second device task must succeed");
    require(device->is_connected(), "device executor must connect the device");

    const auto payloads = device->sent_payloads();
    require(payloads.size() == 2, "device executor must send both commands");
    require(contains(payloads, "spin"), "device command list must include spin");
    require(contains(payloads, "cool"), "device command list must include cool");

    std::cout << "executor pool and device executor passed\n";
    return 0;
}
