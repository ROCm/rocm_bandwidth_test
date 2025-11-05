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
 * Description: plugin_common.cpp
 *
 */

#include <awb/event_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <exception>
#include <filesystem>
#include <functional>
#include <map>


/*
 *  TODO:  We might need to implement a header file, for `plugin_common.hpp` to define some interfaces,
 *         for example:
 */

/*
namespace amd_work_bench::background_services
{

namespace details
{
using ServiceCallback_t = std::function<void()>;

auto start_services() -> void;
auto stop_services() -> void;
}    // namespace details

auto register_service(const std::string service_name, const details::ServiceCallback_t& function_cb) -> void;
auto unregister_service(const std::string service_name, const details::ServiceCallback_t& function_cb) -> void;

}    // namespace amd_work_bench::background_services


namespace amd_work_bench::communication_interface
{

namespace details
{
using NetworkCallback_t = std::function<json::JSonData_t(const json::JSonData_t&)>;

auto get_network_endpoint_list() -> const std::map<std::string, NetworkCallback_t>&;

}    // namespace details

auto register_network_endpoint(const std::string& endpoint_name, const details::NetworkCallback_t& function_cb) -> void;

}    // namespace amd_work_bench::communication_interface
*/


namespace amd_work_bench::plugin::builtin
{

//  TODO:  Check if open_file function is needed.
// static auto open_file(const std::filesystem::path& path) -> void
//{
//    return;
//}


auto register_event_handler() -> void
{
    static bool is_work_bench_closing{false};
    EventCrashRecovery::subscribe([](const std::exception& exc) {
        /*
         *  TODO:   What should we display to the user?
         *          Should we display the exception message? Or should we display a generic message?
         */
        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Builtin: Event crash recovered: {}", exc.what());
        std::ignore = exc;
        return;
    });

    EventDataSourceClosing::subscribe([](const wb_datasource::DataSourceBase_t* datasrc, bool* is_should_close) {
        if (datasrc->is_used()) {
            *is_should_close = true;
        }
    });

    EventDataSourceChanged::subscribe(
        [](wb_datasource::DataSourceBase_t* old_datasrc, wb_datasource::DataSourceBase_t* new_datasrc) {
            std::ignore = old_datasrc;
            std::ignore = new_datasrc;

            return;
        });

    EventDataSourceOpened::subscribe([](wb_datasource::DataSourceBase_t* datasrc) {
        if ((datasrc != nullptr) && (wb_api::datasource::get() == datasrc)) {
        }

        return;
    });

    //  TODO:   Implement the following event handlers.
    // RequestOpenFile::subscribe(open_file);

    //  Data source Initialization
    EventDataSourceCreated::subscribe([](wb_datasource::DataSourceBase_t* datasrc) {
        if (datasrc->is_skip_load_interface()) {
            return;
        }

        if (datasrc->has_file_selector()) {
            if (!datasrc->file_selector_handler()) {
                TaskManagement_t::run_task_later([datasrc]() {
                    wb_api::datasource::remove(datasrc);
                });
                return;
            }

            if (!datasrc->open()) {
                TaskManagement_t::run_task_later([datasrc]() {
                    wb_api::datasource::remove(datasrc);
                });
                return;
            }

            EventDataSourceOpened::post(datasrc);
        } else if (!datasrc->has_load_interface()) {
            if (!datasrc->open() || !datasrc->is_available()) {
                TaskManagement_t::run_task_later([datasrc]() {
                    wb_api::datasource::remove(datasrc);
                });
                return;
            }

            EventDataSourceOpened::post(datasrc);
        }
    });


    //  TODO:   Anything specific needed for when startup procedures are taking place.
    EventStartupDone::subscribe([]() {
        const auto& startup_args = wb_api::system::get_startup_args();
        if (startup_args.size() > 0) {
            //  TODO:   Implement the specific startup arguments handling.
            for (const auto& arg : startup_args) {
                std::ignore = arg;
            }
        }

        return;
    });
}


//  TODO:   Do we need to implement this function?
// auto register_network_endpoints() -> void
//{
//}


//  TODO:   Do we need to implement this function?
// auto register_background_services() -> void
//{


auto create_welcome_banner() -> void
{
    // TODO:    Implement the welcome banner.
    TaskManagement_t::run_task_later([]() {
    });
}


//  TODO:   Do we need to implement this function?
// auto register_report_generators() -> void
//{
//}

//  TODO:   Do we need to implement this function?
// auto register_data_information_sections() -> void
//{
//}


//  TODO: If this is needed and implemented, where should we register (save) the settings to?
// auto register_settings() -> void
//{
//}


//  TODO: If this is needed and implemented, where should we load (read) the settings from?
// auto load_settings() -> void
//{
//}


}    // namespace amd_work_bench::plugin::builtin
