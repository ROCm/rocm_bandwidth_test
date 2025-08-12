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

if (DEFINED ENV{ROCM_PATH})
    set(ROCM_PATH "$ENV{ROCM_PATH}" CACHE STRING "ROCm install directory")
else()
    set(ROCM_PATH "/opt/rocm" CACHE STRING "ROCm install directory")
endif()

set(TARGET_RPATH "\$ORIGIN:\$ORIGIN/llvm/lib")
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${TARGET_RPATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

set(AMD_PROJECT_NAME "libamd_tb")
project(${AMD_PROJECT_NAME} VERSION 1.59.00 LANGUAGES CXX C)


# Default GPU architectures to build
#==================================================================================================
set(DEFAULT_GPUS
      gfx906
      gfx908
      gfx90a
      gfx942
      gfx1030
      gfx1100
      gfx1101
      gfx1102
      gfx1200
      gfx1201)

# Build only for local GPU architecture
if (BUILD_LOCAL_GPU_TARGET_ONLY)
  message(STATUS "Building only for local GPU target")
  if (COMMAND rocm_local_targets)
    rocm_local_targets(DEFAULT_GPUS)
  else()
    message(WARNING "Unable to determine local GPU targets. Falling back to default GPUs.")
  endif()
endif()

# Determine which GPU architectures to build for
set(GPU_TARGETS "${DEFAULT_GPUS}" CACHE STRING "Target default GPUs if GPU_TARGETS is not defined.")

# Check if clang compiler can offload to GPU_TARGETS
if (COMMAND rocm_check_target_ids)
  message(STATUS "Checking for ROCm support for GPU targets: " "${GPU_TARGETS}")
  rocm_check_target_ids(SUPPORTED_GPUS TARGETS ${GPU_TARGETS})
else()
  message(WARNING "Unable to check for supported GPU targets. Falling back to default GPUs.")
  set(SUPPORTED_GPUS ${DEFAULT_GPUS})
endif()

set(COMPILING_TARGETS "${SUPPORTED_GPUS}" CACHE STRING "GPU targets to compile for.")
message(STATUS "Compiling for ${COMPILING_TARGETS}")

foreach(target ${COMPILING_TARGETS})
 list(APPEND static_link_flags --offload-arch=${target})
endforeach()
list(JOIN static_link_flags " " flags_str)
set(CMAKE_CXX_FLAGS "${flags_str} ${CMAKE_CXX_FLAGS}")

##
add_compile_options("-O3")
add_link_options("-L${ROCM_PATH}/lib")

include_directories(${ROCM_PATH}/include)
find_library(IBVERBS_LIBRARY ibverbs)
if (IBVERBS_LIBRARY)
    if (DEFINED ENV{DISABLE_NIC_EXEC})
        message(STATUS "Disabling NIC Executor support")
    else()
        message(STATUS "Found ibverbs: ${IBVERBS_LIBRARY}.  Building with NIC executor support. Can set DISABLE_NIC_EXEC=1 to disable")
        add_definitions(-DNIC_EXEC_ENABLED)
        link_libraries(ibverbs)
    endif()
else()
    message(WARNING "IBVerbs library not found.  Building without NIC executor support")
endif()

link_libraries(numa hsa-runtime64 pthread)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY .)
set(AMD_WORK_BENCH_COMMON_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../common/)
set(AMD_WORK_BENCH_LOCAL_PLUGINS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/Presets)
##add_executable(${AMD_PROJECT_NAME} src/Client.cpp)
add_library(${AMD_PROJECT_NAME} SHARED src/Client.cpp include/plugin_tb_link.hpp)
target_include_directories(${AMD_PROJECT_NAME} PRIVATE ${AMD_WORK_BENCH_COMMON_INCLUDE_DIRECTORY} ${AMD_WORK_BENCH_LOCAL_PLUGINS_INCLUDE_DIRS})
if (BUILD_INTERNAL_BINARY_VERSION) 
    set(BUILD_BINARY_VERSION ${BUILD_INTERNAL_BINARY_VERSION})
else()
    set(BUILD_BINARY_VERSION 1)
endif()

set_target_properties(${AMD_PROJECT_NAME}
    PROPERTIES
        INSTALL_RPATH "${TARGET_RPATH}"
        INSTALL_RPATH_USE_LINK_PATH FALSE
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/"
        #LIBRARY_OUTPUT_NAME "libamd_tb"
        SOVERSION ${BUILD_BINARY_VERSION}
        PREFIX ""
)

set_property(TARGET ${AMD_PROJECT_NAME}
    PROPERTY
        POSITION_INDEPENDENT_CODE ON
)


## Under ROCm build environment, package might not be available/installed yet
#if (NOT AMD_APP_ROCM_BUILD_PACKAGE)
#    find_package(ROCM 0.8 REQUIRED PATHS ${ROCM_PATH})
#endif()

#include(ROCMInstallTargets)
#include(ROCMCreatePackage)
set(ROCMCHECKS_WARN_TOOLCHAIN_VAR OFF)
set(LIBRARY_NAME ${AMD_PROJECT_NAME})
#rocm_install(TARGETS ${AMD_PROJECT_NAME} COMPONENT devel)
#rocm_package_add_dependencies(DEPENDS numactl hsa-rocr)

#rocm_create_package(
#    NAME ${LIBRARY_NAME}
#    DESCRIPTION "TransferBench package"
#    MAINTAINER "RCCL Team <gilbert.lee@amd.com>"
#)

# 
## Install target
set(LIBAMD_TB_FILES_TO_INSTALL "")
file(GLOB LIBAMD_TB_LIST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/libamd_tb.so*")
if(LIBAMD_TB_LIST_FILES) 
    list(APPEND LIBAMD_TB_FILES_TO_INSTALL ${LIBAMD_TB_LIST_FILES})
endif()
#set(TARGET_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib)

# 
## Install target: Provided by top CMakeLists.txt, so we can use a 'dynamic_install()' function
#if(LIBAMD_TB_FILES_TO_INSTALL)
#    file(MAKE_DIRECTORY ${TARGET_INSTALL_DIRECTORY})
#    install(FILES ${LIBAMD_TB_FILES_TO_INSTALL} 
#            DESTINATION ${TARGET_INSTALL_DIRECTORY}
#    )
#endif()    

## End of CMakeLists.txt
