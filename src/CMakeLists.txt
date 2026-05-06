add_library(device_automation_domain INTERFACE)

target_include_directories(device_automation_domain
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_domain
    INTERFACE nlohmann_json::nlohmann_json
)

# 源文件（设计阶段为空，实现时填充）
# target_sources(device_automation_domain
#     PRIVATE
#     domain/Task.cpp
#     domain/TaskStateMachine.cpp
#     domain/TaskGraph.cpp
#     domain/Event.cpp
#     domain/CheckPoint.cpp
#     domain/RetryPolicy.cpp
#     domain/Types.cpp
# )
