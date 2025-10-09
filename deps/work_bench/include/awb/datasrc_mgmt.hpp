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
 * Description: datasrc_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_DATASRC_HPP)
#define AMD_WORK_BENCH_DATASRC_HPP

#include <work_bench.hpp>
#include <awb/concepts.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>


namespace amd_work_bench::datasource
{

class Overlay_t
{
    public:
        Overlay_t() = default;

        [[nodiscard]] auto get_address() const -> u64_t
        {
            return m_address;
        }

        auto set_address(u64_t address) -> void
        {
            m_address = address;
        }

        [[nodiscard]] auto get_data_stream() const -> const DataStream_t&
        {
            return m_data_stream;
        }

        [[nodiscard]] auto get_data_size() const -> u64_t
        {
            return m_data_stream.size();
        }


    protected:
        //

    private:
        u64_t m_address{0};
        DataStream_t m_data_stream{0};
};


/*
 *  This is a interface class that defines the basic operations that a data source should implement.
 */

static constexpr auto kDefaultMaxPageSize = u64_t(0xFFFF'FFFF'FFFF'FFFF);


class DataSourceBase_t
{
    public:
        constexpr auto get_max_page_size() const noexcept -> u64_t
        {
            return kDefaultMaxPageSize;
        }

        struct OptionEntry_t
        {
            public:
                std::string m_name{};
                std::function<void()> m_function_cb;

            private:
        };

        struct SourceDescription_t
        {
            public:
                std::string m_name{};
                std::string m_value{};

            private:
        };


        DataSourceBase_t();
        virtual ~DataSourceBase_t();
        DataSourceBase_t(const DataSourceBase_t&) = delete;
        DataSourceBase_t& operator=(const DataSourceBase_t&) = delete;
        DataSourceBase_t(DataSourceBase_t&& data_source) noexcept = default;
        DataSourceBase_t& operator=(DataSourceBase_t&& data_source) noexcept = default;

        [[nodiscard]] virtual auto open() -> bool = 0;
        [[nodiscard]] virtual auto is_available() const -> bool = 0;
        [[nodiscard]] virtual auto is_readable() const -> bool = 0;
        [[nodiscard]] virtual auto is_writeable() const -> bool = 0;
        [[nodiscard]] virtual auto is_resizable() const -> bool = 0;
        [[nodiscard]] virtual auto is_saveable() const -> bool = 0;
        [[nodiscard]] virtual auto is_dumpable() const -> bool;

        //  If is_overlay is false, same as read_raw
        auto read(void* buffer, u64_t offset, u64_t size, bool is_overlay = true) -> void;
        auto write(const void* buffer, u64_t offset, u64_t size) -> void;

        //  Read/Write not applying overlay
        virtual auto read_raw(void* buffer, u64_t offset, u64_t size) -> void = 0;
        virtual auto write_raw(const void* buffer, u64_t offset, u64_t size) -> void = 0;

        [[nodiscard]] virtual auto get_actual_size() const -> u64_t = 0;
        [[nodiscard]] virtual auto get_type_name() const -> std::string = 0;
        [[nodiscard]] virtual auto get_name() const -> std::string = 0;

        auto insert(u64_t offset, u64_t size) -> void;
        auto remove(u64_t offset, u64_t size) -> void;
        auto resize(u64_t new_size) -> bool;

        virtual auto insert_raw(u64_t offset, u64_t size) -> void;
        virtual auto remove_raw(u64_t offset, u64_t size) -> void;
        virtual auto resize_raw(u64_t new_size) -> void
        {
            //  For compiler warnings
            std::ignore = new_size;
        }

        virtual auto close() -> void = 0;
        virtual auto save() -> void;
        virtual auto save_as(const FSPath_t& file_path) -> void;

        [[nodiscard]] auto new_overlay() -> Overlay_t*;
        auto apply_overlay(void* buffer, u64_t offset, u64_t size) const -> void;
        auto delete_overlay(Overlay_t* overlay) -> void;
        [[nodiscard]] auto get_overlay_list() const -> const std::vector<std::unique_ptr<Overlay_t>>&;

        [[nodiscard]] auto get_page_size() const -> u64_t;
        auto set_page_size(u64_t page_size) -> void;
        [[nodiscard]] auto get_page_count() const -> u32_t;
        [[nodiscard]] auto get_current_page() const -> u32_t;
        auto set_current_page(u32_t page_number) -> void;

        virtual auto set_base_address(u64_t base_address) -> void;
        [[nodiscard]] virtual auto get_base_address() const -> u64_t;
        [[nodiscard]] virtual auto get_current_page_address() const -> u64_t;
        [[nodiscard]] virtual auto get_size() const -> u64_t;
        [[nodiscard]] virtual auto get_page_holding_address(u64_t address) const -> std::optional<u32_t>;
        [[nodiscard]] virtual auto get_source_description() const -> std::vector<SourceDescription_t>;
        //[[nodiscard]] virtual auto query_information(const std::string& category_name, const std::string&
        // category_arg) -> std::variant<std::string, i128_t>;

        [[nodiscard]] virtual auto has_file_selector() const -> bool;
        [[nodiscard]] virtual auto file_selector_handler() -> bool;

        virtual auto get_option_entries() -> std::vector<OptionEntry_t>
        {
            return {};
        }

        [[nodiscard]] virtual auto has_load_interface() const -> bool;
        [[nodiscard]] virtual auto has_interface() const -> bool;
        // virtual auto draw_load_interface() -> bool;
        // virtual auto draw_interface() -> void;

        [[nodiscard]] auto get_source_id() const -> u32_t;
        auto set_source_id(u32_t source_id) -> void;

        //[[nodiscard]] virtual auto store_settings(json::JSon_t settings) const -> json::JSon_t;
        // virtual auto load_settings(const json::JSon_t& settings) -> void;

        auto stamp_it_used(bool is_used = true) -> void
        {
            m_is_used = is_used;
        }

        [[nodiscard]] auto is_used() const -> bool
        {
            return m_is_used;
        }

        auto set_skip_load_interface() -> void
        {
            m_is_skip_load_interface = true;
        }

        [[nodiscard]] auto is_skip_load_interface() const -> bool
        {
            return m_is_skip_load_interface;
        }

        [[nodiscard]] auto get_error_message() const -> const std::string&
        {
            return m_error_message;
        }

        auto set_error_message(const std::string& error_message) -> void
        {
            m_error_message = error_message;
        }


    protected:
        //  If stamped true, it has changes not yet saved
        bool m_is_used{false};
        bool m_is_skip_load_interface{false};
        u32_t m_source_id{0};
        u32_t m_current_page{0};
        u64_t m_base_address{0};
        u64_t m_page_size = get_max_page_size();
        std::vector<std::unique_ptr<Overlay_t>> m_overlay_list{};
        std::string m_error_message{"Error: DataSourceBase() undefined error message"};


    private:
        //
};

}    // namespace amd_work_bench::datasource


// namespace alias
namespace wb_datasource = amd_work_bench::datasource;

#endif    //-- AMD_WORK_BENCH_DATASRC_HPP
