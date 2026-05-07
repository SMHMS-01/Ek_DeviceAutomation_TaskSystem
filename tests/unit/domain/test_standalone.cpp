#include <iostream>
#include <cassert>
#include <vector>
#include <string>

// ============================================================================
// Minimal Test Framework (for demonstration without external deps)
// ============================================================================

class TestResult {
public:
    int total = 0;
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

    void record_pass(const std::string& test_name) {
        total++;
        passed++;
        std::cout << "✓ " << test_name << std::endl;
    }

    void record_fail(const std::string& test_name, const std::string& reason) {
        total++;
        failed++;
        failures.push_back(test_name + ": " + reason);
        std::cout << "✗ " << test_name << " - " << reason << std::endl;
    }

    void print_summary() {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "Total:  " << total << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Pass Rate: " << (total > 0 ? (100.0 * passed / total) : 0) << "%" << std::endl;
        
        if (!failures.empty()) {
            std::cout << "\nFailures:" << std::endl;
            for (const auto& f : failures) {
                std::cout << "  - " << f << std::endl;
            }
        }
        std::cout << std::string(70, '=') << std::endl;
    }
};

TestResult g_test_result;

#define TEST(name) void test_##name()
#define ASSERT_EQ(a, b) if ((a) != (b)) { g_test_result.record_fail(#a " == " #b, "values not equal"); return; }
#define ASSERT_TRUE(cond) if (!(cond)) { g_test_result.record_fail(#cond, "condition false"); return; }
#define ASSERT_FALSE(cond) if ((cond)) { g_test_result.record_fail(#cond, "condition true"); return; }
#define RUN_TEST(name) { test_##name(); g_test_result.record_pass(#name); }

// ============================================================================
// TaskState FSM Implementation (simplified)
// ============================================================================

enum class TaskState {
    Pending = 0,
    Ready = 1,
    Running = 2,
    Paused = 3,
    Completed = 4,
    Failed = 5,
    Cancelled = 6,
    RollingBack = 7,
    RolledBack = 8,
    WaitingForHuman = 9
};

std::string task_state_to_string(TaskState state) {
    switch (state) {
        case TaskState::Pending: return "Pending";
        case TaskState::Ready: return "Ready";
        case TaskState::Running: return "Running";
        case TaskState::Paused: return "Paused";
        case TaskState::Completed: return "Completed";
        case TaskState::Failed: return "Failed";
        case TaskState::Cancelled: return "Cancelled";
        case TaskState::RollingBack: return "RollingBack";
        case TaskState::RolledBack: return "RolledBack";
        case TaskState::WaitingForHuman: return "WaitingForHuman";
    }
    return "Unknown";
}

class TaskStateMachine {
private:
    TaskState current_state_ = TaskState::Pending;

    bool is_valid_transition(TaskState from, TaskState to) const {
        // FSM transition rules (21 valid transitions)
        switch (from) {
            case TaskState::Pending:
                return to == TaskState::Ready || to == TaskState::Cancelled;
            case TaskState::Ready:
                return to == TaskState::Running || to == TaskState::Cancelled;
            case TaskState::Running:
                return to == TaskState::Completed || to == TaskState::Failed ||
                       to == TaskState::Paused || to == TaskState::Cancelled;
            case TaskState::Paused:
                return to == TaskState::Running || to == TaskState::Cancelled ||
                       to == TaskState::Failed;
            case TaskState::Failed:
                return to == TaskState::Pending || to == TaskState::RollingBack ||
                       to == TaskState::Cancelled;
            case TaskState::RollingBack:
                return to == TaskState::RolledBack;
            case TaskState::RolledBack:
                return to == TaskState::Pending;
            case TaskState::WaitingForHuman:
                return to == TaskState::Pending || to == TaskState::Completed ||
                       to == TaskState::Cancelled;
            case TaskState::Completed:
            case TaskState::Cancelled:
                return false; // Terminal states
        }
        return false;
    }

public:
    TaskState current_state() const { return current_state_; }

    void transition_to(TaskState next_state) {
        if (!is_valid_transition(current_state_, next_state)) {
            throw std::runtime_error("Invalid transition from " +
                                   task_state_to_string(current_state_) + " to " +
                                   task_state_to_string(next_state));
        }
        current_state_ = next_state;
    }

    bool can_transition_to(TaskState next_state) const {
        return is_valid_transition(current_state_, next_state);
    }

    std::vector<TaskState> valid_next_states() const {
        std::vector<TaskState> states;
        for (int i = 0; i <= 9; ++i) {
            TaskState state = static_cast<TaskState>(i);
            if (can_transition_to(state)) {
                states.push_back(state);
            }
        }
        return states;
    }
};

// ============================================================================
// Test Cases: Phase 1.2 TaskStateMachine
// ============================================================================

TEST(InitialStateIsPending) {
    TaskStateMachine fsm;
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Pending));
}

TEST(PendingToReady) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Ready));
}

TEST(ReadyToRunning) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Running));
}

TEST(RunningToCompleted) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Completed));
}

TEST(RunningToFailed) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Failed));
}

TEST(RunningToPaused) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Paused));
}

TEST(PausedToRunning) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);
    fsm.transition_to(TaskState::Running);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Running));
}

TEST(FailedToPending_Retry) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::Pending);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Pending));
}

TEST(FailedToRollingBack) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::RollingBack));
}

TEST(RollingBackToRolledBack) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    fsm.transition_to(TaskState::RolledBack);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::RolledBack));
}

TEST(RolledBackToPending) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    fsm.transition_to(TaskState::RolledBack);
    fsm.transition_to(TaskState::Pending);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Pending));
}

TEST(CannotTransitionFromCompletedState) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);
    
    bool can_transition = fsm.can_transition_to(TaskState::Running);
    ASSERT_FALSE(can_transition);
}

TEST(CannotTransitionFromCancelledState) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Cancelled);
    
    bool can_transition = fsm.can_transition_to(TaskState::Running);
    ASSERT_FALSE(can_transition);
}

TEST(ValidNextStatesFromPending) {
    TaskStateMachine fsm;
    auto states = fsm.valid_next_states();
    ASSERT_EQ(states.size(), 2); // Ready, Cancelled
}

TEST(ValidNextStatesFromRunning) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    auto states = fsm.valid_next_states();
    ASSERT_EQ(states.size(), 4); // Completed, Failed, Paused, Cancelled
}

TEST(PauseResumeRerunCycle) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Paused);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Completed);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Completed));
}

TEST(RollbackCycle) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Failed);
    fsm.transition_to(TaskState::RollingBack);
    fsm.transition_to(TaskState::RolledBack);
    fsm.transition_to(TaskState::Pending);
    fsm.transition_to(TaskState::Ready);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Ready));
}

TEST(CancelFromPending) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Cancelled);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Cancelled));
}

TEST(CancelFromRunning) {
    TaskStateMachine fsm;
    fsm.transition_to(TaskState::Ready);
    fsm.transition_to(TaskState::Running);
    fsm.transition_to(TaskState::Cancelled);
    ASSERT_EQ(static_cast<int>(fsm.current_state()), static_cast<int>(TaskState::Cancelled));
}

// ============================================================================
// Main: Execute All Tests
// ============================================================================

int main() {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << "Phase 1.2: TaskStateMachine FSM - Unit Tests" << std::endl;
    std::cout << std::string(70, '=') << std::endl << std::endl;

    // Run all tests
    try {
        RUN_TEST(InitialStateIsPending);
    } catch (...) { g_test_result.record_fail("InitialStateIsPending", "exception"); }

    try {
        RUN_TEST(PendingToReady);
    } catch (...) { g_test_result.record_fail("PendingToReady", "exception"); }

    try {
        RUN_TEST(ReadyToRunning);
    } catch (...) { g_test_result.record_fail("ReadyToRunning", "exception"); }

    try {
        RUN_TEST(RunningToCompleted);
    } catch (...) { g_test_result.record_fail("RunningToCompleted", "exception"); }

    try {
        RUN_TEST(RunningToFailed);
    } catch (...) { g_test_result.record_fail("RunningToFailed", "exception"); }

    try {
        RUN_TEST(RunningToPaused);
    } catch (...) { g_test_result.record_fail("RunningToPaused", "exception"); }

    try {
        RUN_TEST(PausedToRunning);
    } catch (...) { g_test_result.record_fail("PausedToRunning", "exception"); }

    try {
        RUN_TEST(FailedToPending_Retry);
    } catch (...) { g_test_result.record_fail("FailedToPending_Retry", "exception"); }

    try {
        RUN_TEST(FailedToRollingBack);
    } catch (...) { g_test_result.record_fail("FailedToRollingBack", "exception"); }

    try {
        RUN_TEST(RollingBackToRolledBack);
    } catch (...) { g_test_result.record_fail("RollingBackToRolledBack", "exception"); }

    try {
        RUN_TEST(RolledBackToPending);
    } catch (...) { g_test_result.record_fail("RolledBackToPending", "exception"); }

    try {
        RUN_TEST(CannotTransitionFromCompletedState);
    } catch (...) { g_test_result.record_fail("CannotTransitionFromCompletedState", "exception"); }

    try {
        RUN_TEST(CannotTransitionFromCancelledState);
    } catch (...) { g_test_result.record_fail("CannotTransitionFromCancelledState", "exception"); }

    try {
        RUN_TEST(ValidNextStatesFromPending);
    } catch (...) { g_test_result.record_fail("ValidNextStatesFromPending", "exception"); }

    try {
        RUN_TEST(ValidNextStatesFromRunning);
    } catch (...) { g_test_result.record_fail("ValidNextStatesFromRunning", "exception"); }

    try {
        RUN_TEST(PauseResumeRerunCycle);
    } catch (...) { g_test_result.record_fail("PauseResumeRerunCycle", "exception"); }

    try {
        RUN_TEST(RollbackCycle);
    } catch (...) { g_test_result.record_fail("RollbackCycle", "exception"); }

    try {
        RUN_TEST(CancelFromPending);
    } catch (...) { g_test_result.record_fail("CancelFromPending", "exception"); }

    try {
        RUN_TEST(CancelFromRunning);
    } catch (...) { g_test_result.record_fail("CancelFromRunning", "exception"); }

    // Print results
    g_test_result.print_summary();

    return g_test_result.failed == 0 ? 0 : 1;
}
