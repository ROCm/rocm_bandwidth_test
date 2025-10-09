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
 * Description: awb_main.cpp
 *
 */

#include "crash_mgmt.hpp"
#include "startup_mgmt.hpp"
#include "msg_mgmt.hpp"

#include <work_bench.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/task_mgmt.hpp>
#include <awb/linux_utils.hpp>

#include <iostream>
// #include <format>


namespace amd_work_bench::startup
{
auto run_work_bench() -> u32_t;
auto run_command_line(int argc, char** argv) -> void;
auto show_help() -> void;

}    // namespace amd_work_bench::startup


/*
 *  Note:   Main entry point for the workbench.
 */
int main(int argc, char** argv)
{
    using namespace amd_work_bench;

    TaskManagement_t::set_current_task_name("main");
    wb_crash::setup_crash_handler();
    wb_linux::startup_native();

    //  Messaging system setup so commands can be sent to the main amd_work_bench instance
    wb_messaging::setup_messaging();

    /*
     *  Send the information to the command-line parser.
     */
    // Run the command line parser.
    // if (argc <= 1) {
    //    wb_startup::show_help();
    //    std::exit(EXIT_SUCCESS);
    //} else if (argc > 1) {
    wb_startup::run_command_line(argc, argv);
    //}


    /*
     *  Send information to the log system.
     */
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "AMD ROCm Bandwidth started. {}", "...");

    /*
     *  TODO:   Add more information to the log system.
     *          - Version, Branch, Commit,
     *          - OS name, OS version, Architecture
     *          - Linux Distro, etc.
     *          - GPU information
     */
    wb_logger::loginfo(LogLevel::LOGGER_INFO,
                       "AMD ROCm Bandwidth version: {} [Commit: {} / Branch: {} / Build Type: {}]",
                       wb_api_system::get_work_bench_version(),
                       wb_api_system::get_work_bench_commit_hash(),
                       wb_api_system::get_work_bench_commit_branch(),
                       wb_api_system::get_work_bench_build_type());

    wb_logger::loginfo(LogLevel::LOGGER_INFO,
                       "Environment Info: \n -> Kernel: {} \n -> OS: {}",
                       wb_api_system::get_os_kernel_info(),
                       wb_api_system::get_os_distro_info());

    /*
     *  Start the workbench.
     *  We can run it from the wb_startup::run_work_bench() function after parsing the command line.
     */
    // return wb_startup::run_work_bench();
    return (EXIT_SUCCESS);
}
