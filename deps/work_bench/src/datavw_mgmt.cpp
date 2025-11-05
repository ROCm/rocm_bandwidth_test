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
 * Description: datavw_mgmt.cpp
 *
 */


#include <awb/datavw_mgmt.hpp>
#include <work_bench.hpp>
#include <awb/common_utils.hpp>
#include <awb/work_bench_api.hpp>
#include <awb/datasrc_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace amd_work_bench::dataview
{

auto DataViewBase_t::should_sketch() const -> bool
{
    return (wb_api::datasource::is_valid() && wb_api::datasource::get()->is_available());
}

auto DataViewBase_t::should_process() const -> bool
{
    return (should_sketch());
}


auto DataViewBase_t::has_view_category_entry() const -> bool
{
    return true;
}


auto DataViewBase_t::get_name() const -> std::string
{
    return m_name;
}


}    // namespace amd_work_bench::dataview
