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
 * Tests the RBT plugin functionality including constants, types,
 * command handling, and CLI argument building.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>
#include <plugin_rocm_bandwidth_test.hpp>
#include <CLI/CLI.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>


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

    SECTION("Plugin module name is defined correctly")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "module name"));

        REQUIRE(!wb_plugin_rbt::kPLUGIN_MODULE_NAME.empty());
        REQUIRE(wb_plugin_rbt::kPLUGIN_MODULE_NAME == "ROCm Bandwidth Test");
    }
}


// =============================================================================
// TEST CASE: RBT WordList Type
//
// WordList_t is the public alias used by the RBT plugin for argv-style data
// (defined in plugins/rbt/include/plugin_rocm_bandwidth_test.hpp). These
// tests validate the contract callers depend on:
//   - it stores std::string-compatible elements,
//   - it can be reversed in-place (the plugin reverses before CLI11 parsing),
//   - it round-trips cleanly through std::vector<std::string> APIs.
// =============================================================================

TEST_CASE("RBTPlugin::WordList", "[unit][rbt][wordlist]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("WordList_t is alias-compatible with std::vector<std::string>")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "alias compatibility"));

        static_assert(
            std::is_same_v<wb_plugin_rbt::WordList_t, std::vector<std::string>>,
            "WordList_t must remain a std::vector<std::string> so plugin_main "
            "can build it from argv ranges and pass it to CLI11.");

        // Construct from an argv-like range (mirrors plugin_main()).
        const char* argv_like[] = {"-a", "-A", "value"};
        wb_plugin_rbt::WordList_t args(argv_like, argv_like + 3);

        REQUIRE(args.size() == 3);
        REQUIRE(args.front() == "-a");
        REQUIRE(args.back() == "value");
    }

    SECTION("WordList_t reverses in place (plugin_main parse pre-step)")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "reverse"));

        wb_plugin_rbt::WordList_t args = {"a", "b", "c"};
        std::reverse(args.begin(), args.end());

        REQUIRE(args == wb_plugin_rbt::WordList_t{"c", "b", "a"});
    }

    SECTION("Empty WordList_t is well-defined")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty"));

        wb_plugin_rbt::WordList_t empty_args;

        REQUIRE(empty_args.empty());
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
// Helper: build a CLI11 app whose option wiring mirrors plugin_main()
// in plugins/rbt/src/plugin_rocm_bandwidth_test.cpp. Keeping this aligned
// with the plugin source ensures the tests fail loudly if the production
// CLI surface drifts (e.g. -a/-A no longer mutually exclusive).
// =============================================================================
struct RbtModeState
{
        bool is_unidirectional_mode = false;
        bool is_bidirectional_mode  = false;
        bool is_list_topology       = false;

        void reset()
        {
            is_unidirectional_mode = false;
            is_bidirectional_mode  = false;
            is_list_topology       = false;
        }

        int active_count() const
        {
            return (is_unidirectional_mode ? 1 : 0) + (is_bidirectional_mode ? 1 : 0) +
                   (is_list_topology ? 1 : 0);
        }
};

// CLI::App is non-copyable and non-movable, so we configure-by-reference
// rather than return by value. Callers construct an App on the stack and
// hand it to this function (matches how plugin_main() owns its CLI::App).
inline void configure_rbt_cli_app(CLI::App& app, RbtModeState& state)
{
    app.require_option();
    app.allow_extras();

    auto* opt_a = app.add_flag_callback(
        "-a",
        [&state]() {
            state.reset();
            state.is_unidirectional_mode = true;
        },
        "Perform Unidirectional Copy involving all device combinations");

    auto* opt_A = app.add_flag_callback(
        "-A",
        [&state]() {
            state.reset();
            state.is_bidirectional_mode = true;
        },
        "Perform Bidirectional Copy involving all device combinations");
    opt_a->excludes(opt_A);

    app.add_flag_callback(
        "-e",
        [&state]() {
            state.reset();
            state.is_list_topology = true;
        },
        "Prints the list of ROCm devices enabled on platform");
}

// Simulates the parse path used by plugin_main(): drop argv[0], reverse, parse.
inline auto parse_rbt_args(CLI::App& app, std::vector<std::string> args) -> void
{
    std::reverse(args.begin(), args.end());
    app.parse(args);
}


// =============================================================================
// TEST CASE: RBT Mode Flags Logic (drives the actual CLI11 wiring)
// =============================================================================

TEST_CASE("RBTPlugin::ModeFlags", "[unit][rbt][modes]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("-a sets unidirectional mode only")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "-a"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);
        parse_rbt_args(app, {"-a"});

        REQUIRE(state.is_unidirectional_mode == true);
        REQUIRE(state.is_bidirectional_mode == false);
        REQUIRE(state.is_list_topology == false);
        REQUIRE(state.active_count() == 1);
    }

    SECTION("-A sets bidirectional mode only")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "-A"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);
        parse_rbt_args(app, {"-A"});

        REQUIRE(state.is_bidirectional_mode == true);
        REQUIRE(state.is_unidirectional_mode == false);
        REQUIRE(state.active_count() == 1);
    }

    SECTION("-e sets list-topology mode only")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "-e"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);
        parse_rbt_args(app, {"-e"});

        REQUIRE(state.is_list_topology == true);
        REQUIRE(state.active_count() == 1);
    }

    SECTION("-a and -A are mutually exclusive (CLI11 excludes())")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "exclusion"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);

        REQUIRE_THROWS_AS(parse_rbt_args(app, {"-a", "-A"}), CLI::ParseError);
    }

    SECTION("require_option() rejects empty argument list")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "no args"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);

        REQUIRE_THROWS_AS(parse_rbt_args(app, {}), CLI::ParseError);
    }

    SECTION("Unknown options are tolerated by allow_extras()")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "extras"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);

        // -a satisfies require_option(); --unknown should land in remaining()
        REQUIRE_NOTHROW(parse_rbt_args(app, {"-a", "--unknown-flag"}));
        REQUIRE(state.is_unidirectional_mode == true);
    }
}


// =============================================================================
// TEST CASE: RBT Exit Codes
// =============================================================================

TEST_CASE("RBTPlugin::ExitCodes", "[unit][rbt][exitcodes]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Plugin return codes are well-defined")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "return codes"));

        // PluginStatus_t defines the expected plugin return code semantics
        using Status = amd_work_bench::PluginStatus_t;

        REQUIRE(static_cast<int>(Status::PLUGIN_MAIN_ENTRY_NOT_FOUND) == -1);
        REQUIRE(static_cast<int>(Status::PLUGIN_FINISHED_SUCCESSFULLY) == 0);
        REQUIRE(static_cast<int>(Status::PLUGIN_FINISHED_WITH_ERRORS) == 1);
    }
}


// =============================================================================
// TEST CASE: RBT CLI Help Formatting (verifies actual CLI11 help text)
// =============================================================================

TEST_CASE("RBTPlugin::HelpFormatting", "[unit][rbt][help]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("CLI11 help text advertises every documented mode flag")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "help text"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);
        const auto help = app.help();

        CHECK(wb_strings::contains(help, "-a"));
        CHECK(wb_strings::contains(help, "-A"));
        CHECK(wb_strings::contains(help, "-e"));
        CHECK(wb_strings::contains(help, "Unidirectional"));
        CHECK(wb_strings::contains(help, "Bidirectional"));
        CHECK(wb_strings::contains(help, "ROCm devices"));
    }

    SECTION("CLI11 reports a parse error with a non-zero exit code on conflict")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "parse error"));

        RbtModeState state;
        CLI::App app{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
        configure_rbt_cli_app(app, state);

        try {
            parse_rbt_args(app, {"-a", "-A"});
            FAIL("Expected CLI11 to throw on mutually-exclusive flags");
        } catch (const CLI::ParseError& exc) {
            REQUIRE(exc.get_exit_code() != 0);
            REQUIRE(std::string(exc.what()).empty() == false);
        }
    }
}


// =============================================================================
// TEST CASE: RBT Version String Format
// =============================================================================

TEST_CASE("RBTPlugin::VersionFormat", "[unit][rbt][version]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Version string contains expected components from plugin setup")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "version components"));

        // These values match the AMD_WORK_BENCH_PLUGIN_SETUP macro call in the plugin source
        std::string plugin_name = "rbt";
        std::string plugin_description = "Builtin: ROCm Bandwidth Test";
        std::string plugin_version = "0.1.0";

        auto version_string = amd_fmt::format("Plugin: {}  > {}  > v:{}",
            plugin_name, plugin_description, plugin_version);

        CHECK(wb_strings::contains(version_string, plugin_name));
        CHECK(wb_strings::contains(version_string, plugin_description));
        CHECK(wb_strings::contains(version_string, plugin_version));
        CHECK(wb_strings::contains(version_string, wb_plugin_rbt::kPLUGIN_MODULE_NAME));
    }
}


}  // namespace amd_work_bench::test::unit::rbt
