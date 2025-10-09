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
 * Description: default_sets.cpp
 *
 */


#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <awb/common_utils.hpp>
#include <awb/linux_utils.hpp>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <set>
#include <vector>


namespace amd_work_bench::paths
{

/*
 *  TODO: These need to be rechecked.
 */
auto get_config_paths() -> FSPathList_t
{
    return {{amd_work_bench::xdg::current_work_directory()}, {amd_work_bench::xdg::home_directory() / KDEFAULT_INFO_BASE_PATH}};
}


auto get_data_paths() -> FSPathList_t
{
    return {{amd_work_bench::xdg::current_work_directory()}, {amd_work_bench::xdg::home_directory() / KDEFAULT_INFO_BASE_PATH}};
}


auto get_plugin_paths() -> FSPathList_t
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN,
                       "Plugin Search Paths: {} '{}' ",
                       __PRETTY_FUNCTION__,
                       kDEFAULT_PLUGIN_DIRECTORY_NAME);

    auto path_list = FSPathList_t{};
    auto path_unique_list = FSPathSet_t{};
    // clang-format off
    #if defined(SYSTEM_DEFAULT_PLUGIN_INSTALL_PATH)
        auto default_plugin_path = FSPath_t(std::string(SYSTEM_DEFAULT_PLUGIN_INSTALL_PATH));
        default_plugin_path.make_preferred();
        path_unique_list.emplace(default_plugin_path);
        wb_logger::loginfo(LogLevel::LOGGER_WARN, "  -> SYSTEM_DEFAULT_PLUGIN_INSTALL_PATH: {} ", default_plugin_path.string());
    #endif
    #if defined(SYSTEM_PLUGIN_BUILTIN_LOOKUP_PATH_ALL)
        auto system_plugin_paths = std::string(SYSTEM_PLUGIN_BUILTIN_LOOKUP_PATH_ALL);
        if (!system_plugin_paths.empty()) {
            auto plugin_paths = wb_strings::split_str(system_plugin_paths, ':');
            for (const auto& path : plugin_paths) {
                auto plugin_path = FSPath_t(path);
                plugin_path.make_preferred();
                path_unique_list.emplace(plugin_path);
                wb_logger::loginfo(LogLevel::LOGGER_WARN, "  -> SYSTEM_PLUGIN_BUILTIN_LOOKUP_PATH_ALL: {} ", plugin_path.string());
            }
        }
    #endif
    // clang-format on

    // Add the plugin path from the environment variable
    if (auto plugin_library_path = wb_utils::get_env_var(wb_linux::kDEFAULT_VAR_LD_PLUGIN_LIB_PATH);
        plugin_library_path.has_value()) {
        auto plugin_paths = std::string(plugin_library_path.value());
        if (!plugin_paths.empty()) {
            auto env_plugin_paths = wb_strings::split_str(plugin_paths, ':');
            for (const auto& path : env_plugin_paths) {
                auto plugin_path = FSPath_t(path);
                plugin_path.make_preferred();
                path_unique_list.emplace(plugin_path);
                wb_logger::loginfo(LogLevel::LOGGER_WARN, "  -> kDEFAULT_VAR_LD_PLUGIN_LIB_PATH: {} ", plugin_path.string());
            }
        }
    }
    path_unique_list.emplace(amd_work_bench::xdg::current_work_directory());
    path_unique_list.emplace(amd_work_bench::xdg::home_directory() / KDEFAULT_INFO_BASE_PATH);

    // Remove duplicates
    path_list.reserve(path_unique_list.size());
    std::copy(path_unique_list.begin(), path_unique_list.end(), std::back_inserter(path_list));

    return path_list;
}


static auto append_path(FSPathList_t paths, FSPath_t path) -> FSPathList_t
{
    path.make_preferred();
    for (auto& current_path : paths) {
        // Check if the path is already in the list
        if (current_path.filename() != path.string()) {
            current_path = (current_path / path);
        }
    }

    return paths;
}


namespace details
{

auto ConfigPath_t::all() const -> FSPathList_t
{
    return append_path(get_config_paths(), m_config_path);
}

auto DefaultPath_t::read() const -> FSPathList_t
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Path: {} ", __PRETTY_FUNCTION__);
    auto paths = all();

    //  TODO:   This is temporary for debugging only
    for (const auto& path : paths) {
        wb_logger::loginfo(LogLevel::LOGGER_WARN, "    Paths: {} ", path.string());
    }

    std::erase_if(paths, [](const auto& entry) {
        return (!fs::is_directory(entry));
    });

    return paths;
}


auto DefaultPath_t::write() const -> FSPathList_t
{
    auto paths = read();
    std::erase_if(paths, [](const auto& entry) {
        return (!fs::is_path_writeable(entry));
    });

    return paths;
}


auto DataPath_t::all() const -> FSPathList_t
{
    return append_path(get_data_paths(), m_data_path);
}


auto DataPath_t::write() const -> FSPathList_t
{
    auto paths = append_path(get_data_paths(), m_data_path);
    std::erase_if(paths, [](const auto& entry) {
        return (!fs::is_path_writeable(entry));
    });

    return paths;
}


auto PluginPath_t::all() const -> FSPathList_t
{
    /*
     *  Note:   When get_plugins_paths() is called, it checks for a few definitions,
     *          such as SYSTEM_DEFAULT_PLUGIN_INSTALL_PATH and SYSTEM_PLUGIN_BUILTIN_LOOKUP_PATH_ALL
     *          along with kDEFAULT_VAR_LD_PLUGIN_LIB_PATH environment variable.
     *          The list of paths is parsed, and then filtered for uniqueness.
     *          Then, after we check every path, and add 'm_plugin_path' directory suffix to the path
     *          if needed.
     */
    // return (append_path(get_plugin_paths(), m_plugin_path));
    return (get_plugin_paths());
}

}    // namespace details


}    // namespace amd_work_bench::paths
