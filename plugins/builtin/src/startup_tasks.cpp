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
 * Description: startup_tasks.cpp
 *
 */


#include <awb/work_bench_api.hpp>

#include <awb/default_sets.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/http_ops.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <string>


namespace amd_work_bench::plugin::builtin
{

namespace
{

using namespace std::string_literals;


auto load_terminal_settings() -> bool
{
    return true;
}


auto configure_terminal_environment() -> bool
{
    return true;
}


auto run_update_checking() -> bool
{
    // TaskManagement_t::create_background_task();
    return true;
}

}    // namespace


auto startup_tasks() -> void
{
    const auto is_async{true};
    const auto is_not_async{!is_async};
    api::system::add_startup_task("Builtin::StartupTasks", load_terminal_settings, is_not_async);
    api::system::add_startup_task("Builtin::StartupTasks", configure_terminal_environment, is_not_async);
    api::system::add_startup_task("Builtin::StartupTasks", run_update_checking, is_async);
}


}    // namespace amd_work_bench::plugin::builtin
