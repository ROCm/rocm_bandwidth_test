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
 * Description: typedefs.hpp
 *
 */

#if !defined(AMD_TYPE_DEFS_HPP)
#define AMD_TYPE_DEFS_HPP

#include <awb/builtins.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
// #include <format>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

/*
 *  NOTE:   These are the basic type definitions used in the project.
 */
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

using ulong_t = unsigned long int;
using ullong_t = unsigned long long int;

using ilong_t = long int;
using illong_t = long long int;

using f32_t = float;
using f64_t = double;

using uiptr_t = std::uintptr_t;

using size_t = std::size_t;

using Color_t = u32_t;

using WordList_t = std::vector<std::string>;
using DataStream_t = std::vector<u8_t>;
using FSPath_t = std::filesystem::path;
using FSPathList_t = std::vector<std::filesystem::path>;


namespace amd_work_bench::types
{

template<typename PointerTp>
concept PointerTypeCpt_t = std::is_pointer_v<PointerTp>;

template<PointerTypeCpt_t PointerTp>
struct NonNullPtr_t
{
    public:
        NonNullPtr_t(PointerTp ptr) : m_ptr(ptr)
        {
        }
        NonNullPtr_t(bool) = delete;
        NonNullPtr_t(std::integral auto) = delete;
        NonNullPtr_t(std::nullptr_t) = delete;


        [[nodiscard]] PointerTp get() const
        {
            return m_ptr;
        }

        [[nodiscard]] PointerTp operator*() const
        {
            return *m_ptr;
        }

        [[nodiscard]] PointerTp operator->() const
        {
            return m_ptr;
        }

        [[nodiscard]] operator PointerTp() const
        {
            return m_ptr;
        }


    private:
        PointerTp m_ptr{nullptr};
};


template<std::size_t TpSize>
struct FixedSizeString_t
{
    public:
        FixedSizeString_t() = delete;
        constexpr FixedSizeString_t(const char (&src_string)[TpSize])
        {
            std::copy_n(src_string, TpSize, m_str.begin());
        }


    private:
        std::array<char, TpSize> m_str = {};
};


template<typename Tp>
constexpr std::string_view get_type_name()
{
    // clang-format off
    #ifdef __GNUC__
        constexpr auto function_name = std::string_view(__PRETTY_FUNCTION__);
        constexpr auto prefix_name = std::string_view("with Tp = ");
        constexpr auto suffix_name = std::string_view("; ");
    #elif __clang__
        constexpr auto function_name = std::string_view(__PRETTY_FUNCTION__);
        constexpr auto prefix_name = std::string_view("[Tp = ");
        constexpr auto suffix_name = std::string_view("]");
    #else
        #error "Compiler not supported!"
    #endif

    // clang-format on
    const auto type_start = (function_name.find(prefix_name) + prefix_name.size());
    const auto type_end = (function_name.find(suffix_name));
    const auto type_size = (type_end - type_start);

    return function_name.substr(type_start, type_size);
}


template<typename... Args>
// std::string format(std::string_view format, Args... args)
std::string format(std::string_view format, Args&&... args)
{
    // return std::format(std::runtime(format), args...);
    return amd_fmt::vformat((format), amd_fmt::make_format_args(args...));
}


}    // namespace amd_work_bench::types


// namespace alias
namespace wb_types = amd_work_bench::types;

#endif    //-- AMD_TYPE_DEFS_HPP
