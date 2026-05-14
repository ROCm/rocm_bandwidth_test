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
# --- CMake Toolchain File for using Clang from ROCm build environment ---
#
#   To use this, invoke CMake like this:
#   export ROCM_INSTALL_PATH=/opt/rocm[-${rocm_version}]    # Or ROCm path: ROCM_PATH
#   example:    export ROCM_INSTALL_PATH=/opt/rocm-6.5.0
#               export ROCM_INSTALL_PATH=/opt/rocm
#               export ROCM_INSTALL_PATH=$ROCM_PATH
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=./src/cmake/rocm-clang-toolchain.cmake ...
#
# This toolchain file assumes you are building for the host system (e.g., Linux x86_64)
# but specifically using the Clang toolchain provided with ROCm.
#

#
cmake_minimum_required(VERSION 3.20)


#
# --- CMake OS version checkpoint ---
#
#   For some distros/versions, the default compiler and std library versions are not
#   up to the minimum C++20 (or newer). As those cannot be updated and as our compiler
#   'Lightning/Clang++' is *built without* its 'libc++' component, we are not able to
#   use some much needed features part of the source code.
#   Here we check for those distros/versions, so we can skip the build gracefully with
#   no build failures.
#
#   For now, we are checking for:
#   NAME="Red Hat Enterprise Linux"
#   VERSION_ID="8.8"
#   ||
#   NAME="Debian GNU/Linux"
#   VERSION_ID="10"
#
#   Note:   CMake regex does not support multiline mode by default. So '^' and '$' only
#           match the beginning and end of the entire string, not the start and end of
#           individual lines.
#           string(REGEX MATCH "NAME=\"?([^\n\"]+)\"?" _ "${OS_RELEASE_FILE_INFO}") will cause
#           errors when we have:
#               PRETTY_NAME="Debian GNU/Linux 10 (buster)"
#               NAME="Debian GNU/Linux"
#           We will try to fix it with (prepending a newline manually, simulating line-by-line):
#               string(REGEX MATCH "\nNAME=\"([^\"]+)\"" _name_match "\n${OS_RELEASE_FILE_INFO}")

#
# --- ROCm default compiler/toolchain ---
# If already set, skip further processing.
if(IS_LIGHTNING_CLANG_DEFAULT_COMPILER AND ROCM_CLANG_TOOLCHAIN_USED)
    message(STATUS ">> ROCm 'Lightning Clang++' toolchain is already set as default compiler.")
    return()
endif()


set(SKIP_BUILD_PROCESS OFF)
set(OS_RELEASE_FILE "/etc/os-release")
if(EXISTS ${OS_RELEASE_FILE})
    file(READ "${OS_RELEASE_FILE}" OS_RELEASE_FILE_INFO)
    string(REGEX MATCH "\nNAME=\"([^\"]+)\"" _name_match "\n${OS_RELEASE_FILE_INFO}")
    set(DISTRO_NAME "${CMAKE_MATCH_1}")
    string(REGEX MATCH "\nVERSION_ID=\"([^\"]+)\"" _version_match "\n${OS_RELEASE_FILE_INFO}")
    set(DISTRO_VERSION_ID "${CMAKE_MATCH_1}")

    message(STATUS ">> ROCm Clang Toolchain Environment Detected: '${DISTRO_NAME}', v'${DISTRO_VERSION_ID}'")
    ##  Check for unsupported distros/versions
    ##  That is, distros/versions with compilers and std libraries not supporting C++20 fully.
    if((DISTRO_NAME STREQUAL "Debian GNU/Linux" AND (DISTRO_VERSION_ID VERSION_GREATER_EQUAL "10")))
        #   CACHE INTERNAL makes sure the SKIP_BUILD_PROCESS variable survives into the main CMake context
        set(SKIP_BUILD_PROCESS ON CACHE INTERNAL "Skip build process for this OS version")
        file(WRITE "${CMAKE_BINARY_DIR}/rbt_skip_build_process.flag" "1")
        message(WARNING ">> Build not supported: '${DISTRO_NAME}', v'${DISTRO_VERSION_ID}'")
    endif()
else()
    set(SKIP_BUILD_PROCESS ON)
    message(WARNING ">> Unable to read OS release file: '${OS_RELEASE_FILE}'")
endif()


#
# --- ROCm Build Path Setup ---
if(DEFINED ENV{ROCM_INSTALL_PATH})
    set(ROCM_BASE_PATH "$ENV{ROCM_INSTALL_PATH}")
elseif(DEFINED ENV{ROCM_PATH})
    set(ROCM_BASE_PATH "$ENV{ROCM_PATH}")
else()
    message(FATAL_ERROR ">> No ROCM_INSTALL_PATH or ROCM_PATH environment variable is set. "
                        "  That is a requirement to locate 'Lightning Clang++'")
endif()

#
# --- Path to Clang/LLVM root directory, (ie: /opt/rocm/lib/llvm/) ---
if(DEFINED ENV{ROCM_LLVM_PATH})
    set(ROCM_LLVM_BIN_DIR "$ENV{ROCM_LLVM_PATH}/bin")
else()
    set(ROCM_LLVM_BIN_DIR "${ROCM_BASE_PATH}/lib/llvm/bin")
endif()

message(STATUS ">> ROCM_INSTALL_PATH detected: '${ROCM_BASE_PATH}'")
message(STATUS ">> Expecting Clang/LLVM tools in: '${ROCM_LLVM_BIN_DIR}'")
if(NOT IS_DIRECTORY "${ROCM_LLVM_BIN_DIR}")
    message(FATAL_ERROR ">> ROCM_LLVM_BIN_DIR is not a valid directory: '${ROCM_LLVM_BIN_DIR}'\n"
                        "  Check ROCM_INSTALL_PATH and the LLVM binary path structure.")
endif()

#
# --- Compilers and Tools ---
# Find Clang C and C++ compilers within the ROCm LLVM binary directory
# NO_DEFAULT_PATH ensures CMake only looks in the HINTS path first for these specific finds.
# REQUIRED will cause CMake to stop with an error if the compiler is not found there.
find_program(CMAKE_C_COMPILER
    NAMES clang
    HINTS "${ROCM_LLVM_BIN_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
)
find_program(CMAKE_CXX_COMPILER
    NAMES clang++
    HINTS "${ROCM_LLVM_BIN_DIR}"
    NO_DEFAULT_PATH
    REQUIRED
)


# Minimum required version of Clang
if(CMAKE_CXX_COMPILER)
    set(CLANG_COMPILER_MAJOR_VERSION_REQUIRED "19")
    set(CLANG_COMPILER_MINOR_VERSION_REQUIRED "0")
    set(CLANG_COMPILER_REVISION_VERSION_REQUIRED "0")
    set(CLANG_COMPILER_MINIMUM_VERSION_REQUIRED "${CLANG_COMPILER_MAJOR_VERSION_REQUIRED}.${CLANG_COMPILER_MINOR_VERSION_REQUIRED}.${CLANG_COMPILER_REVISION_VERSION_REQUIRED}")

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        OUTPUT_VARIABLE CLANG_COMPILER_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Check if the version is valid
    string(REGEX MATCHALL "[0-9]+" CLANG_COMPILER_VERSION_COMPONENTS "${CLANG_COMPILER_VERSION}")
    if(CLANG_COMPILER_VERSION_COMPONENTS)
        list(GET CLANG_COMPILER_VERSION_COMPONENTS 0 CLANG_COMPILER_VERSION_MAJOR)
        list(GET CLANG_COMPILER_VERSION_COMPONENTS 1 CLANG_COMPILER_VERSION_MINOR)
        list(GET CLANG_COMPILER_VERSION_COMPONENTS 2 CLANG_COMPILER_VERSION_REVISION)
        set(CLANG_COMPILER_FULL_VERSION "${CLANG_COMPILER_VERSION_MAJOR}.${CLANG_COMPILER_VERSION_MINOR}.${CLANG_COMPILER_VERSION_REVISION}")
        ##
        if(CLANG_COMPILER_VERSION_MAJOR GREATER_EQUAL ${CLANG_COMPILER_MAJOR_VERSION_REQUIRED} AND
           CLANG_COMPILER_VERSION_MINOR GREATER_EQUAL ${CLANG_COMPILER_MINOR_VERSION_REQUIRED})
            set(CLANG_COMPILER_VERSION_RESULT TRUE)
        else()
            set(CLANG_COMPILER_VERSION_RESULT FALSE)
        endif()

        if(NOT CLANG_COMPILER_VERSION_RESULT)
            message(FATAL_ERROR ">> 'Clang++' compiler v'${CLANG_COMPILER_VERSION}' is not as default compiler! Minimum version required: 'v${CLANG_COMPILER_MINIMUM_VERSION_REQUIRED}'")
        endif()
    endif()

else()
    message(FATAL_ERROR ">> 'Clang++' compiler not found in ROCM_INSTALL_PATH: '${ROCM_BASE_PATH}'")
endif()


#
# --- Search Behavior ---
# For ROCm, the ROCM_PATH itself is a root for its specific components (headers, libs).
# We add it to CMAKE_FIND_ROOT_PATH so find_package, find_library etc., look there.
# We use list(PREPEND ...) to ensure ROCM_PATH is searched before system paths for relevant items.
list(PREPEND CMAKE_FIND_ROOT_PATH "${ROCM_BASE_PATH}")
list(REMOVE_DUPLICATES CMAKE_FIND_ROOT_PATH)

# Adjust find behavior.
# 'BOTH' allows searching in CMAKE_FIND_ROOT_PATH (ROCm paths) and then system paths.
# This is often suitable for ROCm which overlays on a standard system.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)    # Don't look for host programs in ROCM_PATH
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

#
# --- Confirmation Message ---
# Note: CMAKE_C_COMPILER_VERSION and CMAKE_CXX_COMPILER_VERSION are populated
#       the needed compiler flags are defined by the 'build_utils.cmake'
#       *after* the 'project() command and language enablement', so they won't be available here.
#
# Set a cached variable to indicate this toolchain is used
set(ROCM_CLANG_TOOLCHAIN_USED TRUE CACHE BOOL "Indicates that the ROCm 'Lightning Clang++' toolchain is in use")
set(IS_LIGHTNING_CLANG_DEFAULT_COMPILER TRUE CACHE BOOL "build_utils.cmake: Indicates that 'Lightning Clang++' is the default compiler")
set(CMAKE_C_COMPILER "${CMAKE_C_COMPILER}" CACHE PATH "C compiler")
set(CMAKE_CXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE PATH "C++ compiler")
message(STATUS ">> Using ROCm 'Lightning Clang++' Toolchain: ${CMAKE_CURRENT_LIST_FILE}")
message(STATUS "  >> C   Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "  >> C++ Compiler:   ${CMAKE_CXX_COMPILER}")
