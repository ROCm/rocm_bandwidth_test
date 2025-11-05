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
 * Description: dynlib_mgmt.cpp
 *
 */

#include <dynlib_mgmt.hpp>

#include <dlfcn.h>

#include <mutex>
#include <string>
#include <thread>


namespace amd_shared_library_mgmt
{

std::mutex SharedLibraryDetails_t::m_library_mutex{};


SharedLibraryDetails_t::SharedLibraryDetails_t() : m_library_handle(nullptr)
{
}


SharedLibraryDetails_t::~SharedLibraryDetails_t()
{
    if (m_library_handle != nullptr) {
        dlclose(m_library_handle);
    }
}


auto SharedLibraryDetails_t::load_details(const std::string& library_path, DetailFlags_t flags) -> void
{
    std::scoped_lock<std::mutex> lock(m_library_mutex);

    if (m_library_handle != nullptr) {
        throw SharedLibraryIsLoadedException_t(library_path);
    }

    auto real_os_flags = RTLD_LAZY;
    if ((static_cast<int32_t>(flags)) & static_cast<int32_t>(DetailFlags_t::LOCAL_SHARED_LIB_DETAILS)) {
        (real_os_flags |= RTLD_LOCAL);
    } else {
        (real_os_flags |= RTLD_GLOBAL);
    }

    /*
     * Open the shared object FILE and map it in; return a handle that can be passed to `dlsym' to get symbol values from it
     */
    m_library_handle = dlopen(library_path.c_str(), real_os_flags);
    if (!m_library_handle) {
        const auto dlerror_msg = dlerror();
        throw SharedLibraryLoadException_t(dlerror_msg ? std::string(dlerror_msg) : library_path);
    }

    m_library_path = library_path;
}


auto SharedLibraryDetails_t::unload_details() -> void
{
    std::scoped_lock<std::mutex> lock(m_library_mutex);
    if (m_library_handle) {
        dlclose(m_library_handle);
        m_library_handle = nullptr;
    }
}


auto SharedLibraryDetails_t::is_loaded_details() const -> bool
{
    std::scoped_lock<std::mutex> lock(m_library_mutex);

    return (m_library_handle != nullptr);
}


auto SharedLibraryDetails_t::get_symbol_details(const std::string& symbol_name) -> void*
{
    std::scoped_lock<std::mutex> lock(m_library_mutex);

    auto dlsym_result = static_cast<void*>(nullptr);
    if (m_library_handle) {
        dlsym_result = dlsym(m_library_handle, symbol_name.c_str());
    }

    return dlsym_result;
}


auto SharedLibraryDetails_t::get_path_details() const -> const std::string&
{
    return m_library_path;
}


auto SharedLibraryDetails_t::get_suffix_details() -> std::string
{
    return std::string{".so"};
}


auto SharedLibraryDetails_t::set_search_path_details(const std::string& search_path) -> bool
{
    std::ignore = search_path;
    return false;
}


SharedLibraryMgmt_t::SharedLibraryMgmt_t()
{
}


SharedLibraryMgmt_t::SharedLibraryMgmt_t(const std::string& library_path)
{
    load_details(library_path, DetailFlags_t::NONE_SHARED_LIB_DETAILS);
}


SharedLibraryMgmt_t::SharedLibraryMgmt_t(const std::string& library_path, SharedLibraryFlags_t library_flags)
{
    load_details(library_path, static_cast<DetailFlags_t>(library_flags));
}


SharedLibraryMgmt_t::~SharedLibraryMgmt_t()
{
}


auto SharedLibraryMgmt_t::load(const std::string& library_path) -> void
{
    load_details(library_path, DetailFlags_t::NONE_SHARED_LIB_DETAILS);
}


auto SharedLibraryMgmt_t::load(const std::string& library_path, SharedLibraryFlags_t library_flags) -> void
{
    load_details(library_path, static_cast<DetailFlags_t>(library_flags));
}

auto SharedLibraryMgmt_t::unload() -> void
{
    unload_details();
}


auto SharedLibraryMgmt_t::is_loaded() const -> bool
{
    return is_loaded_details();
}


auto SharedLibraryMgmt_t::has_symbol(const std::string& symbol_name) -> bool
{
    return (get_symbol_details(symbol_name) != nullptr);
}


auto SharedLibraryMgmt_t::get_symbol(const std::string& symbol_name) -> void*
{
    auto dlsym_result = get_symbol_details(symbol_name);
    if (!dlsym_result) {
        throw SharedLibraryNotFoundException_t(symbol_name);
    }

    return dlsym_result;
}


auto SharedLibraryMgmt_t::get_path() const -> const std::string&
{
    return get_path_details();
}


auto SharedLibraryMgmt_t::get_suffix() -> std::string
{
    return get_suffix_details();
}


auto SharedLibraryMgmt_t::set_search_path(const std::string& search_path) -> bool
{
    return set_search_path_details(search_path);
}


}    // namespace amd_shared_library_mgmt
