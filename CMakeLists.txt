cmake_minimum_required(VERSION 3.20)

project(
    device_automation_task_system
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Sophisticated task management framework for laboratory/industrial device automation"
)

# C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 项目配置
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 编译选项
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -fvisibility=hidden
)

# Debug/Release 配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g3 -fno-omit-frame-pointer)
    add_compile_definitions(DEBUG=1)
    
    # AddressSanitizer（可选）
    option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
else()
    # 默认 Release
    set(CMAKE_BUILD_TYPE "Release")
    add_compile_options(-O3 -DNDEBUG)
endif()

# 代码覆盖率支持
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

# 依赖库 - nlohmann_json 是 header-only，通常在 /usr/include/nlohmann
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # Fallback: 如果找不到，直接指定包含路径
    message(STATUS "nlohmann_json not found via find_package, trying include directory")
endif()

find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found via find_package, will use system default")
endif()

# 可选依赖
find_package(GTest CONFIG QUIET)
find_package(benchmark CONFIG QUIET)
find_package(Doxygen QUIET)

# 添加系统包含路径（用于 header-only 库）
include_directories(/usr/include)

# 主源代码
add_subdirectory(src)

# 测试目录（始终添加子目录，子目录内会根据可用的测试框架决定具体 targets）
enable_testing()
add_subdirectory(tests)

# 文档
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs")
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
else()
    message(STATUS "Doxygen not found - documentation generation disabled")
endif()

# 安装
install(
    DIRECTORY src/
    DESTINATION include/device_automation
    FILES_MATCHING PATTERN "*.h"
)

# 摘要
message(STATUS "")
message(STATUS "=== Device Automation Task System Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Code Coverage: ${ENABLE_COVERAGE}")
message(STATUS "ASAN: ${ENABLE_ASAN}")
message(STATUS "Tests: ${GTest_FOUND}")
message(STATUS "Benchmarks: ${benchmark_FOUND}")
message(STATUS "Documentation: ${DOXYGEN_FOUND}")
message(STATUS "==================================================")
message(STATUS "")
