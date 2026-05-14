# ROCm Bandwidth Test (RBT-NG) Unit Tests

## Overview

This directory contains comprehensive unit tests for the ROCm Bandwidth Test Next Generation (RBT-NG) application. The tests are designed to validate the core functionality of the WorkBench framework, plugin system, and TransferBench integration.

## Test Framework

- **Catch2 v3.7.1** - Modern C++ testing framework
- **C++23** - Latest C++ standard features

## Test Organization

```
tests/unit/
├── include/
│   └── tst_unit.hpp          # Common test utilities and fixtures
├── src/
│   ├── test_string_utils.cpp        # String manipulation tests
│   ├── test_plugin_management.cpp   # Plugin system tests
│   ├── test_environment_utils.cpp   # Environment variable tests
│   ├── test_memory_utils.cpp        # Memory utilities tests
│   ├── test_workbench_api.cpp       # WorkBench API tests
│   ├── test_gpu_interface.cpp       # GPU interface abstraction tests
│   ├── test_rbt_plugin.cpp          # RBT plugin tests
│   └── test_transferbench_types.cpp # TransferBench type tests
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## Test Categories

### 1. String Utilities (`test_string_utils.cpp`)

Tests the string manipulation functions in `amd_work_bench::utils::strings`:

| Test Section | Description |
|-------------|-------------|
| `Trimming` | right_trim, left_trim, trim_all operations |
| `Splitting` | split_str with various delimiters |
| `Replace` | replace_all, replace_all_copy |
| `Remove` | remove_all character operations |
| `CaseConversion` | to_lower, to_upper transformations |
| `CaseInsensitiveComparison` | equals_ignore_case, contains_ignore_case |

### 2. Plugin Management (`test_plugin_management.cpp`)

Tests the plugin loading and management system:

| Test Section | Description |
|-------------|-------------|
| `PluginData` | PluginData_t structure validation |
| `PluginFunctionality` | Plugin function pointer structure |
| `PluginStatus` | Status enumeration values |
| `PluginEngineVersion` | Engine version enumeration |
| `Extensions` | Plugin file extensions (.amdplug, .amdlplug) |
| `FunctionNames` | Plugin symbol name constants |
| `Feature` | Feature_t structure |
| `SubCommand` | SubCommand_t structure |
| `PathOperations` | Plugin path retrieval |

### 3. Environment Utilities (`test_environment_utils.cpp`)

Tests environment variable operations:

| Test Section | Description |
|-------------|-------------|
| `GetEnvVar` | Retrieving environment variables |
| `SetEnvVar` | Setting environment variables |
| `UnsetEnvVar` | Removing environment variables |
| `Integration` | Full lifecycle testing |
| `ROCmVariables` | ROCm-specific variable checks |

### 4. Memory Utilities (`test_memory_utils.cpp`)

Tests memory management utilities:

| Test Section | Description |
|-------------|-------------|
| `WrapInUnique` | unique_ptr with custom deleters |
| `ScopeGuard` | RAII cleanup guards |
| `ScopedTryLock` | Mutex try-lock wrapper |
| `StorageSizeUnits` | Size unit enumeration |
| `TimeUnits` | Time magnitude enumeration |
| `PcieThroughput` | PCIe throughput enumeration |
| `OStreamJoiner` | Output stream joining utility |

### 5. WorkBench API (`test_workbench_api.cpp`)

Tests the public WorkBench API:

| Test Section | Description |
|-------------|-------------|
| `TaskProgressState` | Task state enumeration |
| `TaskProgress` | Task progress types |
| `RunArguments` | Command line argument structure |
| `SystemInfo` | OS and version information |
| `StartupArgs` | Startup argument management |
| `MainInstance` | Main instance checking |
| `Messaging` | Message handler registration |
| `GPUInfo` | GPU information retrieval |

### 6. GPU Interface (`test_gpu_interface.cpp`)

Tests the GPU abstraction layer:

| Test Section | Description |
|-------------|-------------|
| `GpuIface` | Base interface lifecycle |
| `GpuIfaceFactory` | Factory pattern implementation |
| `GpuTBWorkBench` | WorkBench GPU integration |
| `Polymorphism` | Virtual method dispatch |

### 7. RBT Plugin (`test_rbt_plugin.cpp`)

Tests the ROCm Bandwidth Test plugin:

| Test Section | Description |
|-------------|-------------|
| `Constants` | Plugin constants validation |
| `WordList` | Argument list handling |
| `CLIParsing` | Command line parsing |
| `ModeFlags` | Unidirectional/Bidirectional modes |
| `ExitCodes` | Return code validation |
| `HelpFormatting` | CLI help format |
| `VersionFormat` | Version string format |

### 8. TransferBench Types (`test_transferbench_types.cpp`)

Tests TransferBench core data structures:

| Test Section | Description |
|-------------|-------------|
| `ExeType` | Executor type enumeration |
| `MemType` | Memory type enumeration |
| `ExeDevice` | Executor device structure |
| `MemDevice` | Memory device structure |
| `Transfer` | Transfer operation structure |
| `GeneralOptions` | Benchmark options |
| `DataOptions` | Data validation options |
| `TransferScenarios` | Common transfer patterns |

## Running Tests

### Build Tests

```bash
cd rocm_bandwidth_test/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make rbt_unit_tests
```

### Execute All Tests

```bash
./rbt_unit_tests
```

### Filter by Category

```bash
# Run only string tests
./rbt_unit_tests [strings]

# Run only plugin tests
./rbt_unit_tests [plugins]

# Run only TransferBench type tests
./rbt_unit_tests [transferbench]

# Run multiple categories
./rbt_unit_tests [unit],[memory]
```

### Filter by Test Name

```bash
# Run specific test case
./rbt_unit_tests "StringUtils::Trimming"

# Run tests matching pattern
./rbt_unit_tests "*Plugin*"
```

### Additional Options

```bash
# List all tests
./rbt_unit_tests --list-tests

# List all tags
./rbt_unit_tests --list-tags

# Show successful tests
./rbt_unit_tests --success

# Verbose output
./rbt_unit_tests -v high
```

## Test Tags

| Tag | Description |
|-----|-------------|
| `[unit]` | All unit tests |
| `[strings]` | String utility tests |
| `[plugins]` | Plugin system tests |
| `[environment]` | Environment variable tests |
| `[memory]` | Memory utility tests |
| `[api]` | WorkBench API tests |
| `[gpu]` | GPU interface tests |
| `[rbt]` | RBT plugin tests |
| `[transferbench]` | TransferBench type tests |
| `[!mayfail]` | Tests that may fail (e.g., no GPU) |
| `[contract]` | Contract/specification tests for types that mirror external headers |

## Test Fixtures

### MockPlugin

Mock plugin for testing plugin management:

```cpp
class MockPlugin
{
public:
    static constexpr const char* NAME = "mock_test_plugin";
    static auto create_functionality() -> PluginFunctionality_t;
};
```

## Adding New Tests

1. Create a new test file in `src/`:
   ```cpp
   #include <catch2/catch_all.hpp>
   #include <unit/include/tst_unit.hpp>

   namespace amd_work_bench::test::unit::myfeature
   {
       TEST_CASE("MyFeature::BasicTest", "[unit][myfeature]")
       {
           SECTION("basic functionality")
           {
               REQUIRE(true);
           }
       }
   }
   ```

2. Add the file to `CMakeLists.txt`:
   ```cmake
   set(AMD_UNIT_TEST_SOURCES
       ...
       src/test_my_feature.cpp
   )
   ```

3. Build and run:
   ```bash
   make rbt_unit_tests && ./rbt_unit_tests [myfeature]
   ```

## Code Coverage

Enable coverage with Debug build:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DAMD_APP_ENABLE_TESTS_COVERAGE=ON
make rbt_unit_tests_coverage
```

Coverage report will be generated in `build/coverage_report/`.

## Best Practices

1. **Use descriptive test names**: `TEST_CASE("ClassName::MethodName", "[tags]")`
2. **Use SECTION for related tests**: Groups related test scenarios
3. **Use INFO for context**: Provides debug information on failure
4. **Use REQUIRE for critical checks**: Stops test on failure
5. **Use CHECK for non-critical checks**: Continues test on failure
6. **Clean up resources**: Use RAII or explicit cleanup
7. **Avoid test dependencies**: Each test should be independent

## License

MIT License - See [LICENSE.md](../../LICENSE.md) for details.

---

*Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.*
