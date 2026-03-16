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

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>


namespace amd_work_bench::test::unit
{

/**
 * @brief Test fixture for common test setup/teardown
 */
class TestFixture
{
    public:
        TestFixture() = default;
        virtual ~TestFixture() = default;

        /**
         * @brief Setup method called before each test
         */
        virtual void setup() {}

        /**
         * @brief Teardown method called after each test
         */
        virtual void teardown() {}

    protected:
        /**
         * @brief Generate a unique temporary file path for testing
         */
        static auto get_temp_file_path(const std::string& prefix = "rbt_test_") -> std::filesystem::path
        {
            auto temp_dir = std::filesystem::temp_directory_path();
            auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            return temp_dir / (prefix + std::to_string(timestamp));
        }

        /**
         * @brief Check if running with GPU support available
         */
        static auto has_gpu_support() -> bool
        {
            // Check for ROCm/HIP availability via environment
            auto rocm_path = wb_utils::get_env_var("ROCM_PATH");
            return rocm_path.has_value();
        }
};


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
        static const char* mock_plugin_get_compatibility() { return "1.0.0"; }
        static const char* mock_plugin_get_version() { return VERSION; }
};


/**
 * @brief Timing utility for performance tests
 */
class ScopedTimer
{
    public:
        explicit ScopedTimer(const std::string& label = "Operation")
            : m_label(label)
            , m_start(std::chrono::high_resolution_clock::now())
        {
        }

        ~ScopedTimer()
        {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
            std::cout << "[TIMING] " << m_label << ": " << duration.count() << " µs\n";
        }

        auto elapsed_us() const -> int64_t
        {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
        }

    private:
        std::string m_label;
        std::chrono::high_resolution_clock::time_point m_start;
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
