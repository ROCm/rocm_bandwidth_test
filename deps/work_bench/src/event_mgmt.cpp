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
 * Description: event_mgmt.cpp
 *
 */


#include <awb/event_mgmt.hpp>

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>


namespace amd_work_bench
{

using EventList_t = EventManagement_t::EventList_t;
using EventListItr_t = EventList_t::iterator;


auto EventManagement_t::get_token_from_store() -> std::multimap<void*, EventListItr_t>&
{
    static std::multimap<void*, EventListItr_t> s_token_from_store{};
    return s_token_from_store;
}


auto EventManagement_t::get_event_list() -> EventManagement_t::EventList_t&
{
    static EventList_t s_event_list{};
    return s_event_list;
}


auto EventManagement_t::event_mtx_get() -> std::recursive_mutex&
{
    static std::recursive_mutex event_list_mutex{};
    return event_list_mutex;
}


auto EventManagement_t::is_already_registered(void* token_info, details::EventId_t event_id) -> bool
{
    if (get_token_from_store().contains(token_info)) {
        auto&& [rng_begin, rng_end] = get_token_from_store().equal_range(token_info);
        return std::any_of(rng_begin, rng_end, [&](auto& event_elem) {
            return (event_elem.second->first == event_id);
        });
    }

    return false;
}


auto EventManagement_t::unsubscribe(void* token_info, details::EventId_t event_id) -> void
{
    auto& token_store = get_token_from_store();
    auto token_itr = std::find_if(token_store.begin(), token_store.end(), [&](auto& event_elem) {
        return ((event_elem.first == token_info) && (event_elem.second->first == event_id));
    });

    if (token_itr != token_store.end()) {
        get_event_list().erase(token_itr->second);
        token_store.erase(token_itr);
    }
}


}    // namespace amd_work_bench
