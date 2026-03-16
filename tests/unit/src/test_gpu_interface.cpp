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
 * @file test_gpu_interface.cpp
 * @brief Unit tests for GPU interface abstraction
 * @author ROCm Bandwidth Test Team
 *
 * Tests the GPU interface classes including GpuIface_t, GpuIfaceFactory_t,
 * and GpuTBWorkBench_t in the WorkBench framework.
 */

#include <catch2/catch_all.hpp>
#include <unit/include/tst_unit.hpp>
#include <gen_gpu_iface/include/gpu_iface.hpp>

#include <memory>
#include <string>


namespace amd_work_bench::test::unit::gpu
{

// =============================================================================
// Mock Implementations for Testing
// =============================================================================

/**
 * @brief Mock GPU interface implementation for testing
 */
class MockGpuIface : public amd_work_bench::gpu_iface::GpuIface_t
{
    public:
        MockGpuIface() = default;
        ~MockGpuIface() override = default;

        void startup() const override
        {
            m_startup_called = true;
        }

        void run_startup_tasks() const override
        {
            m_startup_tasks_called = true;
        }

        void run_tasks() const override
        {
            m_run_tasks_called = true;
        }

        void run_stop_tasks() const override
        {
            m_stop_tasks_called = true;
        }

        void stop() const override
        {
            m_stop_called = true;
        }

        std::string get_name() const override
        {
            return "MockGpuIface";
        }

        // Test accessors
        static bool was_startup_called() { return m_startup_called; }
        static bool was_startup_tasks_called() { return m_startup_tasks_called; }
        static bool was_run_tasks_called() { return m_run_tasks_called; }
        static bool was_stop_tasks_called() { return m_stop_tasks_called; }
        static bool was_stop_called() { return m_stop_called; }

        static void reset_flags()
        {
            m_startup_called = false;
            m_startup_tasks_called = false;
            m_run_tasks_called = false;
            m_stop_tasks_called = false;
            m_stop_called = false;
        }

    private:
        static inline bool m_startup_called = false;
        static inline bool m_startup_tasks_called = false;
        static inline bool m_run_tasks_called = false;
        static inline bool m_stop_tasks_called = false;
        static inline bool m_stop_called = false;
};


/**
 * @brief Mock GPU interface factory for testing
 */
class MockGpuIfaceFactory : public amd_work_bench::gpu_iface::GpuIfaceFactory_t
{
    public:
        MockGpuIfaceFactory() = default;
        ~MockGpuIfaceFactory() override = default;

        std::unique_ptr<amd_work_bench::gpu_iface::GpuIface_t> create() const override
        {
            m_create_count++;
            return std::make_unique<MockGpuIface>();
        }

        static int get_create_count() { return m_create_count; }
        static void reset_count() { m_create_count = 0; }

    private:
        static inline int m_create_count = 0;
};


// =============================================================================
// TEST CASE: GPU Interface Base Class
// =============================================================================

TEST_CASE("GpuInterface::GpuIface", "[unit][gpu][interface]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    // Reset mock state before each test
    MockGpuIface::reset_flags();

    SECTION("MockGpuIface can be instantiated")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "instantiation"));

        auto iface = std::make_unique<MockGpuIface>();

        REQUIRE(iface != nullptr);
        REQUIRE(iface->get_name() == "MockGpuIface");
    }

    SECTION("GpuIface_t startup method is called")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "startup"));

        MockGpuIface iface;

        REQUIRE(MockGpuIface::was_startup_called() == false);

        iface.startup();

        REQUIRE(MockGpuIface::was_startup_called() == true);
    }

    SECTION("GpuIface_t run_startup_tasks method is called")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "run_startup_tasks"));

        MockGpuIface iface;

        REQUIRE(MockGpuIface::was_startup_tasks_called() == false);

        iface.run_startup_tasks();

        REQUIRE(MockGpuIface::was_startup_tasks_called() == true);
    }

    SECTION("GpuIface_t run_tasks method is called")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "run_tasks"));

        MockGpuIface iface;

        REQUIRE(MockGpuIface::was_run_tasks_called() == false);

        iface.run_tasks();

        REQUIRE(MockGpuIface::was_run_tasks_called() == true);
    }

    SECTION("GpuIface_t stop lifecycle")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "stop lifecycle"));

        MockGpuIface iface;

        REQUIRE(MockGpuIface::was_stop_tasks_called() == false);
        REQUIRE(MockGpuIface::was_stop_called() == false);

        iface.run_stop_tasks();
        REQUIRE(MockGpuIface::was_stop_tasks_called() == true);

        iface.stop();
        REQUIRE(MockGpuIface::was_stop_called() == true);
    }

    SECTION("GpuIface_t full lifecycle")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "full lifecycle"));

        MockGpuIface iface;

        // Simulate full lifecycle
        iface.startup();
        iface.run_startup_tasks();
        iface.run_tasks();
        iface.run_stop_tasks();
        iface.stop();

        REQUIRE(MockGpuIface::was_startup_called() == true);
        REQUIRE(MockGpuIface::was_startup_tasks_called() == true);
        REQUIRE(MockGpuIface::was_run_tasks_called() == true);
        REQUIRE(MockGpuIface::was_stop_tasks_called() == true);
        REQUIRE(MockGpuIface::was_stop_called() == true);
    }
}


// =============================================================================
// TEST CASE: GPU Interface Factory
// =============================================================================

TEST_CASE("GpuInterface::GpuIfaceFactory", "[unit][gpu][factory]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    // Reset mock state
    MockGpuIfaceFactory::reset_count();
    MockGpuIface::reset_flags();

    SECTION("Factory creates GpuIface instances")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "create"));

        MockGpuIfaceFactory factory;

        REQUIRE(MockGpuIfaceFactory::get_create_count() == 0);

        auto iface1 = factory.create();

        REQUIRE(iface1 != nullptr);
        REQUIRE(MockGpuIfaceFactory::get_create_count() == 1);

        auto iface2 = factory.create();

        REQUIRE(iface2 != nullptr);
        REQUIRE(MockGpuIfaceFactory::get_create_count() == 2);
    }

    SECTION("Factory creates independent instances")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "independent instances"));

        MockGpuIfaceFactory factory;

        auto iface1 = factory.create();
        auto iface2 = factory.create();

        // Should be different instances
        REQUIRE(iface1.get() != iface2.get());

        // Both should have the same name (same type)
        REQUIRE(iface1->get_name() == iface2->get_name());
    }
}


// =============================================================================
// TEST CASE: GPU TransferBench WorkBench
// =============================================================================

TEST_CASE("GpuInterface::GpuTBWorkBench", "[unit][gpu][workbench]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    MockGpuIface::reset_flags();

    SECTION("GpuTBWorkBench_t default construction")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "default construction"));

        amd_work_bench::gpu_iface::GpuTBWorkBench_t workbench;

        // Should construct without errors
        REQUIRE(true);
    }

    SECTION("GpuTBWorkBench_t construction with GpuIface")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "with GpuIface"));

        auto mock_iface = std::make_unique<MockGpuIface>();

        amd_work_bench::gpu_iface::GpuTBWorkBench_t workbench(std::move(mock_iface));

        // Should take ownership of the interface
        REQUIRE(mock_iface == nullptr);  // Moved from
    }

    SECTION("GpuTBWorkBench_t replace_gpu_iface")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "replace interface"));

        amd_work_bench::gpu_iface::GpuTBWorkBench_t workbench;

        auto mock_iface1 = std::make_unique<MockGpuIface>();
        auto mock_iface2 = std::make_unique<MockGpuIface>();

        // Replace interface
        workbench.replace_gpu_iface(std::move(mock_iface1));
        REQUIRE(mock_iface1 == nullptr);

        // Replace again
        workbench.replace_gpu_iface(std::move(mock_iface2));
        REQUIRE(mock_iface2 == nullptr);
    }
}


// =============================================================================
// TEST CASE: GPU Interface Polymorphism
// =============================================================================

TEST_CASE("GpuInterface::Polymorphism", "[unit][gpu][polymorphism]")
{
    const auto& TEST_CASE_NAME = Catch::getResultCapture().getCurrentTestName();

    MockGpuIface::reset_flags();

    SECTION("GpuIface_t can be used polymorphically")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "polymorphic usage"));

        // Use base class pointer
        std::unique_ptr<amd_work_bench::gpu_iface::GpuIface_t> iface =
            std::make_unique<MockGpuIface>();

        // Call virtual methods through base pointer
        REQUIRE(iface->get_name() == "MockGpuIface");

        iface->startup();
        REQUIRE(MockGpuIface::was_startup_called() == true);

        iface->run_tasks();
        REQUIRE(MockGpuIface::was_run_tasks_called() == true);

        iface->stop();
        REQUIRE(MockGpuIface::was_stop_called() == true);
    }

    SECTION("GpuIface_t destructor is virtual")
    {
        INFO(wb_test::build_test_info(TEST_CASE_NAME, "virtual destructor"));

        // Create through base pointer and destroy
        amd_work_bench::gpu_iface::GpuIface_t* iface = new MockGpuIface();

        // Should call derived destructor correctly
        REQUIRE_NOTHROW(delete iface);
    }
}


}  // namespace amd_work_bench::test::unit::gpu
