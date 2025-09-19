
.. meta::
  :description: ROCm Bandwidth Test is a ROCm application for reporting system information
  :keywords: Install ROCm Bandwidth Test, Build ROCm Bandwidth Test, Install RBT, Build RBT

.. _installing-rbt:

Installing ROCm Bandwidth Test
-----------------------------------

This topic provides information required to build and install ROCm Bandwidth Test (RBT). You can obtain RBT either by installing ROCm repository or build from source.

Prerequisites
==============

- `ROCm compatible hardware <https://rocm.docs.amd.com/en/develop/compatibility/compatibility-matrix.html>`
- AMD GPU hardware for `supported GPUs <https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html#supported-gpus>`_
- Linux system supported by ROCm as described in `system requirements <https://rocm.docs.amd.com/projects/install-on-linux/en/develop/reference/system-requirements.html>`_.
- `ROCm stack installed <https://rocm.docs.amd.com/projects/install-on-linux/en/develop/>`
- Compilers: Clang 19.0.1 or later or GCC 12.4.0 or later
- CMake 3.20 or later
- CURL or libcurl4-openssl-dev (libcurl-devel)

Installing ROCm repository
=============================

The RBT tool is available through the ROCm repositories:

.. code-block:: shell

    ## For Ubuntu/Debian systems:
    sudo apt-get install rocm-bandwidth-test

    ## Or for RHEL/CentOS/Fedora systems:
    sudo yum install rocm-bandwidth-test

Building RBT from source
=========================

Here are the steps to build RBT from source:

1. Create a build directory in the project folder: ``rocm_bandwidth_test``.

   .. code-block:: shell

    mkdir ./build

2. Set working directory to the new build directory.

   .. code-block:: shell

    cd ./build

3. Invoke ``Cmake`` to interpret build rules and generate native build files:

   .. code-block:: shell

    ## Assume that ROCR Runtime has its libraries & headers are located in the path :
    ## libraries : _ABSOLUTE_PATH_TO_ROCR_LIBS_/lib
    ## headers   : _ABSOLUTE_PATH_TO_ROCR_LIBS_/include/hsa
    ## Note : Observe that both include & lib folder are under common path (_ABSOLUTE_PATH_TO_ROCR_LIBS_)

    ## Builds Debug or Release version
    $ pwd
    ../rocm_bandwidth_test/build

    ## Standalone build
    $ cmake -DCMAKE_BUILD_TYPE="Debug | Release" .. \
            -DAMD_APP_STANDALONE_BUILD_PACKAGE=ON \
            -DAMD_APP_ENGINEERING_BUILD_PACKAGE=OFF && cmake --build .

        ..

    ## ROCm build
    ## Assumes ROCm is installed on the build system
    $ cmake -DCMAKE_BUILD_TYPE="Debug | Release" .. \
            -DAMD_APP_STANDALONE_BUILD_PACKAGE=OFF \
            -DAMD_APP_ROCM_BUILD_PACKAGE=ON && cmake --build .

You can build RBT from source available at `GitHub <https://github.com/ROCm/rocm_bandwidth_test>`_. Access to the source is currently limited to approved users. To request permission, file a ticket `here. <https://github.com/ROCm/ROCm/issues/new/choose>`_

Installing RBT
===============

Invoke the ``install`` command to copy build artifacts to predefined folders of the RBT suite. Upon completion, the artifacts are copied to the ``bin`` and ``lib`` directories of the build directory.

.. code-block:: shell

    ## For Makefiles
    make install

    ## For CMake; Default install location is /opt/rocm/
    cmake --install . --prefix /path/to/install/location

.. note::

  You can find all executables in the ``<build_directory>``.

Running RBT
============

To run the RBT tool and find related plugins, set the following environment variables:

- ``PATH``: Includes the location for the executable

- ``LD_LIBRARY_PATH``: Includes the location for the libraries required by the plugins

The RBT plugins are installed at ``…/lib/rocm_bandwidth_test/plugins/``.

For example, under ROCm, if the ``ROCM_PATH`` variable is set to ``/opt/rocm``, the environment variables are set as:

- ``PATH``: ``$ROCM_PATH/bin:…``

- ``LD_LIBRARY_PATH``: ``$ROCM_PATH/lib:…``

The RBT plugins are installed at: ``$ROCM_PATH/lib/rocm_bandwidth_test/plugins/``.

You can set the environment variables manually for a single session or add them to the user's shell configuration file such as ``~/.bashrc`` or ``~/.zshrc`` for the values to persist across the sessions:

.. code-block:: shell

  export PATH=/opt/rocm/bin:$PATH
  export LD_LIBRARY_PATH=/opt/rocm/lib:$LD_LIBRARY_PATH
