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

# Optionally link spdlog when available and enable compile define for code paths
if(spdlog_FOUND)
    message(STATUS "Infrastructure: enabling spdlog integration")
    target_link_libraries(device_automation_infrastructure
        INTERFACE spdlog::spdlog
    )
    target_compile_definitions(device_automation_infrastructure
        INTERFACE USE_SPDLOG=1
    )
else()
    message(STATUS "Infrastructure: spdlog not available, using fallback logger")
endif()

# Implementations and extra dependencies (SQLite, etc.) will be added when implemented

# Implementation library for infrastructure components (SQLite-backed DB, EventBus)
find_package(SQLite3 QUIET)

add_library(device_automation_infrastructure_impl STATIC
    SqliteDatabase.cpp
    EventBus.cpp
)

target_include_directories(device_automation_infrastructure_impl
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(SQLite3_FOUND)
    message(STATUS "Infrastructure: SQLite3 found, building real SqliteDatabase")
    target_compile_definitions(device_automation_infrastructure_impl PRIVATE USE_SQLITE3=1)
    target_link_libraries(device_automation_infrastructure_impl PRIVATE SQLite::SQLite3)
else()
    message(STATUS "Infrastructure: SQLite3 not found, building fallback SqliteDatabase mock")
endif()

target_link_libraries(device_automation_infrastructure_impl PRIVATE device_automation_infrastructure)
