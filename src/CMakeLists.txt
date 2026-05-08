## Domain implementation lives in src/domain
add_subdirectory(domain)
add_subdirectory(infrastructure)
add_subdirectory(scheduler)
add_subdirectory(faulttolerance)
add_subdirectory(application)

add_executable(device_automation_task_system
    tools/TaskSystemCli.cpp
)

target_link_libraries(device_automation_task_system
    PRIVATE device_automation_application
    PRIVATE device_automation_scheduler
    PRIVATE device_automation_infrastructure_impl
    PRIVATE device_automation_domain
)
