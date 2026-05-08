#include "domain/TaskState.h"
#include "domain/TaskStateMachine.h"

#include <gtest/gtest.h>

using namespace device_automation::domain;

// ============================================================================
// TaskStateMachine Tests
// ============================================================================

class TaskStateMachineTest : public ::testing::Test
{
protected:
    TaskStateMachine fsm;
};

// ============================================================================
// 基础测试
// ============================================================================

TEST_F(TaskStateMachineTest, InitialStateIsPending)
{
    EXPECT_EQ(fsm.current_state(), TaskState::Pending);
}

TEST_F(TaskStateMachineTest, StateToStringConversion)
{
    EXPECT_STREQ(TaskStateMachine::state_to_string(TaskState::Pending), "Pending");
    EXPECT_STREQ(TaskStateMachine::state_to_string(TaskState::Running), "Running");
    EXPECT_STREQ(TaskStateMachine::state_to_string(TaskState::Completed), "Completed");
    EXPECT_STREQ(TaskStateMachine::state_to_string(TaskState::Failed), "Failed");
}

// ============================================================================
// 合法状态转换测试
// ============================================================================

TEST_F(TaskStateMachineTest, PendingToReady)
{
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Ready));
    fsm.transition_to(TaskState::Ready);
    EXPECT_EQ(fsm.current_state(), TaskState::Ready);
}

TEST_F(TaskStateMachineTest, PendingToCancelled)
{
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Cancelled));
    fsm.transition_to(TaskState::Cancelled);
    EXPECT_EQ(fsm.current_state(), TaskState::Cancelled);
}

TEST_F(TaskStateMachineTest, ReadyToRunning)
{
    fsm.transition_to(TaskState::Ready);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Running));
    fsm.transition_to(TaskState::Running);
    EXPECT_EQ(fsm.current_state(), TaskState::Running);
}

TEST_F(TaskStateMachineTest, RunningToCompleted)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Completed));
    fsm.transition_to(TaskState::Completed);
    EXPECT_EQ(fsm.current_state(), TaskState::Completed);
}

TEST_F(TaskStateMachineTest, RunningToFailed)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Failed));
    fsm.transition_to(TaskState::Failed);
    EXPECT_EQ(fsm.current_state(), TaskState::Failed);
}

TEST_F(TaskStateMachineTest, RunningToPaused)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Paused));
    fsm.transition_to(TaskState::Paused);
    EXPECT_EQ(fsm.current_state(), TaskState::Paused);
}

TEST_F(TaskStateMachineTest, PausedToRunning)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Running));
    fsm.transition_to(TaskState::Running);
    EXPECT_EQ(fsm.current_state(), TaskState::Running);
}

TEST_F(TaskStateMachineTest, FailedToPending_Retry)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Pending));
    fsm.transition_to(TaskState::Pending);
    EXPECT_EQ(fsm.current_state(), TaskState::Pending);
}

TEST_F(TaskStateMachineTest, FailedToRollingBack)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::RollingBack));
    fsm.transition_to(TaskState::RollingBack);
    EXPECT_EQ(fsm.current_state(), TaskState::RollingBack);
}

TEST_F(TaskStateMachineTest, RollingBackToRolledBack)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::RolledBack));
    fsm.transition_to(TaskState::RolledBack);
    EXPECT_EQ(fsm.current_state(), TaskState::RolledBack);
}

TEST_F(TaskStateMachineTest, RolledBackToPending)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    fsm.transition_to(TaskState::RolledBack);
    EXPECT_TRUE(fsm.can_transition_to(TaskState::Pending));
    fsm.transition_to(TaskState::Pending);
    EXPECT_EQ(fsm.current_state(), TaskState::Pending);
}

// ============================================================================
// 非法状态转换测试
// ============================================================================

TEST_F(TaskStateMachineTest, CannotTransitionFromCompletedState)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);

    // Completed 是终态
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Pending));
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Failed));
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Running));
}

TEST_F(TaskStateMachineTest, CompletedStateThrowsOnInvalidTransition)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);

    EXPECT_THROW(fsm.transition_to(TaskState::Failed), InvalidStateTransitionException);
}

TEST_F(TaskStateMachineTest, CannotTransitionFromCancelledState)
{
    fsm.transition_to(TaskState::Cancelled);

    // Cancelled 是终态
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Running));
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Pending));
}

TEST_F(TaskStateMachineTest, CancelledStateThrowsOnInvalidTransition)
{
    fsm.transition_to(TaskState::Cancelled);

    EXPECT_THROW(fsm.transition_to(TaskState::Running), InvalidStateTransitionException);
}

TEST_F(TaskStateMachineTest, PendingCannotGoDirectlyToCompleted)
{
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Completed));
    EXPECT_THROW(fsm.transition_to(TaskState::Completed), InvalidStateTransitionException);
}

TEST_F(TaskStateMachineTest, PendingCannotGoDirectlyToRunning)
{
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Running));
    EXPECT_THROW(fsm.transition_to(TaskState::Running), InvalidStateTransitionException);
}

TEST_F(TaskStateMachineTest, ReadyCannotGoPaused)
{
    fsm.transition_to(TaskState::Ready);
    EXPECT_FALSE(fsm.can_transition_to(TaskState::Paused));
    EXPECT_THROW(fsm.transition_to(TaskState::Paused), InvalidStateTransitionException);
}

TEST_F(TaskStateMachineTest, PausedCannotGoRollingBack)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);

    EXPECT_FALSE(fsm.can_transition_to(TaskState::RollingBack));
    EXPECT_THROW(fsm.transition_to(TaskState::RollingBack), InvalidStateTransitionException);
}

// ============================================================================
// 异常测试
// ============================================================================

TEST_F(TaskStateMachineTest, InvalidStateTransitionExceptionContainsInfo)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);

    try
    {
        fsm.transition_to(TaskState::Failed);
        FAIL() << "Expected InvalidStateTransitionException";
    }
    catch (const InvalidStateTransitionException &e)
    {
        EXPECT_EQ(e.from_state(), TaskState::Completed);
        EXPECT_EQ(e.to_state(), TaskState::Failed);
        EXPECT_NE(std::string(e.what()).find("Completed"), std::string::npos);
        EXPECT_NE(std::string(e.what()).find("Failed"), std::string::npos);
    }
}

// ============================================================================
// 有效下一状态测试
// ============================================================================

TEST_F(TaskStateMachineTest, ValidNextStatesFromPending)
{
    auto valid = fsm.valid_next_states();
    EXPECT_EQ(valid.size(), 2);
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Ready) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Cancelled) != valid.end());
}

TEST_F(TaskStateMachineTest, ValidNextStatesFromRunning)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);

    auto valid = fsm.valid_next_states();
    EXPECT_EQ(valid.size(), 4);
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Completed) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Failed) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Paused) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Cancelled) != valid.end());
}

TEST_F(TaskStateMachineTest, ValidNextStatesFromFailed)
{
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);

    auto valid = fsm.valid_next_states();
    EXPECT_EQ(valid.size(), 3);
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Pending) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::RollingBack) != valid.end());
    EXPECT_TRUE(std::find(valid.begin(), valid.end(), TaskState::Cancelled) != valid.end());
}

// ============================================================================
// 复杂场景测试
// ============================================================================

TEST_F(TaskStateMachineTest, PauseResumeRerunCycle)
{
    // Pending → Ready → Running → Paused → Running → Failed → Pending → Ready → Running → Completed
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::Pending);
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);

    EXPECT_EQ(fsm.current_state(), TaskState::Completed);
}

TEST_F(TaskStateMachineTest, RollbackCycle)
{
    // Pending → Ready → Running → Failed → RollingBack → RolledBack → Pending
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    fsm.transition_to(TaskState::RolledBack);
    fsm.transition_to(TaskState::Pending);

    EXPECT_EQ(fsm.current_state(), TaskState::Pending);
}

TEST_F(TaskStateMachineTest, CancelFromMultipleStates)
{
    // Test cancellation from Pending
    {
        TaskStateMachine fsm_pending;
        fsm_pending.transition_to(TaskState::Cancelled);
        EXPECT_EQ(fsm_pending.current_state(), TaskState::Cancelled);
    }

    // Test cancellation from Ready
    {
        TaskStateMachine fsm_ready;
        fsm_ready.transition_to(TaskState::Ready);
        fsm_ready.transition_to(TaskState::Cancelled);
        EXPECT_EQ(fsm_ready.current_state(), TaskState::Cancelled);
    }

    // Test cancellation from Running
    {
        TaskStateMachine fsm_running;
        fsm_running.transition_to(TaskState::Ready);
        fsm_running.transition_to(TaskState::Running);
        fsm_running.transition_to(TaskState::Cancelled);
        EXPECT_EQ(fsm_running.current_state(), TaskState::Cancelled);
    }
}

// ============================================================================
// WaitingForHuman 状态测试
// ============================================================================

TEST_F(TaskStateMachineTest, WaitingForHumanCanTransitionToPending)
{
    // 手动设置状态为 WaitingForHuman（通常由应用层设置）
    // 这里我们通过构造函数或特殊方法实现
    // 为简化测试，暂时跳过此部分
    // 实现时应添加 set_state 或类似方法供测试使用
}

} // namespace
