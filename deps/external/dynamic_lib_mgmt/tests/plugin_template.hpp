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
 * Description: plugin_template.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGIN_TEMPLATE_HPP)
#define AMD_WORK_BENCH_PLUGIN_TEMPLATE_HPP

#include "dynlib_mgmt.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using u8_t = std::uint8_t;
using u16_t = std::uint16_t;
using u32_t = std::uint32_t;
using u64_t = std::uint64_t;
using u128_t = __uint128_t;

using i8_t = std::int8_t;
using i16_t = std::int16_t;
using i32_t = std::int32_t;
using i64_t = std::int64_t;
using i128_t = __int128_t;

using FSPath_t = std::filesystem::path;


namespace amd_work_bench::plugin
{

struct Feature_t
{
    public:
        bool m_is_enabled{};
        std::string m_name{};

    private:
};

struct SubCommand_t
{
    public:
        enum class SubCommandType_t : u8_t
        {
            Option,
            SubCommand
        };

        std::string m_long_format{};
        std::string m_short_format{};
        std::string m_description{};
        std::function<void(const std::vector<std::string>&)> m_function_cb;
        SubCommandType_t m_subcmd_type = SubCommandType_t::Option;

    private:
};

struct PluginFunctionality_t
{
    public:
        using PluginInitFunction_t = void (*)();
        using PluginGetNameFunction_t = const char* (*)();
        using PluginGetAuthorFunction_t = const char* (*)();
        using PluginGetDescriptionFunction_t = const char* (*)();
        using PluginGetCompatibilityFunction_t = const char* (*)();
        using PluginGetSubCommandFunction_t = void* (*)();
        using PluginGetFeatureFunction_t = void* (*)();
        using LibraryInitFunction_t = void (*)();
        using LibraryGetNameFunction_t = const char* (*)();
        using PluginMainEntryPoint_v1_t = int (*)(int, char**);

        PluginInitFunction_t m_plugin_init_function = nullptr;
        PluginGetNameFunction_t m_plugin_get_name_function = nullptr;
        PluginGetAuthorFunction_t m_plugin_get_author_function = nullptr;
        PluginGetDescriptionFunction_t m_plugin_get_description_function = nullptr;
        PluginGetCompatibilityFunction_t m_plugin_get_compatibility_function = nullptr;
        PluginGetSubCommandFunction_t m_plugin_get_subcommand_function = nullptr;
        PluginGetFeatureFunction_t m_plugin_get_feature_function = nullptr;

        LibraryInitFunction_t m_library_init_function = nullptr;
        LibraryGetNameFunction_t m_library_get_name_function = nullptr;

    private:
};

struct PluginData_t
{
    public:
        int m_argc{};
        char** m_argv{};
        std::function<int((int, char**))> m_entry_function_cb{};

    private:
};

class PluginIface_t
{
    public:
        PluginIface_t() = default;
        virtual ~PluginIface_t() = default;
        virtual auto plugin_init() const -> bool = 0;
        virtual auto plugin_get_name() const -> std::string = 0;
        virtual auto plugin_get_author() const -> std::string = 0;
        virtual auto plugin_get_description() const -> std::string = 0;
        virtual auto plugin_get_compatibility() const -> std::string = 0;
        virtual auto plugin_get_subcommand() const -> std::span<SubCommand_t> = 0;
        virtual auto plugin_get_feature() const -> std::span<Feature_t> = 0;
        virtual auto plugin_main(PluginData_t& plugin_data) const -> i32_t = 0;
};


class Plugin_t : public PluginIface_t
{
    public:
        Plugin_t() = default;
        explicit Plugin_t(const FSPath_t& library_path) : m_library_path(library_path)
        {
            std::cout << "Plugin_t::Plugin_t()" << "\n";
        }

        explicit Plugin_t(const std::string& plugin_name, const PluginFunctionality_t& plugin_functionality);
        Plugin_t(const Plugin_t&) = delete;
        Plugin_t(Plugin_t&& other) noexcept;
        ~Plugin_t() override
        {
            std::cout << "Plugin_t::~Plugin_t()" << "\n";
        }

        Plugin_t& operator=(const Plugin_t&) = delete;
        Plugin_t& operator=(Plugin_t&& other) noexcept;

        [[nodiscard]] auto plugin_init() const -> bool override
        {
            return true;
        }

        [[nodiscard]] auto plugin_get_name() const -> std::string override
        {
            return "plugin_get_name()";
        }

        [[nodiscard]] auto plugin_get_author() const -> std::string override
        {
            return "plugin_get_author()";
        }

        [[nodiscard]] auto plugin_get_description() const -> std::string override
        {
            return "plugin_get_description()";
        }

        [[nodiscard]] auto plugin_get_compatibility() const -> std::string override
        {
            return "plugin_get_compatibility()";
        }

        [[nodiscard]] auto plugin_get_subcommand() const -> std::span<SubCommand_t> override
        {
            return {};
        }

        [[nodiscard]] auto plugin_get_feature() const -> std::span<Feature_t> override
        {
            return {};
        }
        [[nodiscard]] auto plugin_main(PluginData_t& plugin_data) const -> i32_t override
        {
            plugin_data.m_argc = 100;
            return 0;
        }

    private:
        FSPath_t m_library_path{};
};


}    // namespace amd_work_bench::plugin


// clang-format off
AMD_SHARED_LIBRARY_MGMT_MANIFEST_BEGIN(amd_work_bench::plugin::PluginIface_t)
    AMD_SHARED_LIBRARY_MGMT_EXPORT_CLASS(amd_work_bench::plugin::Plugin_t)
AMD_SHARED_LIBRARY_MGMT_MANIFEST_END
// clang-format on


void shared_library_mgmt_initialize_library()
{
    std::cout << " -> shared_library_mgmt_initialize_library()" << "\n";
}

void shared_library_mgmt_deinitialize_library()
{
    std::cout << " -> shared_library_mgmt_deinitialize_library()" << "\n";
}

AMD_DYN_LIB_VISIBILITY_PREFIX int question_of_life(int num)
{
    return (42 + num);
}


#endif    // AMD_WORK_BENCH_PLUGIN_TEMPLATE_HPP
