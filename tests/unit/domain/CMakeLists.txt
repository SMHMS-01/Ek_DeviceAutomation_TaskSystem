# Domain Layer Unit Tests (Phase 1.2 实现)

# If Google Test is available, build the gtest-based test targets and
# place their runtime output into the build/tests folder. If GTest is
# not available, build the standalone fallback test runner (test_standalone.cpp)
if(GTest_FOUND)
    # TaskStateMachine Tests (gtest)
    add_executable(test_task_state_machine test_task_state_machine.cpp)
    target_link_libraries(test_task_state_machine
        PRIVATE device_automation_domain
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    set_target_properties(test_task_state_machine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
    add_test(NAME TaskStateMachineTest COMMAND ${CMAKE_BINARY_DIR}/tests/test_task_state_machine)

    # Types Tests (gtest)
    add_executable(test_types test_types.cpp)
    target_link_libraries(test_types
        PRIVATE device_automation_domain
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )
    set_target_properties(test_types PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
    add_test(NAME TypesTest COMMAND ${CMAKE_BINARY_DIR}/tests/test_types)
else()
    message(STATUS "GTest not found - building standalone fallback test runner(s)")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_standalone.cpp")
        add_executable(test_phase_1_2_standalone test_standalone.cpp)
        target_link_libraries(test_phase_1_2_standalone PRIVATE device_automation_domain)
        set_target_properties(test_phase_1_2_standalone PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
        add_test(NAME Phase1_2_Standalone COMMAND ${CMAKE_BINARY_DIR}/tests/test_phase_1_2_standalone)
    else()
        message(WARNING "No standalone test runner found at ${CMAKE_CURRENT_SOURCE_DIR}/test_standalone.cpp")
    endif()
endif()

# 待实现的测试：
# - test_task.cpp (Task class)
# - test_task_graph.cpp (TaskGraph DAG)
# - test_checkpoint.cpp (CheckPoint serialization)
# - test_retry_policy.cpp (RetryPolicy backoff)
