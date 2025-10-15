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
 * Description: msg_mgmt.cpp
 *
 */

#include "msg_mgmt.hpp"

#include <awb/work_bench_api.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/logger.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include <string>
#include <vector>


namespace amd_work_bench::messaging
{

//  TODO: This is Linux only, and should be moved to linux_utils.hpp/cpp files
static constexpr auto kLOCKPATHFILE = "/tmp/amd_work_bench.lock";
static constexpr auto kLOCKPATHDIR = "/tmp";
static constexpr auto kCOMMUNICATIONPIPEFILE = "/tmp/amd_work_bench.fifo";

auto setup_event_listener() -> void
{
    //  Setup a FIFO special file
    unlink(kCOMMUNICATIONPIPEFILE);
    if (mkfifo(kCOMMUNICATIONPIPEFILE, 0600) < 0) {
        return;
    }

    static auto fifo_file_descriptor = open(kCOMMUNICATIONPIPEFILE, O_RDWR | O_NONBLOCK);
    static auto thread_listener = std::jthread([](const std::stop_token& stop_token) {
        auto thread_buffer = DataStream_t(0XFFFF);
        while (true) {
            auto bytes_read = read(fifo_file_descriptor, thread_buffer.data(), thread_buffer.size());
            if (bytes_read > 0) {
                EventNativeMessageReceived::post(DataStream_t(thread_buffer.begin(), (thread_buffer.begin() + bytes_read)));
            }

            if (stop_token.stop_requested()) {
                break;
            }

            if (bytes_read <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    });

    std::atexit([]() {
        thread_listener.request_stop();
        close(fifo_file_descriptor);
        thread_listener.join();
    });
}


/*
 *  Not fully implemented yet, but for now we are just assuming this is always the main instance and
 *  events are forwarded to ourselves every time.
 */
//  TODO: This is Linux only, and should be moved to linux_utils.hpp/cpp files
auto setup_native_messaging() -> bool
{
    //  O_RDONLY flag requires that the caller have read permission on the object, even when the subsequent operation
    auto file_descriptor = open(kLOCKPATHFILE, O_RDONLY);
    if (file_descriptor < 0) {
        //  If pathname does not exist, create it as a regular file.
        file_descriptor = open(kLOCKPATHFILE, O_CREAT, 0600);
        if (file_descriptor < 0) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Messaging: Unable to create lock file.");
            return false;
        }
    }

    //  The LOCK_NB flag is used to prevent the process from blocking while waiting for the lock to be acquired.
    //  The LOCK_EX flag is used to acquire an exclusive lock on the file.
    auto is_main_instance = static_cast<bool>(flock(file_descriptor, LOCK_EX | LOCK_NB) == 0);
    if (is_main_instance) {
        setup_event_listener();
    }

    return is_main_instance;
}


/*
 *  Not fully implemented yet, but for now we are just assuming this is always the main instance and
 *  events are forwarded to ourselves every time.
 */
auto send_message_to_other_instance(const std::string& message, const DataStream_t& args) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Messaging: {} event sent to another instance. {}", message, __PRETTY_FUNCTION__);

    auto full_event_data = DataStream_t(message.begin(), message.end());
    full_event_data.push_back('\0');
    full_event_data.insert(full_event_data.end(), args.begin(), args.end());

    auto* data_ptr = &full_event_data[0];
    auto data_size = full_event_data.size();
    auto fifo_file_descriptor = open(kCOMMUNICATIONPIPEFILE, O_WRONLY);
    if (fifo_file_descriptor < 0) {
        return;
    }

    std::ignore = write(fifo_file_descriptor, data_ptr, data_size);
    close(fifo_file_descriptor);
}


auto setup_messaging_events() -> void
{
    MessageSendToMainInstance::subscribe([](const std::string& message, const DataStream_t& args) {
        if (wb_api_system::is_main_instance()) {
            wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Messaging: Executing {} event in main instance.", message);
            EventStartupDone::subscribe([message, args]() {
                wb_api_messaging::details::run_message_handler(message, args);
            });
        } else {
            wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Messaging: Event {} forwarded to existing instance.", message);
            send_message_to_other_instance(message, args);
        }
    });

    EventNativeMessageReceived::subscribe([](const DataStream_t& message) {
        auto null_idx = ssize_t(-1);
        auto message_data = reinterpret_cast<const char*>(message.data());
        auto message_size = message.size();
        for (auto idx = size_t(0); idx < message_size; ++idx) {
            if (message_data[idx] == '\0') {
                null_idx = idx;
                break;
            }
        }

        if (null_idx == (-1)) {
            wb_logger::loginfo(LogLevel::LOGGER_WARN,
                               "Messaging: Event {} with invalid forwarded.",
                               "EventNativeMessageReceived::subscribe[]");
            return;
        }

        auto event_name = std::string(message_data, null_idx);
        auto event_data = DataStream_t((message_data + null_idx + 1), (message_data + message_size));
        message_received(event_name, event_data);
    });
}

auto setup_messaging() -> void
{
    wb_api_system::details::set_main_instance(setup_native_messaging());
    setup_messaging_events();
}


auto message_received(const std::string& message, const DataStream_t& args) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Messaging: Event {} with size {} received.", message, args.size());
    wb_api_messaging::details::run_message_handler(message, args);
}


}    // namespace amd_work_bench::messaging
