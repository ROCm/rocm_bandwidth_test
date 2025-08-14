/*
 * MIT License
 *
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 *  Developed by:
 *
 *                  AMD ML Software Engineering
 *
 *                  Advanced Micro Devices, Inc.
 *
 *                  www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of Advanced Micro Devices, Inc,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
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
 * Author(s):   Daniel Oliveira <daniel.oliveira@amd.com>
 *
 *
 * Description: test_cases_common.cpp
 *
 */

#include <catch2/catch_all.hpp>
#include <common/include/tst_common.hpp>


namespace amd_work_bench::test::common
{
    auto build_test_tile_info = [](const std::string& test_case_name, 
                                   const std::string& section_name) -> std::string
    {
        return amd_fmt::format("Test case: {}\n Section: {}\n", 
                                test_case_name, 
                                section_name);
    };
}       // namespace amd_work_bench::test::common


TEST_CASE("1_Test_case::utils", "[common]")
{
    using namespace amd_work_bench::test::common;
        
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();
    INFO("Running: " << TEST_CASE_NAME);
    SECTION("1_utils::get_env_var()")
    {
        INFO("  -> " << "Non-existing variable");
        const auto TEST_SECTION_NAME = std::string("1_utils::get_env_var()");
        std::cout << build_test_tile_info(TEST_CASE_NAME, TEST_SECTION_NAME) << "\n";

        auto non_existing_env_var = std::string("NON_EXISTING_ENV_VAR");
        auto env_var_result = amd_work_bench::utils::get_env_var(non_existing_env_var);
        REQUIRE(env_var_result.has_value() == false);
    }

    SECTION("2_utils::get_env_var()")
    {
        INFO("  -> " << "Existing variable");
        const auto TEST_SECTION_NAME = std::string("2_utils::get_env_var()");
        std::cout << build_test_tile_info(TEST_CASE_NAME, TEST_SECTION_NAME) << "\n";
        
        auto existing_env_var = std::string("PATH");
        auto env_var_result = amd_work_bench::utils::get_env_var(existing_env_var);
        REQUIRE(env_var_result.has_value() == true);
        REQUIRE(!env_var_result->empty());
    } 

}   // TEST_CASE


TEST_CASE("2_Test_case::linux", "[common]")
{
    using namespace amd_work_bench::test::common;

    SECTION("2_linux::case_1()")
    {
        // Add your test case logic here
        REQUIRE(true); // Example assertion
    }

    SECTION("2_linux::case_2()")
    {
        // Add your test case logic here
        REQUIRE(false); // Example assertion
    }

}   // TEST_CASE

