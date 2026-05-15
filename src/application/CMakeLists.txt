add_library(device_automation_application STATIC
    AuditService.cpp
    InterventionService.cpp
    WorkflowManager.cpp
)

target_include_directories(device_automation_application
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_application
    PUBLIC device_automation_domain
    PUBLIC device_automation_infrastructure
    PUBLIC device_automation_scheduler
    PUBLIC device_automation_faulttolerance
)
