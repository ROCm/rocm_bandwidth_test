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
 * @file test_string_utils.cpp
 * @brief Unit tests for string utility functions
 * @author ROCm Bandwidth Test Team
 *
 * Tests the string manipulation utilities in amd_work_bench::utils::strings
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>


namespace amd_work_bench::test::unit::strings
{

// =============================================================================
// TEST CASE: String Trimming Functions
// =============================================================================

TEST_CASE("StringUtils::Trimming", "[unit][strings][trimming]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("right_trim removes trailing whitespace")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "right_trim"));

        std::string text = "hello world   ";
        wb_strings::right_trim(text);
        REQUIRE(text == "hello world");

        std::string tabs = "test\t\t\t";
        wb_strings::right_trim(tabs);
        REQUIRE(tabs == "test");

        std::string mixed = "data  \t \n";
        wb_strings::right_trim(mixed);
        REQUIRE(mixed == "data");
    }

    SECTION("right_trim_copy returns trimmed copy without modifying original")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "right_trim_copy"));

        std::string original = "hello   ";
        auto trimmed = wb_strings::right_trim_copy(original);

        REQUIRE(original == "hello   ");  // Original unchanged
        REQUIRE(trimmed == "hello");      // Copy is trimmed
    }

    SECTION("left_trim removes leading whitespace")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "left_trim"));

        std::string text = "   hello world";
        wb_strings::left_trim(text);
        REQUIRE(text == "hello world");

        std::string tabs = "\t\t\ttest";
        wb_strings::left_trim(tabs);
        REQUIRE(tabs == "test");
    }

    SECTION("left_trim_copy returns trimmed copy without modifying original")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "left_trim_copy"));

        std::string original = "   hello";
        auto trimmed = wb_strings::left_trim_copy(original);

        REQUIRE(original == "   hello");  // Original unchanged
        REQUIRE(trimmed == "hello");      // Copy is trimmed
    }

    SECTION("trim_all removes both leading and trailing whitespace")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "trim_all"));

        std::string text = "   hello world   ";
        wb_strings::trim_all(text);
        REQUIRE(text == "hello world");

        std::string tabs = "\t\ttest\t\t";
        wb_strings::trim_all(tabs);
        REQUIRE(tabs == "test");
    }

    SECTION("trim_all_copy returns fully trimmed copy")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "trim_all_copy"));

        std::string original = "   hello   ";
        auto trimmed = wb_strings::trim_all_copy(original);

        REQUIRE(original == "   hello   ");  // Original unchanged
        REQUIRE(trimmed == "hello");         // Copy is trimmed
    }

    SECTION("trim functions handle empty strings")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty_strings"));

        std::string empty;
        wb_strings::trim_all(empty);
        REQUIRE(empty.empty());

        auto trimmed = wb_strings::trim_all_copy("");
        REQUIRE(trimmed.empty());
    }

    SECTION("trim functions handle all-whitespace strings")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "all_whitespace"));

        std::string whitespace = "     ";
        wb_strings::trim_all(whitespace);
        REQUIRE(whitespace.empty());
    }
}


// =============================================================================
// TEST CASE: String Splitting
// =============================================================================

TEST_CASE("StringUtils::Splitting", "[unit][strings][split]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("split_str splits by delimiter correctly")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "basic_split"));

        auto parts = wb_strings::split_str("a,b,c,d", ',');

        REQUIRE(parts.size() == 4);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "b");
        REQUIRE(parts[2] == "c");
        REQUIRE(parts[3] == "d");
    }

    SECTION("split_str handles no delimiter present")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "no_delimiter"));

        auto parts = wb_strings::split_str("hello", ',');

        REQUIRE(parts.size() == 1);
        REQUIRE(parts[0] == "hello");
    }

    SECTION("split_str handles empty strings between delimiters")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "empty_between"));

        auto parts = wb_strings::split_str("a,,c", ',');

        REQUIRE(parts.size() == 3);
        REQUIRE(parts[0] == "a");
        REQUIRE(parts[1] == "");
        REQUIRE(parts[2] == "c");
    }

    SECTION("split_str handles path-like strings")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "path_split"));

        auto parts = wb_strings::split_str("/opt/rocm/lib/plugins", '/');

        REQUIRE(parts.size() >= 4);
        CHECK(parts[1] == "opt");
        CHECK(parts[2] == "rocm");
    }
}


// =============================================================================
// TEST CASE: String Replace Operations
// =============================================================================

TEST_CASE("StringUtils::Replace", "[unit][strings][replace]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("replace_all replaces all occurrences in-place")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "replace_all"));

        std::string text = "hello world, hello universe";
        wb_strings::replace_all(text, "hello", "hi");

        REQUIRE(text == "hi world, hi universe");
    }

    SECTION("replace_all_copy returns replaced copy without modifying original")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "replace_all_copy"));

        std::string original = "foo bar foo";
        auto replaced = wb_strings::replace_all_copy(original, "foo", "baz");

        REQUIRE(original == "foo bar foo");  // Original unchanged
        REQUIRE(replaced == "baz bar baz");  // Copy has replacements
    }

    SECTION("replace_all handles no matches")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "no_matches"));

        std::string text = "hello world";
        wb_strings::replace_all(text, "xyz", "abc");

        REQUIRE(text == "hello world");  // Unchanged
    }

    SECTION("replace_all handles replacement longer than original")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "longer_replacement"));

        std::string text = "a b c";
        wb_strings::replace_all(text, " ", "___");

        REQUIRE(text == "a___b___c");
    }

    SECTION("replace_all handles replacement shorter than original")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "shorter_replacement"));

        std::string text = "aaa bbb ccc";
        wb_strings::replace_all(text, "aaa", "x");

        REQUIRE(text == "x bbb ccc");
    }
}


// =============================================================================
// TEST CASE: String Remove Operations
// =============================================================================

TEST_CASE("StringUtils::Remove", "[unit][strings][remove]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("remove_all removes all occurrences of character")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "remove_all"));

        std::string text = "a-b-c-d";
        wb_strings::remove_all(text, '-');

        REQUIRE(text == "abcd");
    }

    SECTION("remove_all_copy returns copy with characters removed")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "remove_all_copy"));

        std::string original = "hello world";
        auto result = wb_strings::remove_all_copy(original, 'l');

        REQUIRE(original == "hello world");  // Original unchanged
        REQUIRE(result == "heo word");        // Copy has removals
    }

    SECTION("remove_all handles character not present")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "not_present"));

        std::string text = "hello";
        wb_strings::remove_all(text, 'x');

        REQUIRE(text == "hello");  // Unchanged
    }
}


// =============================================================================
// TEST CASE: Case Conversion
// =============================================================================

TEST_CASE("StringUtils::CaseConversion", "[unit][strings][case]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("to_lower converts string to lowercase in-place")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "to_lower"));

        std::string text = "HELLO WORLD";
        wb_strings::to_lower(text);

        REQUIRE(text == "hello world");
    }

    SECTION("to_lower_copy returns lowercase copy")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "to_lower_copy"));

        std::string original = "HELLO";
        auto lower = wb_strings::to_lower_copy(original);

        REQUIRE(original == "HELLO");  // Original unchanged
        REQUIRE(lower == "hello");      // Copy is lowercase
    }

    SECTION("to_upper converts string to uppercase in-place")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "to_upper"));

        std::string text = "hello world";
        wb_strings::to_upper(text);

        REQUIRE(text == "HELLO WORLD");
    }

    SECTION("to_upper_copy returns uppercase copy")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "to_upper_copy"));

        std::string original = "hello";
        auto upper = wb_strings::to_upper_copy(original);

        REQUIRE(original == "hello");  // Original unchanged
        REQUIRE(upper == "HELLO");      // Copy is uppercase
    }

    SECTION("case conversion handles mixed alphanumeric")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "mixed_alphanumeric"));

        std::string text = "Hello123World";
        wb_strings::to_lower(text);

        REQUIRE(text == "hello123world");  // Numbers preserved
    }
}


// =============================================================================
// TEST CASE: Case-Insensitive Comparison
// =============================================================================

TEST_CASE("StringUtils::CaseInsensitiveComparison", "[unit][strings][comparison]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("equals_ignore_case compares strings case-insensitively")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "equals_ignore_case"));

        REQUIRE(wb_strings::equals_ignore_case("hello", "HELLO") == true);
        REQUIRE(wb_strings::equals_ignore_case("Hello", "hElLo") == true);
        REQUIRE(wb_strings::equals_ignore_case("abc", "xyz") == false);
        REQUIRE(wb_strings::equals_ignore_case("", "") == true);
    }

    SECTION("contains_ignore_case checks substring presence case-insensitively")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "contains_ignore_case"));

        REQUIRE(wb_strings::contains_ignore_case("Hello World", "WORLD") == true);
        REQUIRE(wb_strings::contains_ignore_case("ROCM_PATH", "rocm") == true);
        REQUIRE(wb_strings::contains_ignore_case("test", "xyz") == false);
    }

    SECTION("contains checks substring presence case-sensitively")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "contains"));

        REQUIRE(wb_strings::contains("hello world", "world") == true);
        REQUIRE(wb_strings::contains("hello world", "WORLD") == false);
        REQUIRE(wb_strings::contains("abc", 'b') == true);
        REQUIRE(wb_strings::contains("abc", 'x') == false);
    }
}


}  // namespace amd_work_bench::test::unit::strings
