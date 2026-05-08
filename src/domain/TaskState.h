#pragma once

namespace device_automation::domain
{

/**
 * @brief 任务状态枚举
 * @details 完整的有限状态机，定义任务生命周期中的所有合法状态
 */
enum class TaskState
{
    Pending,         ///< 已提交，等待依赖完成
    Ready,           ///< 依赖满足，等待执行器
    Running,         ///< 执行中
    Paused,          ///< 人工暂停（保存 Checkpoint）
    Completed,       ///< 成功完成
    Failed,          ///< 失败（等待重试/人工处理）
    Cancelled,       ///< 已取消
    RollingBack,     ///< 回滚中
    RolledBack,      ///< 回滚完成
    WaitingForHuman, ///< 超过重试次数，等待人工干预
};

/**
 * @brief 任务类型
 */
enum class TaskKind
{
    Atomic,    ///< 原子任务，叶节点，直接对应设备操作
    Composite, ///< 复合任务，包含子任务的编排
    Script,    ///< 脚本任务，用户脚本定义（插件扩展）
};

/**
 * @brief 任务图状态
 */
enum class GraphState
{
    Created,   ///< 新创建
    Submitted, ///< 已提交到调度器
    Running,   ///< 工作流运行中
    Completed, ///< 工作流完成
    Failed,    ///< 工作流失败
    Cancelled, ///< 工作流取消
};

/**
 * @brief 执行器状态
 */
enum class ExecutorStatus
{
    Initializing,  ///< 初始化中
    Ready,         ///< 就绪，可接收任务
    Busy,          ///< 忙碌，任务队列非空
    Shutting_Down, ///< 关闭中
    Shutdown,      ///< 已关闭
    Error,         ///< 错误状态
};

} // namespace device_automation::domain
