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
 * Description: cmdline_mgmt.cpp
 *
 */


#include "startup_mgmt.hpp"

#include <awb/common_utils.hpp>
#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/subcommands.hpp>
#include <awb/threading.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <CLI/CLI.hpp>

#include <iostream>
#include <string>
#include <vector>


namespace amd_work_bench::startup
{


namespace cmdline_subcommands
{

enum class PluginInfoType_t
{
    SIMPLE_LIST,
    DETAILED_LIST
};

namespace details
{
struct PluginHandlerData_t
{
    public:
        int m_argc;
        char** m_argv;
        WordList_t m_arg_list;

    private:
};


// Default offset for the plugin arguments
constexpr auto kPLUGIN_ARGS_OFFSET = u32_t(3);

// Thread pool for command line management
constexpr auto kCMDLINE_MGMT_THREAD_POOL_SIZE = u32_t(10);
static auto s_cmdline_thread_pool = wb_threads::ThreadPool_t(kCMDLINE_MGMT_THREAD_POOL_SIZE);

}    // namespace details


auto command_plugin_run_handler(const std::string plugin_name,
                                const details::PluginHandlerData_t& plugin_data_handler,
                                u32_t args_offset = details::kPLUGIN_ARGS_OFFSET) -> i32_t
{
    if (plugin_name.empty()) {
        const auto plugin_name_not_provided_message = amd_fmt::format("[CmdLine] -> Error: Plugin name not provided!");
        std::cout << plugin_name_not_provided_message << "\n\n";
        return (EXIT_FAILURE);
    }

    auto plugin_instance = PluginManagement_t::plugin_get(plugin_name);
    if (!plugin_instance) {
        const auto plugin_not_found_message =
            amd_fmt::format("[CmdLine] -> Error: Plugin not found/registered properly: '{}'", plugin_name);
        std::cout << plugin_not_found_message << "\n\n";
        return (EXIT_FAILURE);
    }

    //  Run the plugin
    //  TODO:   Allocate a thread from the thread pool and run the plugin with that thread, naming it the plugin name.
    //          That should use TaskManagement existing code.
    // auto plugin_library_path = plugin_instance->plugin_get_library_path().string();
    // std::cout << "Running plugin: " << plugin_name << " -> " << plugin_library_path << " argc: " << plugin_data_handler.m_argc
    //          << " argv: " << plugin_data_handler.m_argv << "\n";
    /*
     *  The initial arguments are:
     *    ./amd_work_bench plugin --run Hello --arg1 1 --arg2 2 --arg3 3 --arg4 4
     *
     *  So we cut the ones up to the plugin name and submit the remaining ones to the plugin code.
     *      {./amd_work_bench plugin --run} Hello --arg1 1 --arg2 2 --arg3 3 --arg4 4
     *  Then the plugin can parse the arguments as needed, using or dropping its own name, like we do with 'argv[0]'
     */
    auto plugin_argc = (plugin_data_handler.m_argc - args_offset);
    auto plugin_argv = (plugin_data_handler.m_argv + args_offset);

    // auto plugin_instance_shared = std::shared_ptr<Plugin_t>(plugin_instance);
    // auto plugin_instance_result = plugin_instance->plugin_main_entry_run(plugin_argc, plugin_argv);
    //  TODO:   We might need ot move this part to a different source for a matter of code organization
    auto plugin_instance_run =
        details::s_cmdline_thread_pool.enqueue_task(&Plugin_t::plugin_main_entry_run, plugin_instance, plugin_argc, plugin_argv);
    /*  TODO:   Check if this piece will or not be needed.
    auto test = wb_threads::thread_safe_wrapper(details::s_cmdline_thread_pool,
                                                &Plugin_t::plugin_main_entry_run,
                                                plugin_instance,
                                                plugin_argc,
                                                plugin_argv);
    */

    try {
        auto plugin_instance_result = plugin_instance_run.get();
    } catch (const std::future_error& exc) {
        if (exc.code() == std::future_errc::broken_promise) {
            const auto plugin_run_error_message = amd_fmt::format(
                "[CmdLine] -> Error: Plugin thread exited directly. broken promise: (std::exit()/_exit() called)  -> {}",
                exc.what());
            std::cout << plugin_run_error_message << "\n\n";
        } else {
            const auto plugin_run_error_message = amd_fmt::format("[CmdLine] -> Error: std::future() error: -> {}", exc.what());
            std::cout << plugin_run_error_message << "\n\n";
        }
    } catch (const std::exception& exc) {
        const auto plugin_run_error_message = amd_fmt::format("[CmdLine] -> Error: Main thread exception: -> {}", exc.what());
        std::cout << plugin_run_error_message << "\n\n";
    }

    return (EXIT_SUCCESS);
}


auto command_plugin_list_traverse_detail(const std::string& plugin_filter, PluginInfoType_t plugin_info_type) -> void
{
    if (plugin_filter.empty()) {
        return;
    }

    constexpr auto help_handler_message = R"(
    Plugin Management:

        *Registered plugin(s): )";

    constexpr auto kEXTRA_SPACES = u32_t(6);
    constexpr auto kPLUGIN_PATH_MAX_CHARS = u32_t(90);
    constexpr auto kPLUGIN_AUTHOR_MAX_CHARS = u32_t(40);
    constexpr auto kPLUGIN_DESCRIPTION_MAX_CHARS = u32_t(40);
    constexpr auto kPLUGIN_VERSION_MAX_CHARS = u32_t(20);
    constexpr auto kPLUGIN_TRUE_FALSE_MAX_CHARS = u32_t(5);
    const auto kBUILTIN_PLUGIN_NAMES = std::set<std::string>{"BUILTIN", "BUILT-IN"};
    const auto kALL_PLUGIN_NAMES = std::string{"*"};

    auto largest_plugin_name_format_chars = size_t(0);
    auto largest_plugin_description_format_chars = size_t(0);
    for (const auto& plugin : PluginManagement_t::plugin_get_all()) {
        if (plugin.is_library_plugin()) {
            continue;
        }

        auto plugin_registered_name = wb_strings::to_upper_copy(plugin.plugin_get_name());
        if (!kBUILTIN_PLUGIN_NAMES.contains(plugin_registered_name)) {
            largest_plugin_name_format_chars = std::max(largest_plugin_name_format_chars, plugin.plugin_get_name().size());
            largest_plugin_description_format_chars =
                std::max(largest_plugin_description_format_chars, plugin.plugin_get_description().size());
        }
    }

    std::cout << help_handler_message << "\n";
    for (const auto& plugin : PluginManagement_t::plugin_get_all()) {
        if (plugin.is_library_plugin()) {
            continue;
        }

        auto plugin_registered_name = wb_strings::to_upper_copy(plugin.plugin_get_name());
        if (!kBUILTIN_PLUGIN_NAMES.contains(plugin_registered_name)) {
            if ((plugin_filter != kALL_PLUGIN_NAMES) &&
                (!wb_strings::contains(plugin_registered_name, (wb_strings::to_upper_copy(plugin_filter))))) {
                //(!plugin_registered_name.contains(wb_strings::to_upper_copy(plugin_filter)))) {
                continue;
            }

            if ((plugin_info_type == PluginInfoType_t::SIMPLE_LIST) || (plugin_info_type == PluginInfoType_t::DETAILED_LIST)) {
                //  Enable and disable bold in terminal
                const auto plugin_name_help_message = amd_fmt::format("- {: <{}}\033[1m{}\033[0m",
                                                                      plugin.plugin_get_name(),
                                                                      (largest_plugin_name_format_chars + kEXTRA_SPACES),
                                                                      ">");
                //   Enable and disable faint in terminal
                const auto plugin_description_help_message =
                    amd_fmt::format("\033[2;3m{: <{}}\033[0m",
                                    plugin.plugin_get_description(),
                                    (kPLUGIN_DESCRIPTION_MAX_CHARS + (kEXTRA_SPACES - 1)));

                const auto plugin_version_help_message = amd_fmt::format("> Version: \033[2;3m{: <{}}\033[0m",
                                                                         plugin.plugin_get_version(),
                                                                         ((kPLUGIN_VERSION_MAX_CHARS / 2) + (kEXTRA_SPACES / 2)));

                const auto plugin_simple_help_message_full = amd_fmt::format("\t{} {} {}\n",
                                                                             plugin_name_help_message,
                                                                             plugin_description_help_message,
                                                                             plugin_version_help_message);

                std::cout << plugin_simple_help_message_full;
            }
            if (plugin_info_type == PluginInfoType_t::DETAILED_LIST) {
                //"> Compat.Version: {} \n \t - Loaded: {}    > Path: {}    > Main: {} \n"
                const auto plugin_author_details_help_message = amd_fmt::format("  - Author: {: <{}}",
                                                                                plugin.plugin_get_author(),
                                                                                (kPLUGIN_AUTHOR_MAX_CHARS + kEXTRA_SPACES));
                const auto plugin_framework_version_details_help_message =
                    amd_fmt::format("> Framework Compat.V: {: <{}}",
                                    plugin.plugin_get_compatibility(),
                                    (kPLUGIN_VERSION_MAX_CHARS + (kEXTRA_SPACES / 2)));
                const auto plugin_loaded_details_help_message =
                    amd_fmt::format("  - Loaded: {: <{}}",
                                    plugin.is_loaded() ? "Yes" : "No",
                                    (kPLUGIN_TRUE_FALSE_MAX_CHARS + (kEXTRA_SPACES / 2)));
                const auto plugin_path_details_help_message = amd_fmt::format("> Path: \033[2;3m{: <{}}\033[0m",
                                                                              plugin.plugin_get_library_path().string(),
                                                                              (kPLUGIN_PATH_MAX_CHARS + (kEXTRA_SPACES / 2)));
                const auto plugin_main_entry_details_help_message =
                    amd_fmt::format("[=] Main Entry Avail: {: <{}}",
                                    plugin.has_plugin_main_entry() ? "Yes" : "No",
                                    (kPLUGIN_TRUE_FALSE_MAX_CHARS + (kEXTRA_SPACES / 2)));

                const auto plugin_details_help_message_full = amd_fmt::format("\t{}{}\n\t{}{}{}\n",
                                                                              plugin_author_details_help_message,
                                                                              plugin_framework_version_details_help_message,
                                                                              plugin_loaded_details_help_message,
                                                                              plugin_path_details_help_message,
                                                                              plugin_main_entry_details_help_message);
                std::cout << plugin_details_help_message_full << "\n";
            }
        }
    }
    std::cout << "\n";
}

auto command_plugin_list_handler(const std::string& plugin_filter) -> void
{
    command_plugin_list_traverse_detail(plugin_filter, PluginInfoType_t::SIMPLE_LIST);
}

auto command_plugin_info_handler(const std::string& plugin_filter) -> void
{
    command_plugin_list_traverse_detail(plugin_filter, PluginInfoType_t::DETAILED_LIST);
}


void add_plugin_subcommand_handler(CLI::App& app, const details::PluginHandlerData_t& plugin_data_handler)
{
    //  If the plugin table is empty, load the plugins
    if (PluginManagement_t::plugin_get_all().empty()) {
        load_plugins();
    }

    static auto is_verbose_mode{false};
    static auto is_legacy_mode{true};
    static auto plugin_list_filter = std::string("*");
    static auto plugin_info_filter = std::string("*");
    static auto plugin_run_filter = std::string("");

    /*
     *  'plugin' subcommand and related options/flags.
     */
    auto plugin_subcommand = app.add_subcommand("plugin", "Plugin: subcommand");    //->prefix_command();
    plugin_subcommand->allow_extras();
    // plugin_subcommand->alias("run");

    plugin_subcommand->add_flag("-c,--legacy", is_legacy_mode, "Legacy mode: For plugins supporting legacy output")
        ->default_val(true);

    plugin_subcommand->add_option("-l,--list", plugin_list_filter, "List plugin(s) (all by default)")
        ->default_str("*")
        ->expected(0, 1)
        ->capture_default_str();

    plugin_subcommand->add_option("-i,--info", plugin_info_filter, "Get information about plugin(s)")
        ->default_str("*")
        ->expected(0, 1)
        ->capture_default_str();

    plugin_subcommand->add_option("-r,--run", plugin_run_filter, "Run a plugin")->default_val("")->expected(1, 100);
    plugin_subcommand->add_flag("-v,--verbose", is_verbose_mode, "Verbose output")->default_val(false);

    plugin_subcommand->callback([&]() {
        if (app.got_subcommand("plugin")) {
            if (app.get_subcommand("plugin")->count("--list")) {
                command_plugin_list_handler(plugin_list_filter);
            }

            if (app.get_subcommand("plugin")->count("--info")) {
                command_plugin_info_handler(plugin_info_filter);
            }

            if (app.get_subcommand("plugin")->count("--run")) {
                auto plugin_run_result = command_plugin_run_handler(plugin_run_filter, plugin_data_handler);
                if (plugin_run_result == EXIT_FAILURE) {
                    const auto plugin_run_error_message =
                        amd_fmt::format("[CmdLine] -> Error: Plugin run failed! {}{}", "--run", plugin_run_filter);
                    std::cout << plugin_run_error_message << "\n\n";
                }
            }
        }
    });


    /*
     *  'run' subcommand and related options/flags.
     *  This subcommand is used to run a plugin directly. .
     */
    auto plugin_run_subcommand = app.add_subcommand("run", "Run a plugin")->prefix_command(true);
    plugin_run_subcommand->allow_extras();
    plugin_run_subcommand->callback([&]() {
        if (app.got_subcommand("run")) {
            /*
                ./amd_work_bench run tb plugin-name ...
                Based on this, the subcommand 'run' is plugin_data_handler.m_arg_list[0],
                plugin_data_handler.m_arg_list[1] is the plugin name.

                Then, parameters are passed to the plugin with offset 2.

                This is a shortcut for:
                ./amd_work_bench plugin --run plugin-name ...
            */
            constexpr auto kPLUGIN_ARGS_LOCAL_OFFSET = u32_t(2);
            plugin_run_filter = plugin_data_handler.m_arg_list.at(1);
            auto plugin_run_result =
                command_plugin_run_handler(plugin_run_filter, plugin_data_handler, kPLUGIN_ARGS_LOCAL_OFFSET);
            if (plugin_run_result == EXIT_FAILURE) {
                const auto plugin_run_error_message =
                    amd_fmt::format("[CmdLine] -> Error: Plugin run failed! {}{}", "command: 'run'", plugin_run_filter);
                std::cout << plugin_run_error_message << "\n\n";
            }
        }
    });
}


auto execute_command_line_interface(int argc, char** argv) -> i32_t
{
    auto work_bench_cli_app = CLI::App{"AMD ROCm Bandwidth Test: Command Line Interface"};
    work_bench_cli_app.add_option("--builtin-help", "Print help for builtin plugin/subcommands");
    work_bench_cli_app.add_option("--version", "Print the utility/framework version");
    work_bench_cli_app.require_subcommand();

    //  Skip the executable path (1st argument) and build the list of arguments
    WordList_t args = {(argv + 1), (argv + argc)};

    //  Add subcommands
    auto plugin_data_handler = details::PluginHandlerData_t{argc, argv, args};
    add_plugin_subcommand_handler(work_bench_cli_app, plugin_data_handler);


    //  If not enough args, show help info
    if (args.size() == 0) {
        std::cout << work_bench_cli_app.help() << "\n\n";
        return (EXIT_FAILURE);
    } else if (args.size() == 1) {
        auto subcommand_passed = std::string(args[0]);
        if ((subcommand_passed == "-h") || (subcommand_passed == "--help")) {
            std::cout << work_bench_cli_app.help() << "\n\n";
            return (EXIT_FAILURE);
        } else if ((subcommand_passed == "-v") || (subcommand_passed == "--version")) {
            const auto help_message_build_info =
                amd_fmt::format("AMD ROCm Bandwidth Test: \n -> version: {} \n -> [Commit: {} / Branch: {} / Build Type: {}]",
                                wb_api_system::get_work_bench_version(),
                                wb_api_system::get_work_bench_commit_hash(),
                                wb_api_system::get_work_bench_commit_branch(),
                                wb_api_system::get_work_bench_build_type());

            const auto help_message_env_info = amd_fmt::format("Environment: \n -> Kernel: {} \n -> OS: {}",
                                                               wb_api_system::get_os_kernel_info(),
                                                               wb_api_system::get_os_distro_info());
            std::cout << help_message_build_info << "\n";
            std::cout << help_message_env_info << "\n\n";
            return (EXIT_SUCCESS);
        } else if ((subcommand_passed == "-d") || (subcommand_passed == "--debug")) {
            // wb_logger::set_log_level(LogLevel::LOGGER_DEBUG);
            return (EXIT_SUCCESS);
        }

        try {
            std::cout << work_bench_cli_app.get_subcommand(subcommand_passed)->help() << "\n\n";
        } catch (const CLI::OptionNotFound& exc) {
            const auto subcommand_invalid_help_message =
                amd_fmt::format("[CmdLine] -> Error: Invalid subcommand passed. Please see the help below. '{}' \n\n",
                                subcommand_passed);
            std::cout << subcommand_invalid_help_message << "\n\n";
        }

        return (EXIT_FAILURE);
    }

    // Parse the args
    try {
        std::reverse(args.begin(), args.end());
        work_bench_cli_app.parse(args);
    } catch (const CLI::ParseError& exc) {
        // const auto parse_error_message =
        //     amd_fmt::format("[CmdLine] Error: Command line option not found: {} -> {} \n", exc.get_exit_code(),
        //     exc.what());
        // std::cout << parse_error_message << "\n";
        //  return (EXIT_FAILURE);
        return work_bench_cli_app.exit(exc);
    }

    return (EXIT_SUCCESS);
}

}    // namespace cmdline_subcommands


/*
 *  Command line handler
 *  This function is responsible for parsing the command line arguments
 *      @argc: number of arguments
 *      @argv: list of arguments
 */
auto run_command_line(int argc, char** argv) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Startup: {} ", "run_command_line()");

    //  *  TODO:   std::experimental::scope_exit { }
    // Start of function
    /*
    {
        // Start of scope

        // Exiting scope
        ON_SCOPE_EXIT{
            //  TODO:   Action needed
        };

        // First time
        ON_SCOPE_FIRST_TIME{

        };

        // Last time (cleanup)
        ON_SCOPE_LAST_TIME{

        };
        // End of scope
    }
    */

    //  TODO: Remove, for debugging only
    // for (const auto& arg : args) {
    //    wb_logger::loginfo(LogLevel::LOGGER_WARN, "    Startup params: {} ", arg);
    //}

    //  Load all plugins without initializing them
    PluginManagement_t::library_load();
    // kPLUGIN_PATH.read()
    for (const auto& plugin_dir : paths::kPLUGIN_PATH.all()) {
        PluginManagement_t::plugin_load(plugin_dir);
    }

    /*  Lets 1st check the builtin subcommands
     *  Skip the executable path (1st argument) and build the list of arguments
     *  Arg parser for the builtin subcommands
     */
    WordList_t args = {(argv + 1), (argv + argc)};
    auto builtin_subcommand_result = wb_subcommands::process_args(args);
    //  Execute the command line interface parse, if builtin subcommand was not found
    if (builtin_subcommand_result != EXIT_SUCCESS) {
        cmdline_subcommands::execute_command_line_interface(argc, argv);
    }

    //  Unload plugin
    PluginManagement_t::plugin_unload();
}

}    // namespace amd_work_bench::startup
