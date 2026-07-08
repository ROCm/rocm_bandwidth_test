.. meta::
  :description: Guide for migrating from ROCm Bandwidth Test to TransferBench and ROCm Validation Suite
  :keywords: RBT migration, ROCm Bandwidth Test migration, TransferBench, ROCm Validation Suite, RVS, migrate from RBT

.. _rbt-migration-guide:

============================================
Migrating from RBT to TransferBench and RVS
============================================

ROCm Bandwidth Test (RBT) is deprecated and reaches end-of-life with the TheRock-based ROCm 7.14.0
release. RBT's bandwidth-measurement engine was already migrated to TransferBench, so
:doc:`TransferBench <transferbench:index>` provides the same underlying measurement capability and is
the direct technical successor. :doc:`ROCm Validation Suite (RVS) <rocmvalidationsuite:index>` bundles
TransferBench and adds a curated test framework on top of it.

Benefits of transitioning
==========================

Transitioning to TransferBench and RVS provides the following benefits:

- Removes a redundant, overlapping tool

- Formalizes the de facto standard that customer-facing teams already prefer

- Gives standard users a guardrailed entry point rather than an open-ended interface

Choosing the right replacement tool
====================================

The right replacement depends on how you used RBT. If you previously used RBT for bandwidth
measurement, system validation, or as part of a larger toolchain, use the following table to identify
the right tool according to your requirement:

.. list-table::
  :header-rows: 1
  :widths: 38 22 40

  * - RBT use case
    - Recommended tool
    - Reason

  * - Full-control, ad hoc bandwidth measurement
    - :doc:`TransferBench <transferbench:index>`
    - Direct successor to RBT's bandwidth functionality; exposes the complete low-level transfer interface

  * - System or platform validation and field testing
    - :doc:`RVS <rocmvalidationsuite:index>`
    - Curated, selectable test suite built on TransferBench

  * - Getting all relevant tools in a single install
    - :doc:`RVS <rocmvalidationsuite:index>`
    - TransferBench ships bundled with RVS

Migrating to TransferBench
===========================

TransferBench is the recommended path for power users who need full, open-ended control over transfer configuration and measurement. It exposes the complete low-level interface that RBT's measurement engine was built on.

Command mapping
----------------

The following table maps common RBT commands to their TransferBench equivalents.

.. list-table:: RBT to TransferBench command mapping
  :header-rows: 1
  :widths: 35 30 35

  * - RBT command
    - TransferBench equivalent
    - Description

  * - ``rocm_bandwidth_test -a``
    - ``P2P_MODE=1 TransferBench p2p``
    - Unidirectional copy across all device pairs

  * - ``rocm_bandwidth_test -A``
    - ``P2P_MODE=2 TransferBench p2p``
    - Bidirectional copy across all device pairs

  * - ``rocm_bandwidth_test run tb p2p``
    - ``TransferBench p2p``
    - Peer-to-peer device memory bandwidth

  * - ``rocm_bandwidth_test run tb healthcheck``
    - ``TransferBench healthcheck``
    - HBM and host-device bandwidth health check (MI300X / gfx942 series)

  * - ``rocm_bandwidth_test run tb a2a``
    - ``TransferBench a2a``
    - Parallel transfers between all GPU device pairs

  * - ``rocm_bandwidth_test run tb sweep``
    - ``TransferBench sweep``
    - Ordered sweep through sets of transfers

Configuration
--------------

TransferBench is configured through environment variables rather than command-line flags.
The following variables correspond to the most common RBT behaviour:

.. list-table:: Key TransferBench environment variables
  :header-rows: 1
  :widths: 30 15 55

  * - Variable
    - Default
    - Description

  * - ``NUM_ITERATIONS``
    - ``10``
    - Number of timed iterations; a negative value runs for that many seconds

  * - ``NUM_WARMUPS``
    - ``3``
    - Number of warm-up iterations before timing begins

  * - ``OUTPUT_TO_CSV``
    - ``0``
    - Set to ``1`` to write results in CSV format

  * - ``SHOW_ITERATIONS``
    - ``0``
    - Set to ``1`` to display per-iteration timing alongside aggregate results

For the complete list of environment variables and available preset test names, see the
`TransferBench environment variables <https://rocm.docs.amd.com/projects/TransferBench/en/latest/reference/environment-variables.html>`_.

Migrating to ROCm Validation Suite
===================================

RVS is the recommended path for system validation and field use. RVS bundles TransferBench, so
installing RVS gives you the validation framework and the underlying transfer engine in a single
package. RVS also provides curated, selectable tests built on TransferBench, offering a
lower-complexity entry point for standard users and field teams who don't require TransferBench's full
interface.

For RVS installation and usage instructions, see the :doc:`RVS documentation <rocmvalidationsuite:index>`.

Tool support
=============

RVS maintainers support RVS itself — the executable, the test framework, and the selectable-test
entry points. Issues specific to a bundled low-level tool such as TransferBench remain with that
tool's owners; bundling TransferBench into RVS doesn't transfer support ownership.
