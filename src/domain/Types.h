#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace device_automation::domain {

// ============================================================================
// 基础类型定义
// ============================================================================

/**
 * @brief 任务 ID - 全局唯一标识
 */
class TaskId
{
public:
    /**
     * @brief 生成新的 TaskId
     * @return 新的 TaskId 实例
     */
    static TaskId generate();

    explicit TaskId(const std::string& uuid = "") : uuid_(uuid) {}

    /**
     * @brief 获取 UUID 字符串
     */
    const std::string& to_string() const { return uuid_; }

    /**
     * @brief 比较两个 TaskId
     */
    bool operator==(const TaskId& other) const { return uuid_ == other.uuid_; }
    bool operator!=(const TaskId& other) const { return !(*this == other); }
    bool operator<(const TaskId& other) const { return uuid_ < other.uuid_; }

private:
    std::string uuid_;
};

/**
 * @brief 工作流 ID
 */
class WorkflowId
{
public:
    static WorkflowId generate();

    explicit WorkflowId(const std::string& uuid = "") : uuid_(uuid) {}

    const std::string& to_string() const { return uuid_; }

    bool operator==(const WorkflowId& other) const { return uuid_ == other.uuid_; }
    bool operator!=(const WorkflowId& other) const { return !(*this == other); }

private:
    std::string uuid_;
};

/**
 * @brief 执行器 ID
 */
class ExecutorId
{
public:
    static ExecutorId generate();

    explicit ExecutorId(const std::string& id = "") : id_(id) {}
    explicit ExecutorId(int id) : id_(std::to_string(id)) {}

    const std::string& to_string() const { return id_; }

    bool operator==(const ExecutorId& other) const { return id_ == other.id_; }
    bool operator!=(const ExecutorId& other) const { return !(*this == other); }

private:
    std::string id_;
};

/**
 * @brief 设备 ID
 */
class DeviceId
{
public:
    static DeviceId generate();

    explicit DeviceId(const std::string& id = "") : id_(id) {}

    const std::string& to_string() const { return id_; }

    bool operator==(const DeviceId& other) const { return id_ == other.id_; }
    bool operator!=(const DeviceId& other) const { return !(*this == other); }
    bool is_empty() const { return id_.empty(); }

private:
    std::string id_;
};

/**
 * @brief 事件 ID
 */
class EventId
{
public:
    static EventId generate();

    explicit EventId(const std::string& uuid = "") : uuid_(uuid) {}

    const std::string& to_string() const { return uuid_; }

    bool operator==(const EventId& other) const { return uuid_ == other.uuid_; }
    bool operator!=(const EventId& other) const { return !(*this == other); }

private:
    std::string uuid_;
};

/**
 * @brief 时间戳 - 毫秒级 Unix 时间
 */
class Timestamp
{
public:
    /**
     * @brief 获取当前时间戳
     */
    static Timestamp now();

    /**
     * @brief 从毫秒时间戳创建
     */
    explicit Timestamp(int64_t millis_since_epoch = 0) : millis_since_epoch_(millis_since_epoch) {}

    /**
     * @brief 获取毫秒级时间戳
     */
    int64_t millis() const { return millis_since_epoch_; }

    /**
     * @brief 获取秒级时间戳
     */
    int64_t seconds() const { return millis_since_epoch_ / 1000; }

    /**
     * @brief 计算与另一个时间戳的差值（毫秒）
     */
    int64_t duration_since(const Timestamp& other) const
    {
        return millis_since_epoch_ - other.millis_since_epoch_;
    }

    bool operator==(const Timestamp& other) const
    {
        return millis_since_epoch_ == other.millis_since_epoch_;
    }
    bool operator<(const Timestamp& other) const
    {
        return millis_since_epoch_ < other.millis_since_epoch_;
    }
    bool operator<=(const Timestamp& other) const
    {
        return millis_since_epoch_ <= other.millis_since_epoch_;
    }
    bool operator>(const Timestamp& other) const
    {
        return millis_since_epoch_ > other.millis_since_epoch_;
    }
    bool operator>=(const Timestamp& other) const
    {
        return millis_since_epoch_ >= other.millis_since_epoch_;
    }

private:
    int64_t millis_since_epoch_;
};

} // namespace device_automation::domain

namespace std {

/**
 * @brief TaskId 的哈希函数支持
 */
template <>
struct hash<device_automation::domain::TaskId>
{
    size_t operator()(const device_automation::domain::TaskId& id) const
    {
        return std::hash<std::string>()(id.to_string());
    }
};

/**
 * @brief WorkflowId 的哈希函数支持
 */
template <>
struct hash<device_automation::domain::WorkflowId>
{
    size_t operator()(const device_automation::domain::WorkflowId& id) const
    {
        return std::hash<std::string>()(id.to_string());
    }
};

/**
 * @brief ExecutorId 的哈希函数支持
 */
template <>
struct hash<device_automation::domain::ExecutorId>
{
    size_t operator()(const device_automation::domain::ExecutorId& id) const
    {
        return std::hash<std::string>()(id.to_string());
    }
};

/**
 * @brief DeviceId 的哈希函数支持
 */
template <>
struct hash<device_automation::domain::DeviceId>
{
    size_t operator()(const device_automation::domain::DeviceId& id) const
    {
        return std::hash<std::string>()(id.to_string());
    }
};

} // namespace std
