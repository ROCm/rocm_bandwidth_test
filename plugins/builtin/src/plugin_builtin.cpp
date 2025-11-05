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
 * Description: plugin_builtin.cpp
 *
 */

#include <awb/plugins.hpp>

#include "cmdline_iface.hpp"
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>


namespace amd_work_bench::plugin::builtin
{

auto register_event_handler() -> void;
auto create_welcome_banner() -> void;
auto startup_tasks() -> void;
auto register_main_category_entries() -> void;
auto register_data_sources() -> void;
auto register_views() -> void;


}    // namespace amd_work_bench::plugin::builtin


namespace wb = amd_work_bench;
namespace builtin_plugin = amd_work_bench::plugin::builtin;
using SubCmdType_t = wb::SubCommand_t::SubCommandType_t;

/*
AMD_WORK_BENCH_PLUGIN_SUBCOMMAND(){
    {"builtin-help",      "bh", "Print help about this command", builtin_plugin::command_help_handler, SubCmdType_t::Option},
    {"builtin-version",   "",   "Print AMD-Work-Bench version",  builtin_plugin::command_version_handler, SubCmdType_t::Option},
    {"builtin-plug-list", "",   "Lists all available plugins",   builtin_plugin::command_list_plugins_handler,
SubCmdType_t::Option},
    {"builtin-verbose",   "bv", "Enables verbose debug logging", builtin_plugin::command_verbose_handler, SubCmdType_t::Option},
};
*/

AMD_WORK_BENCH_PLUGIN_SUBCOMMAND(){
    {"builtin-help", "", "Print help about this command",                        builtin_plugin::command_help_handler, SubCmdType_t::Option},
    {"pcie-info",
     "",                 "Display PCIE Print Spec info help about this command",
     builtin_plugin::command_pcie_info_handler,
     SubCmdType_t::Option                                                                                                                  },
};

AMD_WORK_BENCH_PLUGIN_SETUP("Built-in",
                            "Linux System Tools Team (MLSE Linux) @AMD",
                            "Builtin: options for AMD ROCm Bandwidth Test",
                            "1.0.0")
{
    using namespace builtin_plugin;

    startup_tasks();
    register_main_category_entries();
    register_event_handler();
    create_welcome_banner();
    command_register_forwarder();
    register_data_sources();
    register_views();

}    //  AMD_WORK_BENCH_PLUGIN_SETUP
