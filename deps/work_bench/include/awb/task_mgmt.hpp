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
 * Description: task_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_TASK_MGMT_HPP)
#define AMD_WORK_BENCH_TASK_MGMT_HPP

#include <work_bench.hpp>

#include <atomic>
#include <condition_variable>
// #include <format>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <source_location>
#include <vector>


namespace amd_work_bench
{

/*
 *  NOTE:   Task Management System (TMS)
 *          It should focus on multiple tasks running concurrently, with the ability to interrupt them.
 */

static constexpr auto kDEFAULT_NO_PROGRESS = 0;


class TaskManagement_t;
class TaskHolder_t;

class Task_t
{
    public:
        Task_t() = default;
        Task_t(const std::string& task_name, u64_t max_value, std::function<void(Task_t&)> function_cb, bool is_background_task)
            : m_task_name(std::move(task_name)),
              m_max_value(max_value),
              m_function(std::move(function_cb)),
              m_is_background(is_background_task)
        {
        }

        ~Task_t()
        {
            if (!is_completed()) {
                interrupt();
            }
        }

        Task_t(const Task_t&) = delete;
        Task_t(Task_t&& other) noexcept
        {
            {
                std::scoped_lock this_lock(m_mutex);
                std::scoped_lock other_lock(other.m_mutex);

                m_task_name = std::move(other.m_task_name);
                m_function = std::move(other.m_function);
            }
            m_current_value = static_cast<u64_t>(other.m_current_value);
            m_max_value = static_cast<u64_t>(other.m_max_value);
            m_is_interrupted = static_cast<bool>(other.m_is_interrupted);
            m_should_interrupt = static_cast<bool>(other.m_should_interrupt);
            m_is_completed = static_cast<bool>(other.m_is_completed);
            m_was_exception = static_cast<bool>(other.m_was_exception);
        }

        auto update(u64_t value) -> void;
        auto update() const -> void;
        auto increment() -> void;

        auto set_max_value(u64_t value) -> void;
        auto interrupt() -> void;
        auto set_interrupt_callback(std::function<void()> function_cb) -> void;

        [[nodiscard]] auto is_background_task() const -> bool;
        [[nodiscard]] auto is_interrupted() const -> bool;
        [[nodiscard]] auto is_completed() const -> bool;
        [[nodiscard]] auto should_interrupt() const -> bool;
        [[nodiscard]] auto was_exception() const -> bool;

        auto clear_exception() -> void;

        [[nodiscard]] auto get_task_name() const -> std::string;
        [[nodiscard]] auto get_exception_message() const -> std::string;
        [[nodiscard]] auto get_current_value() const -> u64_t;
        [[nodiscard]] auto get_max_value() const -> u64_t;


    private:
        auto complete() -> void;
        auto interruption() -> void;
        auto exception(const std::string& message) -> void;
        std::string m_task_name{};
        std::atomic<u64_t> m_max_value{0};

        mutable std::mutex m_mutex{};
        std::atomic<u64_t> m_current_value{0};
        std::function<void(Task_t&)> m_function{};
        std::function<void()> m_interrupt_cb{};
        std::atomic<bool> m_is_background{true};
        std::atomic<bool> m_is_interrupted{false};
        std::atomic<bool> m_should_interrupt{false};
        std::atomic<bool> m_is_completed{false};
        std::atomic<bool> m_was_exception{false};
        std::string m_exception_message{};

        struct TaskInterruption_t
        {
            public:
                virtual ~TaskInterruption_t() = default;
        };

        friend class TaskManagement_t;
        friend class TaskHolder_t;
};


class TaskHolder_t
{
    public:
        TaskHolder_t() = default;
        explicit TaskHolder_t(std::weak_ptr<Task_t> task) : m_task{std::move(task)}
        {
        }

        TaskHolder_t(const TaskHolder_t&) = delete;
        TaskHolder_t(TaskHolder_t&& other) noexcept;

        auto interrupt() const -> void;
        [[nodiscard]] auto is_running() const -> bool;
        [[nodiscard]] auto is_interrupted() const -> bool;
        [[nodiscard]] auto was_exception() const -> bool;
        [[nodiscard]] auto should_interrupt() const -> bool;
        [[nodiscard]] auto get_progress() const -> u32_t;


    private:
        std::weak_ptr<Task_t> m_task{};
};


class TaskManagement_t
{
    public:
        TaskManagement_t() = delete;

        static auto start() -> void;
        static auto stop() -> void;

        /*
         *  Note:   Async task creation with:
         *              - name
         *              - value
         *              - callback function.
         *
         *          Some tasks are foreground, and some are background.
         */
        static auto create_foreground_task(const std::string& task_name,
                                           u64_t task_value,
                                           std::function<void(Task_t&)> function_cb) -> TaskHolder_t;
        static auto create_foreground_task(const std::string& task_name,
                                           u64_t task_value,
                                           std::function<void()> function_cb) -> TaskHolder_t;

        static auto create_background_task(const std::string& task_name,
                                           std::function<void(Task_t&)> function_cb) -> TaskHolder_t;
        static auto create_background_task(const std::string& task_name, std::function<void()> function_cb) -> TaskHolder_t;

        /*
         *  Note:   Sync task creation with:
         *              - name
         *              - value
         *              - callback function.
         *
         *          Some tasks are foreground, and some are background.
         */
        static auto run_task_later(const std::function<void()>& function_cb) -> void;
        static auto run_task_later_once(const std::function<void()>& function_cb,
                                        std::source_location src_location = std::source_location::current()) -> void;

        static auto run_after_tasks_finished(const std::function<void()>& function_cb) -> void;

        static auto set_current_task_name(const std::string& task_name) -> void;
        static auto get_current_task_name() -> std::string;
        static auto get_current_task() -> Task_t&;
        static auto get_current_foreground_task_count() -> size_t;
        static auto get_current_background_task_count() -> size_t;
        static auto get_running_tasks() -> const std::list<std::shared_ptr<Task_t>>&;

        static auto run_janitorial_work() -> void;
        static auto run_delayed_calls() -> void;


    private:
        static auto add_task(std::shared_ptr<Task_t> task) -> void;
        static auto create_foreground_task(const std::string& task_name,
                                           u64_t task_value,
                                           std::function<void(Task_t&)> function_cb,
                                           bool is_background_task) -> TaskHolder_t;
};

}    // namespace amd_work_bench

#endif    //-- AMD_WORK_BENCH_TASK_MGMT_HPP
