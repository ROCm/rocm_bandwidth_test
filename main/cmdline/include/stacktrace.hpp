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
 * Description: stacktrace.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_STACKTRACE_HPP)
#define AMD_WORK_BENCH_STACKTRACE_HPP

#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <cstdint>
#include <source_location>
// #include <stacktrace>
#include <string>
#include <vector>


namespace amd_work_bench::debug
{

using FramePtr_t = std::uintptr_t;

template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
struct Nullable_t
{
    public:
        Nullable_t& operator=(T value);
        bool operator==(const Nullable_t& other) const noexcept;
        bool operator!=(const Nullable_t& other) const noexcept;

        T raw_value;
        T& value() noexcept;
        const T& value() const noexcept;
        T value_or(T alternative) const noexcept;

        bool has_value() const noexcept;
        void swap(Nullable_t& other) noexcept;
        void reset() noexcept;
        constexpr static Nullable_t null() noexcept;    // returns a null instance

    private:
};


struct ObjectFrame_t
{
    public:
        std::string m_object_path;
        FramePtr_t m_raw_address;
        FramePtr_t m_object_address;

    private:
};


struct StackTraceFrame_t
{
    public:
        FramePtr_t m_raw_address;
        FramePtr_t m_object_address;
        Nullable_t<std::uint32_t> m_line;
        Nullable_t<std::uint32_t> m_column;
        std::string m_file_name;
        std::string m_symbol;
        bool m_is_inline;

        bool operator==(const StackTraceFrame_t& other) const;
        bool operator!=(const StackTraceFrame_t& other) const;

        ObjectFrame_t get_object_info() const;
        std::string to_string() const;

    private:
};


struct StackTrace_t
{
    public:
        std::vector<StackTraceFrame_t> m_frames;
        /*
         *  TODO: Revisit this block of code.
         *  A few other things still need to be added and implemented here.
         *  if/in case std::stacktrace works as expected, we can use it to
         *  get the stacktrace and then convert it to our own format.
         */
        std::string to_string() const;

    private:
};


auto debug_startup() -> void;

}    // namespace amd_work_bench::debug


// namespace alias
namespace wb_debug = amd_work_bench::debug;


#endif    //-- AMD_WORK_BENCH_STACKTRACE_HPP
