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
 * Description: plugins.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_PLUGINS_HPP)
#define AMD_WORK_BENCH_PLUGINS_HPP

#include <work_bench.hpp>
#include <awb/common_utils.hpp>
#include <awb/json.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace
{

struct PluginFunctionInitHelper_t
{
};

}    // namespace


template<typename Tp>
struct PluginFeatureFunctionHelper_t
{
    public:
        static void* plugin_get_feature();
};

template<typename Tp>
struct PluginSubCommandFunctionHelper_t
{
    public:
        static void* plugin_get_subcommand();
};

template<typename Tp>
void* PluginFeatureFunctionHelper_t<Tp>::plugin_get_feature()
{
    return nullptr;
}

template<typename Tp>
void* PluginSubCommandFunctionHelper_t<Tp>::plugin_get_subcommand()
{
    return nullptr;
}

[[maybe_unused]] static auto& get_features_impl()
{
    static std::vector<amd_work_bench::Feature_t> features{};
    return features;
}

template<typename Tp>
struct PluginMainEntryPointFunctionHelper_t
{
    public:
        static void* plugin_get_main_entry(int, char**);
};

template<typename Tp>
void* PluginMainEntryPointFunctionHelper_t<Tp>::plugin_get_main_entry(int, char**)
{
    return nullptr;
}


/*
 *  This is the function that will be called by the plugin to register a feature
 */

// clang-format off
#ifdef AMD_APP_STATIC_LINK_PLUGINS
    #define AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX static
#else
    #define AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX extern "C" [[gnu::visibility("default")]]
#endif


#define PLUGIN_TOKEN_CONCATENATE_IMPL(x, y)             x##y
#define PLUGIN_TOKEN_CONCATENATE(x, y)                  PLUGIN_TOKEN_CONCATENATE_IMPL(x, y)
#define AMD_WORK_BENCH_FEATURE_ENABLED(feature_name)    PLUGIN_TOKEN_CONCATENATE(PLUGIN_TOKEN_CONCATENATE(PLUGIN_TOKEN_CONCATENATE(AMD_WORK_BENCH_PLUGIN_, AMD_WORK_BENCH_PLUGIN_NAME), _FEATURE_), feature_name)

#define AMD_WORK_BENCH_DEFINE_PLUGIN_FEATURES() AMD_WORK_BENCH_DEFINE_PLUGIN_FEATURES_IMPL()
#define AMD_WORK_BENCH_DEFINE_PLUGIN_FEATURES_IMPL()                                        \
    template<>                                                                              \
    struct PluginFeatureFunctionHelper_t<PluginFunctionInitHelper_t> {                      \
        static void* plugin_get_feature();                                                  \
    };                                                                                      \
                                                                                            \
    void* PluginFeatureFunctionHelper_t<PluginFunctionInitHelper_t>::plugin_get_feature() { \
        return &get_features_impl();                                                        \
    }                                                                                       \
    static auto init_features = [](){ get_features_impl() = std::vector<amd_work_bench::Feature_t>({ AMD_WORK_BENCH_PLUGIN_FEATURES_CONTENT }); return 0; }()

#define AMD_WORK_BENCH_PLUGIN_FEATURES ::get_features_impl()


/*
 *  NOTE:   Yes, this is a hack (Macros). But it is a necessary one (for now).
 *          These are used to defined the plugin entry points.
 */
#define AMD_WORK_BENCH_PLUGIN_SETUP(plugin_name, plugin_author, plugin_description, plugin_version) AMD_WORK_BENCH_PLUGIN_SETUP_IMPL(plugin_name, plugin_author, plugin_description, plugin_version)
#define AMD_WORK_BENCH_PLUGIN_SETUP_IMPL(plugin_name, plugin_author, plugin_description, plugin_version)                                \
    namespace {                                                                                                                         \
        static struct ExitHandler_t {                                                                                                   \
            ~ExitHandler_t() {                                                                                                          \
                wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Plugin unload: '{}'", plugin_name);                                         \
            }                                                                                                                           \
        } exit_handler;                                                                                                                 \
    }                                                                                                                                   \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* plugin_get_name() { return plugin_name; }                                       \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* plugin_get_description() { return plugin_description; }                         \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* plugin_get_author() { return plugin_author; }                                   \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* plugin_get_version() { return plugin_version; }                                 \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* plugin_get_compatibility() { return AMD_WORK_BENCH_VERSION; }                   \
                                                                                                                                        \
    AMD_WORK_BENCH_DEFINE_PLUGIN_FEATURES();                                                                                            \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void* plugin_get_feature() { return PluginFeatureFunctionHelper_t<PluginFunctionInitHelper_t>::plugin_get_feature(); }              \
                                                                                                                                                                                \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void* plugin_get_subcommand() { return PluginSubCommandFunctionHelper_t<PluginFunctionInitHelper_t>::plugin_get_subcommand(); }     \
                                                                                                                                                                                \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void plugin_init();                                                                         \
                                                                                                                                        \
    extern "C" [[gnu::visibility("default")]] void PLUGIN_TOKEN_CONCATENATE(force_plugin_link_, AMD_WORK_BENCH_PLUGIN_NAME)() {         \
        amd_work_bench::PluginManagement_t::plugin_add(plugin_name, amd_work_bench::PluginFunctionality_t {                             \
            plugin_init,                                                                                                                \
            plugin_get_name,                                                                                                            \
            plugin_get_author,                                                                                                          \
            plugin_get_description,                                                                                                     \
            plugin_get_compatibility,                                                                                                   \
            plugin_get_version,                                                                                                         \
            plugin_get_subcommand,                                                                                                      \
            plugin_get_feature,                                                                                                         \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
        });                                                                                                                             \
    }                                                                                                                                   \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void plugin_init()


#define AMD_WORK_BENCH_LIBRARY_SETUP(library_name)  AMD_WORK_BENCH_LIBRARY_SETUP_IMPL(library_name)
#define AMD_WORK_BENCH_LIBRARY_SETUP_IMPL(library_name)                                                                                 \
    namespace {                                                                                                                         \
        static struct ExitHandler_t {                                                                                                   \
            ~ExitHandler_t() {                                                                                                          \
                wb_logger::loginfo(LogLevel::LOGGER_DEBUG, "Library unload: '{}'", library_name);                                       \
            }                                                                                                                           \
        } exit_handler;                                                                                                                 \
    }                                                                                                                                   \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void PLUGIN_TOKEN_CONCATENATE(library_init_link_, AMD_WORK_BENCH_PLUGIN_NAME)();            \
                                                                                                                                                            \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX const char* PLUGIN_TOKEN_CONCATENATE(library_get_name_, AMD_WORK_BENCH_PLUGIN_NAME)() { return library_name; }  \
                                                                                                                                                            \
    extern "C" [[gnu::visibility("default")]] void PLUGIN_TOKEN_CONCATENATE(force_plugin_link_, AMD_WORK_BENCH_PLUGIN_NAME)() {         \
        amd_work_bench::PluginManagement_t::plugin_add(library_name, amd_work_bench::PluginFunctionality_t {                            \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            nullptr,                                                                                                                    \
            PLUGIN_TOKEN_CONCATENATE(library_init_link_, AMD_WORK_BENCH_PLUGIN_NAME),                                                   \
            PLUGIN_TOKEN_CONCATENATE(library_get_name_, AMD_WORK_BENCH_PLUGIN_NAME),                                                    \
        });                                                                                                                             \
    }                                                                                                                                   \
                                                                                                                                        \
    AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX void PLUGIN_TOKEN_CONCATENATE(library_init_link_, AMD_WORK_BENCH_PLUGIN_NAME)()


/*
 *  NOTE:   These are used to define; key, description and callback for subcommands.
 *
 */
#define AMD_WORK_BENCH_PLUGIN_SUBCOMMAND() AMD_WORK_BENCH_PLUGIN_SUBCOMMAND_IMPL()

#define AMD_WORK_BENCH_PLUGIN_SUBCOMMAND_IMPL()                                                     \
    extern std::vector<amd_work_bench::SubCommand_t> global_subcommands;                            \
    template<>                                                                                      \
    struct PluginSubCommandFunctionHelper_t<PluginFunctionInitHelper_t> {                           \
        static void* plugin_get_subcommand();                                                       \
    };                                                                                              \
                                                                                                    \
    void* PluginSubCommandFunctionHelper_t<PluginFunctionInitHelper_t>::plugin_get_subcommand() {   \
        return &global_subcommands;                                                                 \
    }                                                                                               \
                                                                                                    \
    std::vector<amd_work_bench::SubCommand_t> global_subcommands

// clang-format on

AMD_WORK_BENCH_PLUGIN_VISIBILITY_PREFIX int32_t plugin_main(int argc, char** argv);


#endif    //-- AMD_WORK_BENCH_PLUGINS_HPP
