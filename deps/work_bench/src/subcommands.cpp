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
 *
 * Description: subcommands.cpp
 *
 */

#include <work_bench.hpp>
#include <awb/subcommands.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/work_bench_api.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

// #include <format>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace amd_work_bench::subcommands
{

using SubCommandOptions_t = std::pair<amd_work_bench::SubCommand_t, WordList_t>;


auto find_subcommand(const std::string& cmd_name) -> std::optional<amd_work_bench::SubCommand_t>
{
    for (const auto& plugin : amd_work_bench::PluginManagement_t::plugin_get_all()) {
        for (const auto& subcommand : plugin.plugin_get_subcommand()) {
            if ((amd_fmt::format("--{}", subcommand.m_long_format) == cmd_name) ||
                (amd_fmt::format("-{}", subcommand.m_short_format) == cmd_name)) {
                return subcommand;
            }
        }
    }

    return std::nullopt;
}


auto forward_subcommand(const std::string& cmd_name, const WordList_t& args) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Subcommand forward: {}", cmd_name);
    DataStream_t data_stream{};
    if (!args.empty()) {
        for (const auto& arg : args) {
            data_stream.insert(data_stream.end(), arg.begin(), arg.end());
            data_stream.push_back('\0');
        }
        data_stream.pop_back();
    }

    MessageSendToMainInstance::post(amd_fmt::format("command/{}", cmd_name), data_stream);
}


auto register_subcommand(const std::string& cmd_name, const ForwardCommandHandler_t& cmd_handler) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Subcommand register: {} ", cmd_name);
    wb_api_messaging::register_message_handler(amd_fmt::format("command/{}", cmd_name),
                                               [cmd_handler](const DataStream_t& message_data) {
                                                   std::string text_data(reinterpret_cast<const char*>(message_data.data()),
                                                                         message_data.size());
                                                   WordList_t args{};
                                                   auto args_list = wb_strings::split_str(text_data, char(0x00));
                                                   for (const auto& arg : args_list) {
                                                       args.push_back(arg);
                                                   }
                                                   cmd_handler(args);
                                               });

    /*
    //  Note:   Clang++ (v19) doesn't like this, g++ (v13+) works well.
       for (const auto& arg : std::views::split(text_data, char(0x00))) {
           std::string arg_str(arg.data(), arg.size());
           args.push_back(arg_str);
       }
       cmd_handler(args);
   });
   */
}


/*
 *  Process the arguments and run the subcommands, for the subcommands that registered with 'global_subcommands',
 *  by using plugin management and AMD_WORK_BENCH_PLUGIN_SUBCOMMAND().
 */
auto process_args(const WordList_t& args) -> int32_t
{
    /*
     * If there are no arguments, there is nothing to do.
     */
    if (args.empty()) {
        return (EXIT_FAILURE);
    }

    auto args_itr = args.begin();
    auto subcommands = std::vector<SubCommandOptions_t>{};
    auto current_subcommand = find_subcommand(*args_itr);

    /*
     *  Note:   If the first argument is not a subcommand, we will assume that the user wants to see the help message.
     *          Otherwise, we will move to the next argument.
     */
    if (current_subcommand.has_value()) {
        args_itr++;
    } else {
        current_subcommand = find_subcommand("--help");
    }

    /*
     * Note:   Check possible subcommands available to run.
     */
    WordList_t current_subcommand_args{};
    for (; args_itr != args.end(); ++args_itr) {
        const std::string& arg_str = *args_itr;
        if (!current_subcommand_args.empty() && arg_str.starts_with("--") &&
            !(current_subcommand.has_value() &&
              current_subcommand->m_subcmd_type == SubCommand_t::SubCommandType_t::SubCommand)) {
            // Command to run is saved in current_subcommand
            if (current_subcommand.has_value()) {
                subcommands.emplace_back(current_subcommand.value(), current_subcommand_args);
                //*current_subcommand
            }
            current_subcommand = std::nullopt;
            current_subcommand_args = {};
        } else if (current_subcommand.has_value()) {
            // Save the arguments for the current subcommand
            current_subcommand_args.push_back(arg_str);
            ++args_itr;
        } else {
            // Get next subcommand from here
            current_subcommand = find_subcommand(arg_str);
            if (!current_subcommand.has_value()) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Subcommand: {} unknown.", arg_str);
                return (EXIT_FAILURE);
            }
            ++args_itr;
        }
    }

    /*
     *  Save last and run commands.
     */
    if (current_subcommand.has_value()) {
        subcommands.emplace_back(current_subcommand.value(), current_subcommand_args);
        //*current_subcommand
    }
    for (const auto& [subcommand, subcommand_args] : subcommands) {
        subcommand.m_function_cb(subcommand_args);
    }

    /*
     * If not main instance, commands were forwarded to a different instance.
     */
    if (!wb_api_system::is_main_instance()) {
        return (EXIT_SUCCESS);
    }

    return (EXIT_SUCCESS);
}


}    // namespace amd_work_bench::subcommands
