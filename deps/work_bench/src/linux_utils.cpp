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
 * Description: linux_utils.cpp
 *
 */


#include <awb/linux_utils.hpp>
#include <awb/common_utils.hpp>
#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <algorithm>
#include <filesystem>
// #include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>


namespace amd_work_bench::fs::io
{


auto FileOps_t::open() -> void
{
    auto file_mode = std::ios_base::openmode(std::ios_base::failbit);
    switch (m_mode) {
        case (FileMode_t::READ):
            file_mode = std::ios_base::in;
            break;

        case (FileMode_t::WRITE):
            file_mode = (std::ios_base::out | std::ios_base::app);
            break;

        case (FileMode_t::CREATE):
            file_mode = (std::ios_base::in);
            break;

        default:
            file_mode = std::ios_base::in;
            break;
    }

    m_file_stream.open(m_path, file_mode);
    if (!m_file_stream.is_open()) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "Failed to open file: {} -> Mode: {}",
                           std::string(m_path),
                           static_cast<u32_t>(file_mode));
    }
}


auto FileOps_t::write_string(const std::string& data) -> void
{
    if (m_file_stream.is_open()) {
        m_file_stream << data;
    }
}


auto FileOps_t::close() -> void
{
    if (m_file_stream.is_open()) {
        m_file_stream.close();
    }
}


auto FileOps_t::seek(u64_t file_offset) -> void
{
    m_file_stream.seekg(file_offset, std::ios_base::beg);
}


auto FileOps_t::clone() -> FileOps_t
{
    return FileOps_t{m_path, m_mode};
}


auto FileOps_t::flush() -> void
{
    m_file_stream.flush();
}


auto FileOps_t::remove() -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::remove(m_path, fs_error) && !fs_error);
}


[[nodiscard]] auto FileOps_t::is_valid() const -> bool
{
    return (m_file_stream.is_open());
}


[[nodiscard]] auto FileOps_t::get_size() const -> u64_t
{
    if (is_valid() && std_fs::is_regular_file(m_path)) {
        auto fs_error(std::error_code{});
        if (auto file_size = std_fs::file_size(m_path, fs_error); file_size != static_cast<std::uintmax_t>(-1)) {
            return file_size;
        }
    }

    return static_cast<uintmax_t>(-1);
}


[[nodiscard]] auto FileOps_t::get_path() const -> std_fs::path
{
    return m_path;
}


[[nodiscard]] auto FileOps_t::get_mode() const -> FileMode_t
{
    return m_mode;
}


[[nodiscard]] auto FileOps_t::get_file_details() const -> std::optional<struct stat>
{
    struct stat file_stat = {};
    if (stat(m_path.c_str(), &file_stat) == 0) {
        return file_stat;
    }

    return std::nullopt;
}


}    // namespace amd_work_bench::fs::io


namespace amd_work_bench::utils::lnx
{


auto execute_command(const WordList_t& args_list) -> void
{
    std::vector<char*> args_char_list{};
    for (const auto& arg : args_list) {
        args_char_list.push_back(const_cast<char*>(arg.c_str()));
    }
    args_char_list.push_back(nullptr);

    //  Note:   Fork to avoid blocking the main process;
    //          0 = new process
    if (fork() == 0) {
        //  Note:   Execute the command and get the error code to string
        execvp(args_char_list[0], &args_char_list[0]);
        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Failed to execute command: {}", strerror(errno));
        std::exit(EXIT_FAILURE);
    }
}


auto is_process_elevated() -> bool
{
    return ((getuid() == 0) || (getuid() != geteuid()));
}


auto is_file_in_path(const FSPath_t& file_path) -> bool
{
    if (auto opt_path_var = wb_utils::get_env_var(kDEFAULT_VAR_PATH); opt_path_var.has_value()) {
        auto dir_path_list = wb_strings::split_str(opt_path_var.value(), ':');
        for (const auto& dir_path : dir_path_list) {
            auto full_path = std_fs::path((dir_path) / file_path);
            if (std_fs::exists(full_path)) {
                return true;
            }
        }
        /*
        //  Note:   Clang++ (v19) doesn't like this, g++ (v13+) works well.
        for (const auto& dir_path : std::views::split(opt_path_var.value(), ':')) {
            auto full_path = std::string(dir_path.begin(), dir_path.end());
            if (std_fs::exists(FSPath_t(full_path) / file_path)) {
                return true;
            }
        }
        */
    } else {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Variable 'PATH' is not set.");
    }

    return false;
}

auto get_executable_path() -> std::optional<std_fs::path>
{
    auto execute_path = std::string(PATH_MAX, '\0');
    //  Note:   Get the path of the executable, and return null if we have errors
    if (readlink("/proc/self/exe", execute_path.data(), PATH_MAX) < 0) {
        return std::nullopt;
    }

    return std_fs::path(wb_utils::strings::trim_all_copy(execute_path));
}


auto native_error_message(const std::string& message) -> void
{
    wb_logger::loginfo(LogLevel::LOGGER_CRITICAL, "{}", message);
    if (is_file_in_path("notify-send")) {
        execute_command({"notify-send", "-i", "script-error", message});
    }
}


auto startup_native() -> void
{
    /*
     *  Add plugin library to search path
     */
    for (const auto& plugin_path : paths::kLIBRARY_PATH.read()) {
        if (std_fs::exists(plugin_path)) {
            wb_utils::set_env_var(
                wb_linux::kDEFAULT_VAR_LD_LIB_PATH,
                amd_fmt::format("{}:{}", wb_utils::get_env_var(kDEFAULT_VAR_LD_LIB_PATH).value_or(""), plugin_path.string()),
                true);
        }
    }

    //  If not running in a terminal, set the log level to ERROR
    if (!wb_utils::is_tty()) {
        //  TODO:   We need to be able to redirect the output to a file
        //          logger::redirect_to_file();
    }
}


auto get_env_var(const std::string& var_name) -> std::optional<std::string>
{
    return wb_utils::get_env_var(var_name);
}


auto get_kernel_version() -> std::string
{
    auto os_info = std::string(wb_literals::kTEXT_UNKNOWN);
    struct utsname os_details
    {
    };

    //
    if (uname(&os_details) == 0) {
        os_info = amd_fmt::format("Host: {}  Kernel: {}  v{}  Arch: {}",
                                  wb_strings::trim_all_copy(os_details.nodename),
                                  wb_strings::trim_all_copy(os_details.release),
                                  wb_strings::trim_all_copy(os_details.version),
                                  wb_strings::trim_all_copy(os_details.machine));
    }

    return os_info;
}


// Note:    /etc/os-release format
//  *  SUSE Linux
/*
NAME="openSUSE Tumbleweed"
# VERSION="20240609"
ID="opensuse-tumbleweed"
ID_LIKE="opensuse suse"
VERSION_ID="20240609"
PRETTY_NAME="openSUSE Tumbleweed"
ANSI_COLOR="0;32"
# CPE 2.3 format, boo#1217921
CPE_NAME="cpe:2.3:o:opensuse:tumbleweed:20240609:*:*:*:*:*:*:*"
#CPE 2.2 format
#CPE_NAME="cpe:/o:opensuse:tumbleweed:20240609"
BUG_REPORT_URL="https://bugzilla.opensuse.org"
SUPPORT_URL="https://bugs.opensuse.org"
HOME_URL="https://www.opensuse.org"
DOCUMENTATION_URL="https://en.opensuse.org/Portal:Tumbleweed"
LOGO="distributor-logo-Tumbleweed"
*/

//  *  Ubuntu Linux
/*
PRETTY_NAME="Ubuntu 24.04.1 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
VERSION="24.04.1 LTS (Noble Numbat)"
VERSION_CODENAME=noble
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=noble
LOGO=ubuntu-logo
*/

auto get_distro_version() -> std::string
{
    const auto kOS_RELEASE_FILE = std::string("/etc/os-release");
    auto distro_info = std::string(wb_literals::kTEXT_UNKNOWN);

    //  Note:   Check if the line contains the key
    auto is_entry_key = [](const std::string& line, const std::string& key) -> bool {
        return wb_strings::contains(line, key);
        // return line.contains(key);
    };

    //  Note:   Get the value of the entry
    auto get_entry_value = [](const std::string& line) -> std::string {
        auto entry_value = (line.substr(line.find("=") + 1));
        std::erase(entry_value, '\"');
        wb_strings::trim_all(entry_value);
        return entry_value;
    };

    //
    if (std_fs::exists(kOS_RELEASE_FILE)) {
        auto os_release_file = std::ifstream(kOS_RELEASE_FILE);
        if (os_release_file.is_open()) {
            auto distro_info_tmp = std::string();
            auto read_line = std::string();
            while (std::getline(os_release_file, read_line)) {
                if (is_entry_key(read_line, "_NAME") || is_entry_key(read_line, "_CODENAME")) {
                    continue;
                } else if (is_entry_key(read_line, "NAME")) {
                    auto entry_value = get_entry_value(read_line);
                    distro_info_tmp += amd_fmt::format("Distro: {}  ", entry_value);
                } else if (is_entry_key(read_line, "VERSION_ID")) {
                    auto entry_value = get_entry_value(read_line);
                    distro_info_tmp += amd_fmt::format("Version: {}  ", entry_value);
                }
            }

            os_release_file.close();
            if (!distro_info_tmp.empty()) {
                distro_info = distro_info_tmp;
            }
        }
    }

    return distro_info;
}


}    // namespace amd_work_bench::utils::lnx
