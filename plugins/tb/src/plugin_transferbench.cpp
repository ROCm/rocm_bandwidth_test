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
 * Description: plugin_transferbench.cpp
 *
 */

#include <awb/plugins.hpp>

#include <awb/content_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <plugin_transferbench.hpp>
#include <view_transferbench.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>


namespace amd_work_bench::plugin::transferbench
{

/*
 *  TODO:   Use this structure so we can define/write unit tests for the plugin.
 */

auto command_run_handler(const WordList_t& args) -> i32_t
{
    std::ignore = args;
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "Plugin: '{}' ", kPLUGIN_MODULE_NAME);
    return 100;
}


auto register_plugin_view() -> void
{
    amd_work_bench::content::views::add_view<ViewTransferBench_t>();
}

}    // namespace amd_work_bench::plugin::transferbench


AMD_WORK_BENCH_PLUGIN_SETUP("tb", "Linux System Tools Team (MLSE Linux) @AMD", "Builtin: TransferBench", "1.0.0")
{
    using namespace wb_plugin_tb;
    register_plugin_view();
}


/*
 *  This is the main entry point for the plugin, on the running code side.
 *  This is the main(int argc, char** argv) function renamed, so it can keep its functionalities and make it easier
 *  to transition existing static executables to dynamic (plugin) build.
 */
extern int plugin_main_entry(int argc, char** argv);

/*
 * This is the main entry point for the plugin.
 * This function is called by the Work Bench Plugin Manager.
 * TODO: Implement the plugin_main() function, so that the plugin can be loaded by the Work Bench Plugin Manager.
 */
int32_t plugin_main(int argc, char** argv)
{
    /*  NOTE:   This block is used for debugging purposes. It will be removed
     *          once the plugin is fully functional.

    // std::ignore = argc;
    // std::ignore = argv;
    std::cout << "Hello from TransferBench Plugin: plugin_main()" << "\n";

    std::cout << "details::plugin_main(): \n";
    std::cout << "  - m_argc:" << argc << "\n";
    std::cout << "  - m_argv:" << argv << "\n";

    for (auto idx = 0; idx < argc; ++idx) {
        std::cout << "  - arg_list[" << idx << "]: " << argv[idx] << "\n";
    }
    std::cout << "\n\n";
    */

    // Call the plugin_main_entry() function, which is the main entry point for the plugin.
    auto plugin_run_result = plugin_main_entry(argc, argv);
    // std::cout << "plugin_main_entry() returned: " << test << "\n\n";
    return plugin_run_result;
}
