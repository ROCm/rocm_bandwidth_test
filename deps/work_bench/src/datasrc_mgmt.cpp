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
 * Description: datasrc_mgmt.cpp
 *
 */


#include <awb/datasrc_mgmt.hpp>

#include <work_bench.hpp>
#include <awb/default_sets.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>


namespace amd_work_bench::datasource
{

namespace
{
// This is the content counter that will be used to generate unique content
auto g_content_counter = u32_t(0);

}    // namespace


DataSourceBase_t::DataSourceBase_t() : m_source_id(g_content_counter++)
{
}


DataSourceBase_t::~DataSourceBase_t()
{
    m_overlay_list.clear();
}


auto DataSourceBase_t::is_dumpable() const -> bool
{
    return true;
}


auto DataSourceBase_t::read(void* buffer, u64_t offset, u64_t size, bool is_overlay) -> void
{
    read_raw(buffer, (offset - get_base_address()), size);
    if (is_overlay) {
        apply_overlay(buffer, offset, size);
    }
}


auto DataSourceBase_t::write(const void* buffer, u64_t offset, u64_t size) -> void
{
    if (!is_writeable()) {
        return;
    }

    EventDataSourceModified::post(this, offset, size, static_cast<const uint8_t*>(buffer));
    stamp_it_used();
}


auto DataSourceBase_t::save() -> void
{
    if (!is_writeable()) {
        return;
    }

    EventDataSourceSaved::post(this);
}


auto DataSourceBase_t::save_as(const FSPath_t& file_path) -> void
{
    using namespace wb_literals;
    const auto is_overlay{true};

    wb_fs_io::FileOps_t save_file(file_path, FileOpsMode_t::CREATE);
    if (save_file.is_valid()) {
        auto data_buffer = DataStream_t(std::min<size_t>(2_MBytes, get_actual_size()), 0x00);
        auto data_size = size_t(0);

        for (auto current_offset = u64_t(0); current_offset < get_actual_size(); current_offset += data_size) {
            data_size = std::min<size_t>(data_buffer.size(), (get_actual_size() - current_offset));
            read(data_buffer.data(), (get_base_address() + current_offset), data_size, is_overlay);
            //  TODO:   Should we write it all or chunks based on data_size? For now, we write it all.
            //          We might need to add an overload for write_data based on size.
            //          save_file.write_data(data_buffer.data(), data_size);
            save_file.write_data(data_buffer.data());    //, data_size);
        }

        EventDataSourceSaved::post(this);
    }
}


auto DataSourceBase_t::insert(u64_t offset, u64_t size) -> void
{
    EventDataSourceAdded::post(this, offset, size);
    stamp_it_used();
}


auto DataSourceBase_t::remove(u64_t offset, u64_t size) -> void
{
    EventDataSourceErased::post(this, offset, size);
    stamp_it_used();
}


auto DataSourceBase_t::resize(u64_t new_size) -> bool
{
    auto is_resized{false};
    if (new_size >> 63) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR, "Data Source size is too large '{}'.", new_size);
        return is_resized;
    }

    is_resized = true;
    auto offset = static_cast<i64_t>(new_size - get_actual_size());
    if (offset > 0) {
        EventDataSourceAdded::post(this, get_actual_size(), offset);
    } else if (offset < 0) {
        EventDataSourceErased::post(this, (get_actual_size() + offset), -offset);
    }

    stamp_it_used();
    return is_resized;
}


auto DataSourceBase_t::insert_raw(u64_t offset, u64_t size) -> void
{
    auto old_size = get_actual_size();
    const auto new_size = static_cast<u64_t>(old_size + size);
    resize_raw(new_size);

    // 4096 is the default buffer size
    constexpr auto kDefaultBufferSize = u64_t(0x1000);
    constexpr auto kBufferInitializer = u8_t(0x00);
    auto data_buffer = DataStream_t(kDefaultBufferSize, kBufferInitializer);
    auto zeroed_data_buffer = DataStream_t(kDefaultBufferSize, kBufferInitializer);

    auto actual_position = old_size;
    while (actual_position > offset) {
        const auto copy_size = std::min<size_t>((actual_position - offset), data_buffer.size());
        actual_position -= copy_size;
        read_raw(data_buffer.data(), actual_position, copy_size);
        write_raw(zeroed_data_buffer.data(), actual_position, (new_size - old_size));
        write_raw(data_buffer.data(), (actual_position + size), copy_size);
    }
}


auto DataSourceBase_t::remove_raw(u64_t offset, u64_t size) -> void
{
    if ((offset == 0) || (offset > get_actual_size())) {
        return;
    }

    if (get_actual_size() < (offset + size)) {
        size = (get_actual_size() - offset);
    }

    auto old_size = get_actual_size();
    const auto new_size = static_cast<u64_t>(old_size - size);

    // 4096 is the default buffer size
    constexpr auto kDefaultBufferSize = u64_t(0x1000);
    auto data_buffer = DataStream_t(kDefaultBufferSize);

    auto actual_position = offset;
    while (actual_position < new_size) {
        const auto copy_size = std::min<size_t>((new_size - actual_position), data_buffer.size());
        read_raw(data_buffer.data(), (actual_position + size), copy_size);
        write_raw(data_buffer.data(), actual_position, copy_size);
        actual_position += copy_size;
    }

    resize_raw(new_size);
}


auto DataSourceBase_t::apply_overlay(void* buffer, u64_t offset, u64_t size) const -> void
{
    for (const auto& overlay : m_overlay_list) {
        auto overlay_offset = overlay->get_address();
        auto overlay_size = overlay->get_data_size();

        auto overlap_min = i128_t(std::max(offset, overlay_offset));
        auto overlap_max = i128_t(std::min((offset + size), (overlay_offset + overlay_size)));
        if (overlap_max > overlap_min) {
            std::memcpy(static_cast<u8_t*>(buffer) + std::max<i128_t>(0, (overlap_min - offset)),
                        (overlay->get_data_stream().data() + std::max<i128_t>(0, (overlap_min - overlay_offset))),
                        (overlap_max - overlap_min));
        }
    }
}


auto DataSourceBase_t::delete_overlay(Overlay_t* overlay) -> void
{
    m_overlay_list.erase(std::remove_if(m_overlay_list.begin(),
                                        m_overlay_list.end(),
                                        [overlay](const auto& ovrly) {
                                            return ovrly.get() == overlay;
                                        }),
                         m_overlay_list.end());
}


auto DataSourceBase_t::new_overlay() -> Overlay_t*
{
    return m_overlay_list.emplace_back(std::make_unique<Overlay_t>()).get();
}


auto DataSourceBase_t::get_overlay_list() const -> const std::vector<std::unique_ptr<Overlay_t>>&
{
    return m_overlay_list;
}


auto DataSourceBase_t::get_page_size() const -> u64_t
{
    return m_page_size;
}


auto DataSourceBase_t::set_page_size(u64_t page_size) -> void
{
    if (page_size == 0) {
        return;
    }

    m_page_size = (page_size > get_max_page_size()) ? get_max_page_size() : page_size;
}


auto DataSourceBase_t::get_page_count() const -> u32_t
{
    return static_cast<u32_t>((get_actual_size() / get_page_size()) + (get_actual_size() % get_page_size() != 0 ? 1 : 0));
}


auto DataSourceBase_t::get_current_page() const -> u32_t
{
    return m_current_page;
}


auto DataSourceBase_t::set_current_page(u32_t page) -> void
{
    if (page < get_page_count()) {
        m_current_page = page;
    }
}


auto DataSourceBase_t::set_base_address(u64_t base_address) -> void
{
    m_base_address = base_address;
    stamp_it_used();
}


auto DataSourceBase_t::get_base_address() const -> u64_t
{
    return m_base_address;
}


auto DataSourceBase_t::get_current_page_address() const -> u64_t
{
    return (get_current_page() * get_page_size());
}


auto DataSourceBase_t::get_size() const -> u64_t
{
    return std::min<u64_t>(get_actual_size() - (get_page_size() * m_current_page), get_page_size());
}


auto DataSourceBase_t::get_page_holding_address(u64_t address) const -> std::optional<u32_t>
{
    auto page_number = static_cast<u32_t>(std::floor((address - get_base_address()) / static_cast<double>(get_page_size())));

    return (page_number < get_page_count()) ? std::make_optional(page_number) : std::nullopt;
}


auto DataSourceBase_t::get_source_description() const -> std::vector<SourceDescription_t>
{
    return {};
}


auto DataSourceBase_t::has_file_selector() const -> bool
{
    return false;
}


auto DataSourceBase_t::file_selector_handler() -> bool
{
    return false;
}


auto DataSourceBase_t::has_load_interface() const -> bool
{
    return false;
}


auto DataSourceBase_t::has_interface() const -> bool
{
    return false;
}


auto DataSourceBase_t::get_source_id() const -> u32_t
{
    return m_source_id;
}


auto DataSourceBase_t::set_source_id(u32_t source_id) -> void
{
    m_source_id = source_id;
    if (source_id > g_content_counter) {
        g_content_counter = ++source_id;
    }
}


}    // namespace amd_work_bench::datasource
