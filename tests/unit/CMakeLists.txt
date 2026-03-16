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
# Unit Tests CMake Configuration for ROCm Bandwidth Test

cmake_minimum_required(VERSION 3.20)

# Define target name
set(AMD_UNIT_TEST_TARGET_NAME "tst_unit")

# Source files for unit tests
set(AMD_UNIT_TEST_SOURCES
    src/test_string_utils.cpp
    src/test_plugin_management.cpp
    src/test_environment_utils.cpp
    src/test_memory_utils.cpp
    src/test_workbench_api.cpp
    src/test_gpu_interface.cpp
    src/test_rbt_plugin.cpp
    src/test_transferbench_types.cpp
)

# Header files
set(AMD_UNIT_TEST_HEADERS
    include/tst_unit.hpp
)

# Create static library for unit tests
add_library(${AMD_UNIT_TEST_TARGET_NAME} STATIC
    ${AMD_UNIT_TEST_SOURCES}
    ${AMD_UNIT_TEST_HEADERS}
)

# Include directories
target_include_directories(${AMD_UNIT_TEST_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${AMD_TEST_PROJECT_BASE_DIRECTORY}>
        $<BUILD_INTERFACE:${AMD_TEST_PROJECT_WORKBENCH_DEPS_INCLUDE_DIRECTORY}>
        $<BUILD_INTERFACE:${AMD_TEST_PROJECT_WORKBENCH_DEPS_AWB_INCLUDE_DIRECTORY}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Add GPU interface include directory
target_include_directories(${AMD_UNIT_TEST_TARGET_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/common>
)

# Link dependencies
target_link_libraries(${AMD_UNIT_TEST_TARGET_NAME}
    PUBLIC
        ${AMD_TEST_PROJECT_LINK_LIBRARIES}
        ${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME}
        ${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_NAME}
)

# Set C++ standard
set_target_properties(${AMD_UNIT_TEST_TARGET_NAME}
    PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
)

# Compile definitions
target_compile_definitions(${AMD_UNIT_TEST_TARGET_NAME}
    PUBLIC
        AMD_WORK_BENCH_PROJECT_NAME="${AMD_TEST_PROJECT_NAME}"
)

# Export the target name
set(AMD_TEST_PROJECT_UNIT_TARGET_NAME "${AMD_UNIT_TEST_TARGET_NAME}" PARENT_SCOPE)

# Debug message
has_build_debug_mode(HAS_DEBUG_MODE_ENABLED)
if(HAS_DEBUG_MODE_ENABLED)
    developer_status_message("DEVEL" "  >> Building unit test library: ${AMD_UNIT_TEST_TARGET_NAME}")
    developer_status_message("DEVEL" "  >> Unit test sources: ${AMD_UNIT_TEST_SOURCES}")
endif()

## End of CMakeLists.txt
