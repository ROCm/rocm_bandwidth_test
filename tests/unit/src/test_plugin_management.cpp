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
 * @file test_plugin_management.cpp
 * @brief Unit tests for plugin management system
 * @author ROCm Bandwidth Test Team
 *
 * Tests the plugin loading, registration, and lifecycle management
 * in the WorkBench framework.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>

#include <filesystem>


namespace amd_work_bench::test::unit::plugins
{

// =============================================================================
// TEST CASE: Plugin Data Structure
// =============================================================================

TEST_CASE("PluginManagement::PluginData", "[unit][plugins][data]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("PluginData_t structure initialization")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "PluginData_t init"));

        amd_work_bench::PluginData_t data{};

        REQUIRE(data.m_argc == 0);
        REQUIRE(data.m_argv == nullptr);
        REQUIRE(data.m_ret_code == 0);
    }

    SECTION("PluginData_t can store argc/argv")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "PluginData_t with args"));

        char* mock_argv[] = {
            const_cast<char*>("program"),
            const_cast<char*>("--flag"),
            const_cast<char*>("value"),
            nullptr
        };

        amd_work_bench::PluginData_t data{
            .m_argc = 3,
            .m_ret_code = 0,
            .m_argv = mock_argv
        };

        REQUIRE(data.m_argc == 3);
        REQUIRE(data.m_argv != nullptr);
        REQUIRE(std::string(data.m_argv[0]) == "program");
        REQUIRE(std::string(data.m_argv[1]) == "--flag");
        REQUIRE(std::string(data.m_argv[2]) == "value");
    }
}


// =============================================================================
// TEST CASE: Plugin Functionality Structure
// =============================================================================

TEST_CASE("PluginManagement::PluginFunctionality", "[unit][plugins][functionality]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("PluginFunctionality_t default initialization")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default init"));

        amd_work_bench::PluginFunctionality_t func{};

        REQUIRE(func.m_plugin_init_function == nullptr);
        REQUIRE(func.m_plugin_get_name_function == nullptr);
        REQUIRE(func.m_plugin_get_author_function == nullptr);
        REQUIRE(func.m_plugin_get_description_function == nullptr);
        REQUIRE(func.m_plugin_get_compatibility_function == nullptr);
        REQUIRE(func.m_plugin_get_version_function == nullptr);
        REQUIRE(func.m_plugin_get_subcommand_function == nullptr);
        REQUIRE(func.m_plugin_get_feature_function == nullptr);
        REQUIRE(func.m_plugin_main_entry_point == nullptr);
        REQUIRE(func.m_library_init_function == nullptr);
        REQUIRE(func.m_library_get_name_function == nullptr);
    }

    SECTION("MockPlugin creates valid functionality structure")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "mock plugin functionality"));

        auto func = wb_test::MockPlugin::create_functionality();

        REQUIRE(func.m_plugin_init_function != nullptr);
        REQUIRE(func.m_plugin_get_name_function != nullptr);
        REQUIRE(func.m_plugin_get_author_function != nullptr);
        REQUIRE(func.m_plugin_get_description_function != nullptr);
        REQUIRE(func.m_plugin_get_compatibility_function != nullptr);
        REQUIRE(func.m_plugin_get_version_function != nullptr);

        // Verify function pointers return expected values
        REQUIRE(std::string(func.m_plugin_get_name_function()) == wb_test::MockPlugin::NAME);
        REQUIRE(std::string(func.m_plugin_get_author_function()) == wb_test::MockPlugin::AUTHOR);
        REQUIRE(std::string(func.m_plugin_get_description_function()) == wb_test::MockPlugin::DESCRIPTION);
        REQUIRE(std::string(func.m_plugin_get_version_function()) == wb_test::MockPlugin::VERSION);
    }
}


// =============================================================================
// TEST CASE: Plugin Status Enumeration
// =============================================================================

TEST_CASE("PluginManagement::PluginStatus", "[unit][plugins][status]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("PluginStatus_t values are correct")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "status enum values"));

        using Status = amd_work_bench::PluginStatus_t;

        REQUIRE(static_cast<int>(Status::PLUGIN_MAIN_ENTRY_NOT_FOUND) == -1);
        REQUIRE(static_cast<int>(Status::PLUGIN_FINISHED_SUCCESSFULLY) == 0);
        REQUIRE(static_cast<int>(Status::PLUGIN_FINISHED_WITH_ERRORS) == 1);
    }
}


// =============================================================================
// TEST CASE: Plugin Engine Version
// =============================================================================

TEST_CASE("PluginManagement::PluginEngineVersion", "[unit][plugins][version]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("PluginEngineVersion_t values are correct")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "engine version enum"));

        using Version = amd_work_bench::PluginEngineVersion_t;

        REQUIRE(static_cast<uint16_t>(Version::VERSION_0_0) == 0);
        REQUIRE(static_cast<uint16_t>(Version::VERSION_1_0) == 1);
    }
}


// =============================================================================
// TEST CASE: Plugin Extension Constants
// =============================================================================

TEST_CASE("PluginManagement::Extensions", "[unit][plugins][constants]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Plugin file extensions are defined correctly")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "file extensions"));

        REQUIRE(amd_work_bench::kLIBRARY_PLUGIN_EXTENSION == ".amdlplug");
        REQUIRE(amd_work_bench::kREGULAR_PLUGIN_EXTENSION == ".amdplug");
    }

    SECTION("Minimum builtin plugins constant is valid")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "builtin minimum"));

        REQUIRE(amd_work_bench::kBUILTIN_PLUGINS_MIN >= 1);
    }
}


// =============================================================================
// TEST CASE: Plugin Function Name Constants
// =============================================================================

TEST_CASE("PluginManagement::FunctionNames", "[unit][plugins][symbols]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Plugin function symbol names are correct")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "symbol names"));

        REQUIRE(amd_work_bench::kPLUGIN_INIT_FUNCTION == "plugin_init");
        REQUIRE(amd_work_bench::kPLUGIN_GET_NAME_FUNCTION == "plugin_get_name");
        REQUIRE(amd_work_bench::kPLUGIN_GET_AUTHOR_FUNCTION == "plugin_get_author");
        REQUIRE(amd_work_bench::kPLUGIN_GET_DESCRIPTION_FUNCTION == "plugin_get_description");
        REQUIRE(amd_work_bench::kPLUGIN_GET_COMPATIBILITY_FUNCTION == "plugin_get_compatibility");
        REQUIRE(amd_work_bench::kPLUGIN_GET_VERSION_FUNCTION == "plugin_get_version");
        REQUIRE(amd_work_bench::kPLUGIN_GET_SUBCOMMAND_FUNCTION == "plugin_get_subcommand");
        REQUIRE(amd_work_bench::kPLUGIN_GET_FEATURE_FUNCTION == "plugin_get_feature");
        REQUIRE(amd_work_bench::kPLUGIN_MAIN_ENTRY_POINT == "plugin_main");
    }
}


// =============================================================================
// TEST CASE: Feature Structure
// =============================================================================

TEST_CASE("PluginManagement::Feature", "[unit][plugins][feature]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Feature_t default initialization")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "Feature_t default"));

        amd_work_bench::Feature_t feature{};

        REQUIRE(feature.m_is_enabled == false);
        REQUIRE(feature.m_name.empty());
    }

    SECTION("Feature_t can be initialized with values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "Feature_t with values"));

        amd_work_bench::Feature_t feature{
            .m_is_enabled = true,
            .m_name = "test_feature"
        };

        REQUIRE(feature.m_is_enabled == true);
        REQUIRE(feature.m_name == "test_feature");
    }
}


// =============================================================================
// TEST CASE: SubCommand Structure
// =============================================================================

TEST_CASE("PluginManagement::SubCommand", "[unit][plugins][subcommand]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("SubCommand_t default initialization")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "SubCommand_t default"));

        amd_work_bench::SubCommand_t subcmd{};

        REQUIRE(subcmd.m_long_format.empty());
        REQUIRE(subcmd.m_short_format.empty());
        REQUIRE(subcmd.m_description.empty());
        REQUIRE(subcmd.m_function_cb == nullptr);
        REQUIRE(subcmd.m_subcmd_type == amd_work_bench::SubCommand_t::SubCommandType_t::Option);
    }

    SECTION("SubCommand_t can store command info")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "SubCommand_t with values"));

        bool callback_called = false;

        amd_work_bench::SubCommand_t subcmd{
            .m_long_format = "--verbose",
            .m_short_format = "-v",
            .m_description = "Enable verbose output",
            .m_function_cb = [&callback_called](const std::vector<std::string>&) {
                callback_called = true;
            },
            .m_subcmd_type = amd_work_bench::SubCommand_t::SubCommandType_t::Option
        };

        REQUIRE(subcmd.m_long_format == "--verbose");
        REQUIRE(subcmd.m_short_format == "-v");
        REQUIRE(subcmd.m_description == "Enable verbose output");
        REQUIRE(subcmd.m_function_cb != nullptr);

        // Test callback invocation
        subcmd.m_function_cb({});
        REQUIRE(callback_called == true);
    }

    SECTION("SubCommandType_t enumeration values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "SubCommandType_t"));

        using Type = amd_work_bench::SubCommand_t::SubCommandType_t;

        // Verify both enum values exist and are distinct
        REQUIRE(Type::Option != Type::SubCommand);
    }
}


// =============================================================================
// TEST CASE: Plugin Path Operations
// =============================================================================

TEST_CASE("PluginManagement::PathOperations", "[unit][plugins][paths]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Plugin paths can be retrieved")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "get_path_all"));

        // This should not throw even if no plugins are loaded
        auto paths = amd_work_bench::PluginManagement_t::plugin_get_path_all();

        // Just verify it returns a valid vector (may be empty)
        REQUIRE_NOTHROW(paths.size());
    }

    SECTION("Plugin load paths can be retrieved")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "get_load_path_all"));

        auto load_paths = amd_work_bench::PluginManagement_t::plugin_get_load_path_all();

        // Verify it returns a valid vector
        REQUIRE_NOTHROW(load_paths.size());
    }
}


}  // namespace amd_work_bench::test::unit::plugins
