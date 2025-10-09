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
 * Description: content_mgmt.cpp
 *
 */


#include <awb/content_mgmt.hpp>

#include <work_bench.hpp>
#include <awb/concepts.hpp>
#include <awb/common_utils.hpp>
#include <awb/datavw_mgmt.hpp>
#include <awb/default_sets.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <functional>
#include <map>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace amd_work_bench::content
{

namespace interface
{

namespace details
{

static auto s_menu_main_category_item_list = wb_memory::AutoReset_t<std::multimap<u32_t, MenuMainCategoryItem_t>>{};
static auto s_menu_category_item_list = wb_memory::AutoReset_t<std::multimap<u32_t, MenuCategoryItem_t>>{};

auto get_menu_main_categories() -> const std::multimap<u32_t, MenuMainCategoryItem_t>&
{
    return *s_menu_main_category_item_list;
}


auto get_menu_categories() -> const std::multimap<u32_t, MenuCategoryItem_t>&
{
    return *s_menu_category_item_list;
}

}    // namespace details


auto register_menu_main_category_item(u32_t order, const std::string& item_name) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Content: New Main Menu Item: {} ", item_name);
    details::s_menu_main_category_item_list->insert({order, {item_name}});
}


auto add_menu_category_item(u32_t order,
                            const std::vector<std::string>& item_names,
                            wb_dataview::DataViewBase_t* view,
                            const details::MenuCategoryCallback_t& menu_cb,
                            const details::EnabledCategoryCallback_t& enabled_cb,
                            const details::SelectedOptionCallback_t& selected_cb) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Content: New Menu Item to menu {} and order {} ", item_names[0], order);
    details::s_menu_category_item_list->insert({
        order,
        details::MenuCategoryItem_t{item_names, view, menu_cb, enabled_cb, selected_cb}
    });
}


auto add_menu_category_item_submenu(u32_t order,
                                    const std::vector<std::string> menu_main_names,
                                    const details::MenuCategoryCallback_t& menu_cb,
                                    const details::EnabledCategoryCallback_t& enabled_cb) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Content: New Submenu Item to menu {} and order {} ", menu_main_names[0], order);
    details::s_menu_category_item_list->insert({
        order,
        details::MenuCategoryItem_t{menu_main_names, nullptr, menu_cb, enabled_cb, []() {
                                        return false;
                                    }}
    });
}


}    // namespace interface


namespace datasource
{

namespace details
{

static auto s_datasource_list = wb_memory::AutoReset_t<std::vector<std::string>>{};

auto add(const std::string& datasrc_name, const DataSourceCreationCallback_t& datasrc_creating_cb) -> void
{
    RequestCreateDataSource::subscribe(
        [datasrc = datasrc_name, datasrc_creating_cb](const std::string& name,
                                                      bool is_skip_load_interface,
                                                      bool is_select_datasrc,
                                                      wb_datasource::DataSourceBase_t** datasrc_ptr) {
            if (name == datasrc) {
                auto new_datasrc = datasrc_creating_cb();
                if (datasrc_ptr != nullptr) {
                    *datasrc_ptr = new_datasrc.get();
                    wb_api_datasource::add_datasource(std::move(new_datasrc), is_skip_load_interface, is_select_datasrc);
                }
            }

            return;
        });
}

auto get_datasources() -> const std::vector<std::string>&
{
    return *s_datasource_list;
}


auto add_datasource(const std::string& datasrc_name) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Content: New Datasource: {}", datasrc_name);
    s_datasource_list->push_back(datasrc_name);
}


}    // namespace details


}    // namespace datasource


namespace board_commands
{

namespace details
{
static auto s_board_command_list = wb_memory::AutoReset_t<std::vector<Entry_t>>{};
static auto s_board_handler_list = wb_memory::AutoReset_t<std::vector<CmdHandler_t>>{};

auto get_commands() -> const std::vector<Entry_t>&
{
    return *s_board_command_list;
}

auto get_cmdhandlers() -> const std::vector<CmdHandler_t>&
{
    return *s_board_handler_list;
}

}    // namespace details


auto add_cmd(CommandType_t cmd_type,
             const std::string& cmd_execute,
             const std::string& cmd_description,
             const details::DisplayCallback_t& display_cb,
             const details::ExecuteCallback_t& execute_cb) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Content: Command Board: {}", cmd_execute);
    details::s_board_command_list->push_back({cmd_type, cmd_execute, cmd_description, display_cb, execute_cb});
}


auto add_handler(CommandType_t cmd_type,
                 const std::string& cmd_execute,
                 const details::QueryCallback_t& query_cb,
                 const details::DisplayCallback_t& display_cb) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Content: Command Handler: {}", cmd_execute);
    details::s_board_handler_list->push_back({cmd_type, cmd_execute, display_cb, query_cb});
}

}    // namespace board_commands


namespace views
{

namespace details
{

static auto s_view_item_list = wb_memory::AutoReset_t<std::map<std::string, std::unique_ptr<wb_dataview::DataViewBase_t>>>{};

auto get_views() -> const std::map<std::string, std::unique_ptr<wb_dataview::DataViewBase_t>>&
{
    return *s_view_item_list;
}

auto add_view(std::unique_ptr<wb_dataview::DataViewBase_t>&& view) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Content: New View: {}", view->get_name());
    s_view_item_list->insert({view->get_name(), std::move(view)});
}

}    // namespace details


auto get_view_by_name(const std::string& view_name) -> wb_dataview::DataViewBase_t*
{
    auto& view_list = details::get_views();
    if (view_list.contains(view_name)) {
        return view_list.at(view_name).get();
    }

    return nullptr;
}


}    // namespace views


namespace tools
{

namespace details
{

static auto s_tools_item_list = wb_memory::AutoReset_t<std::vector<ToolEntry_t>>{};

auto get_tools() -> const std::vector<ToolEntry_t>&
{
    return *s_tools_item_list;
}

}    // namespace details


auto add_tool(const std::string& name, const details::EntryCallback_t& entry_cb) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Content: New Tool: {}", name);
    details::s_tools_item_list->push_back({name, entry_cb});
}


}    // namespace tools


}    // namespace amd_work_bench::content
