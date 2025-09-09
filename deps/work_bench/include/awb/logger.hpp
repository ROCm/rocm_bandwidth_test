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
 * Description: logger.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_LOGGER_HPP)
#define AMD_WORK_BENCH_LOGGER_HPP

#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/linux_utils.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <filesystem>
// #include <format>
#include <memory>
#include <ranges>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/argv.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>


namespace amd_work_bench::logger
{

auto is_global_framework_logging() -> bool;
auto suspend_logging() -> void;
auto resume_logging() -> void;
auto setup_base_logger(const std::string& logger_name, const std::string& logger_file_path) -> void;
auto enable_developer_logger() -> void;
auto disable_developer_logger() -> void;
auto is_logger_enabled() -> bool;
auto is_global_framework_logging_enabled() -> bool;

using LoggerPtr_t = std::shared_ptr<spdlog::logger>;
using LoggerSinkPtr_t = std::shared_ptr<spdlog::sinks::basic_file_sink_mt>;

static const auto kLOGGER_FILE_APPLICATION_PREFIX(std::string("rocm_bandwidth"));
static const auto kLOGGER_FILE_DEBUG_POSTFIX(std::string("debug"));
static const auto kLOGGER_FILE_PLUGIN_POSTFIX(std::string("plugin"));
static const auto kLOGGER_FILE_EXTENSION(std::string("log"));
static const auto kLOGGER_FILE_APPLICATION_PATH(std::string(xdg::home_directory()) + "/" + wb_paths::kDEFAULT_INFO_PATH_STR);
static const auto kLOGGER_VAR_DEBUG_ENABLE = std::string("AWB_LOGGING_ENABLE");

enum class LoggerLevel_t
{
    LOGGER_TRACE = spdlog::level::trace,
    LOGGER_DEBUG = spdlog::level::debug,
    LOGGER_INFO = spdlog::level::info,
    LOGGER_WARN = spdlog::level::warn,
    LOGGER_ERROR = spdlog::level::err,
    LOGGER_CRITICAL = spdlog::level::critical,
    LOGGER_OFF = spdlog::level::off,
    LOGGER_DEVEL = (spdlog::level::debug + 1000)
};


namespace std_fs = std::filesystem;
namespace fs = amd_work_bench::fs;
namespace io = amd_work_bench::fs::io;


namespace details
{

enum class LoggerNameInfo_t
{
    LOGGER_DEFAULT,
    LOGGER_DEBUG,
    LOGGER_DEFAULT_FILE,
    LOGGER_DEBUG_FILE,
    LOGGER_PLUGIN,
    LOGGER_PLUGIN_FILE
};


auto get_logger_name() -> const std::string&;
auto get_logger_file_path() -> const std::string&;
auto get_default_logger() -> LoggerPtr_t;
auto get_default_sink() -> LoggerSinkPtr_t;

}    // namespace details


[[maybe_unused]] auto trace(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto debug(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto info(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto warn(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto error(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto critical(const std::string& message, auto&&... args) -> void;
[[maybe_unused]] auto developer(const std::string& message, auto&&... args) -> void;

/*
 *  Use this function to log messages with all different levels, in a single function.
 */
template<std::size_t TpSize>
struct StaticString_t
{
    public:
        char data[TpSize]{};
        constexpr StaticString_t(const char (&str)[TpSize])
        {
            std::ranges::copy(str, data);
        }
};

template<StaticString_t StrTp>
struct FormatString_t
{
    public:
        static constexpr const char* static_string = StrTp.data;
};

template<StaticString_t StrTp>
constexpr auto operator""_fmt() -> FormatString_t<StrTp>
{
    return {};
}


/* C++20 feature
[[maybe_unused]] auto loginfo(LoggerLevel_t logger_level, const std::string& message, auto&&... args) -> void
{
    if (!is_global_framework_logging()) {
        setup_base_logger(kLOGGER_FILE_APPLICATION_PREFIX, kLOGGER_FILE_APPLICATION_PATH);
    }

    switch (logger_level) {
        case (LoggerLevel_t::LOGGER_TRACE):
            spdlog::trace(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_DEBUG):
            spdlog::debug(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_INFO):
            spdlog::info(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_WARN):
            spdlog::warn(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_ERROR):
            spdlog::error(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_CRITICAL):
            spdlog::critical(message, std::forward<decltype(args)>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_DEVEL):
            spdlog::debug(message, std::forward<decltype(args)>(args)...);
            break;

        default:
            break;
    }
 }
*/


template<typename... Args>
[[maybe_unused]] auto loginfo(LoggerLevel_t logger_level, fmt::format_string<Args...> message, Args&&... args) -> void
{
    // Check if logging is enabled, otherwise don't log anything
    if (!is_global_framework_logging_enabled()) {
        return;
    }

    if (!is_global_framework_logging()) {
        setup_base_logger(kLOGGER_FILE_APPLICATION_PREFIX, kLOGGER_FILE_APPLICATION_PATH);
    }

    switch (logger_level) {
        case (LoggerLevel_t::LOGGER_TRACE):
            spdlog::trace(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_DEBUG):
            spdlog::debug(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_INFO):
            spdlog::info(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_WARN):
            spdlog::warn(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_ERROR):
            spdlog::error(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_CRITICAL):
            spdlog::critical(message, std::forward<Args>(args)...);
            break;

        case (LoggerLevel_t::LOGGER_DEVEL):
            spdlog::debug(message, std::forward<Args>(args)...);
            break;

        default:
            break;
    }
}


class LoggerDefaultBase_t
{
    public:
        LoggerDefaultBase_t() = default;
        LoggerDefaultBase_t(const std::string& logger_name,
                            const std_fs::path& logger_file_path,
                            const io::FileOps_t::FileMode_t mode);
        ~LoggerDefaultBase_t() = default;

    protected:
        io::FileOps_t m_file_ops{};
        io::FileOps_t::FileMode_t m_mode{};
        std::string m_logger_name{};
        std_fs::path m_logger_file_path{};
};


class LoggerDefault_t final : public LoggerDefaultBase_t
{
    public:
        LoggerDefault_t() = default;
        LoggerDefault_t(const std::string& logger_name,
                        const std_fs::path& logger_file_path,
                        const io::FileOps_t::FileMode_t mode);
        ~LoggerDefault_t() = default;

        LoggerDefault_t(const LoggerDefault_t&) = delete;
        LoggerDefault_t& operator=(const LoggerDefault_t&) = delete;
        LoggerDefault_t(LoggerDefault_t&&) = delete;
        LoggerDefault_t& operator=(LoggerDefault_t&&) = delete;

        auto get_logger() -> LoggerPtr_t;
        auto get_sink() -> LoggerSinkPtr_t;


    private:
        LoggerPtr_t m_default_logger{nullptr};
        LoggerSinkPtr_t m_default_sink{nullptr};
};


}    // namespace amd_work_bench::logger


// namespace alias
namespace wb_logger = amd_work_bench::logger;

// type alias
using LogLevel = wb_logger::LoggerLevel_t;


#endif    //-- AMD_WORK_BENCH_LOGGER_HPP
