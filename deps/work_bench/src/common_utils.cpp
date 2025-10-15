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
 * Description: common_utils.cpp
 *
 */


#include <awb/common_utils.hpp>

#include <dlfcn.h>
#include <unistd.h>

#include <cstdlib>
#include <algorithm>
#include <vector>
#include <string>


namespace amd_work_bench::utils
{

auto get_env_var(const std::string& var_name) -> std::optional<std::string>
{
    auto env_var = std::getenv(var_name.c_str());
    if (env_var == nullptr) {
        return std::nullopt;
    }

    return std::string(env_var);
}


auto set_env_var(const std::string& var_name, const std::string& var_value, bool is_overwrite) -> i32_t
{
    return setenv(var_name.c_str(), var_value.c_str(), is_overwrite);
}


auto unset_env_var(const std::string& var_name) -> i32_t
{
    return unsetenv(var_name.c_str());
}


auto is_tty() -> bool
{
    return (isatty(STDOUT_FILENO) == 1);
}

static auto s_file_to_open = std::optional<std::filesystem::path>{};
[[nodiscard]] auto get_startup_file_path() -> std::optional<std::filesystem::path>
{
    return s_file_to_open;
}


[[nodiscard]] auto get_containing_module_handle(void* symbol) -> void*
{
    Dl_info dl_info = {};
    if (dladdr(symbol, &dl_info) == 0) {
        return nullptr;
    }

    /*
     *  RTLD_LAZY:  Resolve symbols only as the code that references them is executed.
     */
    return dlopen(dl_info.dli_fname, RTLD_LAZY);
}


auto start_program(const std::string& command) -> void
{
    wb_linux::execute_command({"xdg-open", command});
}


auto run_command(const std::string& command) -> i32_t
{
    return std::system(command.c_str());
}


auto open_url(const std::string& url) -> void
{
    if (url.empty()) {
        return;
    }

    auto tmp_url = url;
    // if (!tmp_url.contains("://")) {
    if (!wb_strings::contains(tmp_url, "://")) {
        tmp_url = ("http://" + tmp_url);
    }
    wb_linux::execute_command({"xdg-open", tmp_url});
}


}    // namespace amd_work_bench::utils


namespace amd_work_bench::utils::strings
{

auto right_trim(std::string& text) -> void
{
    text.erase(std::find_if(text.rbegin(),
                            text.rend(),
                            [](unsigned char character) {
                                return !std::isspace(character);
                            })
                   .base(),
               text.end());
}

auto right_trim_copy(std::string text) -> std::string
{
    right_trim(text);
    return text;
}

auto left_trim(std::string& text) -> void
{
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char character) {
                   return !std::isspace(character);
               }));
}

auto left_trim_copy(std::string text) -> std::string
{
    left_trim(text);
    return text;
}

auto trim_all(std::string& text) -> void
{
    right_trim(text);
    left_trim(text);
}

auto trim_all_copy(std::string text) -> std::string
{
    trim_all(text);
    return text;
}

auto split_str(const std::string& text, char delimiter) -> std::vector<std::string>
{
    auto tokens = std::vector<std::string>{};
    auto token_stream = std::istringstream(text);
    for (std::string line; std::getline(token_stream, line, delimiter);) {
        tokens.emplace_back(line);
    }
    if (text.back() == delimiter)
        tokens.emplace_back("");

    return tokens;
}

auto replace_all_copy(std::string text, const std::string& src, const std::string& tgt) -> std::string
{
    auto start_position = size_t(0);
    while ((start_position = text.find(src, start_position)) != std::string::npos) {
        text.replace(start_position, src.size(), tgt);
        start_position += tgt.size();
    }

    return text;
}

auto replace_all(std::string& text, const std::string& src, const std::string& tgt) -> void
{
    while (text.find(src) != std::string::npos) {
        text.replace(text.find(src), src.size(), tgt);
    }
}

auto remove_all(std::string& text, char src) -> void
{
    auto start_position = size_t(0);
    while ((start_position = text.find(src, start_position)) != std::string::npos) {
        text.erase(std::find(text.begin(), text.end(), src));
    }
}

auto remove_all_copy(std::string text, char src) -> std::string
{
    remove_all(text, src);
    return text;
}

auto to_lower(std::string& text) -> void
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
        return std::tolower(character);
    });
}

auto to_lower_copy(std::string text) -> std::string
{
    to_lower(text);
    return text;
}

auto to_upper(std::string& text) -> void
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
        return std::toupper(character);
    });
}

auto to_upper_copy(std::string text) -> std::string
{
    to_upper(text);
    return text;
}

auto contains_ignore_case(std::string_view left, std::string_view right) -> bool
{
    auto search_itr = std::search(left.begin(), left.end(), right.begin(), right.end(), [](char character_a, char character_b) {
        return std::tolower(character_a) == std::tolower(character_b);
    });

    return (search_itr != left.end());
}

auto equals_ignore_case(std::string_view left, std::string_view right) -> bool
{
    if (left.size() != right.size()) {
        return false;
    }

    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char character_a, char character_b) {
        return std::tolower(character_a) == std::tolower(character_b);
    });
}

auto contains(std::string_view text, std::string_view substr) -> bool
{
    return (text.find(substr) != std::string_view::npos);
}

auto contains(std::string_view text, char substr) -> bool
{
    return (text.find(substr) != std::string_view::npos);
}


}    // namespace amd_work_bench::utils::strings
