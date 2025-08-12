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

# Find git and set the GIT_EXECUTABLE variable (good practice, used by submodules)
find_package(Git QUIET)
if(NOT GIT_FOUND)
    message(FATAL_ERROR ">> Git is required to build this project. 'git' not found! ")
endif()
set(CMAKE_GIT_EXECUTABLE ${GIT_EXECUTABLE} CACHE FILEPATH "Path to the Git executable.")


# Build is not supported on Windows plaform
if(WIN32)
    message(FATAL_ERROR "Windows platform is not supported!")
    return()
endif()

# Flag to enable / disable verbose output.
set(CMAKE_VERBOSE_MAKEFILE ON)

# Export compile commands for linters and auto-completers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ standard settings
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ Standard in use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#
# CMake specific directories
set(AMD_APP_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Base directory for this project" FORCE)
set(AMD_APP_CMAKE_DIRECTORY ${AMD_APP_BASE_DIRECTORY}/cmake)
set(AMD_APP_CMAKE_MODULES_DIRECTORY ${AMD_APP_BASE_DIRECTORY}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH "${AMD_APP_CMAKE_MODULES_DIRECTORY}")
set(AMD_EXTERNAL_DEPS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/external)


#
# CMake specific directories

#
# CMake options:
#  - "amd_work_bench"
#  - "libwork_bench"
set(AMD_TARGET_NAME "rocm_bandwidth_test")
set(AMD_TARGET_LIBNAME "libamd_work_bench")
option(AMD_APP_DEBUG_INFO_COMPRESS          "Compressed debug information"                                                      ON)
option(AMD_APP_BUILD_PLUGINS                "Build plugins"                                                                     ON)
option(AMD_APP_STORE_PLUGINS_IN_SHARE       "Store the plugins in the share/${AMD_TARGET_NAME}/plugins"                         OFF)
option(AMD_APP_STATIC_LINK_PLUGINS          "Statically link plugins into the '${AMD_TARGET_NAME}' executable"                  OFF)
option(AMD_APP_LTO_ENABLED                  "Link Time Optimization if/when possible"                                           OFF)
option(AMD_APP_TREAT_WARNINGS_AS_ERRORS     "Treat default warnings as errors"                                                  ON)
option(AMD_APP_ENABLE_UNITY_BUILD           "Build the project using unity build"                                               OFF)
option(AMD_APP_DISABLE_STACKTRACE           "Disable stack trace support (if not 'C++23', 'Boost::stacktrace' is used)"         OFF)
option(AMD_APP_SETUP_SDK                    "Setup SDK structure"                                                               OFF)
option(AMD_APP_BUILD_DOCS                   "Build documentation"                                                               ON)
option(AMD_APP_BUILD_EXAMPLES               "Build examples"                                                                    ON)
option(AMD_APP_BUILD_TESTS                  "Build unit tests"                                                                  OFF)
option(AMD_APP_COMPILER_TRY_CLANG           "Give preference to 'Clang++' compiler if installed and requirements met"           ON)
option(AMD_APP_LINKER_TRY_MOLD              "Give preference to 'Mold' linker (faster) if available"                            ON)
option(AMD_APP_BUILD_HARDENING_ENABLED      "Build the project with hardening flags"                                            ON)
option(AMD_APP_SUBMODULES_TRY_CHECKOUT      "Try to 'git checkout' submodules with 'required commits' (if needed)"              ON)
option(AMD_APP_ROCM_BUILD_AUTO_DETECT       "Auto detect ROCm build environment"                                                ON)
option(AMD_APP_ROCM_BUILD_PACKAGE           "Force build/package under ROCm environment (if available: BUILD_AUTO_DETECT)"      OFF)
option(AMD_APP_ROCM_BUILD_TRY_RPATH         "Try to apply 'RPATH' rules for ROCm build binaries (if needed/applicable)"         ON)
option(AMD_APP_STANDALONE_BUILD_PACKAGE     "Build standalone package (not ROCm based)"                                         OFF)
option(AMD_APP_ENGINEERING_BUILD_PACKAGE    "Build and create a BZ2 Engineering package (ROCm & Standalone are ignored)"        OFF)
option(AMD_APP_DEBUG_BUILD_INFO             "Build messages/info (extra info for Developers)"                                   OFF)

# Setup build utils
include(${AMD_APP_CMAKE_DIRECTORY}/build_utils.cmake)

#
# Specify name of project to build, and validate requirements
setup_build_version(AMD_TARGET_VERSION AMD_TARGET_VERSION_TEXT)
set_variable_in_parent(AMD_WORK_BENCH_VERSION ${AMD_TARGET_VERSION})
## Compiler is set dynamically (check_compiler_requirements()), so it must be prior to 'project()' or 'enable_language(CXX)'
setup_cmake(AMD_TARGET_NAME AMD_WORK_BENCH_VERSION)

##
set(AMD_PROJECT_NAME        ${AMD_TARGET_NAME})
set(AMD_PROJECT_DESCRIPTION "rocm_bandwidth_test utility")
set(AMD_PROJECT_AUTHOR_ORGANIZATION "AMD, Inc.")
set(AMD_PROJECT_GITHUB_REPO         "https://github.com/ROCm/rocm_bandwidth_test")
set(AMD_PROJECT_AUTHOR_DOMAIN       "https://www.amd.com")
set(AMD_PROJECT_AUTHOR_MAINTAINER   "MLSE Team <daniel.oliveira@amd.com>")
set(AMD_PROJECT_VERSION_MAJOR       "")
set(AMD_PROJECT_VERSION_MINOR       "")
set(AMD_PROJECT_VERSION_PATCH       "")
if(AMD_TARGET_VERSION AND ((AMD_PROJECT_VERSION_MAJOR STREQUAL "") OR (AMD_PROJECT_VERSION_MINOR STREQUAL "") OR (AMD_PROJECT_VERSION_PATCH STREQUAL "")))
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" AMD_PROJECT_VERSION_MAJOR ${AMD_TARGET_VERSION})
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" AMD_PROJECT_VERSION_MINOR ${AMD_TARGET_VERSION})
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" AMD_PROJECT_VERSION_PATCH ${AMD_TARGET_VERSION})
else()
    message(FATAL_ERROR ">> 'AMD_TARGET_VERSION' was not properly set!")
endif()
set(AMD_PROJECT_VERSION_FULL "${AMD_PROJECT_VERSION_MAJOR}.${AMD_PROJECT_VERSION_MINOR}.${AMD_PROJECT_VERSION_PATCH}")

#
project(${AMD_TARGET_NAME}
    VERSION ${AMD_TARGET_VERSION_TEXT}
    LANGUAGES CXX
    DESCRIPTION ${AMD_PROJECT_DESCRIPTION}
    HOMEPAGE_URL ${AMD_PROJECT_GITHUB_REPO}
)
setup_project()

#
##  Note:
##  # Clang++ Compiler
##      When 'AMD_APP_COMPILER_TRY_CLANG' is set to 'ON', the project will try to use 'Clang++' compiler
##      if it is available and the requirements are met (Clang++ 19+). If 'Clang++' is not available, the
##      project will fallback to the default compiler (g++).
##      That is to simplify the CI/CD process and to ensure that the project can be built in any environment.
##
##  # Mold Linker
##      When 'AMD_APP_LINKER_TRY_MOLD' is set to 'ON', the project will try to use 'Mold' linker if it is
##      available. 'Mold' linker is a faster linker than the default one, and it can be used to speed up the
##      build process. If 'Mold' is not available, the project will fallback to the default linker.
##
##  # ROCm Build Environment
##      When AMD_APP_ROCM_BUILD_AUTO_DETECT=ON, and 'setup_rocm_auto_build_environment()' is called it will
##      check if the system used to build the project has all the required ROCm dependencies and in case it
##      does, the flag 'AMD_APP_ROCM_BUILD_PACKAGE' will be set to 'ON' and the project will be built as a
##      ROCm package.
##
##      When AMD_APP_ROCM_BUILD_AUTO_DETECT=OFF, the flag 'AMD_APP_ROCM_BUILD_PACKAGE' will be set to 'OFF'
##      (default) so the project will be built as a standalone package.
##
setup_rocm_auto_build_environment(AMD_APP_ROCM_BUILD_AUTO_DETECT AMD_APP_ROCM_BUILD_PACKAGE)

#
# Build Version/Type
##install(CODE "set(AMD_TARGET_INSTALL FALSE)")
set(AMD_PROJECT_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Build Type" FORCE)
set(AMD_APP_ROCM_BUILD_PACKAGE_INTERNAL ${AMD_APP_ROCM_BUILD_PACKAGE} CACHE BOOL "Is this a ROCm build (ON) or a Standalone (OFF)" FORCE)
set(AMD_APP_ROCM_BUILD_BINARY_VERSION_INTERNAL ${AMD_TARGET_VERSION} CACHE STRING "Builtin Target version for libraries" FORCE)
message(STATUS "[[ Building... Project: " ${CMAKE_PROJECT_NAME} " v." ${AMD_TARGET_VERSION_TEXT} " / " ${AMD_TARGET_VERSION} " {Build: " ${AMD_PROJECT_BUILD_TYPE} " | ROCm Build/Package Env: " ${AMD_APP_ROCM_BUILD_PACKAGE} "} ]] ...")
message(STATUS "  >> Project will be packaged with: " '${AMD_PROJECT_VERSION_FULL}' " timestamp ...")

#
#   Note:   It should be put at the top or before any targets are defined/created.
#           This is set in rocm_clang_toolchain.cmake, and it is used check if the build process should be skipped.
#           We check it here, but the CI script checks for the flag file to skip the build process.
if(SKIP_BUILD_PROCESS)
    message(WARNING ">> Due to toolchain/OS policy, skipping build process ...")
    return()
endif()

#
add_custom_target(${AMD_TARGET_NAME}_all ALL)
setup_default_build_options()
check_builtin_plugin_set()
check_os_build_definitions()
add_build_definitions()

#
# Packaging and Install
setup_packaging_options()
setup_install_target()
add_bundled_libraries()

#
add_subdirectory(deps/work_bench)
add_subdirectory(main)


# Build additional components
if(AMD_APP_BUILD_PLUGINS)
    add_plugin_directories()
endif()

if(AMD_APP_BUILD_DOCS)
    # add_subdirectory(docs)
endif()

if(AMD_APP_BUILD_EXAMPLES)
    # add_subdirectory(examples)
endif()

# Build unit tests
if(AMD_APP_BUILD_TESTS)
    if (NOT TARGET unit_tests)
        enable_testing()
        add_custom_target(unit_tests)
        add_subdirectory(tests EXCLUDE_FROM_ALL)
    endif()
endif()

#
# Additional resources for packaging/install
if(AMD_APP_SETUP_SDK)
    setup_sdk_options()
endif()

#
# Adjust IDE support targets
adjust_ide_support_targets()

#
# Package the build, standalone builds only
if(AMD_APP_STANDALONE_BUILD_PACKAGE AND AMD_APP_ENGINEERING_BUILD_PACKAGE)
    message(FATAL_ERROR "Building 'standalone' and 'engineering' packages simultaneously is not allowed!")
endif()

#
# Setup the package environment for custom install
setup_distribution_package()

#
# Setup post build steps for install/packaging
set(AMD_PROJECT_BASE_DIRECTORY  "${CMAKE_SOURCE_DIR}")
set(AMD_PROJECT_BUILD_DIRECTORY "${CMAKE_BINARY_DIR}")
configure_file("${AMD_APP_CMAKE_MODULES_DIRECTORY}/post_build_utils.cmake.in"
               "${CMAKE_BINARY_DIR}/post_build_utils.cmake" @ONLY)

#
##
if(AMD_APP_STANDALONE_BUILD_PACKAGE OR AMD_APP_ENGINEERING_BUILD_PACKAGE)
    # Post build utils
    install(SCRIPT ${AMD_PROJECT_BUILD_DIRECTORY}/post_build_utils.cmake)

    #
    ## This is the bridge between the build stage and the packaging stage
    set(AMD_TARGET_INSTALL_STAGING_AREA_BASE "${CMAKE_BINARY_DIR}/${AMD_TARGET_NAME}_staging")
    if(AMD_APP_STANDALONE_BUILD_PACKAGE)
        set(AMD_ROCM_STAGING_INSTALL_PATH "/usr/local")
        set(AMD_STANDALONE_STAGING_INSTALL_LIBDIR "lib")
        set(AMD_STANDALONE_STAGING_INSTALL_PLUGINSDIR "plugins")
        set(AMD_STANDALONE_STAGING_INSTALL_INCLUDEDIR "include")
        set(AMD_STANDALONE_STAGING_INSTALL_BINDIR "bin")
        set(AMD_STANDALONE_STAGING_INSTALL_DOCDIR "share/doc")
        set(AMD_STANDALONE_STAGING_INSTALL_SCRIPTDIR "bin/scripts")
        set(AMD_STANDALONE_STAGING_INSTALL_CONFIGDIR "etc/${AMD_TARGET_NAME}")
        set(AMD_STANDALONE_STAGING_INSTALL_EXPORTDIR "lib/cmake/${AMD_TARGET_NAME}")
        set(AMD_STANDALONE_STAGING_INSTALL_SYMLINKDIR "bin")

        ##  This installs $prefix/lib and $prefix/lib/plugins ($prefix/lib/<your_package_name>/plugins)
        ##  Note: The ending '/' in the is important ${DIRECTORY} is important.
        set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_STANDALONE_STAGING_INSTALL_LIBDIR}")
        install(DIRECTORY "${AMD_TARGET_INSTALL_STAGING_AREA_BASE}/${AMD_TARGET_INSTALL_PATH}/"
                DESTINATION "${AMD_TARGET_INSTALL_PATH}"
                USE_SOURCE_PERMISSIONS
        )

        ##  This installs $prefix/bin
        set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_STANDALONE_STAGING_INSTALL_BINDIR}")
        install(DIRECTORY "${AMD_TARGET_INSTALL_STAGING_AREA_BASE}/${AMD_TARGET_INSTALL_PATH}/"
                DESTINATION "${AMD_TARGET_INSTALL_PATH}"
                USE_SOURCE_PERMISSIONS
        )

        ## This install documentation
        set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_STANDALONE_STAGING_INSTALL_DOCDIR}/${AMD_TARGET_NAME}")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
        install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION ${AMD_TARGET_INSTALL_PATH})

        include(CPack)
    endif()
endif()

#
# ROCm build/package specific settings
if(AMD_APP_ROCM_BUILD_PACKAGE AND (NOT AMD_APP_STANDALONE_BUILD_PACKAGE) AND (NOT AMD_APP_ENGINEERING_BUILD_PACKAGE))
    # Post build utils
    install(SCRIPT ${AMD_PROJECT_BUILD_DIRECTORY}/post_build_utils.cmake)

    #
    ## This is the bridge between the build stage and the packaging stage
    ## Preserve the proper ROCm version path: "/opt/rocm-${rocm_version}"
    get_rocm_install_path(ROCM_PATH)
    set(AMD_TARGET_INSTALL_STAGING_AREA_BASE "${CMAKE_INSTALL_PREFIX}/${AMD_TARGET_NAME}/_staging")
    set(AMD_ROCM_STAGING_INSTALL_PATH "${ROCM_PATH}")
    set(AMD_ROCM_STAGING_INSTALL_LIBDIR "lib")
    set(AMD_ROCM_STAGING_INSTALL_PLUGINSDIR "plugins")
    set(AMD_ROCM_STAGING_INSTALL_INCLUDEDIR "include")
    set(AMD_ROCM_STAGING_INSTALL_BINDIR "bin")
    set(AMD_ROCM_STAGING_INSTALL_DOCDIR "share/doc")
    set(AMD_ROCM_STAGING_INSTALL_SCRIPTDIR "bin/scripts")
    set(AMD_ROCM_STAGING_INSTALL_CONFIGDIR "etc/${AMD_TARGET_NAME}")
    set(AMD_ROCM_STAGING_INSTALL_EXPORTDIR "lib/cmake/${AMD_TARGET_NAME}")
    set(AMD_ROCM_STAGING_INSTALL_SYMLINKDIR "bin")

    ##
    ##  Note:  If for some reason the 'RPATH' rules are not set properly for a shared library or executable,
    ##         changed due to changes in the build process, or an externally built project, such as other
    ##         dependencies, we can try to still apply 'RPATH' rules for the ROCm build binaries.
    ##         In this case, for ROCm we look at the custom staging area, and check all the binary files,
    ##         (executables, and shared libraries), and only if/when needed, patch the 'RPATH' by applying
    ##         our defined rules/policies.
    ##
    if(AMD_APP_ROCM_BUILD_TRY_RPATH)
        install(CODE "
                    include(\"${AMD_APP_CMAKE_DIRECTORY}/build_utils.cmake\")
                    message(STATUS \">> Applying RPATH rules for ROCm build binaries ...\")
                    binary_classify_in_target_directory(
                        TRY_RPATH_SET
                        KEEP_ACCEPTABLE_RPATH
                        DIRECTORY_PATH \"${AMD_TARGET_INSTALL_STAGING_AREA_BASE}\"
                        EXECUTABLES_LIST executable_list
                        SHARED_LIBRARIES_LIST shared_libraries_list
                        STATIC_LIBRARIES_LIST static_libraries_list
                    )"
        )
    endif()

    message(STATUS "[[ Packing up... Project: " ${AMD_PROJECT_NAME} "   v." ${AMD_TARGET_VERSION_TEXT} " / " ${AMD_TARGET_VERSION} "  for ROCm PKG v." ${ROCM_VERSION_FOR_PACKAGE} " ]] ...")
    ##  This installs $prefix/lib
    ##  Note: The ending '/' in the is important ${DIRECTORY} is important.
    set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_ROCM_STAGING_INSTALL_LIBDIR}")
    install(DIRECTORY "${AMD_TARGET_INSTALL_STAGING_AREA_BASE}/${AMD_TARGET_INSTALL_PATH}/"
            DESTINATION "${AMD_TARGET_INSTALL_PATH}"
            USE_SOURCE_PERMISSIONS
    )

    ##  This installs $prefix/lib/plugins
    ##  Note: Due to the options for directories, and the fact '/usr/local/lib/' or '/opt/rocm/lib/' can
    ##        tend to get lots of subdirs and lots of libraries, we try to keep the plugin directory
    ##        separate from the rest of the libraries. This is a good practice for plugins.
    set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_ROCM_STAGING_INSTALL_LIBDIR}/${AMD_TARGET_NAME}/${AMD_ROCM_STAGING_INSTALL_PLUGINSDIR}")
    install(DIRECTORY "${AMD_TARGET_INSTALL_STAGING_AREA_BASE}/${AMD_TARGET_INSTALL_PATH}/"
            DESTINATION "${AMD_TARGET_INSTALL_PATH}"
            USE_SOURCE_PERMISSIONS
    )

    ##  This installs /bin
    set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_ROCM_STAGING_INSTALL_BINDIR}")
    install(DIRECTORY "${AMD_TARGET_INSTALL_STAGING_AREA_BASE}/${AMD_TARGET_INSTALL_PATH}/"
            DESTINATION "${AMD_TARGET_INSTALL_PATH}"
            USE_SOURCE_PERMISSIONS
    )

    ## This install documentation
    set(AMD_TARGET_INSTALL_PATH "${AMD_ROCM_STAGING_INSTALL_PATH}/${AMD_ROCM_STAGING_INSTALL_DOCDIR}/${AMD_TARGET_NAME}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
    install(FILES ${CPACK_RESOURCE_FILE_LICENSE} DESTINATION ${AMD_TARGET_INSTALL_PATH})

    ## Generate installer/source packages
    include(CPack)
endif()

## End of CMakeLists.txt
