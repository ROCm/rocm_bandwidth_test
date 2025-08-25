.. meta::
  :description: ROCm Bandwidth Test is a ROCm application for reporting system information.
  :keywords: ROCm Bandwidth Test documentation, RBT documentation, RBT

===================================
ROCm Bandwidth Test documentation
===================================

The ROCm Bandwidth Test (RBT) is a ROCm application for reporting system information. It is designed to measure the bandwidth between CPU and GPU devices on AMD platforms. RBT captures the performance characteristics of buffer copying and kernel read or write operations.

RBT leverages `TransferBench <https://github.com/ROCm/TransferBench>`_ as its core engine for benchmarking data transfers. This approach facilitates more flexible and efficient testing scenarios. The RBT tool is designed to provide the following advantages:

- Plugin architecture: Easily extends the RBT functionality with custom plugins

- Modular design: Separate concerns for better maintenance and development

- Enhanced performance: Improved algorithms and integration with TransferBench for precise measurements

Here are the RBT features:

- CPU to GPU bandwidth testing: To measure data transfer rates in various scenarios

- GPU to GPU bandwidth testing: For systems with multiple GPUs

- Multi-threaded testing: To simulate real-world applications with concurrent data transfers

- Extensibility: To add new test scenarios through plugins

The code is open source and hosted at: https://github.com/ROCm/rocm_bandwidth_test

.. grid:: 2
  :gutter: 3

  .. grid-item-card:: Install

       * :ref:`Installation <installing-rbt>`

  .. grid-item-card:: How to

      * :ref:`using-rbt`


To contribute to the documentation, refer to
`Contributing to ROCm <https://rocm.docs.amd.com/en/latest/contribute/contributing.html>`_.

You can find licensing information on the
`Licensing <https://rocm.docs.amd.com/en/latest/about/license.html>`_ page.


