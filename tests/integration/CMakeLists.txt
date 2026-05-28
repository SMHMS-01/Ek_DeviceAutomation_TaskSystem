add_executable(test_acceptance_workflow test_acceptance_workflow.cpp)
target_link_libraries(test_acceptance_workflow
    PRIVATE device_automation_domain
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_application
)
set_target_properties(test_acceptance_workflow PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME AcceptanceWorkflow COMMAND ${CMAKE_BINARY_DIR}/tests/test_acceptance_workflow)

add_executable(test_persistence_recovery test_persistence_recovery.cpp)
target_link_libraries(test_persistence_recovery
    PRIVATE device_automation_domain
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_application
)
set_target_properties(test_persistence_recovery PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME PersistenceRecovery COMMAND ${CMAKE_BINARY_DIR}/tests/test_persistence_recovery)

add_executable(test_executor_pool test_executor_pool.cpp)
target_link_libraries(test_executor_pool
    PRIVATE device_automation_domain
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_application
)
set_target_properties(test_executor_pool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME ExecutorPool COMMAND ${CMAKE_BINARY_DIR}/tests/test_executor_pool)

add_executable(test_intervention_service test_intervention_service.cpp)
target_link_libraries(test_intervention_service
    PRIVATE device_automation_domain
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_application
)
set_target_properties(test_intervention_service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME InterventionService COMMAND ${CMAKE_BINARY_DIR}/tests/test_intervention_service)

add_executable(test_device_state_reconciler test_device_state_reconciler.cpp)
target_link_libraries(test_device_state_reconciler
    PRIVATE device_automation_domain
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_application
)
set_target_properties(test_device_state_reconciler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME DeviceStateReconciler COMMAND ${CMAKE_BINARY_DIR}/tests/test_device_state_reconciler)

add_test(
    NAME CliSmoke
    COMMAND $<TARGET_FILE:device_automation_task_system>
            ${CMAKE_SOURCE_DIR}/tests/fixtures/sample_workflow_linear.csv
            /tmp/device_automation_cli_smoke.sqlite
)
