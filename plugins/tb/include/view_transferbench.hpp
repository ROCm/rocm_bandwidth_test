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
 * Description: view_transferbench.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGIN_VIEW_TRANSFERBENCH_HPP)
#define AMD_WORK_BENCH_PLUGIN_VIEW_TRANSFERBENCH_HPP


#include <work_bench.hpp>
#include <awb/datavw_mgmt.hpp>
#include <awb/datasrc_data.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/task_mgmt.hpp>

#include <iostream>
#include <vector>
#include <string>


namespace amd_work_bench::plugin::transferbench
{

class ViewTransferBench_t : public wb_dataview::DataViewBase_t::Window_t
{
    public:
        explicit ViewTransferBench_t();
        ~ViewTransferBench_t() override;
        auto sketch_content() -> void override;


    private:
        TaskHolder_t m_task_holder{};
        DataSourceServices_t<std::vector<std::string>> m_console_messages{};
};


}    // namespace amd_work_bench::plugin::transferbench


#endif    // AMD_WORK_BENCH_PLUGIN_VIEW_TRANSFERBENCH_HPP
