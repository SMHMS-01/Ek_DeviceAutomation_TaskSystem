# Domain Layer Library (Phase 1.2 实现)

add_library(device_automation_domain
    Types.cpp
    TaskStateMachine.cpp
)

target_include_directories(device_automation_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
    PUBLIC /usr/include
)

# nlohmann_json 是 header-only，通过 include_directories 提供
if(nlohmann_json_FOUND)
    target_link_libraries(device_automation_domain
        PUBLIC nlohmann_json::nlohmann_json
    )
endif()

# 待实现的源文件（Phase 1.3-1.8）：
# Task.cpp
# TaskGraph.cpp
# Event.cpp
# CheckPoint.cpp
# RetryPolicy.cpp
