# Copyright (c) Advanced Micro Devices, Inc., or its affiliates.

# Test dependencies
include(FetchContent)

set(ROCM_WARN_TOOLCHAIN_VAR OFF CACHE BOOL "")

# Find or download/install rocm-cmake project
find_package(ROCmCMakeBuildTools 0.11.0 CONFIG QUIET PATHS "${ROCM_PATH}")
if((NOT ROCmCMakeBuildTools_FOUND) OR INSTALL_DEPENDENCIES)
  message(STATUS "ROCmCMakeBuildTools not found. Checking for ROCM (deprecated)")
  find_package(ROCM 0.7.3 CONFIG QUIET PATHS "${ROCM_PATH}") # deprecated fallback
  if((NOT ROCM_FOUND) OR INSTALL_DEPENDENCIES)
    message(STATUS "ROCM (deprecated) not found. Downloading and building ROCmCMakeBuildTools")
    set(PROJECT_EXTERN_DIR ${CMAKE_CURRENT_BINARY_DIR}/extern)
    set(rocm_cmake_tag "rocm-6.4.0" CACHE STRING "rocm-cmake tag to download")
    FetchContent_Declare(
      rocm-cmake
      GIT_REPOSITORY https://github.com/ROCm/rocm-cmake.git
      GIT_TAG ${rocm_cmake_tag}
      SOURCE_SUBDIR "DISABLE ADDING TO BUILD"
    )
    FetchContent_MakeAvailable(rocm-cmake)
    message(STATUS "rocm-cmake_SOURCE_DIR: ${rocm-cmake_SOURCE_DIR}")
    find_package(ROCmCMakeBuildTools CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${rocm-cmake_SOURCE_DIR}")
    message(STATUS "Found ROCmCmakeBuildTools version: ${ROCmCMakeBuildTools_VERSION}")
  endif()
elseif(ROCmCMakeBuildTools_FOUND)
  message(STATUS "Found ROCmCmakeBuildTools version: ${ROCmCMakeBuildTools_VERSION}")
endif()

# Find available local ROCM targets
# NOTE: This will eventually be part of ROCm-CMake and should be removed at that time
function(rocm_local_targets VARIABLE)
  set(${VARIABLE} "NOTFOUND" PARENT_SCOPE)
  find_program(_rocm_agent_enumerator rocm_agent_enumerator HINTS /opt/rocm/bin ENV ROCM_PATH)
  if(NOT _rocm_agent_enumerator STREQUAL "_rocm_agent_enumerator-NOTFOUND")
    execute_process(
      COMMAND "${_rocm_agent_enumerator}"
      RESULT_VARIABLE _found_agents
      OUTPUT_VARIABLE _rocm_agents
      ERROR_QUIET
      )
    if (_found_agents EQUAL 0)
      string(REPLACE "\n" ";" _rocm_agents "${_rocm_agents}")
      unset(result)
      foreach (agent IN LISTS _rocm_agents)
        if (NOT agent STREQUAL "gfx000")
          list(APPEND result "${agent}")
        endif()
      endforeach()
      if(result)
        list(REMOVE_DUPLICATES result)
        set(${VARIABLE} "${result}" PARENT_SCOPE)
      endif()
    endif()
  endif()
endfunction()

include(ROCMSetupVersion)
include(ROCMCreatePackage)
include(ROCMInstallTargets)
include(ROCMPackageConfigHelpers)
include(ROCMInstallSymlinks)
include(ROCMCheckTargetIds)
include(ROCMClients)
include(ROCMHeaderWrapper)
