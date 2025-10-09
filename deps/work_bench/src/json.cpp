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
 * Description: json.cpp
 *
 */


#include <awb/json.hpp>


namespace amd_work_bench::json
{

namespace
{

//  TODO:   We might need to change this to a more generic approach, like what we did with
//          logger::compose_logger_name_info() and logger::setup_base_logger
std::string awb_json_file_name{(kJSON_FILE_APPLICATION_PREFIX + "." + kJSON_FILE_EXTENSION)};
std::string awb_json_file_path{kJSON_FILE_APPLICATION_PATH};
std_fs::path awb_json_file_fspath{(awb_json_file_path + "/" + awb_json_file_name)};
JSon_t awb_json_data{};

}    // namespace


namespace details
{

auto get_json_file_name() -> const std::string&
{
    return awb_json_file_name;
}

auto get_json_file_path() -> const std::string&
{
    return awb_json_file_path;
}

auto get_file_fs_path() -> const std_fs::path&
{
    return awb_json_file_fspath;
}

auto get_json_data(const std_fs::path& path) -> const JSon_t&
{
    //  TODO: Implement this function
    wb_types::disregard(path);
    return awb_json_data;
}


}    // namespace details


auto JSonData_t::load(const std_fs::path& path) -> void
{
    //  TODO: Implement this function
    wb_types::disregard(path);

    return;
}


auto JSonData_t::setup_file_stream() -> void
{
    return;
}


auto JSonData_t::save(const std_fs::path& path) -> void
{
    //  TODO: Implement this function
    wb_types::disregard(path);
    return;
}


auto JSonData_t::print() -> void
{
    return;
}


auto JSonData_t::get() -> JSon_t&
{
    return m_json_data;
}


}    // namespace amd_work_bench::json
