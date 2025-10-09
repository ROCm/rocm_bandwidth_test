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
 * Description: plugin_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGIN_MGMT_HPP)
#define AMD_WORK_BENCH_PLUGIN_MGMT_HPP

#include <awb/common_utils.hpp>
#include <awb/logger.hpp>
#include <dynamic_lib_mgmt/include/dynlib_mgmt.hpp>

#include <filesystem>
#include <functional>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <vector>


namespace amd_work_bench
{

namespace std_fs = std::filesystem;
using FSPath_t = std::filesystem::path;


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


struct PluginData_t
{
    public:
        int32_t m_argc;
        int32_t m_ret_code;
        char** m_argv;
};


const std::string kPLUGIN_INIT_FUNCTION{"plugin_init"};
const std::string kPLUGIN_GET_NAME_FUNCTION{"plugin_get_name"};
const std::string kPLUGIN_GET_AUTHOR_FUNCTION{"plugin_get_author"};
const std::string kPLUGIN_GET_DESCRIPTION_FUNCTION{"plugin_get_description"};
const std::string kPLUGIN_GET_COMPATIBILITY_FUNCTION{"plugin_get_compatibility"};
const std::string kPLUGIN_GET_VERSION_FUNCTION{"plugin_get_version"};
const std::string kPLUGIN_GET_SUBCOMMAND_FUNCTION{"plugin_get_subcommand"};
const std::string kPLUGIN_GET_FEATURE_FUNCTION{"plugin_get_feature"};
const std::string kPLUGIN_MAIN_ENTRY_POINT{"plugin_main"};
// const std::string kLIBRARY_INIT_FUNCTION = (std::format("library_init_link_{}", file_name));
// const std::string kLIBRARY_GET_NAME_FUNCTION = (std::format("library_get_name_{}", file_name));


struct PluginFunctionality_t
{
    public:
        using PluginInitFunction_t = void (*)();
        using PluginGetNameFunction_t = const char* (*)();
        using PluginGetAuthorFunction_t = const char* (*)();
        using PluginGetDescriptionFunction_t = const char* (*)();
        using PluginGetCompatibilityFunction_t = const char* (*)();
        using PluginGetVersionFunction_t = const char* (*)();
        using PluginGetSubCommandFunction_t = void* (*)();
        using PluginGetFeatureFunction_t = void* (*)();
        using LibraryInitFunction_t = void (*)();
        using LibraryGetNameFunction_t = const char* (*)();
        using PluginMainDataEntryPoint_t = int32_t (*)(PluginData_t* plugin_data);
        using PluginMainEntryPoint_t = int32_t (*)(int, char**);

        PluginInitFunction_t m_plugin_init_function = nullptr;
        PluginGetNameFunction_t m_plugin_get_name_function = nullptr;
        PluginGetAuthorFunction_t m_plugin_get_author_function = nullptr;
        PluginGetDescriptionFunction_t m_plugin_get_description_function = nullptr;
        PluginGetCompatibilityFunction_t m_plugin_get_compatibility_function = nullptr;
        PluginGetVersionFunction_t m_plugin_get_version_function = nullptr;
        PluginGetSubCommandFunction_t m_plugin_get_subcommand_function = nullptr;
        PluginGetFeatureFunction_t m_plugin_get_feature_function = nullptr;
        PluginMainEntryPoint_t m_plugin_main_entry_point = nullptr;

        LibraryInitFunction_t m_library_init_function = nullptr;
        LibraryGetNameFunction_t m_library_get_name_function = nullptr;

    private:
};


enum class PluginEngineVersion_t : u16_t
{
    VERSION_0_0 = 0,    // No plugin engine version specified/set
    VERSION_1_0 = 1,    // VERSION_1_0 = 1,
    // VERSION_1_1 = 2,
};

enum class PluginStatus_t : i16_t
{
    PLUGIN_MAIN_ENTRY_NOT_FOUND = -1,
    PLUGIN_FINISHED_SUCCESSFULLY = 0,
    PLUGIN_FINISHED_WITH_ERRORS = 1,
};

const auto kLIBRARY_PLUGIN_EXTENSION = std::string(".amdlplug");
const auto kREGULAR_PLUGIN_EXTENSION = std::string(".amdplug");
constexpr auto kBUILTIN_PLUGINS_MIN = u32_t(1);

class PluginIface_t
{
    public:
        virtual ~PluginIface_t() = default;
        virtual auto plugin_init() const -> bool = 0;
        virtual auto plugin_get_name() const -> std::string = 0;
        virtual auto plugin_get_author() const -> std::string = 0;
        virtual auto plugin_get_description() const -> std::string = 0;
        virtual auto plugin_get_compatibility() const -> std::string = 0;
        virtual auto plugin_get_subcommand() const -> std::span<SubCommand_t> = 0;
        virtual auto plugin_get_feature() const -> std::span<Feature_t> = 0;
        virtual auto plugin_get_library_path() const -> const FSPath_t& = 0;
        virtual auto plugin_get_version() const -> std::string = 0;


    private:
        //

    protected:
};
using PluginIfacePtr_t = std::shared_ptr<PluginIface_t>;
using PluginMainDataEntryPoint_t = PluginIfacePtr_t (*)(PluginData_t* plugin_data);
using PluginMainEntryPoint_t = int32_t (*)(int argc, char** argv);


class Plugin_t : public PluginIface_t
{
    public:
        explicit Plugin_t(const FSPath_t& library_path);
        explicit Plugin_t(const std::string& plugin_name, const PluginFunctionality_t& plugin_functionality);
        Plugin_t(const Plugin_t&) = delete;
        Plugin_t(Plugin_t&& other) noexcept;
        ~Plugin_t() override;

        Plugin_t& operator=(const Plugin_t&) = delete;
        Plugin_t& operator=(Plugin_t&& other) noexcept;

        /*
         *  Note:   If a simple run, with no arguments is needed, plugin_init() should be used.
         */
        [[nodiscard]] auto plugin_init() const -> bool override;
        [[nodiscard]] auto plugin_get_name() const -> std::string override;
        [[nodiscard]] auto plugin_get_author() const -> std::string override;
        [[nodiscard]] auto plugin_get_description() const -> std::string override;
        [[nodiscard]] auto plugin_get_compatibility() const -> std::string override;
        [[nodiscard]] auto plugin_get_subcommand() const -> std::span<SubCommand_t> override;
        [[nodiscard]] auto plugin_get_feature() const -> std::span<Feature_t> override;
        [[nodiscard]] auto plugin_get_version() const -> std::string override;
        [[nodiscard]] auto plugin_get_library_path() const -> const FSPath_t& override
        {
            return m_library_path;
        }

        [[nodiscard]] auto is_valid() const -> bool
        {
            return ((m_handler != 0) || (m_functionality.m_plugin_init_function != nullptr) ||
                    (m_functionality.m_library_init_function != nullptr));
        }

        [[nodiscard]] auto is_loaded() const -> bool
        {
            return m_is_initialized;
        }

        [[nodiscard]] auto is_library_plugin() const -> bool
        {
            return ((m_functionality.m_plugin_init_function == nullptr) && (m_functionality.m_library_init_function != nullptr));
        }

        [[nodiscard]] auto is_regular_plugin() const -> bool
        {
            return ((m_functionality.m_plugin_init_function != nullptr) && (m_functionality.m_library_init_function == nullptr));
        }

        [[nodiscard]] auto was_manually_added() const -> bool
        {
            return m_was_manually_added;
        }

        [[nodiscard]] auto has_plugin_main_entry() const -> bool
        {
            return (m_functionality.m_plugin_main_entry_point != nullptr);
        }

        /*
         *  Note:   If argument passing/parsing is needed plugin_main_entry_run() mimics main()
         *          this will call the following function in the plugin file (if implemented):
         *          'int32_t plugin_main(int argc, char** argv);'
         *          Previously declared in the Plugins.hpp as:
         *          'AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX int32_t plugin_main(int argc, char** argv);'
         */
        [[nodiscard]] auto plugin_main_entry_run(int argc, char** argv) const -> int32_t;


    private:
        bool m_was_manually_added{false};
        mutable bool m_is_initialized{false};
        uiptr_t m_handler{0};
        FSPath_t m_library_path{};
        PluginFunctionality_t m_functionality{};
        PluginEngineVersion_t m_engine_version{PluginEngineVersion_t::VERSION_0_0};

        template<typename Tp>
        [[nodiscard]] auto plugin_get_function(const std::string& function_symbol)
        {
            return reinterpret_cast<Tp>(this->plugin_get_function(function_symbol));
        }

        [[nodiscard]] auto plugin_get_function(const std::string& function_symbol) const -> void*;
};


class PluginManagement_t
{
    public:
        PluginManagement_t() = delete;
        static auto plugin_load() -> bool;
        static auto plugin_load(const FSPath_t& library_path) -> bool;
        static auto plugin_reload() -> void;
        static auto plugin_unload() -> void;
        static auto plugin_initialize_new() -> void;
        static auto plugin_load_path_add(const FSPath_t& library_path) -> void;
        static auto plugin_add(const std::string& plugin_name, const PluginFunctionality_t& plugin_functionality) -> void;
        static auto plugin_get(const std::string& plugin_name) -> Plugin_t*;
        static auto plugin_get_all() -> const std::vector<Plugin_t>&;
        static auto plugin_get_path_all() -> const std::vector<FSPath_t>&;
        static auto plugin_get_load_path_all() -> const std::vector<FSPath_t>&;
        static auto is_plugin_loaded(const FSPath_t& library_path) -> bool;

        static auto library_load() -> bool;
        static auto library_load(const FSPath_t& library_path) -> bool;


    private:
        static auto plugin_get_all_mutable() -> std::vector<Plugin_t>&;
        static wb_memory::AutoReset_t<std::vector<FSPath_t>> m_plugin_paths;
        static wb_memory::AutoReset_t<std::vector<FSPath_t>> m_plugin_loaded_paths;
        static wb_memory::AutoReset_t<std::vector<uiptr_t>> m_library_loaded;
};


}    // namespace amd_work_bench


// namespace alias
namespace wb = amd_work_bench;


#endif    //-- AMD_WORK_BENCH_PLUGIN_MGMT_HPP
