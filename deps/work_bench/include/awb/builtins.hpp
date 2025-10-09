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
 * Description: builtins.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_BUILTINS_HPP)
#define AMD_WORK_BENCH_BUILTINS_HPP

#include <type_traits>


namespace amd_work_bench::types
{

/*
template<typename... Args>
auto disregard(Args&&... args) -> void
{
    (static_cast<void>(args), ...);
}
*/

auto disregard(auto&&... data) -> void
{
    (static_cast<void>(data), ...);
}


template<typename... Ts>
struct Overloaded_t : Ts...
{
    public:
        using Ts::operator()...;
};

template<typename... Ts>
Overloaded_t(Ts...) -> Overloaded_t<Ts...>;

template<typename Tp>
struct AlwaysFalse_t : std::false_type
{
};


}    // namespace amd_work_bench::types


// namespace alias
// namespace wb = amd_work_bench;


#endif    //-- AMD_WORK_BENCH_BUILTINS_HPP
