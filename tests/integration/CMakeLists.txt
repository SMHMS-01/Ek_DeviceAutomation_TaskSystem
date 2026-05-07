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
