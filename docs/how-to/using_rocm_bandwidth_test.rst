


Using ROCm Bandwidth Test
--------------------------

The application ROCm Bandwidth Test (RBT) was developed to allow users discover the performance characteristics of Host-To-Device, Device-To-Host, and Device-to-Device copy operations on a ROCm platform. The application can be run on any compliant ROCm platform. The application provides various options for users to experiment the cost of various copy operations in both unidirectional and bidirectional modes. Users can query the various options that are supported by giving the `-h` option.

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

The following sections show how users can use the test to get performance data for various scenarios.

.. Note:: 

The test will filter out certain operations that are considered not supported. These include:

* No copy requests when both Src and Dst devices are CPU.
* No copy requests when both Src and Dst devices are Same GPU device and the request is either a partial or a full bidirectional copy operation.

.. Note::

Users can build it from source available at GitHub. Currently access to source is limited to approved users. To request permission, file a ticket `here. <https://github.com/ROCm/ROCm/issues/new/choose>`_

Print Help screen test
==========================

Run the following test to print the Help screen,

.. code-block:: shell

      <shell_prompt> ./rocm_bandwidth_test -h

Print ROCm topology test
=========================

Run the following test to print topology of the various devices, their allocatable memory, and access paths,

.. code-block::

      <shell_prompt> ./rocm_bandwidth_test -t

The above command will print the following: 

* Version of RBT
* List of devices and their allocatable memory
* Access matrix 
* Numa Distance among the various devices

Copy overhead determination test
===============================

Run the following test to determine overhead of copy path - copy sizes from ONE byte to hundreds of bytes in increments of power of 2

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

The above command will issue unidirectional and bidirectional copy operations among all the devices of the platform.

Host-to-Device (H2D) bandwidth
================================

Run the test to collect performance characteristics of H2D copy operations of a given ROCm platform.

.. code-block::
            
            <shell_prompt> ./rocm_bandwidth_test -s <cpu_dev_IdX>,<cpu_dev_IdY>,- - - -d <gpu_dev_IdM>,
            <gpu_dev_IdN>, - - -

The above command will issue unidirectional copy operations between Src and Dst devices. Specifically it will pair each device of Src List it
with each device of Dst List i.e. it will launch sizeof(SrcList) x sizeof(DstList) number of copy operations. It is assumed that user has
determined access from Src device to Dst device exists by consulting device access matrix.


Device-to-Host (D2H) bandwidth
===============================

Run the test to collect performance characteristics of D2H copy operations of a given ROCm platform.

.. code-block::

            <shell_prompt> ./rocm_bandwidth_test -s <gpu_dev_IdX>,<gpu_dev_IdY>,- - - -d <cpu_dev_IdM>,
            <cpu_dev_IdN>, - - -

The above command will issue unidirectional copy operations between Src and Dst devices. Specifically, it will pair each device of Src List with each device of Dst List. For example, it will launch sizeof(SrcList) x sizeof(DstList) number of copy operations. It is assumed that users have  determined access from Src device to Dst device exists by consulting device access matrix.


Device-to-Device (D2D) bandwidth
==================================

Run the test to collect performance characteristics of D2D copy operations of a given ROCm platform.

.. code-block::

            <shell_prompt> ./rocm_bandwidth_test -s <gpu_dev_IdX>,<gpu_dev_IdY>,- - - -d <gpu_dev_IdM>,<gpu_
            dev_IdN>, - - -

The above command will issue copy unidirectional operations between Src and Dst devices. Specifically it will pair each device of Src List it
with each device of Dst List i.e. it will launch sizeof(SrcList) x sizeof(DstList) number of copy operations. It is assumed that users have
determined access from Src device to Dst device exists by consulting device access matrix.

Bidirectional bandwidth
===========================

Run the test to collect performance characteristics of bidirectional copy operations of a given ROCm platform.

.. code-block::

            <shell_prompt> ./rocm_bandwidth_test -b <device_IdX>,<device_IdY>,<device_IdZ>,- - -

The above command will issue bidirectional copy operations among all the devices of the list. In the example given it will issue copy(x,x),
copy(x,y), copy(x,z), copy(y,x), copy(y,y), copy(y,z), copy(z,x), copy(z,y) and copy(z,z) operations. The devices can be either be all GPUs
or GPU/CPU combination.

Unidirectional all devices bandwidth
=====================================

Run the test to collect performance characteristics of unidirectional copy operations involving ALL devices of a given ROCm platform.

.. code-platform::

            <shell_prompt> ./rocm_bandwidth_test -a

The above command will issue unidirectional copy operations among all the devices of the platform.

Bidirectional all devices bandwidth
=======================================

Run the test to collect performance characteristics of bidirectional copy operations involving ALL devices of a given ROCm platform.

.. code-block::

            <shell_prompt> ./rocm_bandwidth_test -A

The above command will issue bidirectional copy operations among all the devices of the platform.
