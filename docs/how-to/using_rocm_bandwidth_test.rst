.. meta::
  :description: ROCm Bandwidth Test is an ROCm application for reporting system information
  :keywords: ROCm bandwidth test usage, RBT usage, Use RBT, Use ROCm bandwidth test, ROCm bandwidth test user guide, RBT user guide, RBT user manual, RBT tests, ROCm bandwidth test tests

.. _using-rbt:

Using ROCm Bandwidth Test
--------------------------

The ROCm Bandwidth Test (RBT) tool is a ROCm application for reporting system information and measuring the bandwidth of various copy operations.

RBT is mainly composed of:

* The executable ``rocm_bandwidth_test``

  It is the framework for the RBT plugin architecture:

  * A command-line utility
  * A plugin manager that loads the shared library plugins at runtime
  * Located in the ``/opt/rocm/bin/`` directory

* A library named ``libamd_work_bench.so``

  It is the core library that implements the plugin manager and the command-line utility, among other plugin-related functions.

  * Located in the ``/opt/rocm/lib/`` directory

* A set of plugins that implement various tests, outputs and functionalities

  These are shared libraries ``(*.amdplug)`` that implement various tests. Each plugin is loaded at runtime by the plugin manager.

  * Located in the ``/opt/rocm/lib/rocm_bandwidth_test/plugins/`` directory

Through the plugins, RBT helps you explore the performance characteristics of Host-to-Device, Device-to-Host, and Device-to-Device copy operations on a ROCm platform.

RBT can be run on any ROCm-compliant platform and provides various options to experiment with the costs of different copy operations in both unidirectional and bidirectional modes.
You can query the various supported options using the ``-h`` option.

Command-line options
=====================

The following table lists the command-line options available for the RBT framework:

.. list-table:: RBT options
  :header-rows: 1

  * - Option
    - Description
    - Usage

  * - ``help``
    - Prints general help screen
    - ``./rocm_bandwidth_test --help``

  * - ``version``
    - Prints tool version
    - ``$ ./rocm_bandwidth_test --version``

  * - ``builtin-help``
    - Prints builtin plugin help screen
    - ``$ ./rocm_bandwidth_test --builtin-help``

  * - ``pcie-info``
    - Prints PCIe link performance screen
    - ``$ ./rocm_bandwidth_test --pcie-info``

  * - ``plugin --help``
    - Prints plugin help screen
    - ``$ ./rocm_bandwidth_test plugin --help``

  * - ``plugin --list | plugin -l``
    - Prints list of registered plugins
    - ``$ ./rocm_bandwidth_test plugin -l``

  * - ``plugin --info | plugin -i``
    - Prints detailed information of registered plugins
    - ``$ ./rocm_bandwidth_test plugin -i``

  * - ``plugin --run``
    - Runs a plugin
    - ``$ ./rocm_bandwidth_test plugin --run tb``

  * - ``run``
    - Runs a plugin
    - ``$ ./rocm_bandwidth_test run custom_plugin_shortname`` or ``$ ./rocm_bandwidth_test run tb``

.. note::

  Both command lines are valid:

  * ``rocm_bandwidth_test`` (executable)
  * ``rocm-bandwidth-test`` (symlink to the executable)
