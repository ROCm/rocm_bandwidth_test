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
 * Description: cppstd_hooks.hpp
 *
 */

/*
 *  This file is a workaround for the lack of support for C++20 in some compilers.
 *  It provides a way to use the new C++20 features from the standard library if/when available,
 *  or fallback to a third-party library.
 *  So far, we are covering the following features:
 *      - std::format
 *      - std::source_location
 *
 *  I really wanted to name this file 'pre_cxx20_compatibility_shim_of_despair.hpp', or
 *  'legacy_compiler_cognitive_enhancement.hpp' but I was outvoted. ;-)
 *
 *  Other (fun) names that made to the list:
 *      - 'cxx20_compiler_assistance_package.hpp'
 *      - 'compiler_feature_deficit_compensator.hpp'
 *      - 'the_magical_compiler_fixer_upper.hpp'
 *      - 'cpp20_support_enhancer.hpp'
 *      - 'the_compiler_whisperer.hpp'
 *
 *  __cplusplus values:
 *      -   201103L for the 2011 C++ standard
 *      -   201402L for the 2014 C++ standard
 *      -   201703L for the 2017 C++ standard
 *      -   202002L for the 2020 C++ standard
 *      -   202302L for the 2023 C++ standard
 *      - > 202303L for the experimental languages enabled by '-std=c++26' and '-std=gnu++26'
 *
 */

#if !defined(AMD_WORK_BENCH_CPPSTD_HOOKS_HPP)
#define AMD_WORK_BENCH_CPPSTD_HOOKS_HPP

#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>


/*
 *  std::format / fmt::format support
 *  || defined(__cpp_lib_format)
 */

// clang-format off
#if defined(__has_include)
    #if ((__has_include(<format>) && (__cplusplus >= 202002L) && ((__cpp_lib_format) && (__cpp_lib_format >= 201907L))))
        #include <format>

        namespace amd_fmt = std;

        /*
        *  More explicit control over the function signature, which can be extended with custom logic.
        *  However, it a bit more verbose than `using namespace`, and requires careful handling of the
        *  arguments using variadic templates.
        *
        *  //--
        *  Yet, it is possible to create a namespace alias approach to make it more concise:
        *
        *      namespace work_bench_ns {
        *          using fmt::format; / using std::format;
        *      }
        *      work_bench_ns::format("Hello, {}!", "world");
        *
        *  It resolves to the appropriate function at compile time, and syntax for both format cases is the same.
        *  //--
        *
        */

        inline auto format_wrapper = [](const auto& format_content, const auto& ...args) {
            return std::format(format_content, args...);
        };

    #elif (__has_include(<fmt/core.h>)) || !defined(__cpp_lib_format)
        #include <fmt/core.h>
        #include <fmt/format.h>

        namespace amd_fmt = fmt;

        inline auto format_wrapper = [](const auto& format_content, const auto& ...args) {
            return fmt::format(format_content, args...);
        };

    #else
        #error "Either the compiler or the library lacks support for 'format'. std::format/fmt:format not available."
    #endif


    /*
    *  std::stacktrace / boost::stacktrace support
    *   #define __cpp_lib_stacktrace 202011L
    *   //|| defined(__cpp_lib_stacktrace)
    */
    //#if (__cplusplus >= 202302L) && (__has_include(<stacktrace>))
    #if ((__has_include(<stacktrace>) && (__cpp_lib_stacktrace && (__cpp_lib_stacktrace >= 202011L))))
        #include <stacktrace>

        namespace amd_stacktrace {
            using namespace std;

            inline auto get_current_stacktrace() {
                return std::stacktrace::current();
            }

            inline auto get_current_stacktrace_to_string() {
                return std::to_string(std::stacktrace::current());
            }
        }

    #elif (__has_include(<boost/stacktrace.hpp>)) || !defined(__cpp_lib_stacktrace)
        #include <boost/stacktrace.hpp>

        namespace amd_stacktrace {
            using namespace boost::stacktrace;

            inline auto get_current_stacktrace() {
                return boost::stacktrace::stacktrace();
            }

            inline auto get_current_stacktrace_to_string() {
                auto stack_frames = boost::stacktrace::to_string(boost::stacktrace::stacktrace());
                auto stack_trace = std::string();
                for (const auto& frame : stack_frames) {
                    stack_trace += frame;
                }

                return stack_trace;
            }
        }

    #else
        #error "Either the compiler or the library lacks support for 'stacktrace'. std::stacktrace/boost::stacktrace not available."
    #endif

#else
    #error "Compiler does not support '__has_include'. Please use a compiler that supports C++17 or later."
#endif

// clang-format on

#endif    //-- AMD_WORK_BENCH_CPPSTD_HOOKS_HPP
