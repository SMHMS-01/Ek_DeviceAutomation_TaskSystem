#pragma once

#include <stdexcept>
#include <string>

#include "TaskState.h"

namespace device_automation::domain {

/**
 * @brief 任务状态转换异常
 */
class InvalidStateTransitionException : public std::runtime_error
{
public:
    /**
     * @brief 构造异常
     * @param from 源状态
     * @param to 目标状态
     * @param reason 原因（可选）
     */
    InvalidStateTransitionException(TaskState from, TaskState to, const std::string& reason = "")
        : std::runtime_error("Invalid state transition: " + state_to_string(from) + " -> " +
                             state_to_string(to) + (reason.empty() ? "" : " (" + reason + ")")),
          from_state_(from),
          to_state_(to),
          reason_(reason)
    {
    }

    TaskState from_state() const { return from_state_; }
    TaskState to_state() const { return to_state_; }
    const std::string& reason() const { return reason_; }

private:
    TaskState from_state_;
    TaskState to_state_;
    std::string reason_;

    static std::string state_to_string(TaskState state)
    {
        switch (state) {
        case TaskState::Pending:
            return "Pending";
        case TaskState::Ready:
            return "Ready";
        case TaskState::Running:
            return "Running";
        case TaskState::Paused:
            return "Paused";
        case TaskState::Completed:
            return "Completed";
        case TaskState::Failed:
            return "Failed";
        case TaskState::Cancelled:
            return "Cancelled";
        case TaskState::RollingBack:
            return "RollingBack";
        case TaskState::RolledBack:
            return "RolledBack";
        case TaskState::WaitingForHuman:
            return "WaitingForHuman";
        default:
            return "Unknown";
        }
    }
};

/**
 * @brief 任务有限状态机
 * @details 定义任务的所有合法状态转换规则，防止无效的状态转换
 *
 * 状态转换图：
 * ```
 *                     ┌──────────────────────────────────────────────┐
 *                     │                                              │
 *         submit()    │  dependencies_met()   execute_done()        │
 *   ──────────►  PENDING ──────────► READY ──────────► RUNNING    │
 *                     │                │        │          │        │
 *              cancel │         cancel │        │ error    │ done   │
 *                     ▼                ▼        ▼          ▼        │
 *                 CANCELLED        CANCELLED  FAILED   COMPLETED   │
 *                                               │          │        │
 *                                   retry()     │          │        │
 *                                               └──► PENDING        │
 *                                                                   │
 *                     pause()                                       │
 *               RUNNING ──────────────────────────► PAUSED         │
 *                     ▲                                │            │
 *                     └────────────────────────────────┘            │
 *                              resume()                             │
 *                                                                   │
 *                     rollback()                                     │
 *               FAILED  ──────────────────────────► ROLLING_BACK   │
 *                                                        │          │
 *                                                        ▼          │
 *                                                   ROLLED_BACK     │
 *                                                                   │
 *               RUNNING/PAUSED ──── checkpoint() ──► (snapshot)    │
 * ```
 */
class TaskStateMachine
{
public:
    /**
     * @brief 构造状态机（初始状态为 Pending）
     */
    TaskStateMachine() : state_(TaskState::Pending) {}

    /**
     * @brief 获取当前状态
     */
    TaskState current_state() const { return state_; }

    /**
     * @brief 转换到新状态
     * @param next_state 目标状态
     * @throws InvalidStateTransitionException 如果转换不合法
     */
    void transition_to(TaskState next_state);

    /**
     * @brief 检查是否可以转换到目标状态
     * @param next_state 目标状态
     * @return true 如果转换合法，false 否则
     */
    bool can_transition_to(TaskState next_state) const;

    /**
     * @brief 获取从当前状态出发的所有合法目标状态
     * @return 合法目标状态的集合
     */
    std::vector<TaskState> valid_next_states() const;

    /**
     * @brief 状态转换为字符串表示
     */
    static const char* state_to_string(TaskState state);

private:
    TaskState state_;
};

} // namespace device_automation::domain
