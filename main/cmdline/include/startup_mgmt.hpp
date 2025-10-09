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
 * Description: startup_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_CLI_STARTUP_MGMT_HPP)
#define AMD_WORK_BENCH_CLI_STARTUP_MGMT_HPP

#include <awb/typedefs.hpp>

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <vector>


namespace amd_work_bench::startup
{

using TaskCallback_t = std::function<bool()>;


struct TaskInfo_t
{
    public:
        bool is_async{false};
        bool is_running{false};
        TaskCallback_t task_cb;
        std::string task_name{};

    private:
};


class StartupCommandLine_t
{
    public:
        StartupCommandLine_t();
        ~StartupCommandLine_t();

        auto run_startup_tasks() -> void;
        auto create_task(const TaskInfo_t& task_info) -> void;
        auto add_startup_task(const std::string& task_name, const TaskCallback_t& function_cb, bool is_async) -> void
        {
            std::scoped_lock lock(m_task_mutex);
            m_tasks.emplace_back(is_async, false, function_cb, task_name);
        }


    private:
        float m_linear_progress{0.0};
        std::atomic<bool> m_task_status{true};
        std::atomic<float> m_progress{0.0f};
        std::atomic<u32_t> m_total_task_counter{0};
        std::atomic<u32_t> m_completed_task_counter{0};
        std::mutex m_task_mutex{};
        std::mutex m_progress_mutex{};
        std::future<bool> m_tasks_succeeded{};
        WordList_t m_task_name_list{};
        std::vector<TaskInfo_t> m_tasks{};
        std::unique_ptr<StartupCommandLine_t> m_startup_cmdline{nullptr};

        auto run_async_tasks() -> std::future<bool>;
        auto load_system_assets() -> void;
};


// Common startup tasks
auto file_open_request_handler() -> void;
[[maybe_unused]] auto start_work_bench() -> std::unique_ptr<StartupCommandLine_t>;
auto stop_work_bench() -> void;
auto run_work_bench() -> u32_t;
auto run_exit_tasks() -> void;

auto run_command_line(int arc, char** argv) -> void;
auto show_help() -> void;
auto load_plugins() -> bool;
auto unload_plugins() -> bool;
auto get_startup_tasks() -> std::vector<TaskInfo_t>;
auto get_exiting_tasks() -> std::vector<TaskInfo_t>;

}    // namespace amd_work_bench::startup


// namespace alias
namespace wb_startup = amd_work_bench::startup;


#endif    //-- AMD_WORK_BENCH_CLI_STARTUP_MGMT_HPP
