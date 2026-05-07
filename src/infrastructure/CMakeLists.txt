## Infrastructure Layer (Phase 2 scaffolding)

## Infrastructure Layer (Phase 2 scaffolding)

add_library(device_automation_infrastructure INTERFACE)

target_include_directories(device_automation_infrastructure
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Expose dependency on domain layer interface for headers that consume domain types
target_link_libraries(device_automation_infrastructure
    INTERFACE device_automation_domain
)

# Implementations and extra dependencies (SQLite, etc.) will be added when implemented
