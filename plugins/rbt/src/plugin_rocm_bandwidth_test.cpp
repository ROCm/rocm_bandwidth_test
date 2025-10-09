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
 * Description: plugin_transferbench.cpp
 *
 */

#include <awb/plugins.hpp>

#include <awb/content_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <plugin_rocm_bandwidth_test.hpp>
#include <view_rocm_bandwidth_test.hpp>
#include <CLI/CLI.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>


namespace amd_work_bench::plugin::rocm_bandwidth_test
{

/*
 *  TODO:   Use this structure so we can define/write unit tests for the plugin.
 */

auto command_run_handler(const WordList_t& args) -> i32_t
{
    std::ignore = args;
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Plugin: '{}' ", kPLUGIN_MODULE_NAME);
    return 100;
}


auto register_plugin_view() -> void
{
    amd_work_bench::content::views::add_view<ViewRocmBandwidthTest_t>();
}

}    // namespace amd_work_bench::plugin::rocm_bandwidth_test


AMD_WORK_BENCH_PLUGIN_SETUP("rbt", "Linux System Tools Team (MLSE Linux) @AMD", "Builtin: ROCm Bandwidth Test", "0.1.0")
{
    using namespace wb_plugin_rbt;
    register_plugin_view();
}


/*
 *  This is the main entry point for the plugin, on the running code side.
 *  This is the main(int argc, char** argv) function renamed, so it can keep its functionalities and make it easier
 *  to transition existing static executables to dynamic (plugin) build.
 */
// extern int plugin_main_entry(int argc, char** argv);

/*
 *  This is the main entry point for the plugin.
 *  This function is called by the Work Bench Plugin Manager.
 *  TODO: Implement the plugin_main() function, so that the plugin can be loaded by the Work Bench Plugin Manager.
 */
int32_t plugin_main(int argc, char** argv)
{
    static auto is_unidirectional_mode{false};
    static auto is_bidirectional_mode{false};
    static auto is_list_topology{false};

    auto reset_flags = [&]() {
        is_unidirectional_mode = false;
        is_bidirectional_mode = false;
        is_list_topology = false;
    };

    auto plugin_rbt_cli_app = CLI::App{"rbt", "CLI: ROCm Bandwidth Test Plugin"};
    plugin_rbt_cli_app.require_option();
    plugin_rbt_cli_app.allow_extras();
    auto plugin_rbt_cli_app_option_a = plugin_rbt_cli_app.add_flag_callback(
        "-a",
        [&]() {
            reset_flags();
            is_unidirectional_mode = true;
        },
        "Perform Unidirectional Copy involving all device combinations");

    auto plugin_rbt_cli_app_option_A = plugin_rbt_cli_app.add_flag_callback(
        "-A",
        [&]() {
            reset_flags();
            is_bidirectional_mode = true;
        },
        "Perform Bidirectional Copy involving all device combinations");
    plugin_rbt_cli_app_option_a->excludes(plugin_rbt_cli_app_option_A);


    auto plugin_rbt_cli_app_option_e = plugin_rbt_cli_app.add_flag_callback(
        "-e",
        [&]() {
            reset_flags();
            is_list_topology = true;
        },
        "Prints the list of ROCm devices enabled on platform");

    plugin_rbt_cli_app.add_flag_callback(
        "-v,--version",
        [&]() {
            const auto help_message_build_info =
                amd_fmt::format("Plugin: {}  > {}  > v:{}\n", plugin_get_name(), plugin_get_description(), plugin_get_version());
            std::cout << help_message_build_info << "\n";
        },
        "Prints the plugin version");


    //  Call the plugin_main_entry() function, which is the main entry point for the plugin.
    //  auto test = 0;
    //  test = plugin_main_entry(argc, argv);
    //  std::cout << "plugin_main_entry() returned: " << test << "\n\n";

    //  If not enough args, show help info
    //  We can drop the plugin name off the command line and just work with the flags/options passed in.
    //  WordList_t args = {(argv + 1), (argv + argc)};
    auto new_argc = (argc - 1);
    auto new_argv = (argv + 1);
    WordList_t args = {new_argv, (new_argv + new_argc)};

    std::cout << "  - m_argc:" << argc << "\n";
    std::cout << "  - m_argv:" << argv << "\n";
    for (auto idx = 0; idx < argc; ++idx) {
        std::cout << "  - arg_list[" << idx << "]: " << argv[idx] << "\n";
    }
    std::cout << "\n\n";

    std::cout << "  - new_argc:" << new_argc << "\n";
    std::cout << "  - new_argv:" << new_argv << "\n";
    for (const auto& arg : args) {
        std::cout << "  - arg: " << arg << "\n";
    }
    std::cout << "\n\n";

    if (args.size() == 0) {
        std::cout << plugin_rbt_cli_app.help() << "\n\n";
        return (EXIT_FAILURE);
    }

    // Parse the args
    try {
        std::reverse(args.begin(), args.end());
        plugin_rbt_cli_app.parse(args);

        if (is_unidirectional_mode) {
            std::cout << "  - Unidirectional Mode\n";
        }
        if (is_bidirectional_mode) {
            std::cout << "  - Bidirectional Mode\n";
        }
        if (is_list_topology) {
            std::cout << "  - List Topology\n";
        }

    } catch (const CLI::ParseError& exc) {
        const auto parse_error_message = amd_fmt::format("[{}] Error: Command line option not found: {} -> {} \n",
                                                         wb_plugin_rbt::kPLUGIN_MODULE_NAME,
                                                         exc.get_exit_code(),
                                                         exc.what());
        std::cout << parse_error_message << "\n";
        return plugin_rbt_cli_app.exit(exc);
    }

    return (EXIT_SUCCESS);
}
