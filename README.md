# ROCm Bandwidth Test

ROCm Bandwidth Test is designed to capture the performance characteristics of
buffer copying and kernel read/write operations. The help screen of the benchmark
shows various options one can use in initiating cop/read/writer operations.
In addition one can also query the topology of the system in terms of memory
pools and their agents

## Build Environment

To be able to build ROCm Bandwidth Test, users must ensure that the build platform has
following conditions satisfied:

*   cmake

sudo apt  install cmake
sudo snap install cmake  # version 3.14.5, or
        

## Build Procedure

The following simply lists the steps to build ROCm Bandwidth Test

Create a build directory in the project folder - roc_bandwidth_test, e.g.

    mkdir ./build

Set working directory to be the new build directory, e.g.

    cd ./build

Invoke Cmake to interpret build rules and generate native build files
The argument for cmake should be the root folder of ROCm Bandwidth Test
test suite:

Builds Debug version
Assumes pwd is .../roc_bandwidth_test/build e.g.

    cmake -DROCR_LIB_DIR="Path of ROC Runtime Library Files"  \
          -DROCR_INC_DIR="Path of ROC Runtime Header Files"   \
          -DCMAKE_BUILD_TYPE:STRING=Debug ..
          
Builds Release version - default
Assumes pwd is .../roc_bandwidth_test/build, e.g.

    cmake -DROCR_LIB_DIR="Path of ROC Runtime Library Files"  \
          -DROCR_INC_DIR="Path of ROC Runtime Header Files"   \
          ..

Invoke the native build rules generated by cmake to build the various
object, library and executable files, e.g.

    make

Invoke the install command to copy build artifacts to pre-defined folders
of ROCm Bandwidth Test suite. Upon completion artifacts will be copied to the
bin and lib directories of build directory, e.g.

    sudo make install

Note: All executables will be found in <build_directory>/bin folder
