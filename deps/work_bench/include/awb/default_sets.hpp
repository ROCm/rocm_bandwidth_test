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
 * Description: default_sets.hpp
 *
 */

#if !defined(AMD_TYPE_DEFAULT_SETTINGS_HPP)
#define AMD_TYPE_DEFAULT_SETTINGS_HPP

#include <awb/typedefs.hpp>

#include <unistd.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>


namespace amd_work_bench::urls
{

static constexpr auto kAMD_HOMEPAGE_URL("https://www.amd.com/en.html");
static constexpr auto kAMD_RBT_MAIN_REPO_URL("https://github.com/ROCm/rocm_bandwidth_test");
static constexpr auto kAMD_RBT_DOCS_URL("https://rocm.docs.amd.com/projects/rocm_bandwidth_test");
static constexpr auto kAMD_RBT_API_REPO_URL("https://api.github.com/repos/ROCm/rocm_bandwidth_test");

}    // namespace amd_work_bench::urls


/*
 *  Namespace for XDG Base Directory Specification
 *
 *  The XDG Base Directory Specification is based on the following concepts:
        -   There is a single base directory relative to which user-specific data files should be written. This
 directory is defined by the environment variable $XDG_DATA_HOME.
        -   There is a single base directory relative to which user-specific configuration files should be written. This
 directory is defined by the environment variable $XDG_CONFIG_HOME.
        -   There is a single base directory relative to which user-specific state data should be written. This
 directory is defined by the environment variable $XDG_STATE_HOME.
        -   There is a single base directory relative to which user-specific executable files may be written.
        -   There is a set of preference ordered base directories relative to which data files should be searched. This
 set of directories is defined by the environment variable $XDG_DATA_DIRS.
        -   There is a set of preference ordered base directories relative to which configuration files should be
 searched. This set of directories is defined by the environment variable $XDG_CONFIG_DIRS.
        -   There is a single base directory relative to which user-specific non-essential (cached) data should be
 written. This directory is defined by the environment variable $XDG_CACHE_HOME.
        -   There is a single base directory relative to which user-specific runtime files and other file objects should
 be placed. This directory is defined by the environment variable $XDG_RUNTIME_DIR.
 *  See:
        -   https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 */
namespace amd_work_bench::xdg
{

namespace std_fs = std::filesystem;
using FSPath_t = std::filesystem::path;
using FSPathList_t = std::vector<std::filesystem::path>;


class XDGBaseDirectoryException_t : public std::exception
{
    public:
        explicit XDGBaseDirectoryException_t(std::string exception_message) : m_message(std::move(exception_message))
        {
        }

        [[nodiscard]] auto what() const noexcept -> const char* override
        {
            return m_message.c_str();
        }

        [[nodiscard]] auto msg() const noexcept -> std::string
        {
            return m_message;
        }

    private:
        const std::string m_message;
};

class XDGBaseDirectories_t
{
    public:
        XDGBaseDirectories_t()
        {
            /*
             *  Note: Add extra functionality to XDG base.
             */
            const auto home_env = std::getenv("HOME");
            if (home_env == nullptr) {
                throw XDGBaseDirectoryException_t("$HOME is not set!");
            }

            const auto curr_work_env = std_fs::current_path();
            if (curr_work_env.empty()) {
                throw XDGBaseDirectoryException_t("$CWD is not set!");
            }

            m_home_directory = FSPath_t{home_env};
            m_current_work_directory = FSPath_t{curr_work_env};
            m_data_directories = get_paths_from_env_or_default("XDG_DATA_DIRS", FSPathList_t{"/usr/local/share", "/usr/share"});
            m_data_home_directory = get_absolute_path_from_env_or_default("XDG_DATA_HOME", m_home_directory / ".local" / "share");
            m_config_home_directory = get_absolute_path_from_env_or_default("XDG_CONFIG_HOME", m_home_directory / ".config");
            m_config_directories = get_paths_from_env_or_default("XDG_CONFIG_DIRS", FSPathList_t{"/etc/xdg"});
            m_cache_home_directory = get_absolute_path_from_env_or_default("XDG_CACHE_HOME", m_home_directory / ".cache");

            set_runtime_directory();
        }

        static auto get_instance() -> XDGBaseDirectories_t&
        {
            static XDGBaseDirectories_t directories;
            return directories;
        }

        [[nodiscard]] auto home_directory() -> const FSPath_t&
        {
            return m_home_directory;
        }

        [[nodiscard]] auto current_work_directory() -> const FSPath_t&
        {
            return m_current_work_directory;
        }

        [[nodiscard]] auto data_directories() -> const FSPathList_t&
        {
            return m_data_directories;
        }

        [[nodiscard]] auto data_home_directory() -> const FSPath_t&
        {
            return m_data_home_directory;
        }

        [[nodiscard]] auto config_home_directory() -> const FSPath_t&
        {
            return m_config_home_directory;
        }

        [[nodiscard]] auto config_directories() -> const FSPathList_t&
        {
            return m_config_directories;
        }

        [[nodiscard]] auto cache_home_directory() -> const FSPath_t&
        {
            return m_cache_home_directory;
        }

        [[nodiscard]] auto runtime_directory() -> const std::optional<FSPath_t>&
        {
            return m_runtime_directory;
        }


    private:
        void set_runtime_directory()
        {
            const auto runtime_env = std::getenv("XDG_RUNTIME_DIR");
            if (runtime_env != nullptr) {
                FSPath_t runtime_directory{runtime_env};
                if (runtime_directory.is_absolute()) {
                    if (!std_fs::exists(runtime_directory)) {
                        throw XDGBaseDirectoryException_t("$XDG_RUNTIME_DIR does not exist on the system");
                    }

                    auto runtime_directory_perms = std_fs::status(runtime_directory).permissions();
                    using fs_perms = std_fs::perms;
                    // Check XDG_RUNTIME_DIR permissions are 0700
                    if (((runtime_directory_perms & fs_perms::owner_all) == fs_perms::none) ||
                        ((runtime_directory_perms & fs_perms::group_all) != fs_perms::none) ||
                        ((runtime_directory_perms & fs_perms::others_all) != fs_perms::none)) {
                        throw XDGBaseDirectoryException_t("$XDG_RUNTIME_DIR does not have the required permission '0700'");
                    }
                    m_runtime_directory.emplace(runtime_directory);
                }
            }
        }

        static auto get_absolute_path_from_env_or_default(const char* env_name, FSPath_t&& default_path) -> FSPath_t
        {
            const auto env_var = std::getenv(env_name);
            if (env_var == nullptr) {
                return std::move(default_path);
            }

            auto path_var = FSPath_t{env_var};
            if (!path_var.is_absolute()) {
                return std::move(default_path);
            }

            return path_var;
        }

        static auto get_paths_from_env_or_default(const char* env_name, FSPathList_t&& default_paths) -> FSPathList_t
        {
            const auto env_var = std::getenv(env_name);
            if (env_var == nullptr) {
                return std::move(default_paths);
            }

            auto paths = std::string{env_var};
            auto directory_list = FSPathList_t{};
            auto start = size_t(0);
            auto pos = size_t(0);

            /*
             *  TODO: Redo this part with std::ranges::split_view
             */
            while ((pos = paths.find_first_of(':', start)) != std::string::npos) {
                FSPath_t current_path{paths.substr(start, pos - start)};
                if (current_path.is_absolute() && !is_path_in_list(directory_list, current_path)) {
                    directory_list.emplace_back(current_path);
                }
                start = (pos + 1);
            }

            FSPath_t current_path{paths.substr(start, pos - start)};
            if (current_path.is_absolute() && !is_path_in_list(directory_list, current_path)) {
                directory_list.emplace_back(current_path);
            }

            if (directory_list.empty()) {
                return std::move(default_paths);
            }

            return directory_list;
        }

        static auto is_path_in_list(const FSPathList_t& paths, const FSPath_t& path) -> bool
        {
            return std::find(std::begin(paths), std::end(paths), path) != paths.end();
        }

        FSPath_t m_home_directory;
        FSPath_t m_current_work_directory;
        FSPath_t m_data_home_directory;
        FSPath_t m_config_home_directory;
        FSPath_t m_cache_home_directory;
        FSPathList_t m_data_directories;
        FSPathList_t m_config_directories;
        std::optional<FSPath_t> m_runtime_directory;
};


[[nodiscard]] inline auto home_directory() -> const FSPath_t&
{
    return XDGBaseDirectories_t::get_instance().home_directory();
}

[[nodiscard]] inline auto current_work_directory() -> const FSPath_t&
{
    return XDGBaseDirectories_t::get_instance().current_work_directory();
}

[[nodiscard]] inline auto data_home_directory() -> const FSPath_t&
{
    return XDGBaseDirectories_t::get_instance().data_home_directory();
}

[[nodiscard]] inline auto config_home_directory() -> const FSPath_t&
{
    return XDGBaseDirectories_t::get_instance().config_home_directory();
}

[[nodiscard]] inline auto data_directories() -> const FSPathList_t&
{
    return XDGBaseDirectories_t::get_instance().data_directories();
}

[[nodiscard]] inline auto config_directories() -> const FSPathList_t&
{
    return XDGBaseDirectories_t::get_instance().config_directories();
}

[[nodiscard]] inline auto cache_home_directory() -> const FSPath_t&
{
    return XDGBaseDirectories_t::get_instance().cache_home_directory();
}

[[nodiscard]] inline auto runtime_directory() -> const std::optional<FSPath_t>&
{
    return XDGBaseDirectories_t::get_instance().runtime_directory();
}

}    // namespace amd_work_bench::xdg


namespace amd_work_bench::paths
{

using FSPath_t = std::filesystem::path;
using FSPathList_t = std::vector<std::filesystem::path>;
using FSPathSet_t = std::set<std::filesystem::path>;


namespace details
{

class DefaultPath_t
{
    public:
        DefaultPath_t(const DefaultPath_t&) = delete;
        DefaultPath_t(DefaultPath_t&&) = delete;
        DefaultPath_t& operator=(const DefaultPath_t&) = delete;
        DefaultPath_t& operator=(DefaultPath_t&&) = delete;

        virtual auto all() const -> FSPathList_t = 0;
        virtual auto read() const -> FSPathList_t;
        virtual auto write() const -> FSPathList_t;


    protected:
        constexpr DefaultPath_t() = default;
        virtual ~DefaultPath_t() = default;

    private:
        FSPath_t m_default_path{};
};


class ConfigPath_t : public DefaultPath_t
{
    public:
        explicit ConfigPath_t(FSPath_t config_path) : m_config_path(std::move(config_path))
        {
        }

        auto all() const -> FSPathList_t override;


    private:
        FSPath_t m_config_path{};
};


class PluginPath_t : public DefaultPath_t
{
    public:
        explicit PluginPath_t(FSPath_t plugin_path) : m_plugin_path(std::move(plugin_path))
        {
        }

        auto all() const -> FSPathList_t override;


    private:
        FSPath_t m_plugin_path{};
};


class DataPath_t : public DefaultPath_t
{
    public:
        explicit DataPath_t(FSPath_t log_path) : m_data_path(std::move(log_path))
        {
        }

        auto all() const -> FSPathList_t override;
        auto write() const -> FSPathList_t override;


    private:
        FSPath_t m_data_path{};
};

}    // namespace details

static const auto kDEFAULT_CONFIG_DIRECTORY_NAME = std::string("config");
static const auto kDEFAULT_LOG_DIRECTORY_NAME = std::string("log");
static const auto kDEFAULT_BACKUP_DIRECTORY_NAME = std::string("backups");
static const auto kDEFAULT_PLUGIN_DIRECTORY_NAME = std::string("plugins");
static const auto kDEFAULT_LIBRARY_DIRECTORY_NAME = std::string("libs");
static const auto kDEFAULT_INFO_PATH_STR = std::string("rocm_bandwidth_test");
static const auto KDEFAULT_INFO_BASE_PATH = FSPath_t(kDEFAULT_INFO_PATH_STR);

static const inline details::ConfigPath_t kCONFIG_PATH(KDEFAULT_INFO_BASE_PATH / kDEFAULT_CONFIG_DIRECTORY_NAME);
static const inline details::DataPath_t kDATA_PATH(KDEFAULT_INFO_BASE_PATH / kDEFAULT_LOG_DIRECTORY_NAME);
static const inline details::DataPath_t kBACKUP_PATH(KDEFAULT_INFO_BASE_PATH / kDEFAULT_BACKUP_DIRECTORY_NAME);
static const inline details::PluginPath_t kPLUGIN_PATH(kDEFAULT_PLUGIN_DIRECTORY_NAME);
static const inline details::PluginPath_t kLIBRARY_PATH(kDEFAULT_LIBRARY_DIRECTORY_NAME);

static const inline std::vector<const details::DefaultPath_t*> kALL_DEFAULT_PATHS = {
    &kCONFIG_PATH, &kDATA_PATH, &kPLUGIN_PATH, &kLIBRARY_PATH};
//
// static const inline std::vector<std::reference_wrapper<const DefaultPath_t&>> all_default_paths = {
//     kCONFIG_PATH, kPLUGIN_PATH, kDATA_PATH};
//

auto get_config_paths() -> FSPathList_t;
auto get_plugin_paths() -> FSPathList_t;

}    // namespace amd_work_bench::paths


namespace amd_work_bench::literals
{

static constexpr auto kTEXT_UNKNOWN = std::string_view("Unknown");
static constexpr auto kBYTES_IN_KBYTE = std::uint32_t(1024);
static const auto kEMPTY_STRING = std::string("");


static constexpr auto operator""_Bytes(ullong_t bytes_size) noexcept -> ullong_t
{
    return bytes_size;
}

static constexpr auto operator""_KBytes(ullong_t kbytes_size) noexcept -> ullong_t
{
    return operator""_Bytes(kbytes_size * kBYTES_IN_KBYTE);
}

static constexpr auto operator""_MBytes(ullong_t mbytes_size) noexcept -> ullong_t
{
    return operator""_KBytes(mbytes_size * kBYTES_IN_KBYTE);
}

static constexpr auto operator""_GBytes(ullong_t gbytes_size) noexcept -> ullong_t
{
    return operator""_MBytes(gbytes_size * kBYTES_IN_KBYTE);
}


}    // namespace amd_work_bench::literals


// namespace alias
namespace wb_urls = amd_work_bench::urls;
namespace wb_xdg = amd_work_bench::xdg;
namespace wb_paths = amd_work_bench::paths;
namespace wb_literals = amd_work_bench::literals;


#endif    //-- AMD_TYPE_DEFAULT_SETTINGS_HPP
