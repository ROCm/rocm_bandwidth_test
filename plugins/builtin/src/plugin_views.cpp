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
 * Description: plugin_views.cpp
 *
 */


#include "plugin_views.hpp"

#include <awb/work_bench_api.hpp>
#include <awb/common_utils.hpp>
#include <awb/content_mgmt.hpp>
#include <awb/datavw_mgmt.hpp>
#include <awb/event_mgmt.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <algorithm>
// #include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>


namespace amd_work_bench::plugin::builtin
{

auto register_views() -> void
{
    wb_content::views::add_view<ViewTools_t>();
    wb_content::views::add_view<ViewCommandBoard_t>();

    //  TODO: Check if load/store callbacks will be needed.
    // register_load_cb();
    // register_store_cb();
}

auto register_board_commands() -> void
{
    wb_content::board_commands::add_cmd(
        wb_content::board_commands::CommandType_t::KEYWORD,
        "help",
        "Show help",
        [](auto input) {
            return (amd_fmt::format("Help: {}", input.data()));
        },
        [](auto input) {
            wb_utils::open_url(input);
            // wb_utils::run_command(input);
            return std::nullopt;
        });

    /*
    wb_content::board_commands::add_handler(
        wb_content::board_commands::CommandType_t::KEYWORD,
        ">",
        [](const auto& input) {
            auto results = std::vector<wb_content_board_commands::details::QueryResult_t>{};
            return results;
        },
        [](const auto& input) {
            return std_format::format("{}", input);
        });
    */
}

auto register_data_sources() -> void
{
    const auto is_add_it_to_list{false};
    wb_content::datasource::add<ViewDataSources_t>(is_add_it_to_list);
    // wb_content::datasource::add<ProcessMemoryDataSrc_t>();
}


ViewTools_t::ViewTools_t() : wb_dataview::DataViewBase_t::Window_t("Tools")
{
    m_tool_entry_itr = wb_content::tools::details::get_tools().end();

    // register_load_cb;
    // register_store_cb;
}


auto ViewTools_t::sketch_content() -> void
{
    /*
    auto tool_entries = wb_content::tools::details::get_tools();
    for (auto tool_itr = tool_entries.begin(); tool_itr != tool_entries.end(); ++tool_itr) {
        auto& [name, entry_cb] = *tool_itr;

        if (m_tool_entry_itr == tool_itr) {
            if (entry_cb) {
                entry_cb();
            }
        }
    }
    */
}


auto ViewTools_t::sketch_visible_content_always() -> void
{
    if (!wb_api_datasource::is_valid()) {
        return;
    }

    auto tool_entries = wb_content::tools::details::get_tools();
    for (auto tool_itr = tool_entries.begin(); tool_itr != tool_entries.end(); ++tool_itr) {
        auto& [name, entry_cb] = *tool_itr;

        if (m_tool_entry_itr == tool_itr) {
            if (entry_cb) {
                entry_cb();
            }
        }
    }
}


ViewCommandBoard_t::ViewCommandBoard_t() : wb_dataview::DataViewBase_t::Special_t("CommandBoard")
{
    m_is_command_board_open = true;
}


auto ViewCommandBoard_t::sketch_visible_content_always() -> void
{
    if (!m_is_command_board_open) {
        return;
    }

    m_command_results = get_command_results(m_command_buffer);
    if (m_command_results.empty()) {
        auto& [result, command, execute_cb] = m_command_results.front();
        if (auto cmd_result = execute_cb(command); cmd_result.has_value()) {
            m_command_buffer = cmd_result.value();
        }

        for (const auto& [result, command, execute_cb] : m_command_results) {
            if (auto cmd_result = execute_cb(command); cmd_result.has_value()) {
                m_command_buffer = cmd_result.value();
            }
            break;
        }
    }
}


auto ViewCommandBoard_t::get_command_results(const std::string& input_command) -> std::vector<CommandResult_t>
{
    using CommandMatchingPairType_t = std::pair<CommandMatchingType_t, std::string_view>;

    static constexpr auto kMATCHING_COMMAND = [](const std::string& actual_command,
                                                 const std::string& what_to_match) -> CommandMatchingPairType_t {
        if (actual_command.empty()) {
            return {CommandMatchingType_t::kINFO_MATCH, ""};
        } else if (actual_command.size() <= what_to_match.size()) {
            if (what_to_match.starts_with(actual_command)) {
                return {CommandMatchingType_t::kPARTIAL_MATCH, what_to_match};
            } else {
                return {CommandMatchingType_t::kNO_MATCH, {}};
            }
        } else {
            if (actual_command.starts_with(what_to_match)) {
                return {CommandMatchingType_t::kFULL_MATCH, what_to_match.substr(actual_command.size())};
            } else {
                return {CommandMatchingType_t::kNO_MATCH, {}};
            }
        }
    };

    //  Search for all registered commands and matching input
    std::vector<CommandResult_t> command_results{};
    for (const auto& [type, command, description, display_cb, execute_cb] : wb_content::board_commands::details::get_commands()) {
        auto auto_complete = [this, actual_command = command](auto) {
            m_command_buffer = (actual_command + " ");
            m_command_results = get_command_results(actual_command);
            return std::nullopt;
        };

        //  Symbol commands
        if (type == wb_content::board_commands::CommandType_t::SYMBOL) {
            if (auto [match_type, match_value] = kMATCHING_COMMAND(input_command, command);
                match_type != CommandMatchingType_t::kNO_MATCH) {
                command_results.push_back({amd_fmt::format("{} ({})", command, description), "", auto_complete});
            } else {
                auto matched_command = wb_utils::strings::trim_all_copy(input_command.substr(command.size()));
                command_results.push_back({display_cb(matched_command), matched_command, execute_cb});
            }

        }
        //  Keyword command
        else if (type == wb_content::board_commands::CommandType_t::KEYWORD) {
            if (auto [match_type, match_value] = kMATCHING_COMMAND(input_command, (command + " "));
                match_type != CommandMatchingType_t::kNO_MATCH) {
                if (match_type != CommandMatchingType_t::kFULL_MATCH) {
                    command_results.push_back({amd_fmt::format("{} ({})", command, description), "", auto_complete});
                } else {
                    auto matched_command = wb_utils::strings::trim_all_copy(input_command.substr(command.size() + 1));
                    command_results.push_back({display_cb(matched_command), matched_command, execute_cb});
                }
            }
        }
    }

    // If a command was found, show the full query result for it
    for (const auto& command_handler : wb_content::board_commands::details::get_cmdhandlers()) {
        const auto& [type, command, display_cb, query_cb] = command_handler;
        auto checked_input = input_command;
        if (checked_input.starts_with(command)) {
            checked_input = wb_utils::strings::trim_all_copy(checked_input.substr(command.size()));
        }

        for (const auto& [name, function_cb] : query_cb(checked_input)) {
            if (type == wb_content::board_commands::CommandType_t::SYMBOL) {
                if (auto [match_type, match_value] = kMATCHING_COMMAND(input_command, command);
                    match_type != CommandMatchingType_t::kNO_MATCH) {
                    command_results.push_back({amd_fmt::format("{} ({})", command, name), "", [function_cb](auto... args) {
                                                   function_cb(args...);
                                                   return std::nullopt;
                                               }});
                } else if (type == wb_content::board_commands::CommandType_t::KEYWORD) {
                    if (auto [match_type, match_value] = kMATCHING_COMMAND(input_command, (command + " "));
                        match_type != CommandMatchingType_t::kNO_MATCH) {
                        command_results.push_back({amd_fmt::format("{} ({})", command, name), "", [function_cb](auto... args) {
                                                       function_cb(args...);
                                                       return std::nullopt;
                                                   }});
                    }
                }
            }
        }
    }

    return command_results;
}


auto ViewDataSources_t::open() -> bool
{
    if (m_data_source == this) {
        return false;
    }

    EventDataSourceClosing::subscribe(this, [this](const wb_datasource::DataSourceBase_t* data_source, bool*) {
        if (m_data_source == data_source) {
            wb_api_datasource::remove(this);
        }
    });

    return true;
}


auto ViewDataSources_t::close() -> void
{
    EventDataSourceClosing::unsubscribe(this);
}


auto ViewDataSources_t::is_available() const -> bool
{
    if (m_data_source == nullptr) {
        return false;
    }

    return m_data_source->is_available();
}


auto ViewDataSources_t::is_readable() const -> bool
{
    if (m_data_source == nullptr) {
        return false;
    }

    return m_data_source->is_readable();
}


auto ViewDataSources_t::is_writeable() const -> bool
{
    if (m_data_source == nullptr) {
        return false;
    }

    return m_data_source->is_writeable();
}


auto ViewDataSources_t::is_resizable() const -> bool
{
    return true;
}


auto ViewDataSources_t::is_saveable() const -> bool
{
    if (m_data_source == nullptr) {
        return false;
    }

    return m_data_source->is_saveable();
}


auto ViewDataSources_t::save() -> void
{
    return m_data_source->save();
}


auto ViewDataSources_t::read_raw(void* buffer, u64_t offset, u64_t size) -> void
{
    if (m_data_source == nullptr) {
        return;
    }

    m_data_source->read(buffer, (offset + m_start_address), size);
}


auto ViewDataSources_t::write_raw(const void* buffer, u64_t offset, u64_t size) -> void
{
    if (m_data_source == nullptr) {
        return;
    }

    m_data_source->write(buffer, (offset + m_start_address), size);
}


auto ViewDataSources_t::insert_raw(u64_t offset, u64_t size) -> void
{
    if (m_data_source == nullptr) {
        return;
    }

    m_size += size;
    m_data_source->insert((offset + m_start_address), size);
}


auto ViewDataSources_t::remove_raw(u64_t offset, u64_t size) -> void
{
    if (m_data_source == nullptr) {
        return;
    }

    m_size -= size;
    m_data_source->remove((offset + m_start_address), size);
}


auto ViewDataSources_t::resize_raw(u64_t size) -> void
{
    m_size = size;
}


auto ViewDataSources_t::get_actual_size() const -> u64_t
{
    return m_size;
}


auto ViewDataSources_t::get_type_name() const -> std::string
{
    return "DataSourceView";
}


auto ViewDataSources_t::get_name() const -> std::string
{
    if (!m_name.empty()) {
        return m_name;
    }
    if (m_data_source == nullptr) {
        return "DataSourceView";
    }

    return (amd_fmt::format("{} View", m_data_source->get_name()));
}


auto ViewDataSources_t::get_source_description() const -> std::vector<SourceDescription_t>
{
    if (m_data_source == nullptr) {
        return {};
    }

    return m_data_source->get_source_description();
}


auto ViewDataSources_t::set_data_source(wb_datasource::DataSourceBase_t* data_source, u64_t address, size_t size) -> void
{
    m_data_source = data_source;
    m_start_address = address;
    m_size = size;
}


auto ViewDataSources_t::set_name(const std::string& name) -> void
{
    m_name = name;
}


auto ViewDataSources_t::get_option_entries() -> std::vector<OptionEntry_t>
{
    return {
        OptionEntry_t{"DataSourceView.Rename", [this]() {
                          rename_file("testing.log");
                      }}
    };
}


auto ViewDataSources_t::rename_file(const std::string& new_name) -> void
{
    if (new_name.empty()) {
        return;
    }

    m_name = new_name;
    RequestUpdateData::post();
}


}    // namespace amd_work_bench::plugin::builtin
