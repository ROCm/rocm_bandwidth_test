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
 * Description: common_utils.hpp
 *
 */


#if !defined(AMD_WORK_BENCH_COMMON_UTILS_HPP)
#define AMD_WORK_BENCH_COMMON_UTILS_HPP

#include <work_bench.hpp>
#include <awb/work_bench_api.hpp>
#include <awb/concepts.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cctype>
#include <concepts>
#include <cstring>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>


namespace amd_work_bench::utils
{

/*
 *  Note:   Additional utility functions defined down here.
 */
auto get_env_var(const std::string& var_name) -> std::optional<std::string>;
auto set_env_var(const std::string& var_name, const std::string& var_value, bool is_overwrite) -> i32_t;
auto unset_env_var(const std::string& var_name) -> i32_t;

auto is_tty() -> bool;

[[nodiscard]] auto get_startup_file_path() -> std::optional<std::filesystem::path>;

/*
 *  Get shared library handle (module handle) for a given symbol.
 *  Note:   If called from functions defined in other modules return the handle for current module.
 *          That's due to the fact we are passing a pointer to a thunk defined in the current module.
 */
[[nodiscard]] auto get_containing_module_handle(void* symbol) -> void*;

auto start_program(const std::string& command) -> void;
auto run_command(const std::string& command) -> i32_t;
auto open_url(const std::string& url) -> void;


/*
 *  std::holds_alternative simply checks if a specific type is active, while std::get_if provides
 *  access to the value if it exists.
 *  Note: std::holds_alternative returns a boolean, while std::get_if returns a pointer (or nullptr).
 */
template<typename Tp, typename... VariantTp>
[[nodiscard]] auto get_or(const std::variant<VariantTp...>& variant, Tp default_value) -> Tp
{
    const Tp value = std::get_if<Tp>(&variant);
    if (value != nullptr) {
        return *value;
    }

    return default_value;
    /*
        if (std::holds_alternative<Tp>(variant)) {
            return std::get<Tp>(variant);
        }
        return default_value;
    */
}


//
//  Note: Output iterator that inserts a delimiter between elements.
//
template<typename DelimiterTp, typename CharTp = char, typename TraitsTp = std::char_traits<CharTp>>
class OStreamJoiner_t
{
    public:
        using Char_t = CharTp;
        using Traits_t = TraitsTp;
        using Ostream_t = std::basic_ostream<Char_t, Traits_t>;
        using iterator_category = std::output_iterator_tag;
        using value_type = void;
        using difference_type = void;
        using pointer = void;
        using reference = void;


        OStreamJoiner_t(Ostream_t* outstream,
                        const DelimiterTp& delimiter) noexcept(std::is_nothrow_copy_constructible_v<DelimiterTp>)
            : m_outstream(outstream), m_delimiter(delimiter)
        {
        }

        OStreamJoiner_t(Ostream_t* outstream, DelimiterTp&& delimiter) noexcept(std::is_nothrow_move_constructible_v<DelimiterTp>)
            : m_outstream(outstream), m_delimiter(std::move(delimiter))
        {
        }

        template<typename ValueType>
        OStreamJoiner_t& operator=(const ValueType& value)
        {
            if (!m_is_first) {
                *m_outstream << m_delimiter;
            }
            this->m_is_first = false;
            this->m_value_count++;

            if ((m_value_count % kMAX_VALUES_PER_LINE) == 0) {
                *m_outstream << "\n" << value;
                this->m_value_count = 0;
            } else {
                *m_outstream << value;
            }

            return *this;
        }

        OStreamJoiner_t& operator*() noexcept
        {
            return *this;
        }

        OStreamJoiner_t& operator++() noexcept
        {
            return *this;
        }

        OStreamJoiner_t& operator++(int) noexcept
        {
            return *this;
        }


    private:
        const uint32_t kMAX_VALUES_PER_LINE = 10;
        Ostream_t* m_outstream{};
        DelimiterTp m_delimiter{};
        bool m_is_first{true};
        uint32_t m_value_count{0};
};

/*  Object generator for OStreamJoiner_t.
 */
template<typename CharTp, typename TraitsTp, typename DelimiterTp>
inline OStreamJoiner_t<std::decay_t<DelimiterTp>, CharTp, TraitsTp> make_ostream_joiner(
    std::basic_ostream<CharTp, TraitsTp>* outstream, DelimiterTp&& delimiter)
{
    return {outstream, std::forward<DelimiterTp>(delimiter)};
}


}    // namespace amd_work_bench::utils


namespace amd_work_bench::utils::strings
{

auto right_trim(std::string& text) -> void;
auto right_trim_copy(std::string text) -> std::string;

auto left_trim(std::string& text) -> void;
auto left_trim_copy(std::string text) -> std::string;

auto trim_all(std::string& text) -> void;
auto trim_all_copy(std::string text) -> std::string;

auto split_str(const std::string& text, char delimiter) -> std::vector<std::string>;

auto replace_all_copy(std::string text, const std::string& src, const std::string& tgt) -> std::string;
auto replace_all(std::string& text, const std::string& src, const std::string& tgt) -> void;

auto remove_all(std::string& text, char src) -> void;
auto remove_all_copy(std::string text, char src) -> std::string;

auto to_lower(std::string& text) -> void;
auto to_lower_copy(std::string text) -> std::string;

auto to_upper(std::string& text) -> void;
auto to_upper_copy(std::string text) -> std::string;

auto contains_ignore_case(std::string_view left, std::string_view right) -> bool;
auto equals_ignore_case(std::string_view left, std::string_view right) -> bool;

auto to_string(u128_t text) -> std::string;
auto to_string(i128_t text) -> std::string;

auto contains(std::string_view text, std::string_view substr) -> bool;
auto contains(std::string_view text, char substr) -> bool;


}    // namespace amd_work_bench::utils::strings


namespace amd_work_bench::utils::memory
{

template<typename Tp, typename Td>
auto wrap_in_unique(Tp* ptr_p, Td dtr_d) -> std::unique_ptr<Tp, Td>
{
    return std::unique_ptr<Tp, Td>{ptr_p, std::move(dtr_d)};
}


namespace details
{

class AutoResetBase_t
{
    public:
        AutoResetBase_t() = default;
        virtual ~AutoResetBase_t() = default;
        virtual auto reset() -> void = 0;

    private:
};

}    // namespace details

template<typename Tp>
class AutoReset_t : public details::AutoResetBase_t
{
    public:
        AutoReset_t()
        {
            wb_api_system::details::add_auto_reset_object(this);
        }

        Tp* operator->()
        {
            return &m_value;
        }

        const Tp* operator->() const
        {
            return &m_value;
        }

        Tp& operator*()
        {
            return m_value;
        }

        const Tp& operator*() const
        {
            return m_value;
        }

        operator Tp&()
        {
            return m_value;
        }

        operator const Tp&() const
        {
            return m_value;
        }

        Tp& operator=(const Tp& value)
        {
            m_value = value;
            m_is_valid = true;
            return m_value;
        }

        Tp& operator=(Tp&& value) noexcept
        {
            m_value = std::move(value);
            m_is_valid = true;
            return m_value;
        }

        auto is_valid() const -> bool
        {
            return m_is_valid;
        }


    private:
        bool m_is_valid{true};
        Tp m_value{};

        auto reset() -> void override
        {
            if constexpr (requires { m_value.reset(); }) {
                m_value.reset();
            } else if constexpr (requires { m_value.clear(); }) {
                m_value.clear();
            } else if constexpr (requires(Tp t) { t = nullptr; }) {
                m_value = nullptr;
            } else {
                m_value = {};
            }

            m_is_valid = false;
        }

        friend void wb_api_system::details::auto_reset_objects_cleanup();
};


}    // namespace amd_work_bench::utils::memory


namespace amd_work_bench::utils::units
{

enum class StorageSizeUnit_t
{
    BYTE = 0,
    KB,
    MB,
    GB,
    TB,
    PB,
    EB,
    ZB,
    YB
};

enum class TimeOrderMagnitude_t
{
    NS = 0,
    US,
    MS,
    CS,
    DS,
    S,
};

enum class PcieThroughput_t
{
    X1 = 1,
    X2 = (X1 << 1),
    X4 = (X2 << 1),
    X8 = (X4 << 1),
    X16 = (X8 << 1)
};


}    // namespace amd_work_bench::utils::units


// clang-format off
#define GUARD_TOKEN_CONCATENATE_IMPL(x, y)  x##y
#define GUARD_TOKEN_CONCATENATE(x, y)       GUARD_TOKEN_CONCATENATE_IMPL(x, y)
#define WB_GUARD_ANONYMOUS_VAR(prefix)      GUARD_TOKEN_CONCATENATE(prefix, __COUNTER__)
// clang-format on

namespace amd_work_bench::utils::scope_guard
{

/*
 *  Note:   ScopeGuard_t implementation.
 *          By placing '+' in front of the lambda, the lambda is explicitly converted to a function pointer,
 *          and then the auto variable's type will be deduced as the 'function pointer' type, not the lambda type.
 *
 *          A capturing lambda cannot be converted to a function pointer at all, which is why you get the compiler
 *          error.
 */

// clang-format off
#define SCOPE_GUARD ::amd_work_bench::utils::scope_guard::ScopeGuardOnExit_t() + [&]() -> void
#define ON_SCOPE_EXIT [[maybe_unused]] auto WB_GUARD_ANONYMOUS_VAR(SCOPE_EXIT_) = SCOPE_GUARD
// clang-format on

template<typename Tp>
class ScopeGuard_t
{
    public:
        explicit constexpr ScopeGuard_t(Tp func) : m_func(std::move(func)), m_is_active(true)
        {
        }

        ~ScopeGuard_t()
        {
            if (this->m_is_active) {
                this->m_func();
            }
        }

        ScopeGuard_t(ScopeGuard_t&& other) noexcept
        {
            m_func = std::move(other.m_func);
            m_is_active = other.m_is_active;
            other.release();
        }

        ScopeGuard_t& operator=(ScopeGuard_t&&) = delete;

        auto release() -> void
        {
            this->m_is_active = false;
        }


    private:
        Tp m_func;
        bool m_is_active{false};
};

enum class ScopeGuardOnExit_t
{
};

template<typename Tp>
constexpr auto operator+(ScopeGuardOnExit_t, Tp&& func) -> ScopeGuard_t<Tp>
{
    return ScopeGuard_t<Tp>(std::forward<Tp>(func));
}

// clang-format off
#define SCOPE_TRY_LOCK(mutex)   \
    [[maybe_unused]] auto WB_GUARD_ANONYMOUS_VAR(SCOPE_TRY_LOCK_) = ::amd_work_bench::utils::scope_guard::ScopedTryLock_t(mutex)
// clang-format on
class ScopedTryLock_t
{
    public:
        explicit ScopedTryLock_t(std::mutex& mutex) : m_mutex(&mutex)
        {
            this->m_is_locked = this->m_mutex->try_lock();
        }

        ~ScopedTryLock_t()
        {
            if (this->m_is_locked && this->m_mutex != nullptr) {
                this->m_mutex->unlock();
            }
        }

        ScopedTryLock_t(ScopedTryLock_t&& other) noexcept : m_mutex(other.m_mutex)
        {
            other.m_mutex = nullptr;
            this->m_is_locked = other.m_is_locked;
        }

        ScopedTryLock_t(const ScopedTryLock_t&) = delete;
        ScopedTryLock_t& operator=(const ScopedTryLock_t&) = delete;

        operator bool() const
        {
            return ((this->m_is_locked) && (this->m_mutex != nullptr));
        }


    private:
        bool m_is_locked;
        std::mutex* m_mutex;
};


}    // namespace amd_work_bench::utils::scope_guard


namespace amd_work_bench::utils::first_time_execution
{

// clang-format off
#define ON_SCOPE_FIRST_TIME                                                  \
    [[maybe_unused]] static auto WB_GUARD_ANONYMOUS_VAR(SCOPE_FIRST_TIME_) = \
        ::amd_work_bench::utils::first_time_execution::FirstTimeExecutor_t() + [&]()
// clang-format on

template<typename Tp>
class FirstTimeExecute_t
{
    public:
        explicit constexpr FirstTimeExecute_t(Tp func)
        {
            func();
        }

        FirstTimeExecute_t& operator=(FirstTimeExecute_t&&) = delete;
};

enum class FirstTimeExecutor_t
{
};

template<typename Tp>
constexpr FirstTimeExecute_t<Tp> operator+(FirstTimeExecutor_t, Tp&& func)
{
    return FirstTimeExecute_t<Tp>(std::forward<Tp>(func));
}

}    // namespace amd_work_bench::utils::first_time_execution


namespace amd_work_bench::utils::last_time_execution
{

// clang-format off
#define ON_SCOPE_LAST_TIME                                                  \
    [[maybe_unused]] static auto WB_GUARD_ANONYMOUS_VAR(SCOPE_LAST_TIME_) = \
        ::amd_work_bench::utils::last_time_execution::LastTimeCleanupExecutor_t() + [&]()
// clang-format on

template<typename Tp>
class LastTimeExecute_t
{
    public:
        explicit constexpr LastTimeExecute_t(Tp func) : m_func(func)
        {
        }

        constexpr ~LastTimeExecute_t()
        {
            this->m_func();
        }

        LastTimeExecute_t& operator=(LastTimeExecute_t&&) = delete;


    private:
        Tp m_func;
};

enum class LastTimeCleanupExecutor_t
{
};

template<typename Tp>
constexpr LastTimeExecute_t<Tp> operator+(LastTimeCleanupExecutor_t, Tp&& func)
{
    return LastTimeExecute_t<Tp>(std::forward<Tp>(func));
}

}    // namespace amd_work_bench::utils::last_time_execution


/*
 *  Note:   just so we can adapt to different OS's support for certain features
 */
// clang-format off
#if !defined(OS_LINUX)
    #error "OS not supported!"
#else
    #include <awb/linux_utils.hpp>
#endif
// clang-format on


// namespace alias
namespace wb_utils = amd_work_bench::utils;
namespace wb_strings = amd_work_bench::utils::strings;
namespace wb_memory = amd_work_bench::utils::memory;
namespace wb_units = amd_work_bench::utils::units;
namespace wb_scope_guard = amd_work_bench::utils::scope_guard;


#endif    //-- AMD_WORK_BENCH_COMMON_UTILS_HPP
