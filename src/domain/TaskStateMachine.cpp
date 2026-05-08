#include "TaskStateMachine.h"

#include <vector>

namespace device_automation::domain
{

// 定义合法的状态转换规则
static bool is_valid_transition(TaskState from, TaskState to)
{
    switch (from)
    {
    case TaskState::Pending:
        // Pending 可转移到：Ready, Cancelled
        return to == TaskState::Ready || to == TaskState::Cancelled;

    case TaskState::Ready:
        // Ready 可转移到：Running, Cancelled
        return to == TaskState::Running || to == TaskState::Cancelled;

    case TaskState::Running:
        // Running 可转移到：Completed, Failed, Paused, Cancelled
        return to == TaskState::Completed || to == TaskState::Failed || to == TaskState::Paused ||
               to == TaskState::Cancelled;

    case TaskState::Paused:
        // Paused 可转移到：Running, Cancelled, Failed（由人工强制）
        return to == TaskState::Running || to == TaskState::Cancelled || to == TaskState::Failed;

    case TaskState::Completed:
        // Completed 是终态，不可转移
        return false;

    case TaskState::Failed:
        // Failed 可转移到：Pending（重试）, RollingBack, WaitingForHuman, Cancelled
        return to == TaskState::Pending || to == TaskState::RollingBack ||
               to == TaskState::WaitingForHuman || to == TaskState::Cancelled;

    case TaskState::Cancelled:
        // Cancelled 是终态，不可转移
        return false;

    case TaskState::RollingBack:
        // RollingBack 可转移到：RolledBack
        return to == TaskState::RolledBack;

    case TaskState::RolledBack:
        // RolledBack 可转移到：Pending（重新提交）
        return to == TaskState::Pending;

    case TaskState::WaitingForHuman:
        // WaitingForHuman 可转移到：Pending（人工重试）, Completed（人工标记完成），Cancelled
        return to == TaskState::Pending || to == TaskState::Completed || to == TaskState::Cancelled;

    default:
        return false;
    }
}

void TaskStateMachine::transition_to(TaskState next_state)
{
    if (!is_valid_transition(state_, next_state))
    {
        throw InvalidStateTransitionException(state_, next_state);
    }
    state_ = next_state;
}

bool TaskStateMachine::can_transition_to(TaskState next_state) const
{
    return is_valid_transition(state_, next_state);
}

std::vector<TaskState> TaskStateMachine::valid_next_states() const
{
    std::vector<TaskState> valid_states;
    std::vector<TaskState> all_states = {TaskState::Pending,    TaskState::Ready,
                                         TaskState::Running,    TaskState::Paused,
                                         TaskState::Completed,  TaskState::Failed,
                                         TaskState::Cancelled,  TaskState::RollingBack,
                                         TaskState::RolledBack, TaskState::WaitingForHuman};

    for (TaskState state : all_states)
    {
        if (is_valid_transition(state_, state))
        {
            valid_states.push_back(state);
        }
    }

    return valid_states;
}

const char *TaskStateMachine::state_to_string(TaskState state)
{
    switch (state)
    {
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

} // namespace device_automation::domain
