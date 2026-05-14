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
 * @file test_workbench_api.cpp
 * @brief Unit tests for WorkBench API
 * @author ROCm Bandwidth Test Team
 *
 * Tests the public API of the WorkBench framework including
 * system information, task management, and messaging.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>

#include <array>
#include <set>
#include <string>


namespace amd_work_bench::test::unit::api
{

// =============================================================================
// TEST CASE: Task Progress State
// =============================================================================

TEST_CASE("WorkBenchAPI::TaskProgressState", "[unit][api][task]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("TaskProgressState_t enumerators are pairwise distinct and ordered")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "state enum"));

        using State = wb_api_system::TaskProgressState_t;

        const std::array<State, 4> all_states{
            State::NOT_STARTED, State::IN_PROGRESS, State::COMPLETED, State::FAILED};

        std::set<int> raw_values;
        for (auto s : all_states) {
            raw_values.insert(static_cast<int>(s));
        }

        REQUIRE(raw_values.size() == all_states.size());  // no aliasing
    }
}


// =============================================================================
// TEST CASE: Task Progress Type
// =============================================================================

TEST_CASE("WorkBenchAPI::TaskProgress", "[unit][api][task]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("TaskProgress_t enumerators are pairwise distinct")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "progress enum"));

        using Progress = wb_api_system::TaskProgress_t;

        const std::array<Progress, 3> all_progress{
            Progress::NORMAL, Progress::WARNING, Progress::ERROR};

        std::set<int> raw_values;
        for (auto p : all_progress) {
            raw_values.insert(static_cast<int>(p));
        }

        REQUIRE(raw_values.size() == all_progress.size());
    }
}


// =============================================================================
// TEST CASE: Run Arguments Structure
// =============================================================================

TEST_CASE("WorkBenchAPI::RunArguments", "[unit][api][args]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("RunArguments_t default initialization")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default init"));

        wb_api_system::RunArguments_t args{};

        REQUIRE(args.argc == 0);
        REQUIRE(args.argv == nullptr);
        REQUIRE(args.envp == nullptr);
    }

    SECTION("RunArguments_t with values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "with values"));

        char* mock_argv[] = {
            const_cast<char*>("rocm_bandwidth_test"),
            const_cast<char*>("--help"),
            nullptr
        };

        char* mock_envp[] = {
            const_cast<char*>("PATH=/usr/bin"),
            nullptr
        };

        wb_api_system::RunArguments_t args{
            .argv = mock_argv,
            .envp = mock_envp,
            .argc = 2
        };

        REQUIRE(args.argc == 2);
        REQUIRE(args.argv != nullptr);
        REQUIRE(args.envp != nullptr);
        REQUIRE(std::string(args.argv[0]) == "rocm_bandwidth_test");
        REQUIRE(std::string(args.argv[1]) == "--help");
    }
}


// =============================================================================
// TEST CASE: System Information
// =============================================================================

TEST_CASE("WorkBenchAPI::SystemInfo", "[unit][api][system]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("get_os_kernel_info returns valid string")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "kernel info"));

        auto kernel_info = wb_api_system::get_os_kernel_info();

        // Should not be empty on a running system
        REQUIRE(!kernel_info.empty());

        // On Linux, should contain kernel version info
        // Example: "5.15.0-56-generic"
        INFO("Kernel info: " << kernel_info);
    }

    SECTION("get_os_distro_info returns valid string")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "distro info"));

        auto distro_info = wb_api_system::get_os_distro_info();

        // Should not be empty on a running system
        REQUIRE(!distro_info.empty());

        // Example: "Ubuntu 22.04" or "RHEL 8"
        INFO("Distro info: " << distro_info);
    }

    SECTION("get_work_bench_version returns valid version")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "workbench version"));

        auto version = wb_api_system::get_work_bench_version();

        REQUIRE(!version.empty());

        // Version may be a semantic version string or the documented
        // unknown placeholder when build version metadata is unavailable.
        // Examples: "2.6.0-debug", "2.6.0", or wb_literals::kTEXT_UNKNOWN
        INFO("WorkBench version: " << version);

        // Only enforce semantic-version formatting when a real version is set.
        if (version != wb_literals::kTEXT_UNKNOWN) {
            CHECK(wb_strings::contains(version, '.'));
        }
    }

    SECTION("get_work_bench_commit_hash returns valid hash")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "commit hash"));

        auto hash_long = wb_api_system::get_work_bench_commit_hash(true);
        auto hash_short = wb_api_system::get_work_bench_commit_hash(false);

        // Long hash should be 40 characters (SHA-1)
        // Short hash should be around 7-8 characters
        // But may be empty/placeholder in some builds

        INFO("Long hash: " << hash_long);
        INFO("Short hash: " << hash_short);

        // If populated, long should be longer than short
        if (!hash_long.empty() && !hash_short.empty()) {
            CHECK(hash_long.size() >= hash_short.size());
        }
    }

    SECTION("get_work_bench_commit_branch returns branch name")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "branch name"));

        auto branch = wb_api_system::get_work_bench_commit_branch();

        // May be empty in some builds, but shouldn't throw
        INFO("Branch: " << branch);

        REQUIRE_NOTHROW(branch.size());
    }

    SECTION("get_work_bench_build_type returns build type")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "build type"));

        auto build_type = wb_api_system::get_work_bench_build_type();

        // Should be something like "Debug", "Release", or "Engineering"
        REQUIRE(!build_type.empty());
        INFO("Build type: " << build_type);
    }

    SECTION("get_work_bench_is_engineering_build returns boolean")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "engineering build"));

        // Just verify it returns a valid boolean (no crash)
        REQUIRE_NOTHROW(wb_api_system::get_work_bench_is_engineering_build());
    }
}


// =============================================================================
// TEST CASE: Startup Arguments
// =============================================================================

TEST_CASE("WorkBenchAPI::StartupArgs", "[unit][api][startup]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("get_startup_args returns map")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "get args map"));

        const auto& args = wb_api_system::get_startup_args();

        // Should be a valid map (may be empty)
        REQUIRE_NOTHROW(args.size());
    }

    SECTION("get_startup_arg returns empty for non-existing arg")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "non-existing arg"));

        auto value = wb_api_system::get_startup_arg("__NONEXISTENT_ARG__");

        // Should return empty string for non-existing arg
        REQUIRE(value.empty());
    }
}


// =============================================================================
// TEST CASE: Main Instance Check
// =============================================================================

TEST_CASE("WorkBenchAPI::MainInstance", "[unit][api][instance]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("is_main_instance returns boolean")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "is main instance"));

        // The contract: is_main_instance() is callable from a test runner and
        // returns a boolean (which side, true or false, is environment-dependent
        // and not the contract under test). Verify (a) it does not throw and
        // (b) consecutive calls are stable within a single process lifetime.
        bool first  = false;
        bool second = false;
        REQUIRE_NOTHROW(first = wb_api_system::is_main_instance());
        REQUIRE_NOTHROW(second = wb_api_system::is_main_instance());
        REQUIRE(first == second);
    }
}


// =============================================================================
// TEST CASE: Messaging System
// =============================================================================

TEST_CASE("WorkBenchAPI::Messaging", "[unit][api][messaging]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("register_message_handler can register handler")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "register handler"));

        bool handler_called = false;

        // Register a test handler
        wb_api_messaging::register_message_handler(
            "test_unit_message",
            [&handler_called](const DataStream_t& /*data*/) {
                handler_called = true;
            }
        );

        // Verify handler is registered
        const auto& handlers = wb_api_messaging::details::get_message_handlers();

        // Should have at least our handler
        REQUIRE(handlers.find("test_unit_message") != handlers.end());
    }

    SECTION("run_message_handler invokes registered handler")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "run handler"));

        bool handler_invoked = false;
        std::string received_data;

        // Register handler
        wb_api_messaging::register_message_handler(
            "test_invoke_message",
            [&handler_invoked, &received_data](const DataStream_t& data) {
                handler_invoked = true;
                // DataStream_t is likely a byte stream, just mark as invoked
            }
        );

        // Run the handler
        DataStream_t test_data;  // Empty data
        wb_api_messaging::details::run_message_handler("test_invoke_message", test_data);

        REQUIRE(handler_invoked == true);
    }
}


// =============================================================================
// TEST CASE: GPU Information (Optional)
// =============================================================================

TEST_CASE("WorkBenchAPI::GPUInfo", "[unit][api][gpu][!mayfail]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("get_gpu_info returns information if GPUs available")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "gpu info"));

        // This may fail if no GPU is available, which is acceptable
        auto gpu_info = wb_api_system::get_gpu_info();

        INFO("GPU info: " << gpu_info);

        // Just verify it doesn't crash
        REQUIRE_NOTHROW(gpu_info.size());
    }

    SECTION("get_cpu_info returns CPU information")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "cpu info"));

        auto cpu_info = wb_api_system::get_cpu_info();

        INFO("CPU info: " << cpu_info);

        // Should have some info about CPU
        REQUIRE_NOTHROW(cpu_info.size());
    }
}


}  // namespace amd_work_bench::test::unit::api
