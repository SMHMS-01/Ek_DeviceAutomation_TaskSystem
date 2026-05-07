# Domain Layer Unit Tests (Phase 1.2 实现)

# TaskStateMachine Tests (27 test cases)
add_executable(test_task_state_machine test_task_state_machine.cpp)
target_link_libraries(test_task_state_machine 
    PRIVATE device_automation_domain 
    PRIVATE GTest::gtest 
    PRIVATE GTest::gtest_main
)
add_test(NAME TaskStateMachineTest COMMAND test_task_state_machine)

# Types Tests (19 test cases)
add_executable(test_types test_types.cpp)
target_link_libraries(test_types 
    PRIVATE device_automation_domain 
    PRIVATE GTest::gtest 
    PRIVATE GTest::gtest_main
)
add_test(NAME TypesTest COMMAND test_types)

# 待实现的测试：
# - test_task.cpp (Task class)
# - test_task_graph.cpp (TaskGraph DAG)
# - test_checkpoint.cpp (CheckPoint serialization)
# - test_retry_policy.cpp (RetryPolicy backoff)
