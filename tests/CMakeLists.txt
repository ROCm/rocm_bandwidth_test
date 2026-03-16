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


# Set the default target name if not specified
if(NOT AMD_TEST_TARGET_NAME OR AMD_TEST_TARGET_NAME STREQUAL "")
    set(AMD_TEST_TARGET_NAME "rbt_unit_tests")
    set(AMD_TEST_TARGET_NAME "${AMD_TEST_TARGET_NAME}" PARENT_SCOPE)
endif()
if(NOT AMD_TEST_LINK_LIBRARIES OR AMD_TEST_LINK_LIBRARIES STREQUAL "")
    set(AMD_TEST_LINK_LIBRARIES "libamd_work_bench")
    set(AMD_TEST_LINK_LIBRARIES "${AMD_TEST_LINK_LIBRARIES}" PARENT_SCOPE)
endif()


#
# --- Unit Test CLI Options ---
#
#   Unit Test filters:
#       - usage: 
#           ./rbt_unit_tests [<test name|pattern|tags> ... ] options
#           ./rbt_unit_tests --help
#
#       - list all/matching test cases:
#           ./rbt_unit_tests --list-tests
#
#       - list all/matching tags:
#           ./rbt_unit_tests --list-tags
#
#       - Include successful tests in output:
#           ./rbt_unit_tests --success
#
#       - Run all tests:
#           ./rbt_unit_tests
#
#       - Run Common tests only:
#           ./rbt_unit_tests [common]
#
#       - Run Helpers tests only:
#           ./rbt_unit_tests [helpers]
#
#       - Run Plugins tests only:
#           ./rbt_unit_tests [plugins]
#
#       - Run tests matching multiple tags:
#           ./rbt_unit_tests [common],[helpers]
#
#       - Run tests matching a name: 
#           ./rbt_unit_tests "Test cases: Utils"
#
#       - Run tests matching a section name:
#           ./rbt_unit_tests --section "utils::get_env_var(): non-existing variable"
#

#
# Set the project properties for the tests
set(AMD_TEST_PROJECT_EXTERNAL_DEPS_DIRECTORY "${AMD_EXTERNAL_DEPS_DIRECTORY}" CACHE STRING "Tests: External Dependencies Directory" FORCE)
set(AMD_TEST_PROJECT_3RD_PARTY_DEPS_DIRECTORY "${AMD_3RD_PARTY_DEPS_DIRECTORY}" CACHE STRING "Tests: 3rd Party Dependencies Directory" FORCE)
set(AMD_TEST_PROJECT_WORKBENCH_DEPS_DIRECTORY "${AMD_WORKBENCH_DEPS_DIRECTORY}" CACHE STRING "Tests: Workbench Dependencies Directory" FORCE)
set(AMD_TEST_PROJECT_WORKBENCH_DEPS_INCLUDE_DIRECTORY "${AMD_WORKBENCH_DEPS_DIRECTORY}/include" CACHE STRING "Tests: Workbench Dependencies Include Directory" FORCE)
set(AMD_TEST_PROJECT_WORKBENCH_DEPS_AWB_INCLUDE_DIRECTORY "${AMD_WORKBENCH_DEPS_DIRECTORY}/include/awb" CACHE STRING "Tests: Workbench Dependencies AWB Include Directory" FORCE)

set(AMD_TEST_PROJECT_BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Tests: Base Directory" FORCE)
set(AMD_TEST_PROJECT_COMMON_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/common" CACHE STRING "Tests: Common Directory" FORCE)
set(AMD_TEST_PROJECT_HELPERS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/helpers" CACHE STRING "Tests: Helpers Directory" FORCE)
set(AMD_TEST_PROJECT_PLUGINS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/plugins" CACHE STRING "Tests: Plugins Directory" FORCE)

set(AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME "${AMD_TARGET_NAME}_3RD_PARTY_LIBRARIES" CACHE STRING "Tests: All target bundled interface name" FORCE)
set(AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_LIST "${${AMD_TARGET_NAME}_3RD_PARTY_TARGET_LIST}" CACHE STRING "Tests: All target bundled interface list" FORCE)
set(AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_NAME "${AMD_TARGET_NAME}_3RD_PARTY_TEST_LIBRARIES" CACHE STRING "Tests: All target tests bundled interface name" FORCE)
set(AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_LIST "${${AMD_TARGET_NAME}_3RD_PARTY_UNIT_TEST_TARGET_LIST}" CACHE STRING "Tests: All target tests bundled interface list" FORCE)

set(AMD_TEST_PROJECT_NAME "${AMD_TEST_TARGET_NAME}" CACHE STRING "Tests: Project name" FORCE)
set(AMD_TEST_PROJECT_LINK_LIBRARIES "${AMD_TEST_LINK_LIBRARIES}" CACHE STRING "Tests: Link Libraries" FORCE)
set(AMD_TEST_PROJECT_COMMON_TARGET_NAME "tst_common" CACHE STRING "Tests: Common Target Name" FORCE)
set(AMD_TEST_PROJECT_HELPERS_TARGET_NAME "tst_helpers" CACHE STRING "Tests: Helpers"  FORCE)
set(AMD_TEST_PROJECT_PLUGINS_TARGET_NAME "tst_plugins" CACHE STRING "Tests: Plugins"  FORCE)
set(AMD_TEST_PROJECT_LOCAL_LINK_LIBRARIES "${AMD_TEST_PROJECT_COMMON_TARGET_NAME};${AMD_TEST_PROJECT_HELPERS_TARGET_NAME};${AMD_TEST_PROJECT_PLUGINS_TARGET_NAME};tst_unit")

set(AMD_TEST_PROJECT_WORKBENCH_INCLUDE_DIRECTORIES
    "${AMD_TEST_PROJECT_EXTERNAL_DEPS_DIRECTORY}"
    "${AMD_TEST_PROJECT_WORKBENCH_DEPS_INCLUDE_DIRECTORY}"
    "${AMD_TEST_PROJECT_WORKBENCH_DEPS_AWB_INCLUDE_DIRECTORY}"
)

set(AMD_TEST_PROJECT_INCLUDE_DIRECTORIES 
    "${AMD_TEST_PROJECT_BASE_DIRECTORY}"
    "${AMD_TEST_PROJECT_COMMON_DIRECTORY}/include"
    "${AMD_TEST_PROJECT_HELPERS_DIRECTORY}/include"
    "${AMD_TEST_PROJECT_PLUGINS_DIRECTORY}/include"
)

set(AMD_TEST_PROJECT_ALL_INCLUDE_DIRECTORIES 
    "${AMD_TEST_PROJECT_WORKBENCH_INCLUDE_DIRECTORIES}"
)


# Set the project name and version
project(${AMD_TEST_PROJECT_NAME} 
    VERSION ${AMD_APP_ROCM_BUILD_BINARY_VERSION_INTERNAL}
    LANGUAGES CXX
    DESCRIPTION "${AMD_TARGET_NAME}: Unit Tests"
)
set(AMD_TEST_PROJECT_COVERAGE_NAME "${AMD_TEST_PROJECT_NAME}_coverage")

# 
message(STATUS "[[ Building... Project: " ${PROJECT_NAME} " v." ${AMD_APP_ROCM_BUILD_BINARY_VERSION_INTERNAL} " ]] ...")
has_build_debug_mode(HAS_DEBUG_MODE_ENABLED)
if(HAS_DEBUG_MODE_ENABLED)
    get_target_property(BUNDLE_INTERFACE_TYPE "${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME}" TYPE)
    developer_status_message("DEVEL" ">> Checking '${AMD_TEST_PROJECT_NAME}' target bundled interface: '${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME}' ...")
    developer_status_message("DEVEL" "  >> Interface type: '${BUNDLE_INTERFACE_TYPE}' ...")
    developer_status_message("DEVEL" "  >> Interface list: ${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_LIST} ...")

    # Include the developer status message function
    foreach(target_name ${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_LIST})
        if(TARGET ${target_name})
            get_target_property(target_type ${target_name} TYPE)
        endif()    
        developer_status_message("DEVEL" "  >> Target library: '${target_name}' -> Type: '${target_type}' ...")
    endforeach()

    # Check the 3rd party test libraries interface
    get_target_property(TEST_BUNDLE_INTERFACE_TYPE "${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_NAME}" TYPE)
    developer_status_message("DEVEL" ">> Checking '${AMD_TEST_PROJECT_NAME}' target tests bundled interface: '${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_NAME}' ...")
    developer_status_message("DEVEL" "  >> Interface type: '${TEST_BUNDLE_INTERFACE_TYPE}' ...")
    developer_status_message("DEVEL" "  >> Interface list: ${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_LIST} ...")

    # Include the developer status message function
    foreach(target_name ${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_LIST})
        if(TARGET ${target_name})
            get_target_property(target_type ${target_name} TYPE)
        endif()    
        developer_status_message("DEVEL" "  >> Target library: '${target_name}' -> Type: '${target_type}' ...")
    endforeach()
endif()

#
# Unit Test target dependencies
add_subdirectory(common)
add_subdirectory(helpers)
add_subdirectory(plugins)
add_subdirectory(unit)

# Add the specific/individual plugin tests
if(AMD_APP_BUILD_PLUGIN_TESTS)
    if(HAS_DEBUG_MODE_ENABLED)
        developer_status_message("DEVEL" "  >> Individual plugin unit tests: ${AMD_APP_BUILD_PLUGIN_TESTS} ...")
    endif()
    #add_subdirectory(plugins/unit_tests)
endif()

#
add_executable(${PROJECT_NAME} 
    common/src/test_cases_common.cpp
    helpers/src/test_cases_helpers.cpp
    plugins/src/test_cases_plugins.cpp
    unit/src/test_string_utils.cpp
    unit/src/test_plugin_management.cpp
    unit/src/test_environment_utils.cpp
    unit/src/test_memory_utils.cpp
    unit/src/test_workbench_api.cpp
    unit/src/test_gpu_interface.cpp
    unit/src/test_rbt_plugin.cpp
    unit/src/test_transferbench_types.cpp
)
target_compile_definitions(${PROJECT_NAME} PUBLIC AMD_WORK_BENCH_PROJECT_NAME="${PROJECT_NAME}")

#
# Include directories for external dependencies
foreach(INCLUDE_DIRECTORY IN LISTS AMD_TEST_PROJECT_INCLUDE_DIRECTORIES)
    target_include_directories(${PROJECT_NAME} 
        PRIVATE 
            $<BUILD_INTERFACE:${INCLUDE_DIRECTORY}>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/plugins/common>
    )
endforeach()


target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        ${AMD_TEST_PROJECT_LINK_LIBRARIES}
        ${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME}
        ${AMD_TEST_PROJECT_LOCAL_LINK_LIBRARIES}
        ${AMD_TEST_PROJECT_3RD_PARTY_TEST_BUNDLE_INTERFACE_NAME}
)

set_target_properties(${PROJECT_NAME} 
    PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
)

add_dependencies(${PROJECT_NAME} 
    ${AMD_TARGET_NAME}_all
    ${AMD_TEST_PROJECT_LINK_LIBRARIES}
    ${AMD_TEST_PROJECT_COMMON_TARGET_NAME}
    ${AMD_TEST_PROJECT_HELPERS_TARGET_NAME}
    ${AMD_TEST_PROJECT_PLUGINS_TARGET_NAME}
    tst_unit
)

#   Enabled coverage for the tests with debug build type
if(CMAKE_BUILD_TYPE MATCHES "Debug" AND AMD_APP_ENABLE_TESTS_COVERAGE)
    add_compile_options(${PROJECT_NAME} 
        --coverage -g -O0
    )
    target_link_options(${PROJECT_NAME} 
        --coverage
    )

    #
    find_program(LCOV lcov)
    find_program(GENHTML genhtml)
    if(LCOV AND GENHTML)
        if(HAS_DEBUG_MODE_ENABLED)
            developer_status_message("DEVEL" "  >> Coverage report dependencies: 'LCOV, GENHTML' found ...")
        endif()

        # Add custom target for coverage report generation
        add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}.info
            COMMAND ${CMAKE_CTEST_COMMAND} -V
            COMMAND ${LCOV} --zerocounters --directory .
            COMMAND ${LCOV} --capture --directory . --output-file ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}.info
            COMMAND ${LCOV} --remove ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}.info '/usr/*' '*/3rd_party/*' '*/tests/*' 
                            --output-file ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}_filtered.info
            COMMAND ${GENHTML} ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}_filtered.info 
                            --output-directory ${CMAKE_BINARY_DIR}/coverage_report
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            DEPENDS ${PROJECT_NAME}
            COMMENT "  >> Generating coverage report for: '${PROJECT_NAME}' ..."
        )
        add_custom_target(${AMD_TEST_PROJECT_COVERAGE_NAME} 
            DEPENDS ${CMAKE_BINARY_DIR}/${AMD_TEST_PROJECT_COVERAGE_NAME}.info
        )
    else()
        if(HAS_DEBUG_MODE_ENABLED)
            developer_status_message("DEVEL" "  >> Coverage report dependencies: 'LCOV, GENHTML' not found ...")
            developer_status_message("DEVEL" "  >> Skipping coverage report generation for: '${PROJECT_NAME}' ...")
        endif()
    endif()
endif()

#
# CTest test discovery
include(Catch)
catch_discover_tests(${PROJECT_NAME}
    TEST_SPEC "[common],[helpers],[plugins]"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)


## End of CMakeLists.txt

