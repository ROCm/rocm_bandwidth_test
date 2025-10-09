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
 * Description: cmdline_iface.cpp
 *
 */

#include "cmdline_iface.hpp"

#include <awb/work_bench_api.hpp>
#include <awb/common_utils.hpp>
#include <awb/default_sets.hpp>
#include <awb/event_mgmt.hpp>
#include <awb/logger.hpp>
#include <awb/plugin_mgmt.hpp>
#include <awb/subcommands.hpp>
#include <awb/task_mgmt.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <algorithm>
// #include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <set>
#include <utility>


/*
 * Reference: ANSI colors/codes
    * See: https://en.wikipedia.org/wiki/ANSI_escape_code
    ///
    BLACK = "\033[0;30m"
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    BROWN = "\033[0;33m"
    BLUE = "\033[0;34m"
    PURPLE = "\033[0;35m"
    CYAN = "\033[0;36m"
    LIGHT_GRAY = "\033[0;37m"
    DARK_GRAY = "\033[1;30m"
    LIGHT_RED = "\033[1;31m"
    LIGHT_GREEN = "\033[1;32m"
    YELLOW = "\033[1;33m"
    LIGHT_BLUE = "\033[1;34m"
    LIGHT_PURPLE = "\033[1;35m"
    LIGHT_CYAN = "\033[1;36m"
    LIGHT_WHITE = "\033[1;37m"
    BOLD = "\033[1m"
    FAINT = "\033[2m"
    ITALIC = "\033[3m"
    UNDERLINE = "\033[4m"
    BLINK = "\033[5m"
    NEGATIVE = "\033[7m"
    CROSSED = "\033[9m"
    END = "\033[0m"
    ///
*/


namespace amd_work_bench::plugin::builtin
{

auto command_help_handler(const WordList_t& args) -> void
{
    // Ignore the first argument, so we can silence the compiler warning
    std::ignore = args;

    constexpr auto help_handler_message = R"(
        Help: AMD ROCm Bandwidth Test Command Line Interface
        Usage: rocm_bandwidth_test [subcommand] [options]

        Available subcommands (builtin):
    )";

    constexpr auto kEXTRA_SPACES = u32_t(6);
    auto largest_long_format_chars = size_t(0);
    auto largest_short_format_chars = size_t(0);
    for (const auto& plugin : PluginManagement_t::plugin_get_all()) {
        for (const auto& subcommand : plugin.plugin_get_subcommand()) {
            largest_long_format_chars = std::max(largest_long_format_chars, subcommand.m_long_format.size());
            largest_short_format_chars = std::max(largest_short_format_chars, subcommand.m_short_format.size());
        }
    }

    std::cout << help_handler_message << "\n";
    for (const auto& plugin : PluginManagement_t::plugin_get_all()) {
        for (const auto& subcommand : plugin.plugin_get_subcommand()) {
            //  TODO:   Replace with std::println when available in STL (not in FMTLIB)
            auto subcommand_help_message = amd_fmt::format("    "
                                                           "{}"
                                                           "{: <{}}"
                                                           "{}"
                                                           "{}"
                                                           "{: <{}}"
                                                           "{}",
                                                           subcommand.m_short_format.empty() ? " " : "-",
                                                           subcommand.m_short_format,
                                                           largest_short_format_chars,
                                                           subcommand.m_short_format.empty() ? "  " : ", ",
                                                           subcommand.m_long_format.empty() ? " " : "--",
                                                           subcommand.m_long_format,
                                                           (largest_long_format_chars + kEXTRA_SPACES),
                                                           subcommand.m_description);
            std::cout << subcommand_help_message << "\n";
        }
    }
    std::cout << "\n\n";

    std::exit(EXIT_SUCCESS);
}

auto command_version_handler(const WordList_t& args) -> void
{
    // Ignore the first argument, so we can silence the compiler warning
    std::ignore = args;

    const auto version_handler_message_build_info =
        amd_fmt::format("V: {} \n -> [Commit: {} / Branch: {} / Build Type: {}] \n -> [Build: {}, {}] \n",
                        wb_api_system::get_work_bench_version(),
                        wb_api_system::get_work_bench_commit_hash(),
                        wb_api_system::get_work_bench_commit_branch(),
                        wb_api_system::get_work_bench_build_type(),
                        __DATE__,
                        __TIME__);

    const auto version_handler_message_env_info =
        amd_fmt::format("Kernel: {} \n -> OS: {}", wb_api_system::get_os_kernel_info(), wb_api_system::get_os_distro_info());
    std::cout << (version_handler_message_build_info) << (version_handler_message_env_info) << "\n";

    std::exit(EXIT_SUCCESS);
}


auto command_list_plugins_handler(const WordList_t& args) -> void
{
    if (!args.empty()) {
        for (const auto& arg : args) {
            PluginManagement_t::plugin_load_path_add(arg);
        }
    } else {
        std::cout << "*Plugin(s) loaded:" << "\n";
        for (const auto& plugin : PluginManagement_t::plugin_get_all()) {
            if (plugin.is_library_plugin()) {
                continue;
            }

            //  Enable and disable bold in terminal:
            auto plugin_name_help_message = amd_fmt::format("- \033[1m{}\033[0m", plugin.plugin_get_name());
            auto plugin_author_help_message = amd_fmt::format(" , by: {} \n", plugin.plugin_get_author());
            //  Enable and disable faint in terminal
            auto plugin_description_help_message = amd_fmt::format("  \033[2;3m{}\033[0m \n", plugin.plugin_get_description());
            std::cout << plugin_name_help_message << plugin_author_help_message << plugin_description_help_message << "\n";
        }

        std::exit(EXIT_SUCCESS);
    }
}


auto command_not_implemented_handler(const std::string& function_name, const WordList_t& args) -> void
{
    const auto command_not_yet_implemented_help_message = amd_fmt::format("Error: API '{}' not yet implemented.", function_name);
    std::cout << command_not_yet_implemented_help_message << "\n";
    if (!args.empty()) {
        const auto command_args_passed_help_message = amd_fmt::format("  -> Args: ");
        std::cout << command_args_passed_help_message;

        //  Only build on C++23
        // const auto args_view = (args | std::views::join_with(','));
        for (const auto& arg : args) {
            std::copy(arg.begin(), arg.end(), wb_utils::make_ostream_joiner(&std::cout, ", "));
        }
        std::cout << "\n\n";
    }

    std::exit(EXIT_FAILURE);
}


auto command_register_forwarder() -> void
{
    wb_subcommands::register_subcommand("open", [](const WordList_t& args) {
        for (auto& arg : args) {
            if (arg.starts_with("--")) {
                break;
            }
            RequestOpenFile::post(arg);
        }
    });
}


//  TODO:   Implement the verbose mode, so loggging can be turned on and off
auto command_verbose_handler(const WordList_t& args) -> void
{
    // Ignore the first argument, so we can silence the compiler warning
    std::ignore = args;
    wb_logger::enable_developer_logger();
    wb_logger::loginfo(LogLevel::LOGGER_INFO, "Verbose mode enabled");
}


auto command_pcie_info_handler(const WordList_t& args) -> void
{
    // Ignore the first argument, so we can silence the compiler warning
    std::ignore = args;

    //
    struct PcieLinkInfo_t
    {
        public:
            using PcieLinkThroughputList_t = std::map<u16_t, double>;
            std::string m_revision{};
            std::string m_year_introduced{};
            double m_transfer_rate_per_lane_in_gb{0};
            PcieLinkThroughputList_t m_pcie_link_throughput_list{};

        private:
    };
    using PcieLinkInfoList_t = std::map<std::string, PcieLinkInfo_t>;

    // clang-format off
    static const auto kPcieLinkInfoList = PcieLinkInfoList_t{
        {"1.0", {"1.0", "2003",          2.5, {{1,  0.250}, {2,  0.500}, {4,  1.000}, {8,   2.000}, {16,   4.000}}}},
        {"2.0", {"2.0", "2007",          5.0, {{1,  0.500}, {2,  1.000}, {4,  2.000}, {8,   4.000}, {16,   8.000}}}},
        {"3.0", {"3.0", "2010",          8.0, {{1,  0.985}, {2,  1.969}, {4,  3.938}, {8,   7.877}, {16,  15.754}}}},
        {"4.0", {"4.0", "2017",         16.0, {{1,  1.969}, {2,  3.938}, {4,  7.877}, {8,  15.754}, {16,  31.508}}}},
        {"5.0", {"5.0", "2019",         32.0, {{1,  3.938}, {2,  7.877}, {4, 15.754}, {8,  31.508}, {16,  63.015}}}},
        {"6.0", {"6.0", "2022",         64.0, {{1,  7.563}, {2, 15.125}, {4, 30.250}, {8,  60.500}, {16, 121.000}}}},
        {"7.0", {"7.0", "2025 (plan)", 128.0, {{1, 15.125}, {2, 30.250}, {4, 60.500}, {8, 121.000}, {16, 242.000}}}},
    };
    // clang-format on

    // Build the information table
    constexpr auto kEXTRA_SPACES = u32_t(4);
    constexpr auto kREVISION_MAX_CHARS = u32_t(8);
    constexpr auto kREVISION_YEAR_MAX_CHARS = u32_t(12);
    constexpr auto kTRANSFER_RATE_MAX_CHARS = u32_t(8);
    constexpr auto kTRANSFER_BANDWIDTH_MAX_CHARS = u32_t(30);

    std::cout << "\n";
    const auto pcie_info_help_message_header = amd_fmt::format("\t\033[1m {} \033[0m ", "* PCIe link performance *");
    std::cout << pcie_info_help_message_header << "\n";
    const auto pcie_info_help_message_title = amd_fmt::format(" {: <{}}  {: <{}}  {: <{}}  {: >{}} ",
                                                              "Version",
                                                              kREVISION_MAX_CHARS,
                                                              "Introduced",
                                                              kREVISION_YEAR_MAX_CHARS,
                                                              "Transfer Rate",
                                                              kTRANSFER_RATE_MAX_CHARS,
                                                              "Bandwidth (GB/s)",
                                                              (kTRANSFER_BANDWIDTH_MAX_CHARS + kEXTRA_SPACES));
    std::cout << pcie_info_help_message_title << "\n";
    for (const auto& [pcie_revision, pcie_link_info] : kPcieLinkInfoList) {
        const auto pcie_info_help_message_line = amd_fmt::format(" {: ^{}}  {: ^{}}  {: >{}.3f} {} ",
                                                                 pcie_revision,
                                                                 kREVISION_MAX_CHARS,
                                                                 pcie_link_info.m_year_introduced,
                                                                 kREVISION_YEAR_MAX_CHARS,
                                                                 pcie_link_info.m_transfer_rate_per_lane_in_gb,
                                                                 kTRANSFER_RATE_MAX_CHARS,
                                                                 "GT/s");

        auto pcie_info_help_message_throughput_line = std::string{};
        for (const auto& [pcie_link, pcie_throughput] : pcie_link_info.m_pcie_link_throughput_list) {
            pcie_info_help_message_throughput_line += amd_fmt::format("  x{: <{}} {: >{}.3f}{}",
                                                                      pcie_link,
                                                                      kEXTRA_SPACES,
                                                                      pcie_throughput,
                                                                      kTRANSFER_RATE_MAX_CHARS,
                                                                      " ");
        }

        const auto pcie_info_help_message_throughput_line_full =
            (pcie_info_help_message_line + pcie_info_help_message_throughput_line);
        std::cout << pcie_info_help_message_throughput_line_full << "\n";
    }

    const auto pcie_info_help_message_note =
        amd_fmt::format("\t\033[1m*Reference:\033[0m \033[2;3m{}\033[0m", "https://en.wikipedia.org/wiki/PCI_Express");
    std::cout << pcie_info_help_message_note << "\n\n";
}


auto command_none_handler(const WordList_t& args) -> void
{
    // Ignore the first argument, so we can silence the compiler warning
    std::ignore = args;
    std::cout << "No subcommand provided. Use 'rocm_bandwidth_test help' for more information." << "\n";
}


}    // namespace amd_work_bench::plugin::builtin
