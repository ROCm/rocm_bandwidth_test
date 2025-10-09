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
 * Description: work_bench_api.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_API_HPP)
#define AMD_WORK_BENCH_API_HPP

#include <work_bench.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/datasrc_mgmt.hpp>

#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>


namespace amd_work_bench::datasource
{

class DataSourceBase_t;

}

namespace amd_work_bench::utils::memory::details
{

class AutoResetBase_t;

}


namespace amd_work_bench::api
{

namespace system
{

enum class TaskProgressState_t
{
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED,
    FAILED
};

enum class TaskProgress_t
{
    NORMAL,
    WARNING,
    ERROR
};

struct RunArguments_t
{
        char** argv;
        char** envp;
        int argc;
};

namespace details
{

auto set_main_instance(bool value) -> void;
auto auto_reset_objects_cleanup() -> void;
auto add_auto_reset_object(amd_work_bench::utils::memory::details::AutoResetBase_t* object) -> void;
auto add_startup_arg(const std::string& arg_name, const std::string& arg_value = {}) -> void;

}    // namespace details


auto start_work_bench() -> void;
auto stop_work_bench() -> void;
auto restart_work_bench() -> void;
auto set_taskbar_progress(TaskProgressState_t state, TaskProgress_t progress, u32_t percentage) -> void;
auto get_os_kernel_info() -> std::string;
auto get_os_distro_info() -> std::string;
auto get_cpu_info() -> std::string;
auto get_gpu_info() -> std::string;
auto get_work_bench_version() -> std::string;
auto get_work_bench_commit_hash(bool is_long_version = true) -> std::string;
auto get_work_bench_commit_branch() -> std::string;
auto get_work_bench_is_engineering_build() -> bool;
auto get_work_bench_build_type() -> std::string;
auto add_startup_task(const std::string& task_name, const std::function<bool()>& task, bool is_async) -> void;
auto add_shutdown_task(const std::string& task_name, const std::function<bool()>& task) -> void;
auto is_main_instance() -> bool;
auto get_lib_work_bench_module_handle() -> void*;
auto get_startup_args() -> const std::map<std::string, std::string>&;
auto get_startup_arg(const std::string& arg_name) -> std::string;


}    // namespace system


namespace messaging
{

namespace details
{
using MessageHandler_t = std::function<void(const DataStream_t&)>;

auto get_message_handlers() -> const std::map<std::string, MessageHandler_t>&;
auto run_message_handler(const std::string& message_name, const DataStream_t& args) -> void;

}    // namespace details

auto register_message_handler(const std::string& message_name, const details::MessageHandler_t& handler) -> void;


}    // namespace messaging


namespace datasource
{

namespace details
{

auto reset_closing_datasource() -> void;
auto get_closing_datasources() -> std::set<amd_work_bench::datasource::DataSourceBase_t*>;

}    // namespace details

auto is_valid() -> bool;
auto get() -> amd_work_bench::datasource::DataSourceBase_t*;

auto get_datasources() -> std::vector<amd_work_bench::datasource::DataSourceBase_t*>;
auto set_current_datasource(wb_types::NonNullPtr_t<amd_work_bench::datasource::DataSourceBase_t*> datasource) -> void;

auto set_current_provider_idx(i64_t index) -> void;
auto get_current_provider_idx() -> i64_t;

auto is_valid() -> bool;
auto stamp_it_used() -> void;
auto reset_it_used() -> void;
auto is_it_used() -> bool;

auto add_datasource(std::unique_ptr<amd_work_bench::datasource::DataSourceBase_t>&& datasource,
                    bool is_skip_load_interface,
                    bool is_select_datasrc = true) -> void;

template<std::derived_from<amd_work_bench::datasource::DataSourceBase_t> DataSourceTp>
auto add(auto&&... args) -> void
{
    add_datasource(std::make_unique<DataSourceTp>(std::forward<decltype(args)>(args)...));
}

auto remove(amd_work_bench::datasource::DataSourceBase_t* datasrc) -> void;

auto create_datasource(const std::string& name,
                       bool is_skip_load_interface = false,
                       bool is_select_datasrc = true) -> amd_work_bench::datasource::DataSourceBase_t*;


}    // namespace datasource


}    // namespace amd_work_bench::api


// namespace alias
namespace wb_api = amd_work_bench::api;
namespace wb_api_system = amd_work_bench::api::system;
namespace wb_api_messaging = amd_work_bench::api::messaging;
namespace wb_api_datasource = amd_work_bench::api::datasource;


#endif    //-- AMD_WORK_BENCH_API_HPP
