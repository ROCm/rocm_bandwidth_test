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
 * Description: http_ops.cpp
 *
 */


#include <awb/http_ops.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <curl/curl.h>

// #include <format>
#include <future>
#include <string>


namespace amd_work_bench::http
{

auto HTTPRequest_t::write_to_file(void* src_buff, size_t buff_size, size_t num_mem_blocks, void* tgt_buff) -> size_t
{
    //  TODO: Implement this function
    wb_types::disregard(src_buff);
    wb_types::disregard(tgt_buff);
    // auto& download_file = *static_cast<wb_fs_io::FileOps_t*>(tgt_buff);
    const auto buffer_size = (buff_size * num_mem_blocks);
    // download_file.write_buffer(static_cast<const u8_t*>(src_buff), buffer_size);

    return buffer_size;
}


}    // namespace amd_work_bench::http
