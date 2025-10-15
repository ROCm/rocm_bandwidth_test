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
 * Description: event_mgmt.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_EVENT_MGMT_HPP)
#define AMD_WORK_BENCH_EVENT_MGMT_HPP

#include <work_bench.hpp>
#include <awb/logger.hpp>
#include <awb/typedefs.hpp>
#include <awb/work_bench_api.hpp>

#include <algorithm>
#include <concepts>
#include <filesystem>
// #include <format>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>


/*
 *  NOTE:   Yes, this is a hack (Macros). But they are necessary ones (for now).
 */
// clang-format off
#define EVENT_DEFINE_IMPL(event_name, event_text, is_log_required, ...)                                     \
    struct event_name final : public amd_work_bench::details::Event_t<__VA_ARGS__> {                        \
        static constexpr auto event_id = [](){ return amd_work_bench::details::EventId_t(event_text); }();  \
        static constexpr auto is_logging_required = is_log_required;                                        \
        explicit event_name(Event_t::Callback_t function_cb) noexcept : Event_t(std::move(function_cb)) {}  \
                                                                                                            \
        static EventManagement_t::EventListItr_t subscribe(Event_t::Callback_t function_cb) {               \
            return EventManagement_t::subscribe<event_name>(std::move(function_cb));                        \
        }                                                                                                   \
                                                                                                            \
        static void subscribe(void* token_info, Event_t::Callback_t function_cb) {                          \
            EventManagement_t::subscribe<event_name>(token_info, std::move(function_cb));                   \
        }                                                                                                   \
                                                                                                            \
        static void unsubscribe(void* token_info) noexcept {                                                \
            EventManagement_t::unsubscribe<event_name>(token_info);                                         \
        }                                                                                                   \
                                                                                                            \
        static void unsubscribe(const EventManagement_t::EventListItr_t& token_itr) noexcept {              \
            EventManagement_t::unsubscribe(token_itr);                                                      \
        }                                                                                                   \
                                                                                                            \
        static void post(auto&&... args) {                                                                  \
            EventManagement_t::post<event_name>(std::forward<decltype(args)>(args)...);                     \
        }                                                                                                   \
    }                                                                                                       \

#define EVENT_DEFINE(event_name, ...)           EVENT_DEFINE_IMPL(event_name, #event_name, true, __VA_ARGS__)
#define EVENT_DEFINE_NO_LOG(event_name, ...)    EVENT_DEFINE_IMPL(event_name, #event_name, false, __VA_ARGS__)
// clang-format on

namespace amd_work_bench
{

/*
 *  This class is responsible for event-handling for the different events generated by the workbench.
 *      - When an event is submitted, call the subscriber and send them a copy of the event.
 *      - Within the callback, move the event to a queue so it can be used/consumed on the next update call.
 *
 */

constexpr auto kDEFAULT_OFFSET_BASIS_HASH32 = (0x811C9DC5);
constexpr auto kDEFAULT_OFFSET_BASIS_HASH64 = (0xcbf29ce484222325);
constexpr auto kDEFAULT_PRIME32 = (0x01000193);
constexpr auto kDEFAULT_PRIME64 = (0x100000001b3);


namespace details
{

class EventId_t
{
    public:
        // TODO: Add a list of event ids to be used by the workbench.
        explicit constexpr EventId_t(const u32_t event_id) : m_event_id(event_id)
        {
            m_default_offset_basis_hash = m_event_id;
        }

        explicit constexpr EventId_t(const char* event_name)
        {
            /*  Fowler–Noll–Vo (or FNV) */
            m_default_offset_basis_hash = sizeof(size_t) == 8 ? kDEFAULT_OFFSET_BASIS_HASH64 : kDEFAULT_OFFSET_BASIS_HASH32;
            m_default_prime = sizeof(size_t) == 8 ? kDEFAULT_PRIME64 : kDEFAULT_PRIME32;
            for (const auto c : std::string_view(event_name)) {
                m_default_offset_basis_hash ^= static_cast<size_t>(c);
                m_default_offset_basis_hash *= m_default_prime;
            }
            m_event_id = m_default_offset_basis_hash;
        }

        constexpr auto operator==(const EventId_t& other) const
        {
            return (m_event_id == other.m_event_id);
        }

        constexpr auto operator<=>(const EventId_t& other) const
        {
            return (m_event_id <=> other.m_event_id);
        }


    private:
        u32_t m_event_id{0};
        u64_t m_default_offset_basis_hash{0};
        u64_t m_default_prime{0};
};


class EventBase_t
{
    public:
        EventBase_t() noexcept = default;
        virtual ~EventBase_t() = default;
};

template<typename... EventParams>
struct Event_t : public EventBase_t
{
    public:
        using Callback_t = std::function<void(EventParams...)>;

        explicit Event_t(Callback_t function_cb) noexcept : m_function_cb(std::move(function_cb))
        {
        }

        template<typename EventTp>
        void call(EventParams... event_params) const
        {
            try {
                m_function_cb(event_params...);
            } catch (const std::exception& exc) {
                wb_logger::loginfo(LogLevel::LOGGER_DEBUG,
                                   "Error handing event: {} -> {}",
                                   wb_types::get_type_name<EventTp>(),
                                   exc.what());
                throw;
            }
        }


    private:
        Callback_t m_function_cb;
};

template<typename EventTp>
concept EventTypeCpt_t = std::derived_from<EventTp, EventBase_t>;

}    // namespace details


class EventManagement_t
{
    public:
        using EventList_t = std::multimap<details::EventId_t, std::unique_ptr<details::EventBase_t>>;
        using EventListItr_t = EventList_t::iterator;

        static constexpr auto event_id_create = [](const std::string& event_name_str) {
            return details::EventId_t(event_name_str.c_str());
        };

        template<details::EventTypeCpt_t EventTp>
        static auto subscribe(typename EventTp::Callback_t function_cb) -> EventListItr_t
        {
            std::scoped_lock lock(event_mtx_get());
            auto& event_list = get_event_list();
            return event_list.insert({EventTp::event_id, std::make_unique<EventTp>(function_cb)});
        }

        template<details::EventTypeCpt_t EventTp>
        static auto subscribe(void* token_info, typename EventTp::Callback_t function_cb) -> void
        {
            std::scoped_lock lock(event_mtx_get());
            if (get_token_from_store().contains(token_info)) {
                auto&& [rng_begin, rng_end] = get_token_from_store().equal_range(token_info);
                const auto is_event_registered = std::any_of(rng_begin, rng_end, [&](const auto& event) {
                    return (event.second->first == EventTp::event_id);
                });

                if (is_event_registered) {
                    wb_logger::loginfo(LogLevel::LOGGER_CRITICAL,
                                       "Error event has been already registered: {} -> {}",
                                       wb_types::get_type_name<EventTp>(),
                                       token_info);
                    return;
                }
            }
            // Insert the token and the event.
            get_token_from_store().insert({token_info, subscribe<EventTp>(function_cb)});
        }

        template<details::EventTypeCpt_t EventTp>
        static auto unsubscribe(void* token_info) noexcept -> void
        {
            std::scoped_lock lock(event_mtx_get());
            auto& token_store = get_token_from_store();

            auto tmp_token_itr = std::find_if(token_store.begin(), token_store.end(), [&](auto& token) {
                return ((token.first == token_info) && (token.second->first == EventTp::event_id));
            });

            if (tmp_token_itr != token_store.end()) {
                get_event_list().erase(tmp_token_itr->second);
                token_store.erase(tmp_token_itr);
            }
        }

        static auto unsubscribe(const EventListItr_t& token_itr) noexcept -> void
        {
            std::scoped_lock lock(event_mtx_get());
            get_event_list().erase(token_itr);
        }

        template<details::EventTypeCpt_t EventTp>
        static auto post(auto&&... args) -> void
        {
            std::scoped_lock lock(event_mtx_get());
            // auto& event_list = get_event_list();
            auto [rng_begin, rng_end] = get_event_list().equal_range(EventTp::event_id);
            for (auto tmp_itr = rng_begin; tmp_itr != rng_end; ++tmp_itr) {
                const auto& [event_id, event] = *tmp_itr;
                (*static_cast<EventTp* const>(event.get())).template call<EventTp>(std::forward<decltype(args)>(args)...);

                /*
                auto* event = dynamic_cast<EventTp*>(tmp_itr->second.get());
                if (event) {
                    event->call(std::forward<decltype(args)>(args)...);
                }
                */
            }
        }

        static auto clear() noexcept -> void
        {
            std::scoped_lock lock(event_mtx_get());
            get_event_list().clear();
            get_token_from_store().clear();
        }


    private:
        static auto get_token_from_store() -> std::multimap<void*, EventListItr_t>&;
        static auto get_event_list() -> EventList_t&;
        static auto event_mtx_get() -> std::recursive_mutex&;
        static auto is_already_registered(void* token_info, details::EventId_t event_id) -> bool;
        static auto unsubscribe(void* token_info, details::EventId_t event_id) -> void;
};


namespace datasource
{

class DataSourceBase_t;

}

/*
 *  NOTE:   Yes, this is a hack (Macros). In as much as I don't like them, they are necessary (for now).
 *          The idea is to have a list of events that can be used by the workbench.
 *          The events are grouped by type;
 *              - Event:    Events that are generated by the workbench.
 *              - Request:  Requests that are generated by the workbench.
 *              - Message:  Messages that are generated by the workbench.
 */
EVENT_DEFINE(EventStartupDone);
EVENT_DEFINE(EventAWBClosing);
EVENT_DEFINE(EventFirstRun);
EVENT_DEFINE(EventAbnormalTermination, i32_t);
EVENT_DEFINE(EventCrashRecovery, const std::exception&);
EVENT_DEFINE(EventSetAWBRestart);
EVENT_DEFINE(EventSetTaskbarProgress, u32_t, u32_t, u32_t);

EVENT_DEFINE(EventDataSourceCreated, datasource::DataSourceBase_t*);
EVENT_DEFINE(EventDataSourceChanged, datasource::DataSourceBase_t*, datasource::DataSourceBase_t*);

EVENT_DEFINE(EventDataSourceOpening, datasource::DataSourceBase_t*, bool*);
EVENT_DEFINE(EventDataSourceOpened, datasource::DataSourceBase_t*);
EVENT_DEFINE(EventDataSourceClosing, datasource::DataSourceBase_t*, bool*);
EVENT_DEFINE(EventDataSourceClosed, datasource::DataSourceBase_t*);
EVENT_DEFINE(EventDataSourceSaved, datasource::DataSourceBase_t*);
EVENT_DEFINE(EventDataSourceDeleted, datasource::DataSourceBase_t*);

EVENT_DEFINE(EventDataSourceModified, datasource::DataSourceBase_t*, u64_t, u64_t, const u8_t*);
EVENT_DEFINE(EventDataSourceAdded, datasource::DataSourceBase_t*, u64_t, u64_t);
EVENT_DEFINE(EventDataSourceErased, datasource::DataSourceBase_t*, u64_t, u64_t);
EVENT_DEFINE(EventDataSourceStampedInUse, datasource::DataSourceBase_t*);

EVENT_DEFINE(RequestAddStartupTask, std::string, std::function<bool()>, bool);
EVENT_DEFINE(RequestAddExitingTask, std::string, std::function<bool()>);
EVENT_DEFINE(RequestAWBOpen, bool);
EVENT_DEFINE(RequestAWBClose, bool);
EVENT_DEFINE(RequestAWBRestart);
EVENT_DEFINE(RequestOpenFile, std::filesystem::path);
EVENT_DEFINE(RequestCreateDataSource, std::string, bool, bool, datasource::DataSourceBase_t**);
EVENT_DEFINE(MoveDataSourceData, datasource::DataSourceBase_t*, datasource::DataSourceBase_t*);
EVENT_DEFINE(RequestUpdateData);

EVENT_DEFINE(MessageSendToMainInstance, const std::string&, const DataStream_t&);
EVENT_DEFINE(EventNativeMessageReceived, const DataStream_t&);


}    // namespace amd_work_bench

#endif    //-- AMD_WORK_BENCH_EVENT_MGMT_HPP
