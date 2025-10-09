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
 * Description: subcommands.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_SUBCOMMANDS_HPP)
#define AMD_WORK_BENCH_SUBCOMMANDS_HPP

#include <functional>
#include <string>
#include <vector>


namespace amd_work_bench::subcommands
{

using WordList_t = std::vector<std::string>;
using ForwardCommandHandler_t = std::function<void(const WordList_t&)>;

/*
 *  Note:   We use the following functions to register handlers, forward a given command (to whatever instance,
 *          this or another one). For callbacks, they should run after StartupDone() is done.
 *          Also, all the args received are checked and dispatched to the correct subcommand, or exit the
 *          program if/when needed such as; --version, --help options, which will not then return to the caller.
 */
auto register_subcommand(const std::string& cmd_name, const ForwardCommandHandler_t& cmd_handler) -> void;
auto forward_subcommand(const std::string& cmd_name, const WordList_t& args) -> void;
auto process_args(const WordList_t& args) -> int32_t;


}    // namespace amd_work_bench::subcommands


// namespace alias
namespace wb_subcommands = amd_work_bench::subcommands;


#endif    //-- AMD_WORK_BENCH_SUBCOMMANDS_HPP
