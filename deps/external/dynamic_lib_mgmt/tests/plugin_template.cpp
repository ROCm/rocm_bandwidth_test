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
 * Description: plugin_template.cpp
 *
 */


#include "plugin_template.hpp"
#include <CLI/CLI.hpp>

#include <fmt/core.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>


namespace plugin_template
{

/*  Note:   If long definitions/implementations for these, only declare them here for building the CLI
 *          and implement them in their respective cpp file, ie: optionx.cpp
 */

namespace subcommands
{

void add_option_x(CLI::App* application)
{
    auto get_option_x = std::string();
    auto is_verbose(false);

    // Adding command line options for the subcommand
    auto subcommand = application->add_subcommand("option_x", "Executes the following on the given info/file and output X");
    subcommand->add_option("-x,--option_x", get_option_x, "Option X");
    subcommand->add_flag("-v,--verbose", is_verbose, "Verbose output")->default_val(false);
}

void add_option_y(CLI::App* application)
{
    auto get_option_y = std::string();
    auto is_verbose(false);

    // Adding command line options for the subcommand
    auto subcommand = application->add_subcommand("option_y", "Executes the following on the given info/file and output y");
    subcommand->add_option("-y,--option_y", get_option_y, "Option Y");
    subcommand->add_flag("-v,--verbose", is_verbose, "Verbose output")->default_val(false);
}

}    // namespace subcommands


auto execute_command_line_interface(std::vector<std::string> args)
{
    CLI::App plugin_application{"Plugin Template"};
    plugin_application.require_subcommand();

    // Adding command line options for the main application
    auto output = std::string();
    subcommands::add_option_x(&plugin_application);
    subcommands::add_option_y(&plugin_application);
    plugin_application.add_option("-h,--help", "Help screen")->required();
    plugin_application.add_option("--output", output, "Output file path")->required();

    if (args.size() == 0) {
        fmt::print("{}", plugin_application.help());
        return (EXIT_FAILURE);
    } else if (args.size() == 1) {
        auto subcommand = std::string(args[0]);
        if ((subcommand == "-h") || (subcommand == "--help")) {
            fmt::print("{}", plugin_application.help());
            return (EXIT_FAILURE);
        } else if ((subcommand == "-v") || (subcommand == "--version")) {
            fmt::print("{}", "v0.0.1 \n");
            return (EXIT_SUCCESS);
        }

        try {
            fmt::print("{} \n", plugin_application.get_subcommand(subcommand)->help());
        } catch (CLI::OptionNotFound& exc) {
            fmt::print("Error: Subcommand invalid: '{}' \n", subcommand);
            return (EXIT_FAILURE);
        }
    }

    //  Parse the command line arguments
    try {
        std::reverse(args.begin(), args.end());
        plugin_application.parse(args);
    } catch (CLI::ParseError& exc) {
        // fmt::print("Error: {}\n", exc.what());
        //  std::cout can be replace with a log file.
        return plugin_application.exit(exc, std::cout, std::cout);
        // return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

auto plugin_main(int argc, char** argv) -> i32_t
{
    auto args = std::vector<std::string>((argv + 1), (argv + argc));
    return execute_command_line_interface(args);
}

}    // namespace plugin_template
