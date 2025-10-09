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
 * Description: view_transferbench.cpp
 *
 */

#include "view_transferbench.hpp"

#include <awb/content_mgmt.hpp>
#include <awb/datasrc_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/default_sets.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <filesystem>
// #include <format>
#include <iostream>


namespace amd_work_bench::plugin::transferbench
{

ViewTransferBench_t::ViewTransferBench_t() : DataViewBase_t::Window_t("View.TransferBench")
{
    EventDataSourceDeleted::subscribe(this, [this](const auto* data_source) {
        std::ignore = (this);
        std::ignore = data_source;
        // m_task_holder.get_progress_tracker();
    });
}

ViewTransferBench_t::~ViewTransferBench_t()
{
    EventDataSourceDeleted::unsubscribe(this);
}


auto ViewTransferBench_t::sketch_content() -> void
{
    // auto data_source = wb_api::datasource::get();

    /*
    m_console_messages = TaskManagement_t::create_background_task("sketch_content", [this]() {
        m_task_holder.get_progress_tracker().set_progress(0.0);
        m_task_holder.get_progress_tracker().set_status("Sketching content");

        m_task_holder.get_progress_tracker().set_progress(1.0);
        m_task_holder.get_progress_tracker().set_status("Content sketched");
    });
    */
}

}    // namespace amd_work_bench::plugin::transferbench
