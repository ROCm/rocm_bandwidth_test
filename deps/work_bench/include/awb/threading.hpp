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
 * Description: threading.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_THREADING_UTILS_HPP)
#define AMD_WORK_BENCH_THREADING_UTILS_HPP

#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
// #include <format>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>


namespace amd_work_bench::utils::threads
{

/*
 *  NOTE:   Simple Thread Pool System (STPS)
 *          It should focus on multiple thread concurrently, like commands, plugin related tasks.
 */

static const auto kMAX_NUM_THREADS = std::thread::hardware_concurrency();

class ThreadPool_t
{
    public:
        using ThreadList_t = std::vector<std::thread>;
        using Task_t = std::function<void()>;
        using TaskQueue_t = std::queue<Task_t>;


        explicit ThreadPool_t(size_t thread_count) : m_max_num_threads(thread_count), m_num_active_tasks(0), m_is_stop(false)
        {
            if (thread_count > kMAX_NUM_THREADS) {
                m_max_num_threads = kMAX_NUM_THREADS;
                const auto thread_pool_error_message = amd_fmt::format(
                    "[Threading] : Thread count '{}' exceeds the maximum number of threads available on the system "
                    "'{}'. Setting thread count to maximum number on the system.",
                    thread_count,
                    m_max_num_threads);
                wb_logger::loginfo(LogLevel::LOGGER_WARN, "[Threading]: {} ", thread_pool_error_message);
            }

            m_threads.reserve(m_max_num_threads);
            for (auto idx = size_t(0); idx < m_max_num_threads; ++idx) {
                m_threads.emplace_back([this]() {
                    while (true) {
                        auto task = Task_t{};
                        {
                            auto lock = std::unique_lock<std::mutex>(m_mutex);
                            m_tasks_conditional_var.wait(lock, [this]() {
                                return (m_is_stop.load() || !m_tasks.empty());
                            });

                            if (m_is_stop.load() && m_tasks.empty()) {
                                return;
                            }

                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        ++m_num_active_tasks;
                        task();
                        --m_num_active_tasks;
                    }
                });
            }
        }

        ~ThreadPool_t()
        {
            {
                auto lock = std::unique_lock<std::mutex>(m_mutex);
                m_is_stop.store(true);
            }
            m_tasks_conditional_var.notify_all();

            for (auto& thread : m_threads) {
                thread.join();
            }
        }


        auto get_num_active_tasks() const -> uint32_t
        {
            return m_num_active_tasks.load();
        }


        /*
         *  Should cover most of the normal functions with or without arguments.
         *  The function should be a lambda expression, a function, or a function object.
         *  We should be able to:
         *      - enqueue_task([](int a, int b) { return a + b; }, 1, 2);
         *      - enqueue_task([](const std::string& text) { std::cout << text << "\n"; }, "Hello World!");
         *      - enqueue_task([]() { std::cout << "Hello World!" << "\n"; });
         *      - enqueue_task(&Plugin_t::plugin_main_entry_run, plugin_instance, plugin_argc, plugin_argv);
         *      - enqueue_command("ls", "-la", "/tmp");
         *      - enqueue_command("grep", "test", "/tmp/myfile.txt");   // Given that the file exists.
         *
         *  Those are all std::future<>, so they will only run and return the result when the future is called:
         *      - .get() or .wait() or .wait_for() or .wait_until()
         */
        template<typename FunctionTp, typename... Args>
        auto enqueue_task(FunctionTp&& func, Args&&... args)
            -> std::future<decltype(std::invoke(std::forward<FunctionTp>(func), std::forward<Args>(args)...))>
        {
            using ReturnType_t = decltype(std::invoke(std::forward<FunctionTp>(func), std::forward<Args>(args)...));

            /*
             *  std::packaged_task<> wraps any callable target (function, lambda expression, bind expression, or
             *  another function object) so that it can be invoked asynchronously.
             */
            auto task = std::make_shared<std::packaged_task<ReturnType_t()>>(
                [func = std::forward<FunctionTp>(func), ... args = std::forward<Args>(args)]() mutable {
                    //  Capture the arguments and invoke the function
                    return std::invoke(func, args...);
                });

            /*
             *  get_future() returns a std::future<> object that will eventually hold the result of the task.
             *  The std::future<> object returned by get_future() is a shared state object, which means that
             *  it can be copied and moved, and that all copies and moves of the std::future<> object are
             *  associated with the same shared state as *this
             *
             */
            auto future_async = std::future<ReturnType_t>(task->get_future());

            {
                auto lock = std::unique_lock<std::mutex>(m_mutex);
                m_tasks.emplace([task]() {
                    (*task)();
                });
            }
            m_tasks_conditional_var.notify_one();

            return future_async;
        }

        /*
         *  Specialized for Plugin_t* function, and int32, char**
         *  This is a special case for the plugin system, where we need to pass the plugin instance, argc, and argv
         *  The Plugin_t* could be a std::shared_ptr<>, and it doesn't need to be const, once template deduction can
         *  get it if declared a const. It can/should be a const if the function being called doesn't change the
         *  plugin instance.
         */
        template<typename FunctionTp, typename... Args>
        auto enqueue_task(FunctionTp&& func, Plugin_t* plugin_instance, int32_t argc, char** argv, Args&&... args)
            -> std::future<std::invoke_result_t<FunctionTp, Plugin_t*, int32_t, char**, Args...>>
        {
            using ReturnType_t = std::invoke_result_t<FunctionTp, Plugin_t*, int32_t, char**, Args...>;

            auto task = std::make_shared<std::packaged_task<ReturnType_t()>>([func = std::forward<FunctionTp>(func),
                                                                              plugin_instance,
                                                                              argc,
                                                                              argv,
                                                                              ... args = std::forward<Args>(args)]() mutable {
                auto is_task_finished = std::atomic<bool>(false);
                // try {
                auto task_result =
                    reinterpret_cast<ReturnType_t>(std::invoke(func, plugin_instance, argc, argv, std::forward<Args>(args)...));

                return task_result;
            });

            auto future_async = std::future<ReturnType_t>(task->get_future());

            {
                auto lock = std::unique_lock<std::mutex>(m_mutex);
                m_tasks.emplace([task]() {
                    (*task)();
                });
            }
            m_tasks_conditional_var.notify_one();

            return future_async;
        }

        /*
         *  Specialized for Plugin_t* function, and no args needed.
         */
        template<typename FunctionTp, typename... Args>
        auto enqueue_task(FunctionTp&& func,
                          Plugin_t* plugin_instance,
                          Args&&... args) -> std::future<std::invoke_result_t<FunctionTp, Plugin_t*, Args...>>
        {
            using ReturnType_t = std::invoke_result_t<FunctionTp, Plugin_t*, Args...>;

            auto task = std::make_shared<std::packaged_task<ReturnType_t()>>(
                [func = std::forward<FunctionTp>(func), plugin_instance, ... args = std::forward<Args>(args)]() mutable {
                    if (!plugin_instance) {
                        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "[Threading]: Plugin instance is nullptr.");
                        return ReturnType_t{};
                    }

                    auto is_task_finished = std::atomic<bool>(false);
                    // try {
                    auto task_result =
                        reinterpret_cast<ReturnType_t>(std::invoke(func, plugin_instance, std::forward<Args>(args)...));

                    return task_result;
                });

            auto future_async = std::future<ReturnType_t>(task->get_future());

            {
                auto lock = std::unique_lock<std::mutex>(m_mutex);
                m_tasks.emplace([task]() {
                    (*task)();
                });
            }
            m_tasks_conditional_var.notify_one();

            return future_async;
        }

        /*
         *  Possible implementation for commands.
         */
        template<typename... Args>
        auto enqueue_command(const std::string& command, Args&&... args) -> std::future<int32_t>
        {
            auto task = std::make_shared<std::packaged_task<int32_t()>>(
                [command = std::move(command), ... args = std::forward<Args>(args)]() mutable {
                    //  Append commands and arguments
                    auto full_command = std::ostringstream{};
                    full_command << command;

                    //  Fold expression
                    ((full_command << " " << args), ...);

                    auto proc_id = pid_t(fork());
                    if (proc_id == 0) {
                        //  Using std::format for potential future improvements.
                        execl("/bin/sh", "sh", "-c", full_command.str().c_str(), nullptr);
                        perror("execl");
                        _exit(1);
                    } else if (proc_id < 0) {
                        perror("fork()");
                        return -1;
                    } else {
                        auto status_code = int32_t(0);
                        waitpid(proc_id, &status_code, 0);
                        if (WIFEXITED(status_code)) {
                            return WEXITSTATUS(status_code);
                        } else {
                            return -1;
                        }
                    }
                });

            auto future_async = std::future<int32_t>(task->get_future());

            {
                auto lock = std::unique_lock<std::mutex>(m_mutex);
                m_tasks.emplace([task]() {
                    (*task)();
                });
            }
            m_tasks_conditional_var.notify_one();

            return future_async;
        }


    private:
        ThreadList_t m_threads{};
        TaskQueue_t m_tasks{};
        std::mutex m_mutex{};
        std::condition_variable m_tasks_conditional_var{};
        uint32_t m_max_num_threads{0};
        std::atomic<uint32_t> m_num_active_tasks{0};
        std::atomic<bool> m_is_stop{false};


    protected:
        //
};


/*
 *  TODO:   If we are not going to use this wrapper, we should remove it.
 *          Thread safe wrapper for the ThreadPool_t. It handles exits/exceptions for member functions.
 */
/*
using ThreadResultPair_t = std::pair<bool, int32_t>;
using ThreadFutureResultPair_t = std::future<ThreadResultPair_t>;

template<typename FunctionTp, typename... Args>
auto thread_safe_wrapper(
    ThreadPool_t& thread_pool, FunctionTp&& func, Plugin_t* plugin_instance, int32_t argc, char** argv, Args&&... args)
    -> std::future<std::pair<bool, std::invoke_result_t<FunctionTp, Plugin_t*, int32_t, char**, Args...>>>
{
    using FuncCallReturnType_t = std::invoke_result_t<FunctionTp, Plugin_t*, int32_t, char**, Args...>;
    using ReturnType_t = std::future<std::pair<bool, FuncCallReturnType_t>>;

    auto task = std::make_shared<std::packaged_task<ReturnType_t()>>(
        [func = std::forward<FunctionTp>(func), plugin_instance, argc, argv, ... args = std::forward<Args>(args)]() mutable {
            try {
                if (!plugin_instance) {
                    wb_logger::loginfo(LogLevel::LOGGER_ERROR, "[Threading]: Plugin instance is nullptr.");
                }

                auto task_result = reinterpret_cast<FuncCallReturnType_t>(
                    std::invoke(func, plugin_instance, argc, argv, std::forward<Args>(args)...));
            } catch (const std::exception& exc) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR, "[Threading]: Exception caught: {}", exc.what());
            } catch (...) {
                wb_logger::loginfo(LogLevel::LOGGER_ERROR, "[Threading]: Unknown exception caught.");
            }
        });

    auto future_async = std::future<FuncCallReturnType_t>(task->get_future());

    thread_pool.enqueue_task(task);
    return ReturnType_t{true, task->get_future()};
}
*/

}    // namespace amd_work_bench::utils::threads


// namespace alias
namespace wb_threads = amd_work_bench::utils::threads;


#endif    //-- AMD_WORK_BENCH_THREADING_UTILS_HPP
