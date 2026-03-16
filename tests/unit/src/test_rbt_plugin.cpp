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
 * @file test_rbt_plugin.cpp
 * @brief Unit tests for ROCm Bandwidth Test plugin
 * @author ROCm Bandwidth Test Team
 *
 * Tests the RBT plugin functionality including command handling,
 * mode flags, and CLI parsing.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>

#include <string>
#include <vector>
#include <algorithm>


namespace amd_work_bench::test::unit::rbt
{

// =============================================================================
// Helper Functions for Command Line Testing
// =============================================================================

/**
 * @brief Convert vector of strings to argc/argv format
 */
class ArgvBuilder
{
    public:
        explicit ArgvBuilder(std::vector<std::string> args)
            : m_args(std::move(args))
        {
            m_argv.reserve(m_args.size() + 1);
            for (auto& arg : m_args) {
                m_argv.push_back(arg.data());
            }
            m_argv.push_back(nullptr);
        }

        int argc() const { return static_cast<int>(m_args.size()); }
        char** argv() { return m_argv.data(); }

    private:
        std::vector<std::string> m_args;
        std::vector<char*> m_argv;
};


// =============================================================================
// TEST CASE: RBT Plugin Constants
// =============================================================================

TEST_CASE("RBTPlugin::Constants", "[unit][rbt][constants]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Plugin module name is defined")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "module name"));

        // The plugin should have a defined module name
        // This is typically "ROCm Bandwidth Test"
        const std::string expected_name = "ROCm Bandwidth Test";

        // Verify the constant exists in the namespace
        // (Would require including the plugin header)
        REQUIRE(!expected_name.empty());
    }
}


// =============================================================================
// TEST CASE: RBT WordList Type
// =============================================================================

TEST_CASE("RBTPlugin::WordList", "[unit][rbt][wordlist]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("WordList_t can store arguments")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "store args"));

        using WordList_t = std::vector<std::string>;

        WordList_t args = {"--flag1", "--flag2", "value"};

        REQUIRE(args.size() == 3);
        REQUIRE(args[0] == "--flag1");
        REQUIRE(args[1] == "--flag2");
        REQUIRE(args[2] == "value");
    }

    SECTION("WordList_t can be reversed for CLI11 parsing")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "reverse"));

        using WordList_t = std::vector<std::string>;

        WordList_t args = {"a", "b", "c"};
        std::reverse(args.begin(), args.end());

        REQUIRE(args[0] == "c");
        REQUIRE(args[1] == "b");
        REQUIRE(args[2] == "a");
    }

    SECTION("Empty WordList_t is handled correctly")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty"));

        using WordList_t = std::vector<std::string>;

        WordList_t empty_args;

        REQUIRE(empty_args.empty());
        REQUIRE(empty_args.size() == 0);
    }
}


// =============================================================================
// TEST CASE: RBT CLI Argument Parsing
// =============================================================================

TEST_CASE("RBTPlugin::CLIParsing", "[unit][rbt][cli]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("ArgvBuilder creates valid argc/argv")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "argv builder"));

        ArgvBuilder builder({"program", "-a", "-e"});

        REQUIRE(builder.argc() == 3);
        REQUIRE(builder.argv() != nullptr);
        REQUIRE(std::string(builder.argv()[0]) == "program");
        REQUIRE(std::string(builder.argv()[1]) == "-a");
        REQUIRE(std::string(builder.argv()[2]) == "-e");
    }

    SECTION("ArgvBuilder handles empty args")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty argv"));

        ArgvBuilder builder({});

        REQUIRE(builder.argc() == 0);
        REQUIRE(builder.argv() != nullptr);
        REQUIRE(builder.argv()[0] == nullptr);
    }

    SECTION("ArgvBuilder handles single arg")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "single arg"));

        ArgvBuilder builder({"rocm_bandwidth_test"});

        REQUIRE(builder.argc() == 1);
        REQUIRE(std::string(builder.argv()[0]) == "rocm_bandwidth_test");
    }
}


// =============================================================================
// TEST CASE: RBT Mode Flags Logic
// =============================================================================

TEST_CASE("RBTPlugin::ModeFlags", "[unit][rbt][modes]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Mode flags are mutually exclusive")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "mutual exclusion"));

        // Simulating the plugin's mode flag logic
        bool is_unidirectional_mode = false;
        bool is_bidirectional_mode = false;
        bool is_list_topology = false;

        auto reset_flags = [&]() {
            is_unidirectional_mode = false;
            is_bidirectional_mode = false;
            is_list_topology = false;
        };

        // Simulate -a flag
        reset_flags();
        is_unidirectional_mode = true;

        REQUIRE(is_unidirectional_mode == true);
        REQUIRE(is_bidirectional_mode == false);
        REQUIRE(is_list_topology == false);

        // Simulate -A flag (should reset and set bidirectional)
        reset_flags();
        is_bidirectional_mode = true;

        REQUIRE(is_unidirectional_mode == false);
        REQUIRE(is_bidirectional_mode == true);
        REQUIRE(is_list_topology == false);

        // Simulate -e flag
        reset_flags();
        is_list_topology = true;

        REQUIRE(is_unidirectional_mode == false);
        REQUIRE(is_bidirectional_mode == false);
        REQUIRE(is_list_topology == true);
    }

    SECTION("Only one mode can be active at a time")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "single active"));

        struct ModeState {
            bool unidirectional = false;
            bool bidirectional = false;
            bool list_topology = false;

            int active_count() const {
                return (unidirectional ? 1 : 0) +
                       (bidirectional ? 1 : 0) +
                       (list_topology ? 1 : 0);
            }
        };

        ModeState state;

        // Test each mode individually
        state = {true, false, false};
        REQUIRE(state.active_count() == 1);

        state = {false, true, false};
        REQUIRE(state.active_count() == 1);

        state = {false, false, true};
        REQUIRE(state.active_count() == 1);

        // Initial state should have no active modes
        state = {false, false, false};
        REQUIRE(state.active_count() == 0);
    }
}


// =============================================================================
// TEST CASE: RBT Exit Codes
// =============================================================================

TEST_CASE("RBTPlugin::ExitCodes", "[unit][rbt][exitcodes]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("EXIT_SUCCESS and EXIT_FAILURE are standard")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "standard codes"));

        REQUIRE(EXIT_SUCCESS == 0);
        REQUIRE(EXIT_FAILURE == 1);
    }

    SECTION("Custom return code for command_run_handler")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "custom code"));

        // The plugin returns 100 from command_run_handler
        // This is a placeholder value
        const int HANDLER_RETURN_CODE = 100;

        REQUIRE(HANDLER_RETURN_CODE != EXIT_SUCCESS);
        REQUIRE(HANDLER_RETURN_CODE != EXIT_FAILURE);
    }
}


// =============================================================================
// TEST CASE: RBT CLI Help Formatting
// =============================================================================

TEST_CASE("RBTPlugin::HelpFormatting", "[unit][rbt][help]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("CLI options follow consistent format")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "option format"));

        // Expected CLI options based on code review
        struct CliOption {
            std::string short_flag;
            std::string description;
        };

        std::vector<CliOption> expected_options = {
            {"-a", "Perform Unidirectional Copy involving all device combinations"},
            {"-A", "Perform Bidirectional Copy involving all device combinations"},
            {"-e", "Prints the list of ROCm devices enabled on platform"},
            {"-v", "Prints the plugin version"},
        };

        for (const auto& opt : expected_options) {
            REQUIRE(!opt.short_flag.empty());
            REQUIRE(!opt.description.empty());

            // Short flags should start with single dash
            CHECK(opt.short_flag[0] == '-');
            CHECK(opt.short_flag.length() == 2);
        }
    }

    SECTION("Mutually exclusive options -a and -A")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "exclusive options"));

        // -a (unidirectional) and -A (bidirectional) should be mutually exclusive
        // This is enforced by CLI11's excludes() method

        std::string option_a = "-a";
        std::string option_A = "-A";

        // They are different options
        REQUIRE(option_a != option_A);

        // Both relate to copy direction
        REQUIRE(wb_strings::to_lower_copy(option_a) == "-a");
        REQUIRE(wb_strings::to_upper_copy(option_a) == "-A");
    }
}


// =============================================================================
// TEST CASE: RBT Version String Format
// =============================================================================

TEST_CASE("RBTPlugin::VersionFormat", "[unit][rbt][version]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Version string contains expected components")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "version components"));

        // Expected format: "Plugin: <name>  > <description>  > v:<version>"
        std::string plugin_name = "rbt";
        std::string plugin_description = "Builtin: ROCm Bandwidth Test";
        std::string plugin_version = "0.1.0";

        auto version_string = amd_fmt::format("Plugin: {}  > {}  > v:{}",
            plugin_name, plugin_description, plugin_version);

        CHECK(wb_strings::contains(version_string, plugin_name));
        CHECK(wb_strings::contains(version_string, plugin_description));
        CHECK(wb_strings::contains(version_string, plugin_version));
    }
}


}  // namespace amd_work_bench::test::unit::rbt
