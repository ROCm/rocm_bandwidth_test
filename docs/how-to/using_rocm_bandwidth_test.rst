.. meta::
  :description: ROCm Bandwidth Test is a ROCm application for reporting system information
  :keywords: ROCm bandwidth test usage, RBT usage, Use RBT, Use ROCm bandwidth test, ROCm bandwidth test user guide, RBT user guide, RBT user manual, RBT tests, ROCm bandwidth test tests

.. _using-rbt:

Using ROCm Bandwidth Test
--------------------------

RBT allows you to discover the performance characteristics of Host-To-Device, Device-To-Host, and Device-to-Device copy operations on a ROCm platform.
RBT can be run on any ROCm-compliant platform and it provides various options to experiment with the various copy operation's costs in both unidirectional and bidirectional modes.
You can query the various supported options using the `-h` option.

Tests
=========

This section lists the tests to get performance data for various scenarios.

.. note::

      The tests filter out these unsupported operations:

      * No copy requests when both source (Src) and destination (Dst) devices are CPU.
      * No copy requests when both Src and Dst devices are the same GPU device and the request is either a partial or a full bidirectional copy operation.

Print help screen test
########################

To print the ``help`` screen, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -h

Print ROCm topology test
############################

To print topology, allocatable memory, and access paths of various devices, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -t

The preceding command prints the following:

* RBT version
* List of devices and their allocatable memory
* Access matrix
* Numa distance among the various devices

Copy overhead determination test
######################################

To determine the overhead of the copy path, which includes copy sizes from one byte to hundreds of bytes in increments of power of two, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -s <gpu_dev_IdM> -d <gpu_dev_IdN> -l

The preceding command prints the RBT version and time taken to perform copy for the given device list.

Data path validation test
##############################

To validate data path from a Src to Dst device by copying data, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -v

The preceding command prints the following details:

* RBT version
* List of devices
* Access matrix
* Data path validation among the various devices

Default unidirectional and bidirectional bandwidth test for all devices
##########################################################################

To collect performance characteristics of unidirectional and bidirectional copy operations involving `all` devices on a given Rocm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test

The preceding command issues unidirectional and bidirectional copy operations among all the devices on the platform.

Host-to-Device bandwidth test
##################################

To collect performance characteristics of Host-to-Device (H2D) copy operations on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -s <cpu_dev_IdX>,<cpu_dev_IdY>,- - - -d <gpu_dev_IdM>,<gpu_dev_IdN>, - - -

The preceding command issues unidirectional copy operations between Src and Dst devices. Specifically, it pairs each device from the Src list
with each device from the Dst list. This implies that the command launches sizeof(SrcList) x sizeof(DstList) number of copy operations. Ensure to validate the Src and Dst devices, their device numbers, and the connection between them by looking at the device matrix (or topology) output.

Device-to-Host bandwidth test
##############################

To collect performance characteristics of Device-to-Host (D2H) copy operations on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -s <gpu_dev_IdX>,<gpu_dev_IdY>,- - - -d <cpu_dev_IdM>,<cpu_dev_IdN>, - - -

The preceding command issues unidirectional copy operations between Src and Dst devices. Specifically, it pairs each device from the Src list with each device from the Dst List.
This implies that the command launches sizeof(SrcList) x sizeof(DstList) number of copy operations. Ensure to validate the Src and Dst devices, their device numbers, and the connection between them by looking at the device matrix (or topology) output.

Device-to-Device bandwidth test
################################

To collect performance characteristics of Device-to-Device (D2D) copy operations on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -s <gpu_dev_IdX>,<gpu_dev_IdY>,- - - -d <gpu_dev_IdM>,<gpu_dev_IdN>, - - -

The preceding command issues unidirectional copy operations between Src and Dst devices. Specifically, it pairs each device from the Src list with each device from the Dst List.
This implies that the command launches sizeof(SrcList) x sizeof(DstList) number of copy operations. Ensure to validate the Src and Dst devices, their device numbers, and the connection between them by looking at the device matrix (or topology) output.

Bidirectional bandwidth test
#############################

To collect performance characteristics of bidirectional copy operations on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -b <device_IdX>,<device_IdY>,<device_IdZ>,- - -

The preceding command issues bidirectional copy operations among all the devices specified in the list. The preceding command issues copy(x,x),
copy(x,y), copy(x,z), copy(y,x), copy(y,y), copy(y,z), copy(z,x), copy(z,y), and copy(z,z) operations. The specified devices can either be all GPUs
or a combination of GPUs and CPUs.

Unidirectional bandwidth test for all devices
##############################################

To collect performance characteristics of unidirectional copy operations involving `all` devices on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -a

The preceding command issues unidirectional copy operations among all the devices on the platform.

Bidirectional bandwidth test for all devices
#############################################

To collect performance characteristics of bidirectional copy operations involving `all` devices on a given ROCm platform, use:

.. code-block:: shell

      $ ./rocm_bandwidth_test -A

The preceding command issues bidirectional copy operations among all the devices on the platform.
