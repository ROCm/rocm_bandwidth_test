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
 * Description: cmdline_iface.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_CMDLINE_IFACE_HPP)
#define AMD_WORK_BENCH_CMDLINE_IFACE_HPP

#include <string>
#include <vector>


namespace amd_work_bench::plugin::builtin
{

using WordList_t = std::vector<std::string>;

auto command_register_forwarder() -> void;
auto command_version_handler(const WordList_t& args) -> void;
auto command_help_handler(const WordList_t& args) -> void;
auto command_list_plugins_handler(const WordList_t& args) -> void;
auto command_verbose_handler(const WordList_t& args) -> void;
auto command_open_handler(const WordList_t& args) -> void;
auto command_pcie_info_handler(const WordList_t& args) -> void;

// This is a dummy handler that does nothing
auto command_none_handler(const WordList_t& args) -> void;

}    // namespace amd_work_bench::plugin::builtin

#endif    //-- AMD_WORK_BENCH_CMDLINE_IFACE_HPP
