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
#     OR point CMAKE_PREFIX_PATH to the root directory containing hsa.h and libhsa-runtime??.so
#        (for the default rocm installation, this is /opt/rocm
#         export CMAKE_PREFIX_PATH=/opt/rocm:${CMAKE_PREFIX_PATH}
#        )

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
  add_definitions(-DDEBUG_TRACE=1)
endif()

# Linux Compiler options
set(CMAKE_CXX_FLAGS "-std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dev")
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=1000000")
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
find_file(HSA_HEADER NAME hsa.h PATHS ${ROCR_INC_DIR} /opt/rocm)
find_file(HSA_HEADER NAME hsa.h)
get_filename_component(HSA_HEADER_DIR ${HSA_HEADER} DIRECTORY)
if(${HSA_HEADER_DIR} MATCHES include)
  set(ROCR_INC_DIR ${HSA_HEADER_DIR})
else()
  get_filename_component(ROCR_INC_DIR "${HSA_HEADER_DIR}/.." DIRECTORY)
endif()

find_file(HSA_RUNTIME_LIB NAME ${CORE_RUNTIME_LIBRARY}.so PATHS ${ROCR_LIB_DIR} /opt/rocm/ /opt/rocm/lib)
find_file(HSA_RUNTIME_LIB NAME ${CORE_RUNTIME_LIBRARY}.so)
get_filename_component(ROCR_LIB_DIR ${HSA_RUNTIME_LIB} DIRECTORY)


if(NOT DEFINED CMAKE_MODULE_PATH)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
endif()
include(utils)

#
# Set the package version for the test. It is critical that this
# value track what is used in the test source. The code from utils
# module will parse the string into major, minor and patch sub-fields
#
get_version("1.0.0")

# Bind the Major, Minor and Patch values
set(BUILD_VERSION_MAJOR ${VERSION_MAJOR})
set(BUILD_VERSION_MINOR ${VERSION_MINOR})
set(BUILD_VERSION_PATCH ${VERSION_PATCH})

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

# Specify name of project to build, install and package
set(PROJECT_NAME "rocm_bandwidth_test")
set(TEST_NAME "${PROJECT_NAME}")
project(${PROJECT_NAME})

# Add directories to look for header files to compile
INCLUDE_DIRECTORIES(${ROCR_INC_DIR})

# Add directories to look for library files to link 
LINK_DIRECTORIES(${ROCR_LIB_DIR})

# Add sources that belong to the project
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} Src)

# Build and link the test program
add_executable(${TEST_NAME} ${Src})
target_link_libraries(${TEST_NAME} ${CORE_RUNTIME_TARGET} ${ROC_THUNK_NAME} c stdc++ dl pthread rt)

# Add install directives for rocm_bandwidth_test
install(TARGETS ${TEST_NAME} RUNTIME DESTINATION bin)

# Add packaging directives for rocm_bandwidth_test
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "AMD")
set(CPACK_PACKAGE_VERSION_MAJOR ${BUILD_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${BUILD_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${BUILD_VERSION_PATCH})
set(CPACK_PACKAGE_CONTACT "Advanced Micro Devices Inc.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Test to measure PciE bandwidth on ROCm platforms")

# Debian package specific variables
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/RadeonOpenCompute/rocm_bandwidth_test")

# RPM package specific variables
if(DEFINED CPACK_PACKAGING_INSTALL_PREFIX)
  set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${CPACK_PACKAGING_INSTALL_PREFIX} ${CPACK_PACKAGING_INSTALL_PREFIX}/bin")
endif()

include(CPack)

