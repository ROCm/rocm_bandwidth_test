# MIT License
#
# Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#

#
cmake_minimum_required(VERSION 3.20)

set(AMD_TARGET_NAME "rocm_bandwidth_test")
set(AMD_TARGET_NAME_ALL "${AMD_TARGET_NAME}_all")
set(AMD_TARGET_LIBNAME "libamd_work_bench")

project(${AMD_TARGET_LIBNAME}
    VERSION "0.0.1"
    LANGUAGES CXX C
    DESCRIPTION "amd-work-bench Library"
    HOMEPAGE_URL "https://github.com/ROCm/rocm_bandwidth_test"
)

message(STATUS "[[ Building... Project: " ${AMD_TARGET_LIBNAME} " ]] ...")

# Flag to enable / disable verbose output.
set(CMAKE_VERBOSE_MAKEFILE ON)

# Export compile commands for linters and autocompleters
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ Standard in use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CMake specific directories
set(AMD_APP_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


# TODO: Add tests subdir and sources.
set(LIBRARY_AMD_WORK_BENCH_SOURCES
    src/work_bench_api.cpp
    src/common_utils.cpp
    src/linux_utils.cpp
    src/default_sets.cpp
    src/event_mgmt.cpp
    src/http_ops.cpp
    src/json.cpp
    src/logger.cpp
    src/plugin_mgmt.cpp
    src/subcommands.cpp
    src/task_mgmt.cpp
    src/content_mgmt.cpp
    src/datasrc_mgmt.cpp
    src/datavw_mgmt.cpp
)

if(AMD_WORK_BENCH_EXTERNAL_PLUGIN_BUILD)
    add_library(${AMD_TARGET_LIBNAME} IMPORTED SHARED GLOBAL)
    set(AMD_WORK_BENCH_LIBRARY_TYPE INTERFACE)
else()
    if(AMD_APP_STATIC_LINK_PLUGINS)
        add_library(${AMD_TARGET_LIBNAME} STATIC ${LIBRARY_AMD_WORK_BENCH_SOURCES})
    else()
        add_library(${AMD_TARGET_LIBNAME} SHARED ${LIBRARY_AMD_WORK_BENCH_SOURCES})
    endif()

    set(AMD_WORK_BENCH_LIBRARY_TYPE PUBLIC)
    target_compile_definitions(${AMD_TARGET_LIBNAME} PRIVATE AMD_WORK_BENCH_PROJECT_NAME="${PROJECT_NAME}")
endif()

if(DEFINED AMD_WORK_BENCH_COMMIT_HASH_LONG AND DEFINED AMD_WORK_BENCH_COMMIT_BRANCH)
    set(GIT_COMMIT_HASH_LONG "${AMD_WORK_BENCH_COMMIT_HASH_LONG}")
    set(GIT_BRANCH "${AMD_WORK_BENCH_COMMIT_BRANCH}")
else()
    # Get info about the current branch
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RESULT_BRANCH
        ERROR_QUIET
    )

    # Get hash log info for the current branch
    execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH_LONG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RESULT_HASH_LONG
        ERROR_QUIET
    )
endif()
add_cppcheck(${AMD_TARGET_LIBNAME})

if(GIT_COMMIT_HASH_LONG STREQUAL "" OR GIT_BRANCH STREQUAL "")
    message(WARNING "[[ No commit hash/branch were found. ]]")
else()
    add_source_definitions(src/work_bench_api.cpp "GIT_BRANCH=\"${GIT_BRANCH}\"")
    add_source_definitions(src/work_bench_api.cpp "GIT_COMMIT_HASH_LONG=\"${GIT_COMMIT_HASH_LONG}\"")
endif()

add_source_definitions(src/work_bench_api.cpp "AMD_WORK_BENCH_VERSION=\"${AMD_TARGET_VERSION_TEXT}\"")

setup_unity_build(${AMD_TARGET_LIBNAME})
setup_compiler_flags(${AMD_TARGET_LIBNAME})

include(GenerateExportHeader)
generate_export_header(${AMD_TARGET_LIBNAME})

## Build Library
set(EXTERNAL_BASE_INCLUDE_DIRS ${AMD_EXTERNAL_DEPS_DIRECTORY})
target_include_directories(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} SYSTEM BEFORE PUBLIC ${BOOST_STACKTRACE_INCLUDE_DIRECTORIES})
target_include_directories(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} SYSTEM BEFORE PUBLIC ${FMT_INCLUDE_DIRS})
target_include_directories(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} include ${EXTERNAL_BASE_INCLUDE_DIRS})

if(NOT AMD_WORK_BENCH_EXTERNAL_PLUGIN_BUILD)
    target_link_libraries(${AMD_TARGET_LIBNAME} PRIVATE dl)
    target_link_libraries(${AMD_TARGET_LIBNAME} PUBLIC ${JTHREAD_LIBRARIES})

    # Builds need to link with libcurl ("CURL::libcurl")
    target_link_libraries(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} CURL::libcurl)

    target_include_directories(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} ${BACKTRACE_INCLUDE_DIRECTORIES})
endif()

target_link_libraries(${AMD_TARGET_LIBNAME} ${AMD_WORK_BENCH_LIBRARY_TYPE} ${NLOHMANN_JSON_LIBRARIES} ${SPDLOG_LIBRARIES} ${FMT_LIBRARIES} ${BOOST_LIBRARIES} ${BOOST_STACKTRACE_LIBRARIES})
set_property(TARGET ${AMD_TARGET_LIBNAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
add_dependencies(${AMD_TARGET_NAME_ALL} ${AMD_TARGET_LIBNAME})

# Set output directories for all targets during build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${AMD_APP_BASE_DIRECTORY}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${AMD_APP_BASE_DIRECTORY}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${AMD_APP_BASE_DIRECTORY}/lib")
set(AMD_APP_BUILD_LIB_DIRECTORY "${AMD_APP_BASE_DIRECTORY}/lib")
file(MAKE_DIRECTORY ${AMD_APP_BUILD_LIB_DIRECTORY})

set_target_properties(${AMD_TARGET_LIBNAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${AMD_APP_BUILD_LIB_DIRECTORY})
set_target_properties(${AMD_TARGET_LIBNAME} PROPERTIES PREFIX "")

#
## Install target: Provided by top CMakeLists.txt, so we can use a 'dynamic_install()' function
##install(FILES "$<TARGET_FILE:${AMD_TARGET_LIBNAME}>" DESTINATION "${CMAKE_INSTALL_LIBDIR}" PERMISSIONS ${TARGET_FS_LIBRARY_PERMISSIONS})
#install(TARGETS ${AMD_TARGET_LIBNAME} LIBRARY DESTINATION lib)
#install(TARGETS ${AMD_TARGET_LIBNAME} RUNTIME DESTINATION bin)


## End of CMakeLists.txt
