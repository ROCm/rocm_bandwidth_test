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

## /<project_name>/plugins/tb/transferbench/build
#message("Source directory: ${CMAKE_SOURCE_DIR}")
# /<project_name>/plugins/tb/transferbench/build
#message("Binary directory: ${CMAKE_BINARY_DIR}")
## /<project_name>/plugins/tb/transferbench/build
#message("Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
## /<project_name>/plugins/tb/transferbench/build
#message("Current binary directory: ${CMAKE_CURRENT_BINARY_DIR}")
## /<project_name>/plugins/tb/transferbench
message("Current list directory: ${CMAKE_CURRENT_LIST_DIR}")
message("Current binary directory: ${CMAKE_CURRENT_BINARY_DIR}")
set(EXTERNAL_DEPENDEE_BASE_PATH "${INSTALL_DEPENDEE_BASE_PATH}")
set(EXTERNAL_INSTALL_SOURCE_LIBAMD_TB_PATH "${INSTALL_SOURCE_LIBAMD_TB_PATH}")
set(EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH "${INSTALL_TARGET_LIBAMD_TB_PATH}")

if (EXTERNAL_INSTALL_SOURCE_LIBAMD_TB_PATH STREQUAL "")
    message(FATAL_ERROR "'EXTERNAL_INSTALL_SOURCE_LIBAMD_TB_PATH' was not defined!")
endif()
if (EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH STREQUAL "")
    message(FATAL_ERROR "'EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH' was not defined!")
endif()

file(GLOB LIBAMD_TB_LIST_FILES "${CMAKE_CURRENT_BINARY_DIR}/libamd_tb.*")
message(WARNING ">> Installing 'libamd_tb.so*' from \t${CMAKE_CURRENT_BINARY_DIR} to \t${EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH}")
foreach(lib_tb_file ${LIBAMD_TB_LIST_FILES})
    message(STATUS "  >> Copying '${lib_tb_file}' to ${EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH}")
    file(COPY ${lib_tb_file} DESTINATION ${EXTERNAL_INSTALL_SOURCE_LIBAMD_TB_PATH} FOLLOW_SYMLINK_CHAIN)
    file(COPY ${lib_tb_file} DESTINATION ${EXTERNAL_INSTALL_TARGET_LIBAMD_TB_PATH} FOLLOW_SYMLINK_CHAIN)
endforeach()


## End of install_libamd_tb.cmake
