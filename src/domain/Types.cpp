#include "Types.h"

#include <random>
#include <sstream>
#include <chrono>

namespace device_automation::domain {

namespace {

/**
 * @brief 生成 UUID v4 字符串
 */
std::string generate_uuid_v4()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        int d = dis(gen);
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        ss << "0123456789abcdef"[d];
    }
    return ss.str();
}

} // namespace

// ============================================================================
// TaskId
// ============================================================================

TaskId TaskId::generate()
{
    return TaskId(generate_uuid_v4());
}

// ============================================================================
// WorkflowId
// ============================================================================

WorkflowId WorkflowId::generate()
{
    return WorkflowId(generate_uuid_v4());
}

// ============================================================================
// ExecutorId
// ============================================================================

ExecutorId ExecutorId::generate()
{
    return ExecutorId(generate_uuid_v4());
}

// ============================================================================
// DeviceId
// ============================================================================

DeviceId DeviceId::generate()
{
    return DeviceId(generate_uuid_v4());
}

// ============================================================================
// EventId
// ============================================================================

EventId EventId::generate()
{
    return EventId(generate_uuid_v4());
}

// ============================================================================
// Timestamp
// ============================================================================

Timestamp Timestamp::now()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return Timestamp(millis);
}

} // namespace device_automation::domain
