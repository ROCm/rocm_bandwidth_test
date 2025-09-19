# ROCm Bandwidth Test (RBT-Next Generation, RBT-NG)
[![AMD](https://img.shields.io/badge/AMD-%23000000.svg?style=for-the-badge&logo=amd&logoColor=white)](https://www.amd.com)
[![Language](https://img.shields.io/badge/Language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
[![Standard](https://img.shields.io/badge/C-23-purple.svg)](https://en.wikipedia.org/wiki/C23_(C_standard_revision))
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
<!---
[![Build](https://github.com/ROCm/rocm_bandwidth_test/actions/workflows/unit-test.yml/badge.svg)](https://github.com/ROCm/rocm_bandwidth_test/actions/workflows/unit-test.yml)
-->

**RBT-NG** is the next generation of the ROCm Bandwidth Test, designed to measure the bandwidth between CPU and GPU devices on AMD platforms. This version has been completely rewritten with a focus on modularity and extensibility through a plugin-based architecture.

>[!NOTE]
>This project is a successor to [RBT](https://rocm.docs.amd.com/projects/rocm_bandwidth_test/en/latest/index.html)
>and [TB](https://rocm.docs.amd.com/projects/TransferBench/en/latest/index.html).

## Overview

RBT-NG leverages [TransferBench (TB)](https://github.com/ROCm/TransferBench) as its core engine for benchmarking data transfers. This approach allows for more flexible and efficient testing scenarios.

- **Plugin Architecture:** Easily extend the functionality of RBT-NG with custom plugins.
- **Modular Design:** Separate concerns for better maintenance and development.
- **Enhanced Performance:** Improved algorithms and integration with TransferBench for precise measurements.

See the [full library and API documentation](https://rocm.docs.amd.com/en/latest/deploy/linux/index.html)


## Features

- **CPU to GPU Bandwidth Testing:** Measure data transfer rates in various scenarios.
- **GPU to GPU Bandwidth Testing:** For systems with multiple GPUs.
- **Multi-Threaded Testing:** Simulate real-world applications with concurrent data transfers.
- **Extensibility:** Add new test scenarios through plugins.

## Installation

### Prerequisites

- ROCm compatible hardware
- ROCm stack installed ([Installation Guide](https://rocm.docs.amd.com/en/latest/deploy/linux/index.html))
- *Clang (19.0.1+) / GCC (12.4.0+) compiler(s)
- CMake (3.20+)
- CURL / libcurl4-openssl-dev (libcurl-devel)


> [!NOTE]
> This project uses the following external libraries:
> - [boost](https://github.com/boostorg/boost)
> - [Catch2](https://github.com/catchorg/Catch2)
> - [CLI11](https://github.com/CLIUtils/CLI11)
> - [fmt](https://github.com/fmtlib/fmt)
> - [json](https://github.com/nlohmann/json)
> - [spdlog](https://github.com/gabime/spdlog)
>
> These submodules are checked out and matched with the required versions or commits, according to the tool version.


### Build from Source

See [Build from Source](#Build_from_Source)

```bash
git clone https://github.com/ROCm/rocm_bandwidth_test
cd rocm_bandwidth_test
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug|Release
make
```

## Usage

For the RBT-NG tool to run and be able to find the related plugins properly, we need to make sure the following environment variable(s) are set:

- `PATH`: includes the **location for the executable**
- `LD_LIBRARY_PATH`: includes the **location for the libraries *required by the plugins***

The RBT-NG plugins are installed at: `…/lib/rocm_bandwidth_test/plugins/`

Under `ROCm`, if the `ROCM_PATH` variable is set to `/opt/rocm`, we will have something like:

- `PATH`: `$ROCM_PATH/bin:…`
- `LD_LIBRARY_PATH`: `$ROCM_PATH/lib:…`
- Plugins will be at: `$ROCM_PATH/lib/rocm_bandwidth_test/plugins/`

We can set them manually (current session only) or added to the user’s shell configuration file (e.g., `~/.bashrc`, `~/.zshrc`) for automatic loading in new sessions (persistence across sessions):

```bash
export PATH=/opt/rocm/bin:$PATH
export LD_LIBRARY_PATH=/opt/rocm/lib:$LD_LIBRARY_PATH
```


### Help options:
RBT-NG can be run with various options to customize tests:
```bash
$ ./rocm_bandwidth_test --help

AMD ROCm Bandwidth Test: Command Line Interface

Usage: [OPTIONS] SUBCOMMAND

OPTIONS:
  -h,     --help              Print this help message and exit
          --builtin-help      Print help for builtin plugin/subcommands
          --version           Print the utility/framework version

SUBCOMMANDS:
  plugin                      Plugin: subcommand
  run                         Run a plugin


$ ./rocm_bandwidth_test --version

AMD ROCm Bandwidth Test:
 -> version: 2.6.0-debug
 -> [Commit: 052f4b09b2fd8fffb581e7be64848181c9c802f2 / Branch: develop / Build Type: Engineering]
Environment:
 -> Kernel: Host: amd-dev-tools  Kernel: 6.8.0-64-generic  v#67-Ubuntu SMP PREEMPT_DYNAMIC Sun Jun 22 03:42:59 UTC 2025  Arch: x86_64
 -> OS: Distro: Ubuntu  Version: 24.04


$ ./rocm_bandwidth_test --builtin-help

        Help: AMD ROCm Bandwidth Test Command Line Interface
        Usage: rocm_bandwidth_test [subcommand] [options]

        Available subcommands (builtin):

       --builtin-help      Print help about this command
       --pcie-info         Display PCIE Print Spec info help about this command


$ ./rocm_bandwidth_test --pcie-info

         * PCIe link performance *
 Version   Introduced    Transfer Rate                    Bandwidth (GB/s)
   1.0         2003         2.500 GT/s   x1       0.250   x2       0.500   x4       1.000   x8       2.000   x16      4.000
   2.0         2007         5.000 GT/s   x1       0.500   x2       1.000   x4       2.000   x8       4.000   x16      8.000
   3.0         2010         8.000 GT/s   x1       0.985   x2       1.969   x4       3.938   x8       7.877   x16     15.754
   4.0         2017        16.000 GT/s   x1       1.969   x2       3.938   x4       7.877   x8      15.754   x16     31.508
   5.0         2019        32.000 GT/s   x1       3.938   x2       7.877   x4      15.754   x8      31.508   x16     63.015
   6.0         2022        64.000 GT/s   x1       7.563   x2      15.125   x4      30.250   x8      60.500   x16    121.000
   7.0     2025 (plan)    128.000 GT/s   x1      15.125   x2      30.250   x4      60.500   x8     121.000   x16    242.000
        *Reference: https://en.wikipedia.org/wiki/PCI_Express

```

### Plugin options:
RBT-NG plugin related options:
```bash
$ ./rocm_bandwidth_test plugin --help
Plugin: subcommand

plugin [OPTIONS]

OPTIONS:
  -h,     --help              Print this help message and exit
  -c,     --legacy [1]        Legacy mode: For plugins supporting legacy output
  -l,     --list TEXT [*]     List plugin(s) (all by default)
  -i,     --info TEXT [*]     Get information about plugin(s)
  -r,     --run TEXT          Run a plugin
  -v,     --verbose [0]       Verbose output


$ ./rocm_bandwidth_test plugin --list

    Plugin Management:

        *Registered plugin(s):
        - Hello      > Builtin: Hello                                > Version: 0.0.1
        - tb         > Builtin: TransferBench                        > Version: 1.0.0


$ ./rocm_bandwidth_test plugin --info

    Plugin Management:

        *Registered plugin(s):
        - Hello      > Builtin: Hello                                > Version: 0.0.1
          - Author: Linux System Tools Team (MLSE Linux) @AMD     > Framework Compat.V: 2.6.0-debug
          - Loaded: No      > Path: /opt/rocm-7.0.0/lib/rocm_bandwidth_test/plugins/amd_hello.amdplug                            [=] Main Entry Avail: Yes

        - tb         > Builtin: TransferBench                        > Version: 1.0.0
          - Author: Linux System Tools Team (MLSE Linux) @AMD     > Framework Compat.V: 2.6.0-debug
          - Loaded: No      > Path: /opt/rocm-7.0.0/lib/rocm_bandwidth_test/plugins/transferbench.amdplug                        [=] Main Entry Avail: Yes

## These are equivalent ways of running a plugin.
$ ./rocm_bandwidth_test run custom_plugin_shortname

$ ./rocm_bandwidth_test plugin --run tb
$ ./rocm_bandwidth_test run tb

```


## Plugins

Plugins are the heart of RBT-NG's extensibility. Here's how you can create or use them:

- Creating Plugins: Follow the `Plugin Development Guide`.
- Using Existing Plugins: Check the `plugins` directory for available options.


## Documentation

- [Official RBT Documentation](https://rocm.docs.amd.com/projects/rocm_bandwidth_test/en/latest/index.html)
- [TransferBench Documentation](https://rocm.docs.amd.com/projects/TransferBench/en/latest/index.html)


## Contributing & support

We welcome contributions! Please read our [CONTRIBUTING](./CONTRIBUTING.md) for details on how to get started.

Join the community by reporting issues or asking questions via [GitHub issues](https://github.com/ROCm/rocm_bandwidth_test/issues). All feedback and proposals are welcome.

Please review our [SECURITY](./SECURITY.md) policy for information on reporting security issues.


## License

This project is licensed under the MIT Software License. See accompanying file [LICENSE](./LICENSE.md) file or copy [here](https://opensource.org/licenses/MIT) for legal details.
See [AUTHORS](./AUTHORS.md) file for details about the current maintainers.


## Acknowledgments

- Thanks to AMD ROCm team for providing the foundational work with RBT and TB.
- AMD MLSE Linux team
- Contributors to this project.
- Other projects who inspired this one:
  - [ROCm](https://github.com/ROCm/)
  - [TransferBench (TB)](https://github.com/ROCm/TransferBench)
  - [Developer Hub](https://www.amd.com/en/developer/resources/rocm-hub.html)


## DISCLAIMER

<p align="justify">
The information contained herein is for informational purposes only, and is subject to change without notice. In
addition, any stated support is planned and is also subject to change. While every precaution has been taken in
the preparation of this document, it may contain technical inaccuracies, omissions and typographical errors, and
AMD is under no obligation to update or otherwise correct this information. Advanced Micro Devices, Inc. makes no
representations or warranties with respect to the accuracy or completeness of the contents of this document, and
assumes no liability of any kind, including the implied warranties of noninfringement, merchantability or fitness
for particular purposes, with respect to the operation or use of AMD hardware, software or other products
described herein.
</p>

© 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
