add_library(device_automation_scheduler STATIC
    SimpleScheduler.cpp
)

target_include_directories(device_automation_scheduler
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_scheduler
    PUBLIC device_automation_domain
    PUBLIC device_automation_infrastructure_impl
)
