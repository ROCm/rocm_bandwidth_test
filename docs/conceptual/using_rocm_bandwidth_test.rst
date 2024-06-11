


Using ROCm Bandwidth Test
--------------------------

The application rocm_bandwidth_test was developed to allow users discover the performance characteristics of Host-To-Device, Device-To-Host and Device-To-Device copy operations on a ROCm platform. The application can be run on any compliant Rocm platform. The application
provides various options for users to experiment the cost of various copy operations in both unidirectional and bidirectional modes. Users can query the various options that are supported by giving the `-h` option.

Building ROCm Bandwidth Test
=============================

ROCm Repo Forest

.. code-block::

      cd ~/git/compute/tests/rocm_bandwidth_test
      Execute build_rocm_bandwidth_test.sh script
      Clone of Github Repo:
      cd rocm_bandwidth_test
      mkdir build
      cd build
      export BUILD_TYPE=Debug | Release
      export ROCR_INC_DIR=<Path To ROCr Header Files>
      export ROCR_LIB_DIR=<Path To ROCr Library Files>
      cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DROCR_LIB_DIR=$ROCR_LIB_DIR -
      DROCR_INC_DIR=$ROCR_INC_DIR ..

The following sections show how users can use the test to get performance data for various scenarios:

.. Note:: 

The test will filter out certain operations that are either considered not supported or don't make sense. These include the following:

* No copy requests when both Src and Dst devices are CPU.
* No copy requests when both Src and Dst devices are Same GPU device and the request is either a partial or a full bidirectional copy operation.

.. Note::

Users can build it from source available at github. Currently access to source is limited to approved users. To request permission, contact the author of this page.

Print the Help screen test
==========================

Run the test to print the Help screen,

.. code-block:: shell

      <shell_prompt> ./rocm_bandwidth_test -h

Print ROCm topology test
=========================

Run the test to print topology of the various devices, their allocatable memory and access paths,

.. code-block::

      <shell_prompt> ./rocm_bandwidth_test -t

The above command will print four things: Version of RBT, List of devices and their allocatable memory, Access matrix and Numa Distance among the various devices.

Copy overhead determination test
===============================

Run the test to determine overhead of copy path - copy sizes from ONE byte to hundreds of bytes in increments of power of 2

.. code-block::

      <shell_prompt> ./rocm_bandwidth_test -s <gpu_dev_IdM> -d <gpu_dev_IdN> -l

The above command will print Version of RBT and time taken to perform copy for given device list.

Data path validation test
==============================

Run the test to validate data path from a source device to destination device by by copying data,

.. code-block::

      <shell_prompt> ./rocm_bandwidth_test -v

The above command will print the following four details: 

* Version of ROCm Bandwidth Test (RBT)
* List of devices
* Access matrix 
* Data Path Validation among the various devices


Default unidirectional & bidirectional all devices bandwidth test
==================================================================

Run the test to collect performance characteristics of unidirectional and bidirectional copy operations involving ALL devices of a given Rocm platform.

.. code-block::

      <shell_prompt> ./rocm_bandwidth_test
