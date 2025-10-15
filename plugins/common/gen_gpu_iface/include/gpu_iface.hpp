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
 * Description: gpu_iface.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_GPU_IFACE_HPP)
#define AMD_WORK_BENCH_GPU_IFACE_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


//  TODO:   Implement the following accordingly
//          As well as source file gpu_iface.cpp
namespace amd_work_bench::gpu_iface
{

class GpuIface_t
{
    public:
        GpuIface_t() = default;
        virtual ~GpuIface_t() = default;

        virtual auto startup() const -> void = 0;
        virtual auto run_startup_tasks() const -> void = 0;
        virtual auto run_tasks() const -> void = 0;
        virtual auto run_stop_tasks() const -> void = 0;
        virtual auto stop() const -> void = 0;
        virtual auto get_name() const -> std::string = 0;


    private:
        GpuIface_t(const GpuIface_t&) = delete;
        GpuIface_t& operator=(const GpuIface_t&) = delete;
        GpuIface_t(GpuIface_t&&) = delete;
        GpuIface_t& operator=(GpuIface_t&&) = delete;
};


class GpuIfaceFactory_t
{
    public:
        GpuIfaceFactory_t() = default;
        virtual ~GpuIfaceFactory_t() = default;

        virtual auto create() const -> std::unique_ptr<GpuIface_t> = 0;


    private:
        GpuIfaceFactory_t(const GpuIfaceFactory_t&) = delete;
        GpuIfaceFactory_t& operator=(const GpuIfaceFactory_t&) = delete;
        GpuIfaceFactory_t(GpuIfaceFactory_t&&) = delete;
        GpuIfaceFactory_t& operator=(GpuIfaceFactory_t&&) = delete;
};


class GpuIfaceManager_t
{
    public:
        GpuIfaceManager_t() = default;
        virtual ~GpuIfaceManager_t() = default;

        virtual auto register_factory(const std::string& name, std::unique_ptr<GpuIfaceFactory_t> factory) -> void = 0;
        virtual auto create(const std::string& name) const -> std::unique_ptr<GpuIface_t> = 0;


    private:
        GpuIfaceManager_t(const GpuIfaceManager_t&) = delete;
        GpuIfaceManager_t& operator=(const GpuIfaceManager_t&) = delete;
        GpuIfaceManager_t(GpuIfaceManager_t&&) = delete;
        GpuIfaceManager_t& operator=(GpuIfaceManager_t&&) = delete;
};


class GpuTBEngine_t : public GpuIface_t
{
    public:
        GpuTBEngine_t() = default;
        virtual ~GpuTBEngine_t() = default;

        virtual auto get_gpu_iface_manager() const -> GpuIfaceManager_t& = 0;


    private:
        GpuTBEngine_t(const GpuTBEngine_t&) = delete;
        GpuTBEngine_t& operator=(const GpuTBEngine_t&) = delete;
        GpuTBEngine_t(GpuTBEngine_t&&) = delete;
        GpuTBEngine_t& operator=(GpuTBEngine_t&&) = delete;
};


class GpuAMDDriver_t : public GpuIface_t
{
    public:
        GpuAMDDriver_t() = default;
        virtual ~GpuAMDDriver_t() = default;

        virtual auto get_gpu_iface_manager() const -> GpuIfaceManager_t& = 0;


    private:
        GpuAMDDriver_t(const GpuAMDDriver_t&) = delete;
        GpuAMDDriver_t& operator=(const GpuAMDDriver_t&) = delete;
        GpuAMDDriver_t(GpuAMDDriver_t&&) = delete;
        GpuAMDDriver_t& operator=(GpuAMDDriver_t&&) = delete;
};


class GpuTBWorkBench_t
{
    public:
        GpuTBWorkBench_t() = default;
        GpuTBWorkBench_t(std::unique_ptr<GpuIface_t> gpu_iface) : m_gpu_iface(std::move(gpu_iface))
        {
        }

        virtual ~GpuTBWorkBench_t() = default;

        auto replace_gpu_iface(std::unique_ptr<GpuIface_t> gpu_iface) -> void
        {
            m_gpu_iface = std::move(gpu_iface);
        }


    private:
        std::unique_ptr<GpuIface_t> m_gpu_iface{nullptr};
};


}    // namespace amd_work_bench::gpu_iface


#endif    //-- AMD_WORK_BENCH_GPU_IFACE_HPP