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
 * @file tst_unit.hpp
 * @brief Unit test header for ROCm Bandwidth Test
 * @author ROCm Bandwidth Test Team
 */

#if !defined(AMD_WORK_BENCH_TEST_UNIT_HPP)
#define AMD_WORK_BENCH_TEST_UNIT_HPP

#include <work_bench.hpp>
#include <awb/builtins.hpp>
#include <awb/common_utils.hpp>
#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/plugins.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/typedefs.hpp>
#include <awb/work_bench_api.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <vector>


namespace amd_work_bench::test::unit
{


/**
 * @brief Mock plugin for testing plugin management
 */
class MockPlugin
{
    public:
        static constexpr const char* NAME = "mock_test_plugin";
        static constexpr const char* AUTHOR = "Test Author";
        static constexpr const char* DESCRIPTION = "Mock plugin for unit testing";
        static constexpr const char* VERSION = "1.0.0-test";

        static auto create_functionality() -> amd_work_bench::PluginFunctionality_t
        {
            return amd_work_bench::PluginFunctionality_t{
                mock_plugin_init,
                mock_plugin_get_name,
                mock_plugin_get_author,
                mock_plugin_get_description,
                mock_plugin_get_compatibility,
                mock_plugin_get_version,
                nullptr,  // subcommand
                nullptr,  // feature
                nullptr,  // main entry point
                nullptr,  // library init
            };
        }

    private:
        static void mock_plugin_init() {}
        static const char* mock_plugin_get_name() { return NAME; }
        static const char* mock_plugin_get_author() { return AUTHOR; }
        static const char* mock_plugin_get_description() { return DESCRIPTION; }
        // AMD_WORK_BENCH_VERSION is provided by tests/unit/CMakeLists.txt as a
        // PUBLIC compile definition. The fallback below is intentionally never
        // hit in a normal build; it exists only to keep the header self-contained
        // for tooling (clang-tidy, IDE indexers) when build flags are missing.
#ifndef AMD_WORK_BENCH_VERSION
#define AMD_WORK_BENCH_VERSION "0.0.0-unspecified"
#endif
        static const char* mock_plugin_get_compatibility() { return AMD_WORK_BENCH_VERSION; }
        static const char* mock_plugin_get_version() { return VERSION; }
};


/**
 * @brief Helper to build test case info string
 */
inline auto build_test_info(const std::string& test_case, const std::string& section) -> std::string
{
    return amd_fmt::format("Test case: {}\n  Section: {}\n", test_case, section);
}


}  // namespace amd_work_bench::test::unit


// Convenient namespace aliases
namespace wb_test = amd_work_bench::test::unit;


#endif  // AMD_WORK_BENCH_TEST_UNIT_HPP
