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
 * Description: plugin_mgmt.cpp
 *
 */


#include <awb/plugin_mgmt.hpp>
#include <awb/work_bench_api.hpp>
#include <awb/common_utils.hpp>
#include <awb/default_sets.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <dlfcn.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <vector>


namespace amd_work_bench
{


/*
 *  Note:   The Plugin_t related APIs are defined here.
 */
static auto load_library(const FSPath_t& path) -> uiptr_t
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "PluginManagement: {} / plugin_path: {} ", __PRETTY_FUNCTION__, path.string());

    /*
     *  RTLD_LAZY:      Resolve symbols only as the code that references them is executed (lazy binding).
     *  RTLD_GLOBAL:    The symbols defined by this shared object will be made available for symbol
     *                  resolution of subsequently loaded shared objects.
     *  On success, dlclose() returns 0; on error, it returns a nonzero value.
     */
    auto library_handle = reinterpret_cast<uiptr_t>(dlopen(path.c_str(), RTLD_LAZY));
    if (library_handle == 0) {
        // const auto* tmp_last_dlerror = dlerror();
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "PluginManagement: Loading library: {}, failed: {}. ",
                           path.filename().string(),
                           dlerror());
        return 0;
    }

    return library_handle;
}


static auto unload_library(uiptr_t library_handle, const FSPath_t& path) -> void
{
    if (library_handle != 0) {
        /*
         *  Reset the error state.
         */
        dlerror();
        /*
         *  Decrements the reference count on the dynamic library handle handle. If the reference count
         *  drops to zero and no other loaded libraries use symbols in it, then the dynamic library is
         *  unloaded.
         *  Returns 0 on success, and nonzero on error.
         */
        if (dlclose(reinterpret_cast<void*>(library_handle)) != 0) {
            wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                               "PluginManagement: Unloading library: {}, failed: {}. ",
                               path.filename().string(),
                               dlerror());
        }
    }
}


Plugin_t::Plugin_t(const FSPath_t& library_path) : m_library_path(library_path)
{
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "PluginManagement: Loading plugin: {}. ", library_path.filename().string());
    m_handler = load_library(library_path);
    if (m_handler != 0) {
        const auto file_name = library_path.stem().string();
        m_functionality.m_plugin_init_function = plugin_get_function<PluginFunctionality_t::PluginInitFunction_t>("plugin_init");
        m_functionality.m_plugin_get_name_function =
            plugin_get_function<PluginFunctionality_t::PluginGetNameFunction_t>("plugin_get_name");
        m_functionality.m_plugin_get_author_function =
            plugin_get_function<PluginFunctionality_t::PluginGetAuthorFunction_t>("plugin_get_author");
        m_functionality.m_plugin_get_description_function =
            plugin_get_function<PluginFunctionality_t::PluginGetDescriptionFunction_t>("plugin_get_description");
        m_functionality.m_plugin_get_compatibility_function =
            plugin_get_function<PluginFunctionality_t::PluginGetCompatibilityFunction_t>("plugin_get_compatibility");
        m_functionality.m_plugin_get_version_function =
            plugin_get_function<PluginFunctionality_t::PluginGetVersionFunction_t>("plugin_get_version");
        m_functionality.m_plugin_get_subcommand_function =
            plugin_get_function<PluginFunctionality_t::PluginGetSubCommandFunction_t>("plugin_get_subcommand");
        m_functionality.m_plugin_get_feature_function =
            plugin_get_function<PluginFunctionality_t::PluginGetFeatureFunction_t>("plugin_get_feature");
        m_functionality.m_library_init_function =
            plugin_get_function<PluginFunctionality_t::LibraryInitFunction_t>(amd_fmt::format("library_init_link_{}", file_name));
        m_functionality.m_library_get_name_function = plugin_get_function<PluginFunctionality_t::LibraryGetNameFunction_t>(
            amd_fmt::format("library_get_name_{}", file_name));

        // Check if we have defined entry points where we can pass in params
        m_functionality.m_plugin_main_entry_point =
            plugin_get_function<PluginFunctionality_t::PluginMainEntryPoint_t>("plugin_main");
    }
}


Plugin_t::Plugin_t(const std::string& plugin_name, const PluginFunctionality_t& plugin_functionality)
{
    m_was_manually_added = true;
    m_handler = 0;
    m_library_path = plugin_name;
    m_functionality = plugin_functionality;
    m_engine_version = PluginEngineVersion_t::VERSION_0_0;
}


Plugin_t::~Plugin_t()
{
    if (is_loaded()) {
        wb_logger::loginfo(LogLevel::LOGGER_INFO, "PluginManagement: Unloading plugin: {}. ", plugin_get_name());
        unload_library(m_handler, m_library_path);
    }
}


Plugin_t::Plugin_t(Plugin_t&& other) noexcept
{
    m_was_manually_added = other.m_was_manually_added;
    m_handler = other.m_handler;
    other.m_handler = 0;
    m_library_path = std::move(other.m_library_path);
    m_functionality = other.m_functionality;
    other.m_functionality = {};
}


Plugin_t& Plugin_t::operator=(Plugin_t&& other) noexcept
{
    if (this != &other) {
        m_was_manually_added = other.m_was_manually_added;
        m_handler = other.m_handler;
        other.m_handler = 0;
        m_library_path = std::move(other.m_library_path);
        m_functionality = other.m_functionality;
        other.m_functionality = {};
    }

    return *this;
}


auto Plugin_t::plugin_get_compatibility() const -> std::string
{
    if (m_functionality.m_plugin_get_compatibility_function == nullptr) {
        return std::string();
    }

    return m_functionality.m_plugin_get_compatibility_function();
}


auto Plugin_t::plugin_get_version() const -> std::string
{
    if (m_functionality.m_plugin_get_version_function == nullptr) {
        return amd_fmt::format("Plugin Version: Unknown -> 0x{0:016X}", m_handler);
    }

    return m_functionality.m_plugin_get_version_function();
}


auto Plugin_t::plugin_get_name() const -> std::string
{
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "PluginManagement: {}. ", __PRETTY_FUNCTION__);
    if ((m_functionality.m_plugin_get_name_function == nullptr) && (m_functionality.m_library_get_name_function != nullptr)) {
        return m_functionality.m_library_get_name_function();
    } else if ((m_functionality.m_plugin_get_name_function != nullptr) &&
               (m_functionality.m_library_get_name_function == nullptr)) {
        return m_functionality.m_plugin_get_name_function();
    }

    return amd_fmt::format("Plugin Name: Unknown -> 0x{0:016X}", m_handler);
}


auto Plugin_t::plugin_get_author() const -> std::string
{
    if (m_functionality.m_plugin_get_author_function == nullptr) {
        return amd_fmt::format("Plugin Author: Unknown -> 0x{0:016X}", m_handler);
    }

    return m_functionality.m_plugin_get_author_function();
}


auto Plugin_t::plugin_get_description() const -> std::string
{
    if (m_functionality.m_plugin_get_description_function == nullptr) {
        return amd_fmt::format("Plugin Description: Unknown -> 0x{0:016X}", m_handler);
    }

    return m_functionality.m_plugin_get_description_function();
}


auto Plugin_t::plugin_get_subcommand() const -> std::span<SubCommand_t>
{
    if (m_functionality.m_plugin_get_subcommand_function == nullptr) {
        return {};
    }

    if (const auto subcommand_result = m_functionality.m_plugin_get_subcommand_function(); subcommand_result != nullptr) {
        auto subcommand_vector = static_cast<std::vector<SubCommand_t>*>(subcommand_result);
        return std::span(subcommand_vector->begin(), subcommand_vector->end());
        // return *static_cast<std::vector<SubCommand_t>*>(subcommand_result);
    }

    return {};
}


auto Plugin_t::plugin_get_feature() const -> std::span<Feature_t>
{
    if (m_functionality.m_plugin_get_feature_function == nullptr) {
        return {};
    }

    if (const auto feature_result = m_functionality.m_plugin_get_feature_function(); feature_result != nullptr) {
        auto feature_vector = static_cast<std::vector<Feature_t>*>(feature_result);
        return std::span(feature_vector->begin(), feature_vector->end());
        // return *static_cast<std::vector<Feature_t>*>(feature_result);
    }

    return {};
}


auto Plugin_t::plugin_get_function(const std::string& function_name) const -> void*
{
    /*
     *  Reset the error state.
     */
    dlerror();
    /*
     *  Obtain address of a symbol in a binary object (shared or executable).
     *  On failure, they return NULL; use dlerror() to get more information.
     */
    return dlsym(reinterpret_cast<void*>(m_handler), function_name.c_str());
}


auto Plugin_t::plugin_init() const -> bool
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "PluginManagement: {} ", __PRETTY_FUNCTION__);

    /*
     *  If the plugin is a library, run initialization steps.
     */
    const auto plugin_name = m_library_path.filename().string();
    if (is_library_plugin()) {
        m_functionality.m_library_init_function();
        wb_logger::loginfo(LogLevel::LOGGER_INFO, "PluginManagement: Library initialization {}, successful. ", plugin_name);
        m_is_initialized = true;

        return m_is_initialized;
    }

    /*
     *  Versioning check.
     *  TODO: Implement a version check based on numbers (semver.org)
     */
    const auto supported_version = plugin_get_compatibility();
    const auto work_bench_version = wb_api_system::get_work_bench_version();

    if (work_bench_version != std::string(wb_literals::kTEXT_UNKNOWN) && !work_bench_version.starts_with(supported_version)) {
        if (!supported_version.empty()) {
            wb_logger::loginfo(LogLevel::LOGGER_WARN,
                               "PluginManagement: Plugin: {}, version: {} is not supported. ",
                               plugin_name,
                               supported_version);
            return false;
        } else {
            wb_logger::loginfo(LogLevel::LOGGER_WARN,
                               "PluginManagement: Plugin: {}, does not have a version requirement. Compatibility assumed true. ",
                               plugin_name);
        }
    }

    if (m_functionality.m_plugin_init_function == nullptr) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "PluginManagement: Plugin: {}, does not have an initialization function. ",
                           plugin_name);
        return false;
    }

    /*
     *  Run the plugin initialization function.
     */
    try {
        m_functionality.m_plugin_init_function();
    } catch (const std::exception& exc) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "PluginManagement: Plugin: {}, initialization failed. Exception: {}. ",
                           plugin_name,
                           exc.what());
        return false;
    } catch (...) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "PluginManagement: Plugin: {}, initialization failed. Unknown exception. ",
                           plugin_name);
        return false;
    }

    /*
     * Initialization successful.
     */
    m_is_initialized = true;
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "PluginManagement: Plugin initialization {}, successful. ", plugin_name);

    return m_is_initialized;
}

auto Plugin_t::plugin_main_entry_run(int argc, char** argv) const -> int32_t
{
    auto plugin_main_entry_result(int32_t(0));

    if (m_functionality.m_plugin_main_entry_point == nullptr) {
        wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                           "PluginManagement: Plugin: {}, does not have an entry point '{}'. ",
                           plugin_get_name(),
                           kPLUGIN_MAIN_ENTRY_POINT);
        return static_cast<int32_t>(PluginStatus_t::PLUGIN_MAIN_ENTRY_NOT_FOUND);
    }

    if (m_handler != 0) {
        /*
         *  NOTE:   This block is used for debugging purposes. It will be removed
         *          in the final version.
         */
        // std::cout << "Plugin: " << plugin_get_name() << " ->Handle: " << m_handler << " is running." << std::endl;
        plugin_main_entry_result = m_functionality.m_plugin_main_entry_point(argc, argv);
        m_is_initialized = true;
    }

    return plugin_main_entry_result;
}


/*
 *  Note:   The PluginManagement_t related APIs are defined here.
 */
wb_memory::AutoReset_t<std::vector<std_fs::path>> PluginManagement_t::m_plugin_paths{};
wb_memory::AutoReset_t<std::vector<std_fs::path>> PluginManagement_t::m_plugin_loaded_paths{};
wb_memory::AutoReset_t<std::vector<uiptr_t>> PluginManagement_t::m_library_loaded{};

auto PluginManagement_t::plugin_get_all_mutable() -> std::vector<Plugin_t>&
{
    static auto plugin_list = std::vector<Plugin_t>{};
    return plugin_list;
}


auto PluginManagement_t::is_plugin_loaded(const FSPath_t& library_path) -> bool
{
    return std::ranges::any_of(plugin_get_all(), [&library_path](const Plugin_t& plugin_instance) {
        return plugin_instance.plugin_get_library_path().filename() == library_path.filename();
    });
}


auto PluginManagement_t::plugin_load() -> bool
{
    auto is_load_successful(true);
    for (const auto& plugin_path : plugin_get_load_path_all()) {
        is_load_successful = (PluginManagement_t::plugin_load(plugin_path) && is_load_successful);
    }

    return is_load_successful;
}

/*
 *  Note:   These should be coming from /plugins/ directory (*amdlplug, *.amdplug)
 */
auto PluginManagement_t::plugin_load(const FSPath_t& plugin_path) -> bool
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN,
                       "PluginManagement: {} / plugin_path: {} ",
                       __PRETTY_FUNCTION__,
                       plugin_path.string());
    if (fs::is_exists(plugin_path)) {
        /*
         *  Update list of plugin paths.
         */
        m_plugin_paths->emplace_back(plugin_path);

        /*
         *  Load the libraries.
         */
        for (const auto& plugin_instance : std_fs::directory_iterator(plugin_path)) {
            if (plugin_instance.is_regular_file() && plugin_instance.path().extension() == kLIBRARY_PLUGIN_EXTENSION) {
                if (!is_plugin_loaded(plugin_instance.path())) {
                    plugin_get_all_mutable().emplace_back(plugin_instance.path());
                }
            }
        }

        /*
         *  Load the normal plugins.
         */
        for (const auto& plugin_instance : std_fs::directory_iterator(plugin_path)) {
            if (plugin_instance.is_regular_file() && plugin_instance.path().extension() == kREGULAR_PLUGIN_EXTENSION) {
                if (!is_plugin_loaded(plugin_instance.path())) {
                    plugin_get_all_mutable().emplace_back(plugin_instance.path());
                }
            }
        }

        /*
         *  Prune the plugin list if instance is not valid.
         */
        std::erase_if(plugin_get_all_mutable(), [](const Plugin_t& plugin_instance) {
            return !plugin_instance.is_valid();
        });

        return true;
    }

    return false;
}


auto PluginManagement_t::library_load() -> bool
{
    wb_logger::loginfo(LogLevel::LOGGER_WARN, "PluginManagement: {} ", __PRETTY_FUNCTION__);
    auto is_load_successful(true);
    for (const auto& plugin_path : paths::kLIBRARY_PATH.read()) {
        wb_logger::loginfo(LogLevel::LOGGER_WARN, "    rocm_bandwidth_test: {} ", plugin_path.string());
        is_load_successful = (library_load(plugin_path) && is_load_successful);
    }

    return is_load_successful;
}


/*
 *  Note:   These should be coming from /lib/ directory (*.so*)
 *  TODO:   We might need to create a list of the extensions we want to load.
 *          For now, we are only loading the .so files.
 */
auto PluginManagement_t::library_load(const FSPath_t& library_path) -> bool
{
    auto is_load_successful(true);
    for (const auto& library_instance : std_fs::directory_iterator(library_path)) {
        if ((library_instance.path().extension() == ".so")) {
            auto library_handle = load_library(library_instance);
            is_load_successful = static_cast<bool>(library_handle != 0);
            m_library_loaded->emplace_back(library_handle);
        }
    }

    return is_load_successful;
}


auto PluginManagement_t::plugin_initialize_new() -> void
{
    for (const auto& plugin_instance : plugin_get_all()) {
        if (!plugin_instance.is_loaded()) {
            /*
             *  TODO:   Check if disregard(); is needed
             */
            wb_types::disregard(plugin_instance.plugin_init());
        }
    }
}


auto PluginManagement_t::plugin_load_path_add(const FSPath_t& library_path) -> void
{
    m_plugin_loaded_paths->emplace_back(library_path);
}


auto PluginManagement_t::plugin_unload() -> void
{
    m_plugin_paths->clear();

    /*
     *  The last ones will be unloaded first.
     */
    auto& all_plugins_list = plugin_get_all_mutable();
    auto saved_plugins_list = std::vector<Plugin_t>();

    while (!all_plugins_list.empty()) {
        if (all_plugins_list.back().was_manually_added()) {
            const auto saved_list_front_itr = saved_plugins_list.cbegin();
            saved_plugins_list.insert(saved_list_front_itr, std::move(all_plugins_list.back()));
        }
        all_plugins_list.pop_back();
    }

    while (!m_plugin_loaded_paths->empty()) {
        unload_library(m_library_loaded->back(), "");
        m_library_loaded->pop_back();
    }

    plugin_get_all_mutable() = std::move(saved_plugins_list);
}


auto PluginManagement_t::plugin_get(const std::string& plugin_name) -> Plugin_t*
{
    for (auto& plugin_instance : plugin_get_all_mutable()) {
        if (plugin_instance.plugin_get_name() == plugin_name) {
            return &plugin_instance;
        }
    }

    return nullptr;
}


auto PluginManagement_t::plugin_get_path_all() -> const std::vector<FSPath_t>&
{
    return m_plugin_paths;
}


auto PluginManagement_t::plugin_get_load_path_all() -> const std::vector<FSPath_t>&
{
    return m_plugin_loaded_paths;
}


auto PluginManagement_t::plugin_add(const std::string& plugin_name, const PluginFunctionality_t& plugin_functionality) -> void
{
    plugin_get_all_mutable().emplace_back(plugin_name, plugin_functionality);
}


auto PluginManagement_t::plugin_get_all() -> const std::vector<Plugin_t>&
{
    return plugin_get_all_mutable();
}


}    // namespace amd_work_bench
