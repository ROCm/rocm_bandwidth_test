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
 * Description: logger.cpp
 *
 */


#include <awb/logger.hpp>
#include <awb/default_sets.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

// #include <format>
#include <string>


namespace amd_work_bench::logger
{

namespace
{

std::string awb_logger_name{};
std::string awb_logger_file_path{};
std::string awb_debug_logger_file_path{};
LoggerPtr_t awb_default_logger{nullptr};
LoggerPtr_t awb_debug_logger{nullptr};
LoggerSinkPtr_t awb_default_sink{nullptr};
LoggerSinkPtr_t awb_debug_sink{nullptr};
std_fs::path awb_logger_file_fspath{};

}    // namespace


namespace details
{

auto is_developer_logging{false};
auto is_logging_suspended{false};
auto is_logging_enabled{false};


auto compose_logger_name_info(const std::string& logger_name, LoggerNameInfo_t logger_name_type) -> std::string
{
    switch (logger_name_type) {
        case (LoggerNameInfo_t::LOGGER_DEFAULT):
            return logger_name;
            break;

        case (LoggerNameInfo_t::LOGGER_DEFAULT_FILE):
            return (logger_name + "." + kLOGGER_FILE_EXTENSION);
            break;


        case (LoggerNameInfo_t::LOGGER_DEBUG):
            return (logger_name + "-" + kLOGGER_FILE_DEBUG_POSTFIX);
            break;

        case (LoggerNameInfo_t::LOGGER_DEBUG_FILE):
            return (logger_name + "-" + kLOGGER_FILE_DEBUG_POSTFIX + "." + kLOGGER_FILE_EXTENSION);
            break;

        case (LoggerNameInfo_t::LOGGER_PLUGIN):
            return (logger_name + "-" + kLOGGER_FILE_PLUGIN_POSTFIX);
            break;

        case (LoggerNameInfo_t::LOGGER_PLUGIN_FILE):
            return (logger_name + "-" + kLOGGER_FILE_PLUGIN_POSTFIX + "." + kLOGGER_FILE_EXTENSION);
            break;

        default:
            return std::string();
            break;
    }
}


auto get_logger_name() -> const std::string&
{
    return awb_logger_name;
}

auto get_logger_file_path() -> const std::string&
{
    return awb_logger_file_path;
}

auto get_default_logger() -> LoggerPtr_t
{
    return awb_default_logger;
}

auto get_default_sink() -> LoggerSinkPtr_t
{
    return awb_default_sink;
}


}    // namespace details


//
auto setup_base_logger(const std::string& logger_name, const std::string& logger_file_path) -> void
{
    if (logger_name.empty() || logger_file_path.empty()) {
        throw std::runtime_error("Logger name/path cannot be empty.");
        return;
    }

    // Set up the logger basic information
    awb_logger_name = logger_name;
    awb_logger_file_path =
        (logger_file_path + "/" + details::compose_logger_name_info(logger_name, details::LoggerNameInfo_t::LOGGER_DEFAULT_FILE));
    awb_debug_logger_file_path =
        (logger_file_path + "/" + details::compose_logger_name_info(logger_name, details::LoggerNameInfo_t::LOGGER_DEBUG_FILE));
    awb_logger_file_fspath = (awb_logger_file_path);
    awb_default_logger = std::make_shared<spdlog::logger>(
        details::compose_logger_name_info(logger_name, details::LoggerNameInfo_t::LOGGER_DEFAULT));
    awb_debug_logger =
        std::make_shared<spdlog::logger>(details::compose_logger_name_info(logger_name, details::LoggerNameInfo_t::LOGGER_DEBUG));
    awb_default_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(awb_logger_file_path);
    awb_debug_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(awb_debug_logger_file_path);
    awb_default_sink->set_level(spdlog::level::trace);
    awb_debug_sink->set_level(spdlog::level::debug);
    awb_default_logger->sinks().push_back(awb_default_sink);
    awb_debug_logger->sinks().push_back(awb_debug_sink);
    spdlog::set_default_logger(awb_default_logger);
    spdlog::register_logger(awb_debug_logger);

    // m_default_logger->log(spdlog::level::trace, "Logger {} initialized.", "");
    // m_default_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    // spdlog::get(details::compose_logger_name_info(logger_name, LoggerNameInfo_t::LOGGER_DEBUG))->warn("This is DEBUG
    // msg only.");
    // spdlog::flush_every(std::chrono::seconds(2));
    // spdlog::file_event_handlers file_handlers;
    // file_handlers.after_open = [](spdlog::filename_t filename) {
    //    std::string msg = std::format("File {} opened.", filename);
    //};
    return;
}


auto enable_developer_logger() -> void
{
    details::is_developer_logging = true;
    awb_default_logger->set_level(spdlog::level::debug);
    awb_debug_logger->set_level(spdlog::level::debug);
    return;
}


auto disable_developer_logger() -> void
{
    details::is_developer_logging = false;
    awb_default_logger->set_level(spdlog::level::trace);
    awb_debug_logger->set_level(spdlog::level::off);
    return;
}


auto is_logger_enabled() -> bool
{
    return details::is_logging_enabled;
}


auto suspend_logging() -> void
{
    if (!details::is_logging_suspended) {
        details::is_logging_suspended = true;
        awb_default_logger->set_level(spdlog::level::off);
        awb_debug_logger->set_level(spdlog::level::off);
    }

    return;
}


auto resume_logging() -> void
{
    if (details::is_logging_suspended) {
        details::is_logging_suspended = false;
        awb_default_logger->set_level(spdlog::level::trace);
        awb_debug_logger->set_level(spdlog::level::debug);
    }

    return;
}


[[maybe_unused]] auto trace(const std::string& message, auto&&... args) -> void
{
    spdlog::trace(message, std::forward<decltype(args)>(args)...);
}

[[maybe_unused]] auto debug(const std::string& message, auto&&... args) -> void
{
    spdlog::debug(message, std::forward<decltype(args)>(args)...);
}

[[maybe_unused]] auto info(const std::string& message, auto&&... args) -> void
{
    spdlog::info(message, std::forward<decltype(args)>(args)...);
}

[[maybe_unused]] auto warn(const std::string& message, auto&&... args) -> void
{
    spdlog::warn(message, std::forward<decltype(args)>(args)...);
}

[[maybe_unused]] auto error(const std::string& message, auto&&... args) -> void
{
    spdlog::error(message, std::forward<decltype(args)>(args)...);
}

[[maybe_unused]] auto critical(const std::string& message, auto&&... args) -> void
{
    spdlog::critical(message, std::forward<decltype(args)>(args)...);
}

//  TODO: Implement the proper tag for "[Developer]:" in the log message
[[maybe_unused]] auto developer(const std::string& message, auto&&... args) -> void
{
    spdlog::debug(message, std::forward<decltype(args)>(args)...);
}


/*
 *  Lets make sure global framefork logging was already all set up.
 */
auto is_global_framework_logging() -> bool
{
    return ((details::get_default_logger() != nullptr) && (!details::get_default_logger()->sinks().empty()) &&
            (!details::get_logger_name().empty()) && (!details::get_logger_file_path().empty()));
}

auto is_global_framework_logging_enabled() -> bool
{
    details::is_logging_enabled = false;

    // clang-format off
    #if defined(DEBUG)
        details::is_logging_enabled = true;
    #endif
    if (!details::is_logging_enabled) {
        if (auto is_debug_enabled = wb_linux::get_env_var(kLOGGER_VAR_DEBUG_ENABLE); is_debug_enabled.has_value()) {
            details::is_logging_enabled = true;
        }
    }
    // clang-format on

    return details::is_logging_enabled;
}

}    // namespace amd_work_bench::logger
