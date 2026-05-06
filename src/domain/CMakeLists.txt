# Domain Layer Library (设计阶段框架)

add_library(device_automation_domain INTERFACE)

target_include_directories(device_automation_domain
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(device_automation_domain
    INTERFACE nlohmann_json::nlohmann_json
)

# 实现时添加源文件：
# add_library(device_automation_domain_impl
#     Task.cpp
#     TaskStateMachine.cpp
#     TaskGraph.cpp
#     Event.cpp
#     CheckPoint.cpp
#     RetryPolicy.cpp
#     Types.cpp
# )
# target_link_libraries(device_automation_domain_impl PUBLIC nlohmann_json::nlohmann_json)
