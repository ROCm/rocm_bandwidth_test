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
 * Description: work_bench_api.cpp
 *
 */


#include <awb/work_bench_api.hpp>
#include <awb/common_utils.hpp>
#include <awb/datasrc_mgmt.hpp>
#include <awb/datasrc_data.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <algorithm>
#include <memory>
#include <mutex>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>


namespace amd_work_bench::api::messaging
{

namespace details
{

static wb_memory::AutoReset_t<std::map<std::string, MessageHandler_t>> s_event_handlers_map{};
auto get_message_handlers() -> const std::map<std::string, MessageHandler_t>&
{
    return *s_event_handlers_map;
}


auto run_message_handler(const std::string& message_name, const DataStream_t& args) -> void
{
    const auto& handler_map = get_message_handlers();
    if (const auto handler_it = handler_map.find(message_name); handler_it != handler_map.end()) {
        handler_it->second(args);
    } else {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Messaging: forward message handler: {} was not found.", message_name);
    }
}

}    // namespace details


auto register_message_handler(const std::string& message_name, const details::MessageHandler_t& handler) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Messaging: New forward message handler: {} ", message_name);
    details::s_event_handlers_map->insert({message_name, handler});
}

}    // namespace amd_work_bench::api::messaging


namespace amd_work_bench::api::system
{

namespace details
{

static const auto s_is_confirmation_required(false);

static auto s_is_main_instance(false);
auto set_main_instance(bool is_status) -> void
{
    s_is_main_instance = is_status;
}

static wb_memory::AutoReset_t<std::map<std::string, std::string>> s_startup_args{};
static std::vector<amd_work_bench::utils::memory::details::AutoResetBase_t*> s_auto_reset_objects_list{};
auto add_auto_reset_object(amd_work_bench::utils::memory::details::AutoResetBase_t* object) -> void
{
    s_auto_reset_objects_list.push_back(object);
}


auto auto_reset_objects_cleanup() -> void
{
    for (const auto& object : s_auto_reset_objects_list) {
        object->reset();
    }
}


auto add_startup_arg(const std::string& arg_name, const std::string& arg_value) -> void
{
    static std::mutex startup_args_mutex{};
    std::scoped_lock lock(startup_args_mutex);
    //(*s_startup_args)[arg_name] = arg_value;
    details::s_startup_args->insert_or_assign(arg_name, arg_value);
}

}    // namespace details


auto is_main_instance() -> bool
{
    return details::s_is_main_instance;
}


auto start_work_bench() -> void
{
    RequestAWBOpen::post(details::s_is_confirmation_required);
}


auto stop_work_bench() -> void
{
    RequestAWBClose::post(details::s_is_confirmation_required);
}


auto restart_work_bench() -> void
{
    RequestAWBRestart::post();
    RequestAWBClose::post(details::s_is_confirmation_required);
}


auto set_taskbar_progress(TaskProgressState_t state, TaskProgress_t progress, u32_t percentage) -> void
{
    EventSetTaskbarProgress::post(static_cast<u32_t>(state), static_cast<u32_t>(progress), percentage);
}


auto get_os_kernel_info() -> std::string
{
    return wb_linux::get_kernel_version();
}


auto get_os_distro_info() -> std::string
{
    return wb_linux::get_distro_version();
}


auto get_cpu_info() -> std::string
{
    return std::string(__PRETTY_FUNCTION__);
}


auto get_gpu_info() -> std::string
{
    return std::string(__PRETTY_FUNCTION__);
}


auto get_work_bench_version() -> std::string
{
    auto version = std::string(wb_literals::kTEXT_UNKNOWN);
    // clang-format off
    #if defined(AMD_WORK_BENCH_VERSION)
        version = AMD_WORK_BENCH_VERSION;
    #endif
    // clang-format on

    return version;
}


auto get_work_bench_commit_hash(bool is_long_version) -> std::string
{
    auto commit_hash = std::string(wb_literals::kTEXT_UNKNOWN);
    // clang-format off
    #if defined(GIT_COMMIT_HASH_LONG)
        commit_hash = GIT_COMMIT_HASH_LONG;
        // Get the initial 8 characters of the commit hash
        if (!is_long_version) {
            commit_hash = commit_hash.substr(0, 7);
        }
    #endif
    // clang-format on

    return commit_hash;
}


auto get_work_bench_commit_branch() -> std::string
{
    auto commit_branch = std::string(wb_literals::kTEXT_UNKNOWN);
    // clang-format off
    #if defined(GIT_BRANCH)
        commit_branch = GIT_BRANCH;
    #endif
    // clang-format on

    return commit_branch;
}


auto get_work_bench_is_engineering_build() -> bool
{
    auto is_engineering_build{false};
    // clang-format off
    #if defined(DEBUG)
        is_engineering_build = true;
    #endif
    // clang-format on

    return is_engineering_build;
}


auto get_work_bench_is_production_build() -> bool
{
    auto is_production_build{false};
    // clang-format off
    #if defined(NDEBUG)
        is_production_build = true;
    #endif
    // clang-format on

    return is_production_build;
}


auto get_work_bench_build_type() -> std::string
{
    const auto kRELEASE_BUILD = std::string("Release");
    const auto kENGINEERING_BUILD = std::string("Engineering");
    auto build_type = get_work_bench_is_production_build() ? kRELEASE_BUILD : kENGINEERING_BUILD;
    return build_type;
}


auto add_startup_task(const std::string& task_name, const std::function<bool()>& task, bool is_async) -> void
{
    RequestAddStartupTask::post(task_name, task, is_async);
}


auto add_shutdown_task(const std::string& task_name, const std::function<bool()>& task) -> void
{
    RequestAddExitingTask::post(task_name, task);
}


auto get_lib_work_bench_module_handle() -> void*
{
    return wb_utils::get_containing_module_handle(reinterpret_cast<void*>(&get_lib_work_bench_module_handle));
}


auto get_startup_args() -> const std::map<std::string, std::string>&
{
    return *details::s_startup_args;
}


auto get_startup_arg(const std::string& arg_name) -> std::string
{
    if (details::s_startup_args->contains(arg_name)) {
        return details::s_startup_args->at(arg_name);
    }

    return wb_literals::kEMPTY_STRING;
}

}    // namespace amd_work_bench::api::system


namespace amd_work_bench::api::datasource
{

static auto s_current_data_source = i64_t(-1);
static auto s_data_source_list = wb_memory::AutoReset_t<std::vector<std::unique_ptr<wb_datasource::DataSourceBase_t>>>{};
static auto s_data_source_remove_list =
    wb_memory::AutoReset_t<std::map<wb_datasource::DataSourceBase_t*, std::unique_ptr<wb_datasource::DataSourceBase_t>>>{};


namespace details
{
static std::recursive_mutex s_data_source_mutex{};
static auto s_closing_data_source_list = std::set<wb_datasource::DataSourceBase_t*>{};    // NOLINT

auto reset_closing_datasource() -> void
{
    s_closing_data_source_list.clear();
}


auto get_closing_datasources() -> std::set<wb_datasource::DataSourceBase_t*>
{
    return s_closing_data_source_list;
}

}    // namespace details


auto is_valid() -> bool
{
    return (!s_data_source_list->empty() &&
            ((s_current_data_source >= 0) && (s_current_data_source < static_cast<i64_t>(s_data_source_list->size()))));
}


auto get() -> wb_datasource::DataSourceBase_t*
{
    if (is_valid()) {
        // return (*s_data_source_list)[s_current_data_source].get();
        return s_data_source_list->at(s_current_data_source).get();
    }

    return nullptr;
}


auto get_datasources() -> std::vector<wb_datasource::DataSourceBase_t*>
{
    std::vector<wb_datasource::DataSourceBase_t*> data_sources;
    data_sources.reserve(s_data_source_list->size());

    for (const auto& data_source : *s_data_source_list) {
        data_sources.push_back(data_source.get());
    }

    return data_sources;
}


auto set_current_datasource(wb_types::NonNullPtr_t<wb_datasource::DataSourceBase_t*> datasource) -> void
{
    std::scoped_lock lock(details::s_data_source_mutex);
    if (TaskManagement_t::get_current_foreground_task_count() > 0) {
        return;
    }

    const auto data_sources_list = get_datasources();
    auto data_source_itr = std::ranges::find(data_sources_list, datasource.get());
    auto data_source_idx = std::distance(data_sources_list.begin(), data_source_itr);
    set_current_provider_idx(data_source_idx);
}


auto set_current_provider_idx(i64_t index) -> void
{
    std::scoped_lock lock(details::s_data_source_mutex);
    if (TaskManagement_t::get_current_foreground_task_count() > 0) {
        return;
    }

    if (std::cmp_less(index, s_data_source_list->size()) && (s_current_data_source != index)) {
        auto last_data_source = get();
        s_current_data_source = index;
        EventDataSourceChanged::post(last_data_source, get());
    }
}


auto get_current_provider_idx() -> i64_t
{
    return s_current_data_source;
}


auto stamp_it_used() -> void
{
    const auto data_source = get();
    if (data_source->is_used()) {
        return;
    }

    data_source->stamp_it_used();
    EventDataSourceStampedInUse::post(data_source);
}


auto reset_it_used() -> void
{
    const auto is_used{false};
    for (const auto& data_source : *s_data_source_list) {
        data_source->stamp_it_used(is_used);
    }
}


auto is_it_used() -> bool
{
    return std::ranges::any_of(*s_data_source_list, [](const auto& data_source) {
        return data_source->is_used();
    });
}


auto add_datasource(std::unique_ptr<wb_datasource::DataSourceBase_t>&& data_source,
                    bool is_skip_load_interface,
                    bool is_select_datasrc) -> void
{
    std::scoped_lock lock(details::s_data_source_mutex);
    if (TaskManagement_t::get_current_foreground_task_count() > 0) {
        return;
    }

    if (is_skip_load_interface) {
        data_source->set_skip_load_interface();
    }

    EventDataSourceCreated::post(data_source.get());
    s_data_source_list->emplace_back(std::move(data_source));

    if (is_select_datasrc || s_data_source_list->size() == 1) {
        set_current_provider_idx((s_data_source_list->size() - 1));
    }
}


auto remove(wb_datasource::DataSourceBase_t* datasrc) -> void
{
    std::scoped_lock lock(details::s_data_source_mutex);
    if ((datasrc == nullptr) || (TaskManagement_t::get_current_foreground_task_count() > 0)) {
        return;
    }

    //  No questions/confirmation required.
    auto is_should_close{true};
    details::s_closing_data_source_list.insert(datasrc);
    EventDataSourceClosing::post(datasrc, &is_should_close);
    if (!is_should_close) {
        return;
    }

    const auto data_source_itr = std::ranges::find_if(*s_data_source_list, [datasrc](const auto& datasrc_ptr) {
        return datasrc_ptr.get() == datasrc;
    });

    // Couldn't find the data source.
    if (data_source_itr == s_data_source_list->end()) {
        return;
    }

    if (!s_data_source_list->empty()) {
        //  In case the 1st data source is about to be closed, get the one that is the 1st at the moment
        if (data_source_itr == s_data_source_list->begin()) {
            set_current_provider_idx(0);
            if (s_data_source_list->size() > 1) {
                EventDataSourceChanged::post(s_data_source_list->at(0).get(), s_data_source_list->at(1).get());
            }
        }
        //  In case the current data source is about to be closed, get the one right before it
        else if (std::distance(s_data_source_list->begin(), data_source_itr) == s_current_data_source) {
            set_current_provider_idx(s_current_data_source - 1);
        }
        //  Any other data source is about to be closed, find the current data source and reselect it
        else {
            const auto current_data_source = get();
            const auto current_data_source_itr =
                std::ranges::find_if(*s_data_source_list, [current_data_source](const auto& current_datasrc_ptr) {
                    return current_datasrc_ptr.get() == current_data_source;
                });

            if (current_data_source_itr != s_data_source_list->end()) {
                auto new_index = std::distance(s_data_source_list->begin(), current_data_source_itr);
                if ((s_current_data_source == new_index) && (new_index != 0)) {
                    new_index--;
                }

                set_current_provider_idx(new_index);
            }
            //  In case the current data source is not available in the list, get the 1st one
            else {
                set_current_provider_idx(0);
            }
        }
    }

    //  Remove the data source from the list, by moving it to a list of data sources to be removed
    static std::mutex remove_datasrc_mutex{};
    remove_datasrc_mutex.lock();
    auto remove_datasrc = data_source_itr->get();
    (*s_data_source_remove_list)[remove_datasrc] = std::move(*data_source_itr);
    remove_datasrc_mutex.unlock();

    //  Clean up any references from the main data source list
    s_data_source_list->erase(data_source_itr);
    details::s_closing_data_source_list.erase(datasrc);

    if ((s_current_data_source >= static_cast<i64_t>(s_data_source_list->size())) && (!s_data_source_list->empty())) {
        set_current_provider_idx((s_data_source_list->size() - 1));
    }

    if (s_data_source_list->empty()) {
        EventDataSourceChanged::post(datasrc, nullptr);
    }

    //  Once we don't have any tasks running, run the data source destruction
    EventDataSourceClosed::post(data_source_itr->get());
    TaskManagement_t::run_after_tasks_finished([remove_datasrc]() {
        EventDataSourceDeleted::post(remove_datasrc);
        TaskManagement_t::create_background_task("API: Closing Data Source", [remove_datasrc](Task_t&) {
            remove_datasrc_mutex.lock();
            auto data_source = std::move((*s_data_source_remove_list)[remove_datasrc]);
            s_data_source_remove_list->erase(remove_datasrc);
            remove_datasrc_mutex.unlock();
        });
    });
}


auto create_datasource(const std::string& name,
                       bool is_skip_load_interface,
                       bool is_select_datasrc) -> wb_datasource::DataSourceBase_t*
{
    //  Returns the raw pointer and releases the ownership of the unique pointer
    // auto new_datasrc = std::unique_ptr<wb_datasource::DataSourceBase_t>(nullptr);
    // RequestCreateDataSource::post(name, is_skip_load_interface, is_select_datasrc, new_datasrc.get());

    wb_datasource::DataSourceBase_t* new_datasrc = nullptr;
    RequestCreateDataSource::post(name, is_skip_load_interface, is_select_datasrc, &new_datasrc);
    return new_datasrc;
}


}    // namespace amd_work_bench::api::datasource
