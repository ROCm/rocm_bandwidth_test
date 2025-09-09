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


#
project(${AMD_TEST_PROJECT_COMMON_TARGET_NAME})

#
message(STATUS "  >> Building... Component: " ${PROJECT_NAME} " ]] ...")

#
add_library(${PROJECT_NAME} 
    STATIC 
        src/tst_common.cpp
)

# Local include directories for this submodule of 'Unit Test' project
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Include directories for 'Unit Test' project
foreach(INCLUDE_DIRECTORY IN LISTS AMD_TEST_PROJECT_ALL_INCLUDE_DIRECTORIES)
    target_include_directories(${PROJECT_NAME} 
        PUBLIC 
            $<BUILD_INTERFACE:${INCLUDE_DIRECTORY}>
    )
endforeach()

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        #${AMD_TEST_PROJECT_LINK_LIBRARIES}
        ${AMD_TEST_PROJECT_3RD_PARTY_BUNDLE_INTERFACE_NAME}
)

set_target_properties(${PROJECT_NAME} 
    PROPERTIES
        CXX_STANDARD ${CMAKE_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
)

add_dependencies(${PROJECT_NAME} 
    ${AMD_TEST_PROJECT_LINK_LIBRARIES}
)


## End of CMakeLists.txt

