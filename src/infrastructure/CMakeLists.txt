## Infrastructure Layer (Phase 2 scaffolding)

add_library(device_automation_infrastructure INTERFACE)

target_include_directories(device_automation_infrastructure
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

## Provide an interface target for other layers to depend on during scaffolding
# Infrastructure Layer Library (设计阶段框架)

add_library(device_automation_infrastructure INTERFACE)

target_include_directories(device_automation_infrastructure
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_infrastructure
    INTERFACE device_automation_domain
    INTERFACE SQLite::SQLite3
)

# 实现时添加源文件和依赖
