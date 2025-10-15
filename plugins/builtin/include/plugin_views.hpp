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
 * Description: plugin_views.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGIN_VIEWS_HPP)
#define AMD_WORK_BENCH_PLUGIN_VIEWS_HPP

#include <awb/content_mgmt.hpp>
#include <awb/datasrc_mgmt.hpp>
#include <awb/datavw_mgmt.hpp>

#include <map>
#include <string>
#include <vector>


namespace amd_work_bench::plugin::builtin
{

class ViewTools_t : public wb_dataview::DataViewBase_t::Window_t
{
    public:
        ViewTools_t();
        ~ViewTools_t() override = default;

        auto sketch_content() -> void override;
        auto sketch_visible_content_always() -> void override;

    private:
        std::vector<wb_content::tools::details::ToolEntry_t>::const_iterator m_tool_entry_itr;
};


class ViewCommandBoard_t : public wb_dataview::DataViewBase_t::Special_t
{
    public:
        ViewCommandBoard_t();
        ~ViewCommandBoard_t() override = default;

        auto sketch_content() -> void override {};
        auto sketch_visible_content_always() -> void override;

        auto should_sketch() const -> bool override
        {
            return false;
        }

        auto should_process() const -> bool override
        {
            return true;
        }

        auto has_view_category_entry() const -> bool override
        {
            return false;
        }


    private:
        struct CommandResult_t
        {
            public:
                std::string m_result{};
                std::string m_command{};
                wb_content::board_commands::details::ExecuteCallback_t m_execute_cb{nullptr};
                //

            private:
        };

        enum class CommandMatchingType_t
        {
            kNO_MATCH,
            kINFO_MATCH,
            kPARTIAL_MATCH,
            kFULL_MATCH,
        };

        bool m_is_command_board_open{false};
        std::string m_command_buffer{};
        std::string m_full_results{};
        std::vector<CommandResult_t> m_command_results{};
        auto get_command_results(const std::string& input_command) -> std::vector<CommandResult_t>;
};

class ViewDataSources_t : public wb_datasource::DataSourceBase_t
{
    public:
        ViewDataSources_t() = default;
        ~ViewDataSources_t() override = default;

        [[nodiscard]] auto open() -> bool override;
        [[nodiscard]] auto is_available() const -> bool override;
        [[nodiscard]] auto is_readable() const -> bool override;
        [[nodiscard]] auto is_writeable() const -> bool override;
        [[nodiscard]] auto is_resizable() const -> bool override;
        [[nodiscard]] auto is_saveable() const -> bool override;
        auto close() -> void override;
        auto save() -> void override;

        auto read_raw(void* buffer, u64_t offset, u64_t size) -> void override;
        auto write_raw(const void* buffer, u64_t offset, u64_t size) -> void override;
        auto insert_raw(u64_t offset, u64_t size) -> void override;
        auto remove_raw(u64_t offset, u64_t size) -> void override;
        auto resize_raw(u64_t new_size) -> void override;

        [[nodiscard]] auto get_actual_size() const -> u64_t override;
        [[nodiscard]] auto get_type_name() const -> std::string override;
        [[nodiscard]] auto get_name() const -> std::string override;
        [[nodiscard]] auto get_source_description() const -> std::vector<SourceDescription_t> override;
        auto set_data_source(wb_datasource::DataSourceBase_t* data_source, u64_t address, size_t size) -> void;
        auto set_name(const std::string& name) -> void;
        auto get_option_entries() -> std::vector<OptionEntry_t> override;


    private:
        u64_t m_start_address{0};
        size_t m_size{0};
        std::string m_name{};
        wb_datasource::DataSourceBase_t* m_data_source{nullptr};
        auto rename_file(const std::string& new_name) -> void;
};


}    // namespace amd_work_bench::plugin::builtin

#endif    //-- AMD_WORK_BENCH_PLUGIN_VIEWS_HPP
