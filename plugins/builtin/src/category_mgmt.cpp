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
 * Description: category_mgmt.cpp
 *
 */


// #include "cmdline_iface.hpp"

#include <awb/content_mgmt.hpp>
#include <awb/work_bench_api.hpp>

#include <awb/datavw_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <algorithm>
#include <iostream>
#include <utility>


namespace amd_work_bench::plugin::builtin
{

constexpr auto kVIEW_CATEGORY_ID = 1000;
constexpr auto kEXTRAS_CATEGORY_ID = 2000;
constexpr auto kHELP_CATEGORY_ID = 8000;

namespace
{

auto no_running_tasks() -> bool
{
    return (TaskManagement_t::get_current_background_task_count() == 0);
}


auto no_running_task_and_validate_data_source() -> bool
{
    return (no_running_tasks() && wb_api_datasource::is_valid());
}


auto no_running_task_and_writeable_data_source() -> bool
{
    return (no_running_tasks() && wb_api_datasource::is_valid() && wb_api_datasource::get()->is_writeable());
}


}    // namespace


static auto make_view_category_entry() -> void
{
    wb_content_interface::register_menu_main_category_item(kVIEW_CATEGORY_ID, "View");
    wb_content_interface::add_menu_category_item_submenu((kVIEW_CATEGORY_ID + 200), {"View"}, [] {
        for (const auto& [name, view] : content::views::details::get_views()) {
            if (view->has_view_category_entry()) {
                // if (view->get_name().empty()) {
                // }
            }
        }
    });
}


static auto make_extras_category_entry() -> void
{
    wb_content_interface::register_menu_main_category_item(kEXTRAS_CATEGORY_ID, "Extras");
}


static auto make_help_category_entry() -> void
{
    wb_content_interface::register_menu_main_category_item(kHELP_CATEGORY_ID, "Help");
}

auto register_main_category_entries() -> void
{
    make_view_category_entry();
    make_extras_category_entry();
}


}    //  namespace amd_work_bench::plugin::builtin
