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
 * Description: plugin_amd_hello.cpp
 *
 */

#include <awb/plugins.hpp>

#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "plugin_amd_hello.hpp"


namespace amd_work_bench::plugin::hello
{

auto handle_hello_command(const WordList_t& args) -> void
{
    std::ignore = args;
    std::cout << "Hello from AMD ROCm Bandwidth Test Plugin: handle_hello_command()" << std::endl;
}

auto command_help_handler(const WordList_t& args) -> void
{
    std::ignore = args;
    std::cout << "Hello from AMD ROCm Bandwidth Test Plugin: command_help_handler()" << std::endl;
}

}    // namespace amd_work_bench::plugin::hello


namespace plugin_hello = amd_work_bench::plugin::hello;
using SubCmdType_t = wb::SubCommand_t::SubCommandType_t;

/*
AMD_WORK_BENCH_PLUGIN_SUBCOMMAND(){
    {"hello", "", "Prints a hello message", plugin_hello::handle_hello_command, SubCmdType_t::Option}
};
*/


AMD_WORK_BENCH_PLUGIN_SETUP("Hello", "Linux System Tools Team (MLSE Linux) @AMD", "Builtin: Hello", "0.0.1")
{
    using namespace plugin_hello;
}


/*
 * This is the main entry point for the plugin.
 * This function is called by the Work Bench Plugin Manager.
 * TODO: Implement the plugin_main() function, so that the plugin can be loaded by the Work Bench Plugin Manager.
 */
int32_t plugin_main(int argc, char** argv)
{
    // std::ignore = argc;
    // std::ignore = argv;
    std::cout << "Hello from AMD ROCm Bandwidth Test Plugin: plugin_main()" << std::endl;
    std::cout << "details::plugin_main(): \n";
    std::cout << "  - m_argc:" << argc << "\n";
    std::cout << "  - m_argv:" << argv << "\n";

    for (auto idx = 0; idx < argc; ++idx) {
        std::cout << "  - arg_list[" << idx << "]: " << argv[idx] << "\n";
    }
    std::cout << "\n\n";

    auto Hello = plugin_hello::Hello_t();
    Hello.say_hello("World");

    return 0;
}
