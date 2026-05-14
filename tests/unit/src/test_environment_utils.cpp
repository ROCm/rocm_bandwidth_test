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
 * @file test_environment_utils.cpp
 * @brief Unit tests for environment variable utilities
 * @author ROCm Bandwidth Test Team
 *
 * Tests the environment variable get/set/unset operations
 * in the WorkBench utility library.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>

#include <cstdlib>
#include <string>


namespace amd_work_bench::test::unit::environment
{

// =============================================================================
// TEST CASE: Get Environment Variables
// =============================================================================

TEST_CASE("EnvironmentUtils::GetEnvVar", "[unit][environment][get]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("get_env_var returns nullopt for non-existing variable")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "non_existing"));

        auto result = wb_utils::get_env_var("RBT_TEST_NONEXISTENT_VAR_12345");

        REQUIRE(result.has_value() == false);
    }

    SECTION("get_env_var returns value for existing variable PATH")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "PATH exists"));

        auto result = wb_utils::get_env_var("PATH");

        REQUIRE(result.has_value() == true);
        REQUIRE(!result->empty());
    }

    SECTION("get_env_var returns value for HOME variable")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "HOME exists"));

        auto result = wb_utils::get_env_var("HOME");

        // HOME should exist on Linux systems
        if (result.has_value()) {
            REQUIRE(!result->empty());
        }
    }

    SECTION("get_env_var returns value for USER variable")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "USER exists"));

        auto result = wb_utils::get_env_var("USER");

        // USER should exist on Linux systems
        if (result.has_value()) {
            REQUIRE(!result->empty());
        }
    }

    SECTION("get_env_var checks ROCM_PATH if available")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "ROCM_PATH"));

        auto result = wb_utils::get_env_var("ROCM_PATH");

        // If ROCM_PATH is set, verify it's a valid path
        if (result.has_value()) {
            REQUIRE(!result->empty());
            // Typically /opt/rocm or similar
            CHECK(wb_strings::contains(*result, "rocm"));
        }
    }
}


// =============================================================================
// TEST CASE: Set Environment Variables
// =============================================================================

TEST_CASE("EnvironmentUtils::SetEnvVar", "[unit][environment][set]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    // Use a unique test variable name to avoid conflicts
    const std::string TEST_VAR = "RBT_UNIT_TEST_VAR_SET";

    SECTION("set_env_var creates new variable")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "create new"));

        // Ensure variable doesn't exist
        wb_utils::unset_env_var(TEST_VAR);

        auto result = wb_utils::set_env_var(TEST_VAR, "test_value", true);

        REQUIRE(result == 0);  // Success

        auto retrieved = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(retrieved.has_value() == true);
        REQUIRE(*retrieved == "test_value");

        // Cleanup
        wb_utils::unset_env_var(TEST_VAR);
    }

    SECTION("set_env_var with overwrite=true replaces existing value")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "overwrite existing"));

        // Set initial value
        wb_utils::set_env_var(TEST_VAR, "original", true);

        // Overwrite
        auto result = wb_utils::set_env_var(TEST_VAR, "new_value", true);

        REQUIRE(result == 0);

        auto retrieved = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(retrieved.has_value() == true);
        REQUIRE(*retrieved == "new_value");

        // Cleanup
        wb_utils::unset_env_var(TEST_VAR);
    }

    SECTION("set_env_var with overwrite=false preserves existing value")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "no overwrite"));

        // Set initial value
        wb_utils::set_env_var(TEST_VAR, "original", true);

        // Try to set without overwrite
        wb_utils::set_env_var(TEST_VAR, "new_value", false);

        auto retrieved = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(retrieved.has_value() == true);
        REQUIRE(*retrieved == "original");  // Should keep original

        // Cleanup
        wb_utils::unset_env_var(TEST_VAR);
    }

    SECTION("set_env_var handles empty value")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty value"));

        auto result = wb_utils::set_env_var(TEST_VAR, "", true);

        REQUIRE(result == 0);

        auto retrieved = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(retrieved.has_value() == true);
        REQUIRE(retrieved->empty());

        // Cleanup
        wb_utils::unset_env_var(TEST_VAR);
    }
}


// =============================================================================
// TEST CASE: Unset Environment Variables
// =============================================================================

TEST_CASE("EnvironmentUtils::UnsetEnvVar", "[unit][environment][unset]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    const std::string TEST_VAR = "RBT_UNIT_TEST_VAR_UNSET";

    SECTION("unset_env_var removes existing variable")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "remove existing"));

        // Create variable
        wb_utils::set_env_var(TEST_VAR, "to_be_removed", true);

        // Verify it exists
        auto before = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(before.has_value() == true);

        // Unset
        auto result = wb_utils::unset_env_var(TEST_VAR);
        REQUIRE(result == 0);

        // Verify it's gone
        auto after = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(after.has_value() == false);
    }

    SECTION("unset_env_var handles non-existing variable gracefully")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "non_existing"));

        // Ensure variable doesn't exist
        wb_utils::unset_env_var(TEST_VAR);

        // Try to unset again - should not fail
        auto result = wb_utils::unset_env_var(TEST_VAR);

        // unsetenv typically returns 0 even for non-existing vars
        REQUIRE(result == 0);
    }
}


// =============================================================================
// TEST CASE: Environment Variable Integration
// =============================================================================

TEST_CASE("EnvironmentUtils::Integration", "[unit][environment][integration]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    const std::string TEST_VAR = "RBT_UNIT_TEST_INTEGRATION";

    SECTION("full lifecycle: set, get, modify, unset")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "full lifecycle"));

        // 1. Create
        REQUIRE(wb_utils::set_env_var(TEST_VAR, "initial", true) == 0);

        // 2. Read
        auto val1 = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(val1.has_value());
        REQUIRE(*val1 == "initial");

        // 3. Modify
        REQUIRE(wb_utils::set_env_var(TEST_VAR, "modified", true) == 0);

        // 4. Read again
        auto val2 = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(val2.has_value());
        REQUIRE(*val2 == "modified");

        // 5. Delete
        REQUIRE(wb_utils::unset_env_var(TEST_VAR) == 0);

        // 6. Verify deleted
        auto val3 = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(!val3.has_value());
    }

    SECTION("environment variables with special characters")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "special characters"));

        // Test with various special characters in value
        std::string special_value = "/path/to/dir:another/path=/opt/rocm";

        wb_utils::set_env_var(TEST_VAR, special_value, true);

        auto result = wb_utils::get_env_var(TEST_VAR);
        REQUIRE(result.has_value());
        REQUIRE(*result == special_value);

        // Cleanup
        wb_utils::unset_env_var(TEST_VAR);
    }
}


// =============================================================================
// TEST CASE: ROCm-specific Environment Variables
// =============================================================================

TEST_CASE("EnvironmentUtils::ROCmVariables", "[unit][environment][rocm]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("check common ROCm environment variables")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "ROCm vars"));

        // These may or may not be set depending on the system
        auto rocm_path = wb_utils::get_env_var("ROCM_PATH");
        auto hip_path = wb_utils::get_env_var("HIP_PATH");
        auto ld_library_path = wb_utils::get_env_var("LD_LIBRARY_PATH");

        // If ROCM_PATH is set, it should point to a valid location
        if (rocm_path.has_value()) {
            INFO("ROCM_PATH = " << *rocm_path);
            CHECK(!rocm_path->empty());
        }

        // LD_LIBRARY_PATH is commonly set
        if (ld_library_path.has_value()) {
            INFO("LD_LIBRARY_PATH = " << *ld_library_path);
            CHECK(!ld_library_path->empty());
        }
    }
}


}  // namespace amd_work_bench::test::unit::environment
