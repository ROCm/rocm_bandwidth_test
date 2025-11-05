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
 * Description: crash_mgmt.cpp
 *
 */


#include "crash_mgmt.hpp"
#include "startup_mgmt.hpp"
#include "stacktrace.hpp"

#include <awb/work_bench_api.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/linux_utils.hpp>
#include <awb/task_mgmt.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <csignal>
#include <cstdio>
// #include <format>
#include <exception>
#include <source_location>
// #include <stacktrace>
#include <string>
#include <typeinfo>


namespace amd_work_bench::crash
{

static constexpr auto kSignals = {SIGILL, SIGABRT, SIGFPE, SIGSEGV};


extern "C" void trigger_safe_shutdown(int32_t signal_code = 0)
{
    /*
     *  Note:   The following are done in sequence:
     *          - Trigger event
     *          - Run exit tasks
     *          - Terminate all async tasks
     */
    EventAbnormalTermination::post(signal_code);
    wb_startup::run_exit_tasks();
    TaskManagement_t::stop();

    // clang-format off
    #if !defined(DEBUG)
        if (signal_code != 0) {
            std::exit(signal_code);
        }
        else {
            std::abort();
        }
    #else
        static bool is_bp_triggered = false;
        if (!is_bp_triggered) {
            is_bp_triggered = true;
            std::raise(SIGTRAP);
        }
        std::exit(signal_code);
    #endif
    // clang-format on
}


static auto send_native_message(const std::string& message) -> void
{
    wb_linux::native_error_message(amd_fmt::format("work-bench crashed during startup: {}", message));
}


using CrashCallback_t = void (*)(const std::string&);
static CrashCallback_t crash_cb = send_native_message;


auto reset_crash_handler() -> void
{
    std::set_terminate(nullptr);
    for (const auto signal : kSignals) {
        std::signal(signal, SIG_DFL);
    }
}


static auto dump_stacktrace() -> void
{
    auto current_stacktrace = amd_stacktrace::get_current_stacktrace_to_string();
    wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "Stacktrace: \n{}", current_stacktrace);
}


static auto save_crash_file(const std::string& crash_message) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "{}.", crash_message);
    json::JSon_t crash_json{
        {"logfile",    wb_logger::details::get_logger_file_path()        },
        {"message",    crash_message                                     },
        {"stacktrace", amd_stacktrace::get_current_stacktrace_to_string()}
    };

    const auto crash_dump_file = (wb_json::details::get_file_fs_path().stem().string() + "-" +
                                  (wb_crash::kJSON_CRASH_FILE_POSTFIX + "." + wb_json::kJSON_FILE_EXTENSION));
    for (const auto& path : paths::kDATA_PATH.write()) {
        wb_fs_io::FileOps_t json_dump_file(path / crash_dump_file, FileOpsMode_t::CREATE);
        if (json_dump_file.is_valid()) {
            json_dump_file.write_data(crash_json.dump(4));
            json_dump_file.close();
            wb_logger::loginfo(LogLevel::LOGGER_INFO,
                               "File: {}, written to: {}",
                               "work-bench-crash.json",
                               json_dump_file.get_path().string());
            return;
        }
    }

    wb_logger::loginfo(LogLevel::LOGGER_WARN, "File: {}, could not be written.", crash_dump_file);
}


auto crash_handler(const std::string& crash_message) -> void
{
    crash_cb(crash_message);
    dump_stacktrace();

    std::fflush(stderr);
    std::fflush(stdout);
}


static auto signal_handler(int32_t signal_code, const std::string& signal_name) -> void
{
    // clang-format off
    #if !defined(DEBUG)
    if (signal_code == SIGINT) {
        wb_api_system::stop_work_bench();
        return;
    }
    #endif
    // clang-format on

    /*
     *  Note: Avoid recursion upon code crashes
     */
    reset_crash_handler();
    crash_handler(amd_fmt::format("Signal: ({}) {}", signal_code, signal_name));
    if (std::uncaught_exceptions() > 0) {
        wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "Uncaught exception during signal handling");
    }
    trigger_safe_shutdown(signal_code);
}


static auto uncaught_exception_handler() -> void
{
    /*
     *  Note: Avoid recursion upon code crashes
     */
    reset_crash_handler();

    /*
     *  Current exception handling info
     */
    try {
        std::rethrow_exception(std::current_exception());
    } catch (const std::exception& exc) {
        const auto exception_description = amd_fmt::format("{}()::what() -> {}", typeid(exc).name(), exc.what());
        crash_handler(exception_description);
        wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "Terminated with uncaught exception: {}", exception_description);
    }
    trigger_safe_shutdown();
}


auto setup_crash_handler() -> void
{
    wb_debug::debug_startup();

    /*
     *  Note:   Setup signal handlers for all signals
     *          - SIGILL, SIGABRT, SIGFPE, SIGSEGV
     */
    {
        // clang-format off
        #define SIGNAL_HANDLER(signal_name)             \
        std::signal(signal_name, [](int32_t signal) {   \
            signal_handler(signal, #signal_name);       \
        });                                             \

        SIGNAL_HANDLER(SIGINT);
        SIGNAL_HANDLER(SIGILL);
        SIGNAL_HANDLER(SIGABRT);
        SIGNAL_HANDLER(SIGFPE);
        SIGNAL_HANDLER(SIGSEGV);

        #if defined(SIGBUS)
            SIGNAL_HANDLER(SIGBUS);
        #endif

        #undef SIGNAL_HANDLER
    }

    /*
    *  Note:   Setup new uncaught exception handler
    */
    std::set_terminate(uncaught_exception_handler);

    /*
    *  Do we need to subscribe for any events?
    */
    EventStartupDone::subscribe([]() {
        EventAbnormalTermination::subscribe([](int32_t signal_code) {
                //  Note: Silently disregard the signal code, and the compiler warnings.
                amd_work_bench::types::disregard(signal_code);
            // trigger_safe_shutdown(signal_code);
        });
    });

    EventStartupDone::subscribe([]() {
        crash_cb = save_crash_file;
    });

    // clang-format on

}    // namespace amd_work_bench::crash


}    // namespace amd_work_bench::crash
