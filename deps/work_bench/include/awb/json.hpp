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
 * Description: json.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_JSON_HPP)
#define AMD_WORK_BENCH_JSON_HPP

#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <filesystem>
// #include <format>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>


namespace amd_work_bench::json
{

namespace json = nlohmann;
using JSon_t = json::json;

static auto kJSON_FILE_APPLICATION_PREFIX(std::string("amd-workbench"));
static auto kJSON_FILE_DEBUG_POSTFIX(std::string("debug"));
static auto kJSON_FILE_PLUGIN_POSTFIX(std::string("plugin"));
static auto kJSON_FILE_EXTENSION(std::string("json"));
static auto kJSON_FILE_APPLICATION_PATH(std::string("./work_bench_info/log"));


namespace std_fs = std::filesystem;
namespace fs = amd_work_bench::fs;
namespace io = amd_work_bench::fs::io;


namespace details
{

auto get_json_file_name() -> const std::string&;
auto get_json_file_path() -> const std::string&;
auto get_file_fs_path() -> const std_fs::path&;
auto get_json_data(const std_fs::path& path) -> const JSon_t&;

}    // namespace details


class JsonBase_t
{
    public:
        JsonBase_t() = default;
        JsonBase_t(const std_fs::path& path, io::FileOps_t::FileMode_t mode) : m_path(path), m_mode(mode)
        {
            // setup_file_stream();
        }

        ~JsonBase_t() = default;
        // virtual auto setup_file_stream() -> void = 0;
        virtual auto setup_file_stream() -> void {};


    protected:
        std_fs::path m_path{};
        io::FileOps_t::FileMode_t m_mode{};
        io::FileOps_t m_file_ops{};
        std::fstream m_file_stream{};
        JSon_t m_json_data{};

    private:
};


class JSonData_t final : public JsonBase_t
{
    public:
        JSonData_t() = default;
        JSonData_t(const std_fs::path& path, io::FileOps_t::FileMode_t mode) : JsonBase_t(path, mode)
        {
            setup_file_stream();
        }
        ~JSonData_t() = default;

        JSonData_t(const JSonData_t&) = delete;
        JSonData_t& operator=(const JSonData_t&) = delete;
        JSonData_t(JSonData_t&&) = delete;
        JSonData_t& operator=(JSonData_t&&) = delete;

        virtual auto setup_file_stream() -> void override;    // {};

        auto load(const std_fs::path& path) -> void;
        auto save(const std_fs::path& path) -> void;
        auto print() -> void;
        auto get() -> JSon_t&;

    protected:
        //

    private:
};


}    // namespace amd_work_bench::json


// namespace alias
namespace wb_json = amd_work_bench::json;


#endif    //-- AMD_WORK_BENCH_JSON_HPP
