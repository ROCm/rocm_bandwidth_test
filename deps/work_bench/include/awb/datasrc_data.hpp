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
 * Description: datasrc_data.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_DATASRC_DATA_HPP)
#define AMD_WORK_BENCH_DATASRC_DATA_HPP

#include <awb/work_bench_api.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>


namespace amd_work_bench
{

namespace datasource
{
class DataSourceBase_t;
}    // namespace datasource


template<typename DataSourceTp>
class DataSourceServices_t
{
    public:
        DataSourceServices_t()
        {
            on_create();
        };
        DataSourceServices_t(const DataSourceServices_t&) = delete;
        DataSourceServices_t& operator=(const DataSourceServices_t&) = delete;
        DataSourceServices_t(DataSourceServices_t&&) = delete;
        DataSourceServices_t& operator=(DataSourceServices_t&&) = delete;
        ~DataSourceServices_t()
        {
            on_destroy();
        };

        DataSourceTp* operator->()
        {
            return &get();
        }

        const DataSourceTp* operator->() const
        {
            return &get();
        }

        DataSourceTp& operator*()
        {
            return get();
        }

        const DataSourceTp& operator*() const
        {
            return get();
        }

        DataSourceServices_t& operator=(const DataSourceTp& data)
        {
            set(data);
            return *this;
        }

        DataSourceServices_t& operator=(DataSourceTp&& data)
        {
            set(std::move(data));
            return *this;
        }

        operator DataSourceTp&()
        {
            return get();
        }

        DataSourceTp& get(const datasource::DataSourceBase_t* data_source = amd_work_bench::api::datasource::get())
        {
            return m_data[data_source];
        }

        const DataSourceTp& get(const datasource::DataSourceBase_t* data_source = amd_work_bench::api::datasource::get()) const
        {
            return m_data.at(data_source);
        }

        auto set(const DataSourceTp& data,
                 const datasource::DataSourceBase_t* data_source = amd_work_bench::api::datasource::get()) -> void
        {
            m_data[data_source] = data;
        }

        auto set(DataSourceTp&& data,
                 const datasource::DataSourceBase_t* data_source = amd_work_bench::api::datasource::get()) -> void
        {
            m_data[data_source] = std::move(data);
        }

        auto all_values()
        {
            // Returns only the values (pair::second) of the map. std::views::keys returns (pair::first)
            return (m_data | std::views::values);
        }

        auto set_on_create_cb(std::function<void(datasource::DataSourceBase_t*, DataSourceTp&)> on_create_cb) -> void
        {
            m_on_create_cb = std::move(on_create_cb);
        }

        auto set_on_destroy_cb(std::function<void(datasource::DataSourceBase_t*, DataSourceTp&)> on_destroy_cb) -> void
        {
            m_on_destroy_cb = std::move(on_destroy_cb);
        }


    private:
        std::map<const datasource::DataSourceBase_t*, DataSourceTp> m_data{};
        std::function<void(datasource::DataSourceBase_t*, DataSourceTp&)> m_on_create_cb{};
        std::function<void(datasource::DataSourceBase_t*, DataSourceTp&)> m_on_destroy_cb{};

        auto on_create() -> void
        {
            EventDataSourceOpened::subscribe(this, [this](datasource::DataSourceBase_t* data_source) {
                auto [elem_itr, is_elem_inserted] = m_data.emplace(data_source, DataSourceTp());
                auto& [key, value] = *elem_itr;
                if (m_on_create_cb) {
                    m_on_create_cb(data_source, value);
                }
            });

            EventDataSourceDeleted::subscribe(this, [this](datasource::DataSourceBase_t* data_source) {
                if (auto data_source_itr = m_data.find(data_source); data_source_itr != m_data.end()) {
                    if (m_on_destroy_cb) {
                        m_on_destroy_cb(data_source, m_data.at(data_source));
                    }
                    m_data.erase(data_source_itr);
                }
            });

            MoveDataSourceData::subscribe(
                this,
                [this](datasource::DataSourceBase_t* old_data_source, datasource::DataSourceBase_t* new_data_source) {
                    //  From the old data source, get the value and remove it from the container, and make sure the value is not
                    //  empty
                    auto old_elem = m_data.extract(old_data_source);
                    if (old_elem.empty()) {
                        return;
                    }

                    //  From the new data source, erase the value we want to replace with
                    m_data.erase(new_data_source);

                    //  Insert the old value into the new data source
                    m_data.insert(std::move(old_elem));
                });
        }

        auto on_destroy() -> void
        {
            EventDataSourceOpened::unsubscribe(this);
            EventDataSourceDeleted::unsubscribe(this);
            EventAWBClosing::unsubscribe(this);
            MoveDataSourceData::unsubscribe(this);
        }
};

}    // namespace amd_work_bench

#endif    //-- AMD_WORK_BENCH_DATASRC_DATA_HPP
