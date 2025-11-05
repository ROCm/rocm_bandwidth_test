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
 * Description: content_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_CONTENT_MGMT_HPP)
#define AMD_WORK_BENCH_CONTENT_MGMT_HPP

#include <work_bench.hpp>
#include <awb/concepts.hpp>
#include <awb/datavw_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
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

using MenuCategoryCallback_t = std::function<void()>;
using EnabledCategoryCallback_t = std::function<bool()>;
using SelectedOptionCallback_t = std::function<bool()>;
using ExecuteOptionCallback_t = std::function<void()>;


struct MenuMainCategoryItem_t
{
    public:
        std::string s_name{};

    private:
};

struct MenuCategoryItem_t
{
    public:
        std::vector<std::string> s_names{};
        wb_dataview::DataViewBase_t* s_view{};
        MenuCategoryCallback_t m_menu_cb;
        EnabledCategoryCallback_t m_enabled_cb;
        SelectedOptionCallback_t m_selected_cb;

    private:
};


auto get_menu_main_categories() -> const std::multimap<u32_t, MenuMainCategoryItem_t>&;
auto get_menu_categories() -> const std::multimap<u32_t, MenuCategoryItem_t>&;

}    // namespace details

auto register_menu_main_category_item(u32_t order, const std::string& item_name) -> void;

auto add_menu_category_item(
    u32_t order,
    const std::vector<std::string>& item_names,
    wb_dataview::DataViewBase_t* view,
    const details::MenuCategoryCallback_t& menu_cb,
    const details::EnabledCategoryCallback_t& enabled_cb =
        []() {
            return true;
        },
    const details::SelectedOptionCallback_t& selected_cb =
        []() {
            return false;
        }) -> void;


auto add_menu_category_item_submenu(
    u32_t order,
    std::vector<std::string> menu_main_names,
    const details::MenuCategoryCallback_t& menu_cb,
    const details::EnabledCategoryCallback_t& enabled_cb = []() {
        return true;
    }) -> void;


}    // namespace interface


namespace datasource
{

namespace details
{
using DataSourceCreationCallback_t = std::function<std::unique_ptr<wb_datasource::DataSourceBase_t>()>;

auto add_datasource(const std::string& name) -> void;
auto add(const std::string& datasrc_name, const DataSourceCreationCallback_t& datasrc_creating_cb) -> void;
auto get_datasources() -> const std::vector<std::string>&;

}    // namespace details


template<std::derived_from<wb_datasource::DataSourceBase_t> DataSourceTp>
auto add(bool is_add_to_list = true) -> void
{
    auto datasrc_type_name = DataSourceTp().get_type_name();
    details::add(datasrc_type_name, []() -> std::unique_ptr<wb_datasource::DataSourceBase_t> {
        return std::make_unique<DataSourceTp>();
    });

    if (is_add_to_list) {
        details::add_datasource(datasrc_type_name);
    }
}

}    // namespace datasource


namespace board_commands
{

enum class CommandType_t
{
    SYMBOL,
    KEYWORD
};

namespace details
{

struct CmdHandler_t;
struct Entry_t;
struct QueryResult_t;

using DisplayCallback_t = std::function<std::string(std::string)>;
using ExecuteCallback_t = std::function<std::optional<std::string>(std::string)>;
using QueryCallback_t = std::function<std::vector<QueryResult_t>(std::string)>;


struct CmdHandler_t
{
    public:
        CommandType_t m_type;
        std::string m_command{};
        DisplayCallback_t m_display_cb{nullptr};
        QueryCallback_t m_query_cb{nullptr};

    private:
};

struct Entry_t
{
    public:
        CommandType_t m_type;
        std::string m_command{};
        std::string m_description{};
        DisplayCallback_t m_display_cb{nullptr};
        ExecuteCallback_t m_execute_cb{nullptr};

    private:
};

struct QueryResult_t
{
    public:
        std::string m_name{};
        std::function<void(std::string)> m_function_cb{nullptr};

    private:
};


auto get_commands() -> const std::vector<Entry_t>&;
auto get_cmdhandlers() -> const std::vector<CmdHandler_t>&;

}    // namespace details


auto add_cmd(
    CommandType_t cmd_type,
    const std::string& cmd_execute,
    const std::string& cmd_description,
    const details::DisplayCallback_t& display_cb,
    const details::ExecuteCallback_t& execute_cb = [](auto) {
        return std::nullopt;
    }) -> void;

auto add_handler(CommandType_t cmd_type,
                 const std::string& cmd_execute,
                 const details::QueryCallback_t& query_cb,
                 const details::DisplayCallback_t& display_cb) -> void;

}    // namespace board_commands


namespace views
{

namespace details
{

auto add_view(std::unique_ptr<wb_dataview::DataViewBase_t>&& view) -> void;
auto get_views() -> const std::map<std::string, std::unique_ptr<wb_dataview::DataViewBase_t>>&;

}    // namespace details

template<std::derived_from<wb_dataview::DataViewBase_t> ViewTp, typename... Args>
auto add_view(Args&&... args) -> void
{
    return details::add_view(std::make_unique<ViewTp>(std::forward<Args>(args)...));
}

auto get_view_by_name(const std::string& view_name) -> wb_dataview::DataViewBase_t*;

}    // namespace views


namespace tools
{

namespace details
{

using EntryCallback_t = std::function<void()>;

struct ToolEntry_t
{
    public:
        std::string m_name{};
        EntryCallback_t m_entry_cb;

    private:
};

auto get_tools() -> const std::vector<ToolEntry_t>&;

}    // namespace details


auto add_tool(const std::string& name, const details::EntryCallback_t& entry_cb) -> void;

}    // namespace tools


namespace background_services
{
}    // namespace background_services


namespace communication_interface
{
}    // namespace communication_interface


namespace experiments
{
}    // namespace experiments


}    // namespace amd_work_bench::content


// namespace alias
namespace wb_content = amd_work_bench::content;
namespace wb_content_interface = amd_work_bench::content::interface;
namespace wb_content_datasource = amd_work_bench::content::datasource;
namespace wb_content_board_commands = amd_work_bench::content::board_commands;
namespace wb_content_views = amd_work_bench::content::views;
namespace wb_content_tools = amd_work_bench::content::tools;
namespace wb_content_background_services = amd_work_bench::content::background_services;
namespace wb_content_communication_interface = amd_work_bench::content::communication_interface;
namespace wb_content_experiments = amd_work_bench::content::experiments;


#endif    //-- AMD_WORK_BENCH_CONTENT_MGMT_HPP
