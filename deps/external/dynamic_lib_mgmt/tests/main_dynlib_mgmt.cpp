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
 * Description: main_dynlib_mgmt.cpp
 *
 */


#include <dynlib_mgmt.hpp>
#include <plugin_template.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>


int main(int argc, char** argv)
{
    std::cout << "dynlib_mgmg: Test" << "\n";
    auto shared_library =
        std::string("./rocm_bandwidth_test/deps/external/DynLibMgmt/build/libplugin_template.so");
    auto dynlib_mgmt = amd_shared_library_mgmt::SharedLibraryMgmt_t(shared_library);

    auto worker_plugin = amd_shared_library_mgmt::ClassWorker_t<amd_work_bench::plugin::PluginIface_t>();
    worker_plugin.load_library(shared_library);


    auto plugin1 = amd_work_bench::plugin::Plugin_t(std::filesystem::path(shared_library));
    auto plugin2 = amd_work_bench::plugin::Plugin_t(std::filesystem::path(shared_library));

    if (worker_plugin.find_class("PluginIface_t")) {
        std::cout << "Class found: PluginIface_t" << "\n";
    } else {
        std::cout << "Class not found: PluginIface_t" << "\n";
    }

    if (worker_plugin.find_class("amd_work_bench::plugin::PluginIface_t")) {
        std::cout << "Class found: amd_work_bench::plugin::PluginIface_t" << "\n";
    } else {
        std::cout << "Class not found: amd_work_bench::plugin::PluginIface_t" << "\n";
    }

    using slm_initialize_library_t = void (*)();
    using slm_quesition_of_life_t = int (*)(int);

    if (dynlib_mgmt.has_symbol("shared_library_mgmt_initialize_library")) {
        std::cout << "Symbol found: shared_library_mgmt_initialize_library() " << "\n";
        auto slm_initialize_library =
            reinterpret_cast<slm_initialize_library_t>(dynlib_mgmt.get_symbol("shared_library_mgmt_initialize_library"));
        slm_initialize_library();
    } else {
        std::cout << "Symbol not found: shared_library_mgmt_initialize_library()" << "\n";
    }

    if (dynlib_mgmt.has_symbol("shared_library_mgmt_deinitialize_library")) {
        std::cout << "Symbol found: shared_library_mgmt_deinitialize_library() " << "\n";
        auto slm_deinitialize_library =
            reinterpret_cast<slm_initialize_library_t>(dynlib_mgmt.get_symbol("shared_library_mgmt_deinitialize_library"));
        slm_deinitialize_library();
    } else {
        std::cout << "Symbol not found: shared_library_mgmt_deinitialize_library()" << "\n";
    }

    if (dynlib_mgmt.has_symbol("question_of_life")) {
        std::cout << "Symbol found: question_of_life() " << "\n";
        auto slm_question_of_life = reinterpret_cast<slm_quesition_of_life_t>(dynlib_mgmt.get_symbol("question_of_life"));
        auto answer = slm_question_of_life(8);
        std::cout << "Answer: " << answer << "\n";
    } else {
        std::cout << "Symbol not found: question_of_life()" << "\n";
    }

    if (dynlib_mgmt.has_symbol("Plugin_t::plugin_init")) {
        std::cout << "Symbol found: plugin_init() " << "\n";
        using slm_plug_init_t = bool (*)();
        auto plugin_init = reinterpret_cast<slm_plug_init_t>(dynlib_mgmt.get_symbol("Plugin_t::plugin_init"));
        plugin_init();
    } else {
        std::cout << "Symbol not found: plugin_init()" << "\n";
    }

    return 0;
}
