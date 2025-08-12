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

cmake_minimum_required(VERSION 3.20)

include(ExternalProject)
if (NOT USE_SYSTEM_TB_ENGINE)
    message(WARNING ">> Building: TransferBench (TB) as a shared library (plugin)...")

    # Build the TransferBench shared library at config time
    #execute_process(
    #    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build_libamd_tb.sh
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/
    #    RESULT_VARIABLE BUILD_SCRIPT_RESULT
    #    OUTPUT_VARIABLE BUILD_SCRIPT_OUTPUT
    #)
    #if (NOT BUILD_SCRIPT_RESULT EQUAL 0)
    #    message(FATAL_ERROR ">> Failed to build TransferBench (TB) shared library (plugin) with error: ${BUILD_SCRIPT_OUTPUT}")
    #endif()

    # Build the TransferBench shared library at build time
    #add_custom_command(
    #    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build_libamd_tb_done.log
    #    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build_libamd_tb.sh
    #    COMMAND touch ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build_libamd_tb_done.log
    #    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/
    #    COMMENT "Building 'libamd_tb' dependency library ..."
    #)
    #add_custom_target(build_libamd_tb DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/deps/libamd_tb.so)
    #add_subdirectory(transferbench ${CMAKE_CURRENT_BINARY_DIR}/transferbench EXCLUDE_FROM_ALL)

    # Set the include directories
    if(DEFINED ENV{ROCM_PATH})
        set(ROCM_PATH $ENV{ROCM_PATH})
    else()
        if(DEFINED ENV{ROCM_INSTALL_PATH})
            set(ROCM_PATH $ENV{ROCM_INSTALL_PATH})
        elseif(DEFINED ENV{ROCM_VERSION})
            set(ROCM_PATH "/opt/rocm-$ENV{ROCM_VERSION}")
        else()
            set(ROCM_PATH "/opt/rocm")
        endif()
    endif()
    set(HIPCC_CXX_PATH "${ROCM_PATH}")
    if (NOT HIPCC_CXX_PATH STREQUAL "")
        set(HIPCC_CXX_PATH "${HIPCC_CXX_PATH}/bin")
    endif()

    set(AMD_WORK_BENCH_COMMON_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../common/)
    set(TB_PLUGIN_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/include ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/include/Presets ${ROCM_PATH}/include)
else()
    find_package(TransferBench REQUIRED)
endif()

set(DEPS_DIRECTORY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(LIB_DIRECTORY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
file(MAKE_DIRECTORY ${DEPS_DIRECTORY_PATH})
file(MAKE_DIRECTORY ${LIB_DIRECTORY_PATH})
set(FILE_DEPS_LIST_COPY "")
message(WARNING ">> Trying to find 'hipcc', required for this component (AMD_TB_LIBRARY) ...")

find_program(HIPCC_COMPILER_CXX hipcc HINTS ${HIPCC_CXX_PATH} ${CMAKE_CXX_COMPILER_PATH} ${CMAKE_CXX_COMPILER})
if(HIPCC_COMPILER_CXX)
    message(STATUS "  >> Found 'hipcc' compiler: ${HIPCC_COMPILER_CXX}")
else()
    message(FATAL_ERROR "  >> Failed to find 'hipcc' compiler")
endif()

get_property(BUILD_TYPE GLOBAL PROPERTY CMAKE_BUILD_TYPE)
message(STATUS "  >> Build type: '${AMD_PROJECT_BUILD_TYPE}' v:${AMD_APP_ROCM_BUILD_BINARY_VERSION_INTERNAL}")
ExternalProject_Add(
    AMD_TB_LIBRARY
    ##PREFIX "lib"
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/
    BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build
    ##INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/
    CONFIGURE_COMMAND ""
    ##  Defined as empty string to avoid the default install step, and uses:
    ##  'ExternalProject_Add_Step(AMD_TB_LIBRARY install'
    ## INSTALL_COMMAND ""
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E make_directory ${LIB_DIRECTORY_PATH} &&
        ${CMAKE_COMMAND}
            -DINSTALL_DEPENDEE_BASE_PATH=${CMAKE_CURRENT_SOURCE_DIR}
            -DINSTALL_SOURCE_LIBAMD_TB_PATH=${DEPS_DIRECTORY_PATH}
            -DINSTALL_TARGET_LIBAMD_TB_PATH=${LIB_DIRECTORY_PATH}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/install_libamd_tb.cmake

    BUILD_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build_libamd_tb.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/transferbench/build
        ${AMD_PROJECT_BUILD_TYPE}
        ${AMD_APP_ROCM_BUILD_BINARY_VERSION_INTERNAL}
        ${HIPCC_COMPILER_CXX}
    COMMENT ">> Building 'libamd_tb' dependency library ..."
)

foreach(file_dependency ${FILE_DEPS_LIST_COPY})
    get_filename_component(file_name ${file_dependency} NAME)
    #ExternalProject_Add_Step(AMD_TB_LIBRARY copy_${file_name}
    ##  CMake will execute the commands defined here overriding both the external project's default
    ##  install target and any INSTALL_COMMAND provided in the ExternalProject_Add call.
    ##  DEPENDS build; build is an automagically step created by the ExternalProject_Add call, so we
    ##  don't need to define it/one
    ExternalProject_Add_Step(AMD_TB_LIBRARY install
        COMMENT "  >> Copying ${file_name} to ${DEPS_DIRECTORY_PATH} ..."
        file(MAKE_DIRECTORY ${DEPS_DIRECTORY_PATH})
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file_dependency} ${DEPS_DIRECTORY_PATH}/${file_name}
        DEPENDS build
    )
endforeach()


add_custom_target(libamd_tb_dependency ALL DEPENDS AMD_TB_LIBRARY)
add_library(amd_tb_lib SHARED IMPORTED)
set_target_properties(amd_tb_lib PROPERTIES IMPORTED_LOCATION "${LIB_DIRECTORY_PATH}/libamd_tb.so")

##
# Add the plugin
set(AMD_WORK_BENCH_PLUGIN_NAME_BUILD "transferbench")
add_amd_work_bench_plugin(
    NAME
        ${AMD_WORK_BENCH_PLUGIN_NAME_BUILD}

    SOURCES
        src/plugin_transferbench.cpp
        src/view_transferbench.cpp
        transferbench/include/plugin_tb_link.hpp


    INCLUDES
        include
        ${AMD_WORK_BENCH_COMMON_INCLUDE_DIRECTORY}
        ${TB_PLUGIN_INCLUDE_DIRS}


    LIBRARIES
        ${JTHREAD_LIBRARIES}
        ${TB_LIBRARY}
        pthread
)

## Add required libraries
add_definitions(-DAMD_LEGACY_PLUGIN_VIEW)
add_dependencies(${AMD_WORK_BENCH_PLUGIN_NAME_BUILD} libamd_tb_dependency)
target_link_libraries(${AMD_WORK_BENCH_PLUGIN_NAME_BUILD} PRIVATE amd_tb_lib)

## End of CMakeLists.txt
