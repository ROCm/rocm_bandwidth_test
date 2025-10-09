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
 * Description: task_mgmt.cpp
 *
 */


#include <awb/task_mgmt.hpp>
#include <awb/logger.hpp>

#include <pthread.h>

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <ranges>
#include <source_location>
#include <stop_token>
#include <string>
#include <vector>


/*
 *  Note:   The Tasks_t related APIs are defined here.
 */
namespace
{

struct SourceLocationWrapper_t
{
    public:
        std::source_location source_location{};

        bool operator==(const SourceLocationWrapper_t& other) const
        {
            return ((source_location.file_name() == other.source_location.file_name()) &&
                    (source_location.function_name() == other.source_location.function_name()) &&
                    (source_location.column() == other.source_location.column()) &&
                    (source_location.line() == other.source_location.line()));
        }
};

}    // namespace


template<>
struct std::hash<SourceLocationWrapper_t>
{
        std::size_t operator()(const SourceLocationWrapper_t& src) const noexcept
        {
            auto hash1 = std::hash<std::string>{}(src.source_location.file_name());
            auto hash2 = std::hash<std::string>{}(src.source_location.function_name());
            auto hash3 = std::hash<u32_t>{}(src.source_location.column());
            auto hash4 = std::hash<u32_t>{}(src.source_location.line());
            return ((hash1 << 0) ^ (hash2 << 1) ^ (hash3 << 2) ^ (hash4 << 3));
        }
};

namespace amd_work_bench
{

namespace
{

constexpr u16_t kMAX_THREAD_NAME_SIZE(250);

std::mutex queue_mutex{};
std::recursive_mutex deferred_calls_mutex{};
std::recursive_mutex tasks_finished_mutex{};
std::condition_variable tasks_conditional_var{};

/*
 *  Note:   std::list<> is used due to pop_front() and push_back() operations, as opposed to std::vector<>.
 */
std::list<std::shared_ptr<Task_t>> task_list{};
std::list<std::shared_ptr<Task_t>> task_queue_list{};

std::list<std::function<void()>> tasks_deferred_list{};
std::list<std::function<void()>> tasks_finished_cb_list{};

std::unordered_map<SourceLocationWrapper_t, std::function<void()>> deferred_once_calls_list_map{};
std::vector<std::jthread> thread_workers{};

/*
 *  Note: All variables declared with the thread_local have thread storage duration.
 */
thread_local Task_t* current_task{nullptr};
//  For constexpr access to the array data, std::array<> is used.
thread_local std::array<char, kMAX_THREAD_NAME_SIZE> current_thread_name{};
// thread_local std::vector<std::string> current_thread_name{};

}    // namespace


auto Task_t::is_completed() const -> bool
{
    return m_is_completed;
}


auto Task_t::interrupt() -> void
{
    m_should_interrupt = true;
    if (m_interrupt_cb) {
        m_interrupt_cb();
    }
}


auto Task_t::update(u64_t value) -> void
{
    // Current progress value is updated
    m_current_value.store(value, std::memory_order_relaxed);

    // Check if the task was interrupted by main tread and throw an exception
    if (m_should_interrupt.load(std::memory_order_relaxed)) [[unlikely]] {
        throw TaskInterruption_t();
    }
}


auto Task_t::update() const -> void
{
    // Check if the task was interrupted by main tread and throw an exception
    if (m_should_interrupt.load(std::memory_order_relaxed)) [[unlikely]] {
        throw TaskInterruption_t();
    }
}


auto Task_t::increment() -> void
{
    // Current progress value is updated
    m_current_value.fetch_add(1, std::memory_order_relaxed);

    // Check if the task was interrupted by main tread and throw an exception
    if (m_should_interrupt.load(std::memory_order_relaxed)) [[unlikely]] {
        throw TaskInterruption_t();
    }
}


auto Task_t::set_max_value(u64_t max_value) -> void
{
    m_max_value = max_value;
}


auto Task_t::set_interrupt_callback(std::function<void()> interrupt_cb) -> void
{
    m_interrupt_cb = std::move(interrupt_cb);
}


auto Task_t::is_background_task() const -> bool
{
    return m_is_background;
}


auto Task_t::was_exception() const -> bool
{
    return m_was_exception;
}


auto Task_t::should_interrupt() const -> bool
{
    return m_should_interrupt;
}


auto Task_t::is_interrupted() const -> bool
{
    return m_is_interrupted;
}


auto Task_t::clear_exception() -> void
{
    m_was_exception = false;
}


auto Task_t::get_exception_message() const -> std::string
{
    std::scoped_lock lock(m_mutex);
    return m_exception_message;
}


auto Task_t::complete() -> void
{
    m_is_completed = true;
}


auto Task_t::interruption() -> void
{
    m_is_interrupted = true;
}


auto Task_t::exception(const std::string& message) -> void
{
    std::scoped_lock lock(m_mutex);
    m_was_exception = true;
    m_exception_message = message;
}


auto Task_t::get_task_name() const -> std::string
{
    return m_task_name;
}


auto Task_t::get_current_value() const -> u64_t
{
    return m_current_value;
}


auto Task_t::get_max_value() const -> u64_t
{
    return m_max_value;
}


/*
 *  Note:   The TaskHolder_t related APIs are defined here.
 */
auto TaskHolder_t::is_running() const -> bool
{
    const auto& running_task = m_task.lock();
    if (!running_task) {
        return false;
    }

    return !running_task->is_completed();
}


auto TaskHolder_t::is_interrupted() const -> bool
{
    const auto& running_task = m_task.lock();
    if (!running_task) {
        return false;
    }

    return running_task->is_interrupted();
}


auto TaskHolder_t::was_exception() const -> bool
{
    const auto& running_task = m_task.lock();
    if (!running_task) {
        return false;
    }

    return running_task->was_exception();
}

auto TaskHolder_t::should_interrupt() const -> bool
{
    const auto& running_task = m_task.lock();
    if (!running_task) {
        return false;
    }

    return running_task->should_interrupt();
}


auto TaskHolder_t::get_progress() const -> u32_t
{
    const auto& running_task = m_task.lock();
    if (!running_task) {
        return 0;
    }

    // Check if the tasks has no progress at all yet
    if (running_task->get_max_value() == 0) {
        return 0;
    }

    // Compute the progress percentage (0..100)
    return static_cast<u32_t>((running_task->get_current_value() * 100) / running_task->get_max_value());
}


/*
 *  Note:   The TaskManagement_t related APIs are defined here.
 */
auto TaskManagement_t::start() -> void
{
    const auto kMaxThreads = std::thread::hardware_concurrency();
    wb_logger::loginfo(LogLevel::LOGGER_INFO,
                       "TaskManagement_t::start() - Starting the task management with '{}' worker threads",
                       kMaxThreads);

    // Start the worker threads
    for (auto thread = u32_t(0); thread < kMaxThreads; ++thread) {
        thread_workers.emplace_back([=](const std::stop_token& stop_token) {
            while (true) {
                auto task = std::shared_ptr<Task_t>{nullptr};
                TaskManagement_t::set_current_task_name("Idle_Task_#" + std::to_string(thread));
                {
                    // Wait for a task to be queued
                    std::unique_lock lock(queue_mutex);
                    tasks_conditional_var.wait(lock, [&] {
                        return (!task_queue_list.empty() || stop_token.stop_requested());
                    });


                    // Check if the thread should stop
                    if (stop_token.stop_requested()) {
                        break;
                    }

                    // Get the next task to execute
                    task = std::move(task_queue_list.front());
                    task_queue_list.pop_front();
                    current_task = task.get();
                }

                if (task) {
                    try {
                        // Set the task name for the current thread
                        TaskManagement_t::set_current_task_name(task->m_task_name);
                        task->m_function(*task);
                        wb_logger::loginfo(LogLevel::LOGGER_WARN, "Task '{}' is done.", task->m_task_name);
                    } catch (const Task_t::TaskInterruption_t& exc) {
                        wb_logger::loginfo(LogLevel::LOGGER_WARN,
                                           "Task '{}' was interrupted by a user request.",
                                           task->m_task_name);
                        task->interruption();
                    } catch (const std::exception& exc) {
                        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                                           "Task '{}' failed with exception: {}",
                                           task->m_task_name,
                                           exc.what());
                        task->exception(exc.what());
                    } catch (...) {
                        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Task '{}' failed with unknown exception.", task->m_task_name);
                        task->exception("Exception unknown");
                    }
                    current_task = nullptr;
                    task->complete();
                }
            }
        });
    }
}


auto TaskManagement_t::stop() -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "TaskManagement_t::stop() - Stopping the task management");

    // Interrupt the whole list of tasks
    for (const auto& task : task_list) {
        task->interrupt();
    }

    // Get the worker threads to exit once they finish their current tasks
    for (auto& thread : thread_workers) {
        thread.request_stop();
    }

    // Notify all threads (including sleeping/idle) to stop so they can be exited
    tasks_conditional_var.notify_all();

    // Clear the worker threads, then exit
    thread_workers.clear();
    task_list.clear();
    task_queue_list.clear();
    tasks_deferred_list.clear();
    deferred_once_calls_list_map.clear();
    tasks_finished_cb_list.clear();
}


auto TaskManagement_t::set_current_task_name(const std::string& task_name) -> void
{
    // Initialize the task management
    std::ranges::fill(current_thread_name, '\0');
    std::ranges::copy(task_name | std::views::take(kMAX_THREAD_NAME_SIZE - 1), current_thread_name.begin());
    pthread_setname_np(pthread_self(), task_name.c_str());
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "set_current_task_name(): {} -> {} ", task_name, task_name.c_str());
}


auto TaskManagement_t::get_current_task_name() -> std::string
{
    return current_thread_name.data();
}


auto TaskManagement_t::create_foreground_task(const std::string& task_name,
                                              u64_t task_value,
                                              std::function<void(Task_t&)> function_cb,
                                              bool is_background_task) -> TaskHolder_t
{
    std::scoped_lock lock(queue_mutex);
    // Create a new task and add it to the task list
    auto task = std::make_shared<Task_t>(task_name, task_value, std::move(function_cb), is_background_task);
    task_list.emplace_back(task);
    task_queue_list.emplace_back(std::move(task));
    tasks_conditional_var.notify_one();
    return TaskHolder_t(task_list.back());
}


auto TaskManagement_t::create_foreground_task(const std::string& task_name,
                                              u64_t task_value,
                                              std::function<void(Task_t&)> function_cb) -> TaskHolder_t
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Creating foreground task: {} ", task_name);
    return create_foreground_task(task_name, task_value, std::move(function_cb), false);
}


auto TaskManagement_t::create_foreground_task(const std::string& task_name,
                                              u64_t task_value,
                                              std::function<void()> function_cb) -> TaskHolder_t
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Creating foreground task: {} ", task_name);
    return create_foreground_task(
        task_name,
        task_value,
        [function = std::move(function_cb)](Task_t&) {
            function();
        },
        false);
}


auto TaskManagement_t::create_background_task(const std::string& task_name,
                                              std::function<void(Task_t&)> function_cb) -> TaskHolder_t
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Creating background task: {} ", task_name);
    return create_foreground_task(task_name, 0, std::move(function_cb), true);
}


auto TaskManagement_t::create_background_task(const std::string& task_name, std::function<void()> function_cb) -> TaskHolder_t
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Creating background task: {} ", task_name);
    return create_foreground_task(
        task_name,
        0,
        [function = std::move(function_cb)](Task_t&) {
            function();
        },
        true);
}


auto TaskManagement_t::run_janitorial_work() -> void
{
    std::scoped_lock lock(queue_mutex);
    std::erase_if(task_list, [](const auto& task) {
        return (task->is_completed() && !task->was_exception());
    });

    if (task_list.empty()) {
        std::scoped_lock lock(deferred_calls_mutex);
        for (auto& finished_cb : tasks_finished_cb_list) {
            finished_cb();
        }

        tasks_finished_cb_list.clear();
    }
}


auto TaskManagement_t::get_current_task() -> Task_t&
{
    return *current_task;
}


auto TaskManagement_t::get_running_tasks() -> const std::list<std::shared_ptr<Task_t>>&
{
    return task_list;
}


auto TaskManagement_t::get_current_foreground_task_count() -> size_t
{
    std::scoped_lock lock(queue_mutex);
    return static_cast<size_t>(std::ranges::count_if(task_list, [](const auto& task) {
        return !task->is_background_task();
    }));
}


auto TaskManagement_t::get_current_background_task_count() -> size_t
{
    std::scoped_lock lock(queue_mutex);
    return static_cast<size_t>(std::ranges::count_if(task_list, [](const auto& task) {
        return task->is_background_task();
    }));
}


auto TaskManagement_t::run_after_tasks_finished(const std::function<void()>& function_cb) -> void
{
    std::scoped_lock lock(tasks_finished_mutex);
    for (const auto& task : task_list) {
        task->interrupt();
    }
    tasks_finished_cb_list.push_back(std::move(function_cb));
}


auto TaskManagement_t::run_task_later(const std::function<void()>& function_cb) -> void
{
    std::scoped_lock lock(deferred_calls_mutex);
    tasks_deferred_list.push_back(function_cb);
}


auto TaskManagement_t::run_task_later_once(const std::function<void()>& function_cb, std::source_location source_location) -> void
{
    std::scoped_lock lock(deferred_calls_mutex);
    deferred_once_calls_list_map[SourceLocationWrapper_t{source_location}] = function_cb;
}


auto TaskManagement_t::run_delayed_calls() -> void
{
    std::scoped_lock lock(deferred_calls_mutex);
    while (!tasks_deferred_list.empty()) {
        auto function_cb = tasks_deferred_list.front();
        tasks_deferred_list.pop_front();
        function_cb();
    }

    while (!deferred_once_calls_list_map.empty()) {
        auto deferred_cb = deferred_once_calls_list_map.extract(deferred_once_calls_list_map.begin());
        deferred_cb.mapped()();
    }
}


}    // namespace amd_work_bench
