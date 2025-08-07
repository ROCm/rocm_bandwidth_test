/*
 * MIT License
 *
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 *  Developed by:
 *
 *                  AMD ML Software Engineering
 *
 *                  Advanced Micro Devices, Inc.
 *
 *                  www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of Advanced Micro Devices, Inc,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
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
 * Author(s):   Daniel Oliveira <daniel.oliveira@amd.com>
 *
 *
 * Description: startup_mgmt.cpp
 *
 */


#include "startup_mgmt.hpp"

#include <awb/work_bench_api.hpp>
#include <awb/common_utils.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/linux_utils.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/task_mgmt.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
// #include <format>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>


namespace amd_work_bench::startup
{

auto get_startup_tasks() -> std::vector<TaskInfo_t>;
auto get_exiting_tasks() -> std::vector<TaskInfo_t>;


/*
 *  Specific startup tasks
 */

/*
 *  Note:   The reason why the constructors were implemented here (as opposed to the header) was because
 *          we are likely to change what happens during startup and we want to keep all those details and
 *          changes away from the header file.
 */
StartupCommandLine_t::StartupCommandLine_t() : m_startup_cmdline{nullptr}
{
    // Initialize the startup command line
    m_linear_progress = 0.0;

    /*
     *  Anything that needs to be done during startup should be done here.
     */
    load_system_assets();

    /*
     *  Resources/Info that can be gathered, created, reported, and done during startup should be done here.
     */
    {
        wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Startup: {} ", __PRETTY_FUNCTION__);
    }

    RequestAddStartupTask::subscribe([this](const std::string& task_name, const TaskCallback_t& task, bool is_async) {
        m_tasks.push_back({is_async, false, task, task_name});
    });
}


StartupCommandLine_t::~StartupCommandLine_t()
{
    /*
     *  Anything that needs to be done during shutdown should be done here.
     */
    {
        wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Shutdown: {} ", __PRETTY_FUNCTION__);
    }
}


auto StartupCommandLine_t::load_system_assets() -> void
{
    /*
     *  Load any system assets here.
     */
    {
        wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Loading System Assets: {} ", __PRETTY_FUNCTION__);
    }
}


auto StartupCommandLine_t::create_task(const TaskInfo_t& task) -> void
{
    //  TODO: Need to fix the async task creation. High priority.
    return;

    auto run_task = [&, task]() {
        try {
            //  Placeholder for current task name iterator
            auto task_name_list_itr = decltype(m_task_name_list)::iterator();
            {
                std::lock_guard<std::mutex> guard(m_progress_mutex);
                m_task_name_list.push_back(task.task_name + "...");
                task_name_list_itr = std::prev(m_task_name_list.end());
            }

            //  Placeholder for current task progress
            //  *  TODO:   std::experimental::scope_exit { }
            ON_SCOPE_EXIT
            {
                ++m_completed_task_counter;
                m_progress = (static_cast<float>(m_completed_task_counter) / static_cast<float>(m_total_task_counter));
            };

            //
            auto start_time = std::chrono::high_resolution_clock::now();
            auto task_status = task.task_cb();
            auto end_time = std::chrono::high_resolution_clock::now();
            auto task_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            if (task_status) {
                wb_logger::loginfo(LogLevel::LOGGER_INFO,
                                   "Startup: Task '{}' successfully finished in '{}' ms.",
                                   task.task_name,
                                   task_in_ms);
            } else {
                wb_logger::loginfo(LogLevel::LOGGER_WARN,
                                   "Startup: Task '{}' unsuccessfully finished in '{}' ms.",
                                   task.task_name,
                                   task_in_ms);
            }

            //  Overall task status
            m_task_status = (m_task_status && task_status);

            //  Delete the placeholder for current task name iterator
            {
                std::lock_guard<std::mutex> guard(m_progress_mutex);
                m_task_name_list.erase(task_name_list_itr);
            }
        } catch (const std::exception& exc) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                               "Startup: Task '{}' failed with exception: {}",
                               task.task_name,
                               exc.what());
            m_task_status = false;
        } catch (...) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Startup: Task '{}' failed with unknown exception.", task.task_name);
            m_task_status = false;
        }
    };

    //  Total task counter updated
    ++m_total_task_counter;

    //  If this is an async task, we can run it in a different thread, or this same one otherwise.
    if (task.is_async) {
        std::thread([task_name = task.task_name, run_task = std::move(run_task)] {
            TaskManagement_t::set_current_task_name(task_name);
            run_task();
        }).detach();
    } else {
        run_task();
    }
}


auto StartupCommandLine_t::run_async_tasks() -> std::future<bool>
{
    return std::async(std::launch::async, [this]() {
        TaskManagement_t::set_current_task_name("Startup_Tasks");

        //  Traverse through all the tasks in the list, and create them
        auto start_time = std::chrono::high_resolution_clock::now();
        wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "Startup: m_tasks {}.", m_tasks.size());

        //  Revisit the task status to see if all tasks ran
        while (true) {
            for (auto tasks_itr = m_tasks.begin(); tasks_itr != m_tasks.end(); ++tasks_itr) {
                //  Creates the new task callback
                if (!tasks_itr->is_running) {
                    create_task(*tasks_itr);
                    tasks_itr->is_running = true;
                }
            }

            {
                std::scoped_lock lock(m_task_mutex);
                if (m_completed_task_counter >= m_total_task_counter) {
                    break;
                }
            }

            //  Sleep for a bit (100 ms) to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto task_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        wb_logger::loginfo(LogLevel::LOGGER_INFO, "Startup: rocm_bandwidth_test startup finished in {} ms.", task_in_ms);

        // Is there any progress step left? Small extra delay to show 100% progress
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return m_task_status.load();
    });
}


auto StartupCommandLine_t::run_startup_tasks() -> void
{
    m_tasks_succeeded = run_async_tasks();
}


/*
 *  Common startup tasks
 */
auto file_open_request_handler() -> void
{
    //  TODO: Fix this
    // if (auto request_open_file = wb_utils::get_startup_file_path(); request_open_file.has_value()) {
    // RequestOpenFile::post(request_open_file.value());
    //}
}


[[maybe_unused]] auto start_work_bench() -> std::unique_ptr<StartupCommandLine_t>
{
    auto startup_cmdline = std::make_unique<StartupCommandLine_t>();
    // wb_logger::loginfo(LogLevel::LOGGER_INFO, "Startup: The following topology was found: {}.", topology_list);

    //  Run the startup tasks
    TaskManagement_t::start();

    // Info:
    /*
        struct TaskInfo_t
        {
            public:
                bool is_async{false};
                bool is_running{false};
                TaskCallback_t task_cb;
                std::string task_name{};
        };
    */
    for (const auto& [is_async, is_running, function_cb, task_name] : get_startup_tasks()) {
        startup_cmdline->add_startup_task(task_name, function_cb, is_async);
    }
    startup_cmdline->run_startup_tasks();

    return startup_cmdline;
}


auto stop_work_bench() -> void
{
    wb_startup::run_exit_tasks();
}


auto run_work_bench() -> u32_t
{
    auto should_restart{false};
    do {
        // Event handler registration
        should_restart = false;
        RequestAWBRestart::subscribe([&]() {
            should_restart = true;
        });

        {
            auto startup_cmdline = start_work_bench();
            file_open_request_handler();
        }

        //
        //  *  TODO:   std::experimental::scope_exit { }
        stop_work_bench();
    } while (should_restart);

    return (EXIT_SUCCESS);
}


/*
 *  TODO: Implement proper default help screen
 */
auto show_help() -> void
{
    const auto help_message_build_info =
        amd_fmt::format("AMD ROCm Bandwidth Test: \n -> version: {} \n -> [Commit: {} / Branch: {} / Build Type: {}]",
                        wb_api_system::get_work_bench_version(),
                        wb_api_system::get_work_bench_commit_hash(),
                        wb_api_system::get_work_bench_commit_branch(),
                        wb_api_system::get_work_bench_build_type());

    const auto help_message_env_info = amd_fmt::format("Environment: \n -> Kernel: {} \n -> OS: {}",
                                                       wb_api_system::get_os_kernel_info(),
                                                       wb_api_system::get_os_distro_info());

    auto help_message = R"(
        Help: AMD ROCm Bandwidth Test Command Line Interface
        Usage: rocm_bandwidth_test [options]

        Options:
            -h, --help        Display the main help screen
            -v, --version     Print version information
            -d, --debug       Run in debug mode

        Report bugs to: <rocm_bandwidth_test@amd.com>
    )";

    std::cout << help_message_build_info << "\n";
    std::cout << help_message_env_info << "\n";
    std::cout << help_message << "\n";
}


auto setup_exit() -> bool
{
    //  All async tasks need to be terminated
    TaskManagement_t::stop();

    /*
     *  Note: [** IMPORTANT: Read Carefully **]
     *  In case there is a crash or abnormal termination, we need to log it.
     *  That is why we always wrap static heap allocated object been used in amd_work_bench (awb) like;
     *  std::string(), std::vector(), std::function(), in an `AutoReset_t<Tp> (memory::AutoReset_t())`.
     *
     *  That's due the fact that dynamic libraries and each individual plugin get its `std::allocator`
     *  instance, which will try to deallocate the memory when the object is destroyed.
     *
     *  As this is a static storage, it will only happen after main() returns and then `awb` is unloaded.
     *  At that given point, all plugins should have been unloaded and the memory deallocation `std::allocator`
     *  will try to deallocate memory that is no longer valid, causing a crash.
     *
     *  By using the `AutoReset_t<Tp>` to wrap objects in, the `EventAWBClosing` event will do all the clean up
     *  before the heap is invalidated.
     *
     *  Until `PluginManagement_t::plugin_unload()` is called, the heap is still valid.
     */
    EventAbnormalTermination::subscribe([](i32_t) {
        wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "Startup: While cleaning up resources, a crash has happened.");
        wb_logger::loginfo(
            LogLevel::LOGGER_CRITICAL,
            "       : There is a possibility that a heap allocated object wasn't wrapped around 'AutoReset_t<T>'.");
        wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "       : There comments above (^^^) should be a good starting point.");
    });

    // Clean up all the resources
    wb_api_system::details::auto_reset_objects_cleanup();
    EventAWBClosing::post();
    EventManagement_t::clear();

    return true;
}


auto unload_plugins() -> bool
{
    PluginManagement_t::plugin_unload();

    return true;
}


auto cleanup_old_files() -> bool
{
    auto is_success{true};
    auto keep_newest_files = [&](u32_t files_counter, const paths::details::DefaultPath_t& path_type) {
        for (const auto& path : path_type.write()) {
            try {
                auto file_list = std::vector<std_fs::directory_entry>{};
                for (const auto& entry : std_fs::directory_iterator(path)) {
                    file_list.emplace_back(entry);
                }

                if (files_counter >= file_list.size()) {
                    return;
                }

                std::sort(file_list.begin(), file_list.end(), [](const auto& lhs, const auto& rhs) {
                    return (lhs.last_write_time() > rhs.last_write_time());
                });

                for (auto file_itr = file_list.begin() + files_counter; file_itr != file_list.end(); ++file_itr) {
                    std_fs::remove(file_itr->path());
                }

            } catch (const std::exception& exc) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Startup: Cleanup failed with exception: {}", exc.what());
                is_success = false;
            }
        }
    };
    keep_newest_files(10, paths::kDATA_PATH);
    keep_newest_files(20, paths::kBACKUP_PATH);

    return is_success;
}


auto get_exiting_tasks() -> std::vector<TaskInfo_t>
{
    return {
        {false, false, setup_exit,        "setup_exit"       },
        {false, false, unload_plugins,    "unload_plugins"   },
        {false, false, cleanup_old_files, "cleanup_old_files"},
    };
}


auto run_exit_tasks() -> void
{
    for (const auto& [is_async, is_running, function_cb, task_name] : get_exiting_tasks()) {
        const auto is_result = function_cb();
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "Startup: During exit task {0} was finished {1}",
                           task_name,
                           is_result ? "successfully" : "unsuccessfully");
    }
}


//  TODO: Implement what need for setting up the environment
auto setup_environment() -> bool
{
    return true;
}


//  TODO: Implement what need for creating local directories
auto create_directories() -> bool
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Startup: create_directories()");
    auto result_fs{false};
    for (const auto& path : paths::kALL_DEFAULT_PATHS) {
        for (const auto& folder_path : path->all()) {
            try {
                if (wb_fs::is_path_write_allowed(folder_path.parent_path())) {
                    wb_fs::is_create_directories(folder_path);
                }
            } catch (...) {
                result_fs = false;
                wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Startup: Failed to create directory '{}'.", folder_path.string());
            }
        }
    }

    return result_fs;
}


//  TODO: Implement what need for loading any settings, like local config files or environment variables
auto load_settings() -> bool
{
    return true;
}


/*
 *  Some heavy lifting tasks, related to plugin loading and initialization are done here.
 */
auto load_plugins() -> bool
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Startup: load_plugins()");
    // clang-format off
    //  Load the plugins
    #if !defined(AMD_APP_STATIC_LINK_PLUGINS)
        //kPLUGIN_PATH.read()
        for (const auto& plugin_path : paths::kPLUGIN_PATH.all()) {
            PluginManagement_t::plugin_load_path_add(plugin_path);
        }

        PluginManagement_t::library_load();
        PluginManagement_t::plugin_load();
    #endif
    // clang-format on

    // Build a list of the loaded plugins
    const auto& plugin_list = PluginManagement_t::plugin_get_all();
    if (plugin_list.empty()) {
        wb_logger::loginfo(LogLevel::LOGGER_WARN, "Startup: No plugins were loaded.");
        wb_api_system::details::add_startup_arg("no-plugins", "were found");
        return false;
    }

    const auto should_load_plugin = [executable_path = wb_linux::get_executable_path()](const Plugin_t& plugin) {
    /*
     *  TODO:   What to do when running in debug mode? Just ignore all plugins that are not in the same
     *          path or directory as the executable? like a quick; 'return'.
     */

    // clang-format off
        #if defined(AMD_APP_DEBUG)
            return true;
        #endif
        // clang-format on

        if (!executable_path.has_value() || !PluginManagement_t::plugin_get_load_path_all().empty()) {
            return true;
        }

        // Is the plugin in the same directory tree as the executable?
        return !std_fs::relative(plugin.plugin_get_library_path(), executable_path->parent_path()).string().starts_with("..");
    };

    //  Load the plugins, starting with the library based ones
    auto load_errors_counter = u32_t(0);
    auto plug_name_set = std::set<std::string>{};
    for (const auto& plugin : plugin_list) {
        if (plugin.is_library_plugin()) {
            if (!should_load_plugin(plugin)) {
                wb_logger::loginfo(LogLevel::LOGGER_DEBUG,
                                   "Startup: Library plugin {} was not loaded. Skipping it.",
                                   plugin.plugin_get_library_path().string());
                continue;
            }

            // Initialize it
            if (!plugin.plugin_init()) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                                   "Startup: Library plugin {} failed to initialize.",
                                   plugin.plugin_get_library_path().filename().string());
                ++load_errors_counter;
            }

            plug_name_set.insert(plugin.plugin_get_name());
        }
    }


    // Load all regular plugins
    for (const auto& plugin : plugin_list) {
        if (!plugin.is_library_plugin()) {
            if (!should_load_plugin(plugin)) {
                wb_logger::loginfo(LogLevel::LOGGER_DEBUG,
                                   "Startup: Plugin {} was not loaded. Skipping it.",
                                   plugin.plugin_get_library_path().string());
                continue;
            }

            // Initialize it
            if (!plugin.plugin_init()) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                                   "Startup: Plugin {} failed to initialize.",
                                   plugin.plugin_get_library_path().filename().string());
                ++load_errors_counter;
            }

            plug_name_set.insert(plugin.plugin_get_name());
        }
    }

    // Did you have errors for every plugin we tried to load?
    if ((load_errors_counter == plugin_list.size()) || (plugin_list.size() != plug_name_set.size())) {
        if (load_errors_counter == plugin_list.size()) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Startup: All plugins failed to load.");
            wb_api_system::details::add_startup_arg("no-plugins", "all failed to load");
        } else if (plugin_list.size() != plug_name_set.size()) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Startup: Detected duplicated plugins.");
            wb_api_system::details::add_startup_arg("duplicate-plugins", "duplicated plugins detected");
        }

        return false;
    }

    return true;
}


auto get_startup_tasks() -> std::vector<TaskInfo_t>
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Startup: get_startup_tasks.");
    return {
        {false, false, setup_environment,  "setup_environment" },
        {false, false, create_directories, "create_directories"},
        {false, false, load_settings,      "load_settings"     },
        {false, false, load_plugins,       "load_plugins"      },
    };
}


}    // namespace amd_work_bench::startup
