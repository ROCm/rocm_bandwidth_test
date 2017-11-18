cmake_minimum_required(VERSION 2.8.0)

#
#  Setup build environment
#
#  1) Setup env var ROCR_INC_DIR and ROCR_LIB_DIR to point to 
#     ROC Runtime header and libraries seperately
#     
#     export ROCR_INC_DIR="Path to ROC Runtime headers"
#`
#     export ROCR_LIB_DIR="Path to ROC Runtime libraries"
#
#  2) Make an new folder called build under root folder
#
#     mkdir build
#
#  3) Enter into folder of build, and run CMAKE to generate makefile
#     and make it
#
#     cd build
#     cmake -DROCR_INC_DIR=ROCR_INC_DIR -DROCR_LIB_DIR=$ROCR_LIB_DIR ..
#     make
#
#     @note: Add -DCMAKE_BUILD_TYPE=Debug if you want to build Debug
#

# Build is not supported on Windows plaform
if(WIN32)
  message("Windows platfom is not supported")
  RETURN()
endif()

# Flag to enable / disable verbose output.
SET(CMAKE_VERBOSE_MAKEFILE on)

# Compiler Preprocessor definitions.
add_definitions(-D__linux__)
add_definitions(-DUNIX_OS)
add_definitions(-DLINUX)
add_definitions(-D__AMD64__)
add_definitions(-D__x86_64__)
add_definitions(-DAMD_INTERNAL_BUILD)
add_definitions(-DLITTLEENDIAN_CPU=1)
add_definitions(-DHSA_LARGE_MODEL=)
add_definitions(-DHSA_DEPRECATED=)

# Enable debug trace
if(DEFINED ENV{CMAKE_DEBUG_TRACE})
  add_definitions (-DDEBUG_TRACE=1)
endif()


# Linux Compiler options
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-math-errno")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmerge-all-constants")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")

# CLANG options
if("$ENV{CXX}" STREQUAL "/usr/bin/clang++")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=1000000")
endif()

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bdynamic -Wl,-z,noexecstack")
set(CMAKE_SKIP_BUILD_RPATH TRUE)

# Check Env vars
if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
  if(DEFINED ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})
  endif()
endif()

if(NOT DEFINED CMAKE_PREFIX_PATH AND DEFINED ENV{CMAKE_PREFIX_PATH})
  set(CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH})
endif()

# Extend Compiler flags based on build type
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
if("${CMAKE_BUILD_TYPE}" STREQUAL debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
  set(CMAKE_BUILD_TYPE "debug")
else()
  set(CMAKE_BUILD_TYPE "release")
endif()

# Extend Compiler flags based on Processor architecture
if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(NBIT 64)
  set(NBITSTR "64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64  -msse -msse2")
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86")
  set(NBIT 32)
  set(NBITSTR "")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif()

# Set project requirements
set(ROC_THUNK_NAME "hsakmt")
set(CORE_RUNTIME_NAME "hsa-runtime")
set(ROC_THUNK_LIBRARY "lib${ROC_THUNK_NAME}")
set(CORE_RUNTIME_TARGET "${CORE_RUNTIME_NAME}64")
set(CORE_RUNTIME_LIBRARY "lib${CORE_RUNTIME_TARGET}")

# Determine Roc Runtime header files are accessible
if(NOT EXISTS ${ROCR_INC_DIR}/hsa/hsa.h)
  message("ERROR: ROC Runtime headers can't be found under specified path")
  RETURN()
endif()

if(NOT EXISTS ${ROCR_LIB_DIR}/${CORE_RUNTIME_LIBRARY}.so)
  message("ERROR: ROC Runtime libraries can't be found under sprcified path")
  RETURN()
endif()

# Basic Tool Chain Information
message(" ")
message("----------------NBIT: ${NBIT}")
message("-----------BuildType: ${CMAKE_BUILD_TYPE}")
message("------------Compiler: ${CMAKE_CXX_COMPILER}")
message("----Compiler-Version: ${CMAKE_CXX_COMPILER_VERSION}")
message("-----HSA-Runtime-Inc: ${ROCR_INC_DIR}")
message("-----HSA-Runtime-Lib: ${ROCR_LIB_DIR}")
message("-----CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("---CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(" ")

set(PROJECT_NAME "rocm_bandwidth_test")
set(TEST_NAME "${PROJECT_NAME}")
project(${PROJECT_NAME})

INCLUDE_DIRECTORIES(${ROCR_INC_DIR})

LINK_DIRECTORIES(${ROCR_LIB_DIR})

# Add sources that belong to the project
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} Src)

add_executable(${TEST_NAME} ${Src})
target_link_libraries(${TEST_NAME} ${CORE_RUNTIME_TARGET} ${ROC_THUNK_NAME} c stdc++ dl pthread rt)

