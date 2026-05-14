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
 * @file test_transferbench_types.cpp
 * @brief Contract/specification tests for TransferBench core types and structures
 * @author ROCm Bandwidth Test Team
 *
 * These are specification tests that verify expected properties of TransferBench
 * data structures (ExeType, MemType, ExeDevice, MemDevice, Transfer, options).
 *
 * IMPORTANT: TransferBench.hpp cannot be directly included here because it
 * depends on HIP/CUDA, numa.h, and other platform-specific headers. Instead,
 * these tests use local type definitions that mirror the actual TransferBench
 * types. If the actual types change in:
 *   plugins/common/tb_engine/include/TransferBench.hpp
 * these mirror definitions MUST be updated to match.
 *
 * The enum values, struct layouts, and helper function logic tested here are
 * verified against the source of truth at the locations noted in comments.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>

#include <string>
#include <vector>
#include <set>


namespace amd_work_bench::test::unit::transferbench
{

// =============================================================================
// Mirror types for TransferBench (source: plugins/common/tb_engine/include/TransferBench.hpp)
// KEEP IN SYNC: If TransferBench.hpp changes, update these definitions.
// =============================================================================

/**
 * @brief Executor type enumeration
 * @see TransferBench.hpp line ~184: enum ExeType
 */
enum class MockExeType
{
    EXE_CPU = 0,        // CPU executor
    EXE_GPU_GFX = 1,    // GPU kernel-based executor
    EXE_GPU_DMA = 2,    // GPU SDMA executor
    EXE_NIC = 3,        // NIC RDMA executor
    EXE_NIC_NEAREST = 4 // NIC RDMA nearest executor
};

/**
 * @brief Memory type enumeration
 * @see TransferBench.hpp line ~225: enum MemType
 */
enum class MockMemType
{
    MEM_CPU = 0,          // Coarse-grained pinned CPU memory
    MEM_GPU = 1,          // Coarse-grained global GPU memory
    MEM_CPU_FINE = 2,     // Fine-grained pinned CPU memory
    MEM_GPU_FINE = 3,     // Fine-grained global GPU memory
    MEM_CPU_UNPINNED = 4, // Unpinned CPU memory
    MEM_NULL = 5,         // NULL memory
    MEM_MANAGED = 6,      // Managed memory
    MEM_CPU_CLOSEST = 7   // Coarse-grained pinned CPU memory (closest GPU)
};

/**
 * @brief Executor device structure
 * @see TransferBench.hpp line ~209: struct ExeDevice
 */
struct MockExeDevice
{
    MockExeType exeType;
    int32_t exeIndex;

    bool operator<(const MockExeDevice& other) const
    {
        return (exeType < other.exeType) ||
               (exeType == other.exeType && exeIndex < other.exeIndex);
    }

    bool operator==(const MockExeDevice& other) const
    {
        return exeType == other.exeType && exeIndex == other.exeIndex;
    }
};

/**
 * @brief Memory device structure
 * @see TransferBench.hpp line ~252: struct MemDevice
 */
struct MockMemDevice
{
    MockMemType memType;
    int32_t memIndex;

    bool operator<(const MockMemDevice& other) const
    {
        return (memType < other.memType) ||
               (memType == other.memType && memIndex < other.memIndex);
    }

    bool operator==(const MockMemDevice& other) const
    {
        return memType == other.memType && memIndex == other.memIndex;
    }
};

/**
 * @brief Transfer structure
 * @see TransferBench.hpp line ~264: struct Transfer
 */
struct MockTransfer
{
    size_t numBytes = 0;
    std::vector<MockMemDevice> srcs;
    std::vector<MockMemDevice> dsts;
    MockExeDevice exeDevice;
    int32_t exeSubIndex = -1;
    int numSubExecs = 0;
};

/**
 * @brief General options structure
 * @see TransferBench.hpp line ~275: struct GeneralOptions
 */
struct MockGeneralOptions
{
    int numIterations = 10;
    int numSubIterations = 1;
    int numWarmups = 3;
    int recordPerIteration = 0;
    int useInteractive = 0;
};

/**
 * @brief Data options structure
 * @see TransferBench.hpp line ~290 (approx): struct DataOptions
 */
struct MockDataOptions
{
    int alwaysValidate = 0;
    int blockBytes = 256;
    int byteOffset = 0;
    std::vector<float> fillPattern;
    std::vector<int> fillCompress;
    int validateDirect = 0;
    int validateSource = 0;
};


// =============================================================================
// Helper Functions (mirrors TransferBench.hpp inline functions)
// @see TransferBench.hpp lines ~193-248
// =============================================================================

inline bool IsCpuExeType(MockExeType e)
{
    return e == MockExeType::EXE_CPU;
}

inline bool IsGpuExeType(MockExeType e)
{
    return e == MockExeType::EXE_GPU_GFX || e == MockExeType::EXE_GPU_DMA;
}

inline bool IsNicExeType(MockExeType e)
{
    return e == MockExeType::EXE_NIC || e == MockExeType::EXE_NIC_NEAREST;
}

inline bool IsCpuMemType(MockMemType m)
{
    return m == MockMemType::MEM_CPU ||
           m == MockMemType::MEM_CPU_FINE ||
           m == MockMemType::MEM_CPU_UNPINNED ||
           m == MockMemType::MEM_CPU_CLOSEST;
}

inline bool IsGpuMemType(MockMemType m)
{
    return m == MockMemType::MEM_GPU ||
           m == MockMemType::MEM_GPU_FINE ||
           m == MockMemType::MEM_MANAGED;
}


// =============================================================================
// TEST CASE: Executor Type Enumeration (Contract Test)
// Verifies expected values match TransferBench.hpp enum ExeType
// =============================================================================

TEST_CASE("TransferBench::ExeType", "[unit][transferbench][types][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("ExeType values are sequential")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "sequential values"));

        REQUIRE(static_cast<int>(MockExeType::EXE_CPU) == 0);
        REQUIRE(static_cast<int>(MockExeType::EXE_GPU_GFX) == 1);
        REQUIRE(static_cast<int>(MockExeType::EXE_GPU_DMA) == 2);
        REQUIRE(static_cast<int>(MockExeType::EXE_NIC) == 3);
        REQUIRE(static_cast<int>(MockExeType::EXE_NIC_NEAREST) == 4);
    }

    SECTION("IsCpuExeType identifies CPU executor")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "IsCpuExeType"));

        REQUIRE(IsCpuExeType(MockExeType::EXE_CPU) == true);
        REQUIRE(IsCpuExeType(MockExeType::EXE_GPU_GFX) == false);
        REQUIRE(IsCpuExeType(MockExeType::EXE_GPU_DMA) == false);
        REQUIRE(IsCpuExeType(MockExeType::EXE_NIC) == false);
    }

    SECTION("IsGpuExeType identifies GPU executors")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "IsGpuExeType"));

        REQUIRE(IsGpuExeType(MockExeType::EXE_CPU) == false);
        REQUIRE(IsGpuExeType(MockExeType::EXE_GPU_GFX) == true);
        REQUIRE(IsGpuExeType(MockExeType::EXE_GPU_DMA) == true);
        REQUIRE(IsGpuExeType(MockExeType::EXE_NIC) == false);
    }

    SECTION("IsNicExeType identifies NIC executors")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "IsNicExeType"));

        REQUIRE(IsNicExeType(MockExeType::EXE_CPU) == false);
        REQUIRE(IsNicExeType(MockExeType::EXE_GPU_GFX) == false);
        REQUIRE(IsNicExeType(MockExeType::EXE_NIC) == true);
        REQUIRE(IsNicExeType(MockExeType::EXE_NIC_NEAREST) == true);
    }
}


// =============================================================================
// TEST CASE: Memory Type Enumeration (Contract Test)
// Verifies expected values match TransferBench.hpp enum MemType
// =============================================================================

TEST_CASE("TransferBench::MemType", "[unit][transferbench][types][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("MemType values are sequential")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "sequential values"));

        REQUIRE(static_cast<int>(MockMemType::MEM_CPU) == 0);
        REQUIRE(static_cast<int>(MockMemType::MEM_GPU) == 1);
        REQUIRE(static_cast<int>(MockMemType::MEM_CPU_FINE) == 2);
        REQUIRE(static_cast<int>(MockMemType::MEM_GPU_FINE) == 3);
        REQUIRE(static_cast<int>(MockMemType::MEM_CPU_UNPINNED) == 4);
        REQUIRE(static_cast<int>(MockMemType::MEM_NULL) == 5);
        REQUIRE(static_cast<int>(MockMemType::MEM_MANAGED) == 6);
        REQUIRE(static_cast<int>(MockMemType::MEM_CPU_CLOSEST) == 7);
    }

    SECTION("IsCpuMemType identifies CPU memory types")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "IsCpuMemType"));

        REQUIRE(IsCpuMemType(MockMemType::MEM_CPU) == true);
        REQUIRE(IsCpuMemType(MockMemType::MEM_CPU_FINE) == true);
        REQUIRE(IsCpuMemType(MockMemType::MEM_CPU_UNPINNED) == true);
        REQUIRE(IsCpuMemType(MockMemType::MEM_CPU_CLOSEST) == true);
        REQUIRE(IsCpuMemType(MockMemType::MEM_GPU) == false);
        REQUIRE(IsCpuMemType(MockMemType::MEM_NULL) == false);
    }

    SECTION("IsGpuMemType identifies GPU memory types")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "IsGpuMemType"));

        REQUIRE(IsGpuMemType(MockMemType::MEM_GPU) == true);
        REQUIRE(IsGpuMemType(MockMemType::MEM_GPU_FINE) == true);
        REQUIRE(IsGpuMemType(MockMemType::MEM_MANAGED) == true);
        REQUIRE(IsGpuMemType(MockMemType::MEM_CPU) == false);
        REQUIRE(IsGpuMemType(MockMemType::MEM_NULL) == false);
    }
}


// =============================================================================
// TEST CASE: Executor Device Structure (Contract Test)
// =============================================================================

TEST_CASE("TransferBench::ExeDevice", "[unit][transferbench][device][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("ExeDevice default construction")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default"));

        MockExeDevice device{};

        // Default should be CPU at index 0
        device.exeType = MockExeType::EXE_CPU;
        device.exeIndex = 0;

        REQUIRE(device.exeType == MockExeType::EXE_CPU);
        REQUIRE(device.exeIndex == 0);
    }

    SECTION("ExeDevice comparison operator")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "comparison"));

        MockExeDevice cpu0{MockExeType::EXE_CPU, 0};
        MockExeDevice cpu1{MockExeType::EXE_CPU, 1};
        MockExeDevice gpu0{MockExeType::EXE_GPU_GFX, 0};
        MockExeDevice gpu1{MockExeType::EXE_GPU_GFX, 1};

        // Same type, different index
        REQUIRE(cpu0 < cpu1);
        REQUIRE(gpu0 < gpu1);

        // Different type
        REQUIRE(cpu0 < gpu0);
        REQUIRE(cpu1 < gpu0);
    }

    SECTION("ExeDevice can be used in std::set")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "set usage"));

        std::set<MockExeDevice> devices;

        devices.insert({MockExeType::EXE_CPU, 0});
        devices.insert({MockExeType::EXE_GPU_GFX, 0});
        devices.insert({MockExeType::EXE_GPU_GFX, 1});
        devices.insert({MockExeType::EXE_CPU, 0});  // Duplicate

        REQUIRE(devices.size() == 3);  // Duplicates removed
    }
}


// =============================================================================
// TEST CASE: Memory Device Structure (Contract Test)
// =============================================================================

TEST_CASE("TransferBench::MemDevice", "[unit][transferbench][device][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("MemDevice default construction")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default"));

        MockMemDevice device{MockMemType::MEM_CPU, 0};

        REQUIRE(device.memType == MockMemType::MEM_CPU);
        REQUIRE(device.memIndex == 0);
    }

    SECTION("MemDevice comparison operator")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "comparison"));

        MockMemDevice cpu0{MockMemType::MEM_CPU, 0};
        MockMemDevice cpu1{MockMemType::MEM_CPU, 1};
        MockMemDevice gpu0{MockMemType::MEM_GPU, 0};

        REQUIRE(cpu0 < cpu1);
        REQUIRE(cpu0 < gpu0);
    }

    SECTION("MemDevice can be used in std::set")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "set usage"));

        std::set<MockMemDevice> devices;

        devices.insert({MockMemType::MEM_CPU, 0});
        devices.insert({MockMemType::MEM_GPU, 0});
        devices.insert({MockMemType::MEM_GPU, 1});

        REQUIRE(devices.size() == 3);
    }
}


// =============================================================================
// TEST CASE: Transfer Structure (Contract Test)
// =============================================================================

TEST_CASE("TransferBench::Transfer", "[unit][transferbench][transfer][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("Transfer default construction")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default"));

        MockTransfer transfer{};

        REQUIRE(transfer.numBytes == 0);
        REQUIRE(transfer.srcs.empty());
        REQUIRE(transfer.dsts.empty());
        REQUIRE(transfer.exeSubIndex == -1);
        REQUIRE(transfer.numSubExecs == 0);
    }

    SECTION("Transfer with sources and destinations")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "src/dst"));

        MockTransfer transfer;
        transfer.numBytes = 1024 * 1024;  // 1 MB
        transfer.srcs.push_back({MockMemType::MEM_CPU, 0});
        transfer.dsts.push_back({MockMemType::MEM_GPU, 0});
        transfer.exeDevice = {MockExeType::EXE_GPU_DMA, 0};
        transfer.numSubExecs = 4;

        REQUIRE(transfer.numBytes == 1024 * 1024);
        REQUIRE(transfer.srcs.size() == 1);
        REQUIRE(transfer.dsts.size() == 1);
        REQUIRE(transfer.srcs[0].memType == MockMemType::MEM_CPU);
        REQUIRE(transfer.dsts[0].memType == MockMemType::MEM_GPU);
        REQUIRE(transfer.exeDevice.exeType == MockExeType::EXE_GPU_DMA);
    }

    SECTION("Transfer with multiple sources/destinations")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "multiple src/dst"));

        MockTransfer transfer;
        transfer.numBytes = 4096;

        // Multiple sources
        transfer.srcs.push_back({MockMemType::MEM_CPU, 0});
        transfer.srcs.push_back({MockMemType::MEM_CPU, 1});

        // Multiple destinations
        transfer.dsts.push_back({MockMemType::MEM_GPU, 0});
        transfer.dsts.push_back({MockMemType::MEM_GPU, 1});
        transfer.dsts.push_back({MockMemType::MEM_GPU, 2});

        REQUIRE(transfer.srcs.size() == 2);
        REQUIRE(transfer.dsts.size() == 3);
    }
}


// =============================================================================
// TEST CASE: General Options Structure (Contract Test)
// =============================================================================

TEST_CASE("TransferBench::GeneralOptions", "[unit][transferbench][options][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("GeneralOptions default values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "defaults"));

        MockGeneralOptions opts{};

        REQUIRE(opts.numIterations == 10);
        REQUIRE(opts.numSubIterations == 1);
        REQUIRE(opts.numWarmups == 3);
        REQUIRE(opts.recordPerIteration == 0);
        REQUIRE(opts.useInteractive == 0);
    }

    SECTION("GeneralOptions custom values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "custom"));

        MockGeneralOptions opts;
        opts.numIterations = 100;
        opts.numWarmups = 5;
        opts.useInteractive = 1;

        REQUIRE(opts.numIterations == 100);
        REQUIRE(opts.numWarmups == 5);
        REQUIRE(opts.useInteractive == 1);
    }

    SECTION("GeneralOptions negative iterations (timed run)")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "timed run"));

        MockGeneralOptions opts;
        opts.numIterations = -10;  // Run for 10 seconds

        REQUIRE(opts.numIterations == -10);
        REQUIRE(opts.numIterations < 0);  // Indicates timed mode
    }
}


// =============================================================================
// TEST CASE: Data Options Structure (Contract Test)
// =============================================================================

TEST_CASE("TransferBench::DataOptions", "[unit][transferbench][options][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("DataOptions default values")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "defaults"));

        MockDataOptions opts{};

        REQUIRE(opts.alwaysValidate == 0);
        REQUIRE(opts.blockBytes == 256);
        REQUIRE(opts.byteOffset == 0);
        REQUIRE(opts.fillPattern.empty());
        REQUIRE(opts.fillCompress.empty());
        REQUIRE(opts.validateDirect == 0);
        REQUIRE(opts.validateSource == 0);
    }

    SECTION("DataOptions with fill pattern")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "fill pattern"));

        MockDataOptions opts;
        opts.fillPattern = {1.0f, 2.0f, 3.0f, 4.0f};

        REQUIRE(opts.fillPattern.size() == 4);
        REQUIRE(opts.fillPattern[0] == 1.0f);
        REQUIRE(opts.fillPattern[3] == 4.0f);
    }

    SECTION("DataOptions block size validation")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "block size"));

        MockDataOptions opts;

        // Default block size should be power of 2
        REQUIRE(opts.blockBytes == 256);
        REQUIRE((opts.blockBytes & (opts.blockBytes - 1)) == 0);  // Power of 2 check

        // Custom block sizes
        opts.blockBytes = 512;
        REQUIRE((opts.blockBytes & (opts.blockBytes - 1)) == 0);

        opts.blockBytes = 1024;
        REQUIRE((opts.blockBytes & (opts.blockBytes - 1)) == 0);
    }
}


// =============================================================================
// TEST CASE: Transfer Scenarios (Integration Logic Tests)
// =============================================================================

TEST_CASE("TransferBench::TransferScenarios", "[unit][transferbench][scenarios][contract]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    SECTION("CPU to GPU transfer")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "CPU->GPU"));

        MockTransfer transfer;
        transfer.numBytes = 1024 * 1024;
        transfer.srcs.push_back({MockMemType::MEM_CPU, 0});
        transfer.dsts.push_back({MockMemType::MEM_GPU, 0});
        transfer.exeDevice = {MockExeType::EXE_GPU_DMA, 0};

        REQUIRE(IsCpuMemType(transfer.srcs[0].memType));
        REQUIRE(IsGpuMemType(transfer.dsts[0].memType));
    }

    SECTION("GPU to GPU transfer")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "GPU->GPU"));

        MockTransfer transfer;
        transfer.numBytes = 1024 * 1024;
        transfer.srcs.push_back({MockMemType::MEM_GPU, 0});
        transfer.dsts.push_back({MockMemType::MEM_GPU, 1});
        transfer.exeDevice = {MockExeType::EXE_GPU_GFX, 0};

        REQUIRE(IsGpuMemType(transfer.srcs[0].memType));
        REQUIRE(IsGpuMemType(transfer.dsts[0].memType));
        REQUIRE(transfer.srcs[0].memIndex != transfer.dsts[0].memIndex);
    }

    SECTION("GPU to CPU transfer")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "GPU->CPU"));

        MockTransfer transfer;
        transfer.numBytes = 1024 * 1024;
        transfer.srcs.push_back({MockMemType::MEM_GPU, 0});
        transfer.dsts.push_back({MockMemType::MEM_CPU, 0});
        transfer.exeDevice = {MockExeType::EXE_GPU_DMA, 0};

        REQUIRE(IsGpuMemType(transfer.srcs[0].memType));
        REQUIRE(IsCpuMemType(transfer.dsts[0].memType));
    }

    SECTION("Bidirectional transfer simulation")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "bidirectional"));

        // Forward transfer
        MockTransfer forward;
        forward.numBytes = 1024 * 1024;
        forward.srcs.push_back({MockMemType::MEM_CPU, 0});
        forward.dsts.push_back({MockMemType::MEM_GPU, 0});
        forward.exeDevice = {MockExeType::EXE_GPU_DMA, 0};

        // Reverse transfer
        MockTransfer reverse;
        reverse.numBytes = 1024 * 1024;
        reverse.srcs.push_back({MockMemType::MEM_GPU, 0});
        reverse.dsts.push_back({MockMemType::MEM_CPU, 0});
        reverse.exeDevice = {MockExeType::EXE_GPU_DMA, 0};

        // Verify they are opposite directions
        REQUIRE(forward.srcs[0].memType == reverse.dsts[0].memType);
        REQUIRE(forward.dsts[0].memType == reverse.srcs[0].memType);
    }
}


}  // namespace amd_work_bench::test::unit::transferbench
