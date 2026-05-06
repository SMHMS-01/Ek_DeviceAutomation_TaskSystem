# Application Layer Library (设计阶段框架)

add_library(device_automation_application INTERFACE)

target_include_directories(device_automation_application
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_application
    INTERFACE device_automation_domain
    INTERFACE device_automation_infrastructure
    INTERFACE device_automation_scheduler
    INTERFACE device_automation_faulttolerance
)

# 实现时添加源文件
