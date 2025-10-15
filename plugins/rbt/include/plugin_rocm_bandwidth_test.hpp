/*
 * SPDX-License-Identifier: MIT License
 *
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * Author(s):   Daniel Oliveira <daniel.oliveira@amd.com>
 *
 * Description: plugin_rocm_bandwidth_test.hpp
 *
 */


#if !defined(AMD_WORK_BENCH_PLUGIN_ROCM_BANDWIDTH_TEST_HPP)
#define AMD_WORK_BENCH_PLUGIN_ROCM_BANDWIDTH_TEST_HPP

#include <string>
#include <vector>


namespace amd_work_bench::plugin::rocm_bandwidth_test
{

using WordList_t = std::vector<std::string>;
const auto kPLUGIN_MODULE_NAME = std::string("ROCm Bandwidth Test");


//  For RBT we are passing the command line directly to the plugin and skipping the command handler here

}    // namespace amd_work_bench::plugin::rocm_bandwidth_test


// namespace alias
namespace wb_plugin_rbt = amd_work_bench::plugin::rocm_bandwidth_test;

#endif    // AMD_WORK_BENCH_PLUGIN_ROCM_BANDWIDTH_TEST_HPP
