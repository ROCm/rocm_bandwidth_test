<!-- Based on:
Changelog: https://keepachangelog.com/en/1.1.0/
Semantic Versioning: https://semver.org/spec/v2.0.0.html

Types of changes:
    - `Unreleased`: Keep this section at the top to track upcoming changes
        - So people can see what changes they might expect in upcoming releases
        - By the release time, we can just move this section changes into a new release version section
    - `Added`: for new features
    - `Changed`: for changes in existing functionality
    - `Deprecated`: for soon-to-be removed features
    - `Removed`: for now removed features
    - `Fixed`: for any bug fixes
        - Links to Issues/PRs (Optional, but very helpful). It helps trace the origin of the change:
            - Resolved race condition in multithreaded mode. [#123](https://github.com/ROCm/rocm_bandwidth_test/issues/123)
    - `Security`: in case of vulnerabilities
    - `Known Issues`: it's optional.
        - Use it when:
            - Releasing a new version that introduces a regression or has unresolved bugs we want users to be aware of
            - We are maintaining an open source project where stability matters (e.g., libraries, CLI tools, frameworks)
            - We want to communicate transparency and avoid users filing duplicate bug reports
        - Avoid it if:
            - The issues are minor, obscure, or already addressed in the next patch
            - It would clutter the changelog for small/personal projects
        - Best Practices:
            - Be specific
                - Include symptoms, conditions, and known workarounds if any
            - Avoid vague language
                – Say "Performance degradation on large files" instead of "Performance issues."
            - Track with issues
                – Link to the relevant GitHub issues
                    - Framework might not find plugins. [#456](https://github.com/ROCm/rocm_bandwidth_test/issues/456) -->

# Change Log for RBT

Full documentation for RBT is available at [ROCm Bandwidth Test Documentation](https://rocm.docs.amd.com/projects/rocm_bandwidth_test/en/latest/index.html).


## RBT for ROCm 7.2.0

### Fixed

- [541223/562920]: rocm-bandwidth-test folder no longer present after driver uninstallation

## RBT for ROCm 7.1.1

### Fixed

- [544642]: RBT test failed with error cannot make canonical path
- [550724]: healthcheck test fails with seg fault on gfx942
- [553891]: schmoo & one2all observed Segmentation fault when executed on sgpu setup


### Known Issues

- [541223/562920]: rocm-bandwidth-test folder present after driver uninstallation
    * After running `amdgpu-uninstall`, the `rocm-bandwidth-test` folder and package are still present
    * Workaround: Remove the package manually; `sudo apt-get remove -y rocm-bandwidth-test`



## RBT for ROCm 7.0.0 (2.6.0)

### Added

- Plugin architecture:
    - `rocm_bandwidth_test` is now the `framework` for individual `plugins` and features.

        The **framework** is available at: `/opt/rocm/bin/`

    - Individual `plugins`:

        The **plugins (shared libraries)** are available at: `/opt/rocm/lib/rocm_bandwidth_test/plugins/`

>[!NOTE]
>Please review the [README](./README.md) file for details about the new options and outputs.


### Changed

- The `CLI` and options/parameters have changed due to the new *plugin architecture*, where the plugin parameters are parsed by the plugin.


### Removed

- The old CLI, parameters, switches used.


### Fixed

- N/A

### Security

- N/A


### Known Issues

- N/A



## RBT for ROCm 6.4.x

### Added

- The latest `RBT (2.6.x and higher)` is not built/distributed with `ROCm 6.4.x` by default. However, it should be straightforward to built it under that environment.
