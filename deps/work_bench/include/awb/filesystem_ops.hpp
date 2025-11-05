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
 * Description: filesystem_ops.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_FILE_SYSTEM_OPS_HPP)
#define AMD_WORK_BENCH_FILE_SYSTEM_OPS_HPP

#include <awb/typedefs.hpp>

#include <sys/stat.h>

#include <atomic>
#include <cstdio>
#include <filesystem>
// #include <format>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>


namespace amd_work_bench::fs
{

namespace std_fs = std::filesystem;

namespace io
{

class FileOps_t
{
    public:
        enum class FileMode_t
        {
            READ,
            WRITE,
            CREATE
        };

        explicit FileOps_t(const std_fs::path& path, FileMode_t mode) noexcept : m_path(path), m_mode(mode)
        {
            open();
        }

        FileOps_t() noexcept = default;

        ~FileOps_t() noexcept
        {
            close();
        }

        FileOps_t(const FileOps_t&) = delete;

        FileOps_t(FileOps_t&& other) noexcept
        {
            m_mode = other.m_mode;
            m_file_stream = std::move(other.m_file_stream);
            m_path = std::move(other.m_path);
        }

        FileOps_t& operator=(FileOps_t&& other) noexcept
        {
            m_mode = other.m_mode;
            m_file_stream = std::move(other.m_file_stream);
            m_path = std::move(other.m_path);
            return *this;
        }

        auto open() -> void;
        auto seek(u64_t file_offset) -> void;
        auto close() -> void;
        auto clone() -> FileOps_t;
        auto flush() -> void;
        auto remove() -> bool;

        auto get_file_stream() -> std::fstream&
        {
            return m_file_stream;
        }

        template<typename Tp>
        auto write_data(const Tp& data) -> void
        {
            if (m_file_stream.is_open()) {
                m_file_stream.write(reinterpret_cast<const char*>(&data), sizeof(data));
            }
        }

        auto write_string(const std::string& data) -> void;

        [[nodiscard]] auto get_size() const -> u64_t;
        [[nodiscard]] auto is_valid() const -> bool;
        [[nodiscard]] auto get_path() const -> std_fs::path;
        [[nodiscard]] auto get_mode() const -> FileMode_t;
        [[nodiscard]] auto get_file_details() const -> std::optional<struct stat>;


    private:
        std_fs::path m_path{};
        FileMode_t m_mode{FileMode_t::READ};
        std::fstream m_file_stream;
};

}    // namespace io


[[maybe_unused]]
inline auto is_exists(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::exists(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_regular_file(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::is_regular_file(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_directory(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::is_directory(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_create_directories(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::create_directories(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_copy_file(const std_fs::path& source,
                         const std_fs::path& target,
                         std_fs::copy_options = std_fs::copy_options::none) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::copy_file(source, target, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto get_file_size(const std_fs::path& path) -> std::uintmax_t
{
    auto fs_error(std::error_code{});
    if (auto file_size = std_fs::file_size(path, fs_error); fs_error) {
        return 0;
    } else {
        return file_size;
    }
}


[[maybe_unused]]
inline auto is_relative_sub_path(const std_fs::path& base_path, const std_fs::path& target_path) -> bool
{
    const auto relative_path = std_fs::relative(target_path, base_path).u8string();
    return ((relative_path[0] != '.' && relative_path[1] != '.') || (relative_path.size() == 1));
}


[[maybe_unused]]
inline auto is_remove(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::remove(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_remove_all(const std_fs::path& path) -> bool
{
    auto fs_error(std::error_code{});
    return (std_fs::remove_all(path, fs_error) && (!fs_error));
}


[[maybe_unused]]
inline auto is_path_writeable(const std_fs::path& path) -> bool
{
    static constexpr auto WriteableTestFile = "__awb_writeable_tmp__";
    /*
     *  If there is already a file with that name in-place, delete it.
     */
    {
        auto tmp_file = io::FileOps_t(path / WriteableTestFile, io::FileOps_t::FileMode_t::READ);
        if (tmp_file.is_valid() && !tmp_file.remove()) {
            return false;
        }
    }

    /*
     *  Create a file and check if it is writeable.
     */
    auto tmp_file = io::FileOps_t(path / WriteableTestFile, io::FileOps_t::FileMode_t::CREATE);
    if (auto fs_result = tmp_file.is_valid(); fs_result) {
        if (!tmp_file.remove()) {
            return false;
        }

        return fs_result;
    }

    // TODO: Recheck if we need to check for write/read permissions instead of just write/delete a temp file.
    // auto fs_error(std::error_code{});
    // return (std_fs::status(path, fs_error).permissions() & (std_fs::perms::owner_write && std_fs::perms::owner_write)
    // && (!fs_error);
    return false;
}


[[maybe_unused]]
inline auto is_path_write_allowed(const std_fs::path& path) -> bool
{
    auto fs_result{false};
    if (is_exists(path)) {
        auto path_status = std_fs::status(path);
        if ((path_status.permissions() & std_fs::perms::owner_write) != std_fs::perms::none) {
            fs_result = true;
        }
    }

    return fs_result;
}

}    // namespace amd_work_bench::fs


// namespace alias
namespace wb_fs = amd_work_bench::fs;
namespace wb_fs_io = amd_work_bench::fs::io;

// type alias
using FileOpsMode_t = wb_fs_io::FileOps_t::FileMode_t;


#endif    //-- AMD_WORK_BENCH_FILE_SYSTEM_OPS_HPP
