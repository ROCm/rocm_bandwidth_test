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
 * Description: datavw_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_DATAVW_HPP)
#define AMD_WORK_BENCH_DATAVW_HPP

#include <work_bench.hpp>
#include <awb/common_utils.hpp>
#include <awb/work_bench_api.hpp>
#include <awb/datasrc_mgmt.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <algorithm>
#include <functional>
#include <thread>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>


namespace amd_work_bench::dataview
{


class DataViewBase_t
{
    public:
        virtual ~DataViewBase_t() = default;
        virtual auto sketch() -> void = 0;
        virtual auto sketch_content() -> void = 0;
        virtual auto sketch_visible_content_always() -> void {};
        [[nodiscard]] virtual auto should_sketch() const -> bool;
        [[nodiscard]] virtual auto should_process() const -> bool;
        [[nodiscard]] virtual auto has_view_category_entry() const -> bool;
        [[nodiscard]] auto get_name() const -> std::string;

        //
        class Window_t;
        class Special_t;


    private:
        explicit DataViewBase_t(const std::string& view_name) : m_name(std::move(view_name)){};
        std::string m_name;
};


class DataViewBase_t::Window_t : public DataViewBase_t
{
    public:
        explicit Window_t(const std::string& view_name) : DataViewBase_t(std::move(view_name)){};
        auto sketch() -> void final
        {
            if (should_sketch()) {
                sketch_content();
            }
        }

    private:
        //
};


class DataViewBase_t::Special_t : public DataViewBase_t
{
    public:
        explicit Special_t(const std::string& view_name) : DataViewBase_t(std::move(view_name)){};
        auto sketch() -> void final
        {
            if (should_sketch()) {
                sketch_content();
            }
        }

    private:
        //
};

}    // namespace amd_work_bench::dataview


// namespace alias
namespace wb_dataview = amd_work_bench::dataview;


#endif    //-- AMD_WORK_BENCH_DATAVW_HPP
