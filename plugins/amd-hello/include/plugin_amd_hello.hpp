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
 * Description: amd_hello.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGIN_EXTERNAL_HELLO_HPP)
#define AMD_WORK_BENCH_PLUGIN_EXTERNAL_HELLO_HPP


#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <work_bench.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <iostream>
// #include <format>
#include <string>


namespace amd_work_bench::plugin::hello
{

class Hello_t
{
    public:
        Hello_t() = default;
        ~Hello_t() = default;

        void say_hello(const std::string& name)
        {
            auto hello_message = amd_fmt::format("== Hello '{}'! From AMD Hello Plugin. ==\n", name);
            std::cout << hello_message << "\n";
        }
};

}    // namespace amd_work_bench::plugin::hello

#endif    // AMD_WORK_BENCH_PLUGIN_EXTERNAL_HELLO_HPP
