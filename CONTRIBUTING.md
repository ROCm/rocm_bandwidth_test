<!-- Based on:
https://github.com/ROCm/.github/blob/main/docs/templates/contributing_template.md
-->

# Contributing to RBT (ROCm Bandwidth Test) #

We welcome contributions to RBT.  Please follow these details to help ensure that your contributions are successfully accepted.


## Issue Discussion ##

Please use the GitHub Issues tab to report any issues you encounter.

* Use your best judgment for issue creation. If your issue is already listed, upvote the issue and
  comment or post to provide additional details, such as how you reproduced this issue.
* If you're not sure if your issue is the same, err on the side of caution and file your issue.
  You can add a comment to include the issue number (and link) for the similar issue. If we determine that your issue is the same as an existing one, we'll close the duplicate.
* If your issue doesn't exist, use the `issue template` to submit a new issue.
  * When filing an issue, be sure to provide as much information as possible, including your `amdgpu` driver version, GPUs used, and commands ran. This helps reduce the time required to reproduce your issue.
  * Check your issue regularly, as we may require additional information to reproduce the issue successfully.
* You may also open an issue to ask the maintainers questions about whether a proposed change meets the acceptance criteria, or to discuss an idea about the library.


## Acceptance Criteria ##

The goal of the RBT project is to provide a `central hub for all GPU bandwidth analysis` by using a unified plugin architecture.

Contributors wanting to submit changes to the main framework or additional plugins must follow the guidelines below.


## Code Structure ##



## Coding Style ##

Please refer to `.clang-format`. It is suggested you use the `pre-commit` tool.
It mostly follows Google C++ formatting, with a 100-character line limit.


## Pull Request Guidelines ##

When creating a pull request (PR), target the default branch **amd-staging**, which serves as our *integration* branch.

> [!NOTE]
> By creating a PR, you agree to the statements made in the [code license](#code-license) section.


### Deliverables ###


   /*
    * SPDX-License-Identifier: MIT License
    *
    * Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
    *
    * Permission is hereby granted, free of charge, to any person obtaining a
    * copy of this software and associated documentation files (the "Software"),
    * to deal in the Software without restriction, including without limitation
    * the rights to use, copy, modify, merge, publish, distribute, sublicense,
    * and/or sell copies of the Software, and to permit persons to whom the
    * Software is furnished to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in
    * all copies or substantial portions of the Software.
    *
    * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
    * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    * OTHER DEALINGS IN THE SOFTWARE.
    *
    */

   /**
    * Author(s):   Author's Name <authors_emails>
    *
    * Description: file_name.[cpp/hpp]
    *              file_description
    *
    */


### Process ###

* Reviewers are listed in the `.github/CODEOWNERS` file
* Code format guidelines

RBT uses the `clang-format tool` for formatting code in source files.
The formatting style is captured in `.clang-format`, which is located at
the root of the project. There are different options to follow:

    1. Using *pre-commit* and *docker*: `pre-commit run`
    2. Using *only clang-format*: `clang-format -i \<path-to-the-source-file\>`


* Code format guidelines:

    1. If it's inserting something into the existing classes/functions, try to follow the existing style as closely as possible.
    2. If it's brand new code or refactoring a complete class or area of the code, please follow as Modern C++ of a style as you can and reference the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines) as much as you possibly can.


## Code License ##

All code contributed to this project will be licensed under the MIT Software License. See accompanying file [LICENSE](./LICENSE.md) file or copy [here](https://opensource.org/licenses/MIT) for legal details.


## References ##

1. [pre-commit](https://github.com/pre-commit/pre-commit)
2. [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
3. [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)

