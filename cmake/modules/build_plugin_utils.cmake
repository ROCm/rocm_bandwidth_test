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

set(AMD_TARGET_NAME "rocm_bandwidth_test")
set(AMD_TARGET_LIBNAME "libamd_work_bench")

macro(add_amd_work_bench_plugin)
    # Parse arguments
    set(options AMD_WORK_BENCH_LIBRARY_PLUGIN)
    set(oneValueArgs NAME AMD_WORK_BENCH_VERSION)
    set(multiValueArgs SOURCES INCLUDES LIBRARIES FEATURES)
    cmake_parse_arguments(AMD_WORK_BENCH_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(AMD_WORK_BENCH_PLUGIN_AMD_WORK_BENCH_VERSION)
        message(STATUS ">> Building plugin: ${AMD_WORK_BENCH_PLUGIN_NAME} for: ${AMD_TARGET_NAME} v${AMD_WORK_BENCH_PLUGIN_AMD_WORK_BENCH_VERSION}")
        set(AMD_TARGET_VERSION_TEXT "${AMD_WORK_BENCH_PLUGIN_AMD_WORK_BENCH_VERSION}")
    endif()

    if(AMD_APP_STATIC_LINK_PLUGINS)
        set(AMD_WORK_BENCH_PLUGIN_LIBRARY_TYPE STATIC)
        target_link_libraries(${AMD_TARGET_LIBNAME} PUBLIC ${AMD_WORK_BENCH_PLUGIN_NAME})

        #
        # The 'dist/web folder contains minified bundles of the plugins, along with source mapped versions of the library
        #
        configure_file(${CMAKE_SOURCE_DIR}/dist/web/plugin-bundle.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/plugin-bundle.cpp @ONLY)
        target_sources(awb_main PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/plugin-bundle.cpp)
        set(AMD_WORK_BENCH_PLUGIN_SUFFIX ".amdplug")
    else()
        if(AMD_WORK_BENCH_PLUGIN_AMD_WORK_BENCH_LIBRARY_PLUGIN)
            set(AMD_WORK_BENCH_PLUGIN_LIBRARY_TYPE SHARED)
            set(AMD_WORK_BENCH_PLUGIN_SUFFIX ".amdlplug")
        else()
            set(AMD_WORK_BENCH_PLUGIN_LIBRARY_TYPE MODULE)
            set(AMD_WORK_BENCH_PLUGIN_SUFFIX ".amdplug")
        endif()
    endif()

    # Define new project for plugin
    project(${AMD_WORK_BENCH_PLUGIN_NAME})

    # Create a new shared library for the plugin source code
    add_library(${AMD_WORK_BENCH_PLUGIN_NAME} ${AMD_WORK_BENCH_PLUGIN_LIBRARY_TYPE} ${AMD_WORK_BENCH_PLUGIN_SOURCES})

    # Add include directories and link libraries
    target_include_directories(${AMD_WORK_BENCH_PLUGIN_NAME} PUBLIC ${AMD_WORK_BENCH_PLUGIN_INCLUDES})
    target_link_libraries(${AMD_WORK_BENCH_PLUGIN_NAME} PUBLIC ${AMD_WORK_BENCH_PLUGIN_LIBRARIES})
    target_link_libraries(${AMD_WORK_BENCH_PLUGIN_NAME} PRIVATE ${AMD_TARGET_LIBNAME} ${FMT_LIBRARIES})

    # add_include_from_library(${AMD_WORK_BENCH_PLUGIN_NAME} libzip)

    # Add project name and version
    target_compile_definitions(${AMD_WORK_BENCH_PLUGIN_NAME} PRIVATE AMD_WORK_BENCH_PROJECT_NAME="${AMD_WORK_BENCH_PLUGIN_NAME}")
    target_compile_definitions(${AMD_WORK_BENCH_PLUGIN_NAME} PRIVATE AMD_WORK_BENCH_VERSION="${AMD_TARGET_VERSION_TEXT}")
    target_compile_definitions(${AMD_WORK_BENCH_PLUGIN_NAME} PRIVATE AMD_WORK_BENCH_PLUGIN_NAME=${AMD_WORK_BENCH_PLUGIN_NAME})

    # Enable required compiler flags
    setup_unity_build(${AMD_WORK_BENCH_PLUGIN_NAME})
    setup_compiler_flags(${AMD_WORK_BENCH_PLUGIN_NAME})
    add_cppcheck(${AMD_WORK_BENCH_PLUGIN_NAME})

    # Configure build properties
    set_target_properties(${AMD_WORK_BENCH_PLUGIN_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_MAIN_OUTPUT_DIRECTORY}/plugins"
        CXX_STANDARD 20
        PREFIX ""
        SUFFIX ${AMD_WORK_BENCH_PLUGIN_SUFFIX}
        POSITION_INDEPENDENT_CODE ON
    )


    # Add features
    set(FEATURE_DEFINE_CONTENT)

    if(AMD_WORK_BENCH_PLUGIN_FEATURES)
        list(LENGTH AMD_WORK_BENCH_PLUGIN_FEATURES AMD_WORK_BENCH_PLUGIN_FEATURES_COUNT)
        math(EXPR AMD_WORK_BENCH_PLUGIN_FEATURES_COUNT "${AMD_WORK_BENCH_PLUGIN_FEATURES_COUNT} - 1" OUTPUT_FORMAT DECIMAL)

        foreach(index RANGE 0 ${AMD_WORK_BENCH_PLUGIN_FEATURES_COUNT} 2)
            list(SUBLIST AMD_WORK_BENCH_PLUGIN_FEATURES ${index} 2 AMD_WORK_BENCH_PLUGIN_FEATURE)
            list(GET AMD_WORK_BENCH_PLUGIN_FEATURE 0 feature_define)
            list(GET AMD_WORK_BENCH_PLUGIN_FEATURE 1 feature_description)

            string(TOUPPER ${feature_define} feature_define)
            add_definitions(-DAMD_WORK_BENCH_PLUGIN_${AMD_WORK_BENCH_PLUGIN_NAME}_FEATURE_${feature_define}=0)
            set(FEATURE_DEFINE_CONTENT "${FEATURE_DEFINE_CONTENT}{ \"${feature_description}\", AMD_WORK_BENCH_FEATURE_ENABLED(${feature_define}) },")
        endforeach()
    endif()

    target_compile_options(${AMD_WORK_BENCH_PLUGIN_NAME} PRIVATE -DAMD_WORK_BENCH_PLUGIN_FEATURES_CONTENT=${FEATURE_DEFINE_CONTENT})

    # Build the dependency list so the new plugin can be added and built
    if(TARGET ${AMD_TARGET_NAME}_all)
        add_dependencies(${AMD_TARGET_NAME}_all ${AMD_WORK_BENCH_PLUGIN_NAME})
    endif()

    if(AMD_WORK_BENCH_EXTERNAL_PLUGIN_BUILD)
        install(TARGETS ${AMD_WORK_BENCH_PLUGIN_NAME} DESTINATION ".")
    endif()

    # Fix rpath
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(PLUGIN_RPATH "")
        list(APPEND PLUGIN_RPATH "$ORIGIN")

        if(AMD_WORK_BENCH_PLUGIN_ADD_INSTALL_PREFIX_TO_RPATH)
            list(APPEND PLUGIN_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
        endif()

        set_target_properties(
            ${AMD_WORK_BENCH_PLUGIN_NAME}
            PROPERTIES
            INSTALL_RPATH_USE_ORIGIN ON
            INSTALL_RPATH "${PLUGIN_RPATH}")
    endif()

    # TODO: Fix and retest it with proper 'test' directories in-place
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt AND AMD_APP_BUILD_TESTS AND AMD_WORK_BENCH_ENABLE_PLUGIN_TESTS)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
        target_link_libraries(${AMD_WORK_BENCH_PLUGIN_NAME} PUBLIC ${AMD_WORK_BENCH_PLUGIN_NAME}_tests)
        target_compile_definitions(${AMD_WORK_BENCH_PLUGIN_NAME}_tests PRIVATE AMD_WORK_BENCH_PROJECT_NAME="${AMD_WORK_BENCH_PLUGIN_NAME}-tests")
    endif()
endmacro()

macro(enable_plugin_feature feature_name)
    string(TOUPPER ${feature_name} feature_name)

    if(NOT(feature_name IN_LIST AMD_WORK_BENCH_PLUGIN_FEATURES))
        message(FATAL_ERROR ">> Feature ${feature_name} is not enabled for plugin ${AMD_WORK_BENCH_PLUGIN_NAME}")
    endif()

    remove_definitions(-DAMD_WORK_BENCH_PLUGIN_${AMD_WORK_BENCH_PLUGIN_NAME}_FEATURE_${feature_name}=0)
    add_definitions(-DAMD_WORK_BENCH_PLUGIN_${AMD_WORK_BENCH_PLUGIN_NAME}_FEATURE_${feature_name}=1)
endmacro()
