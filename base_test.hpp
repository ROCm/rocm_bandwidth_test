////////////////////////////////////////////////////////////////////////////////
//
// The University of Illinois/NCSA
// Open Source License (NCSA)
// 
// Copyright (c) 2014-2015, Advanced Micro Devices, Inc. All rights reserved.
// 
// Developed by:
// 
//                 AMD Research and AMD HSA Software Development
// 
//                 Advanced Micro Devices, Inc.
// 
//                 www.amd.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//  - Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimers.
//  - Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimers in
//    the documentation and/or other materials provided with the distribution.
//  - Neither the names of Advanced Micro Devices, Inc,
//    nor the names of its contributors may be used to endorse or promote
//    products derived from this Software without specific prior written
//    permission.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS WITH THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef ROC_BANDWIDTH_TEST_BASE_H_
#define ROC_BANDWIDTH_TEST_BASE_H_

#include "hsa/hsa.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// @Brief: An interface for tests to do some basic things,

class BaseTest {

 public:

  BaseTest(size_t num = 3);

  virtual ~BaseTest();

  // @Brief: Allows setup proceedures to be completed
  // before running the benchmark test case
  virtual void SetUp() = 0;

  // @Brief: Launches the proceedures of test scenario
  virtual void Run() = 0;

  // @Brief: Allows clean up proceedures to be invoked
  virtual void Close() = 0;

  // @Brief: Display the results
  virtual void Display() const = 0;

  // @Brief: Set number of iterations to run
  void set_num_iteration(size_t num) {
    num_iteration_ = num;
    return;
  }

  // @Brief: Pre-declare some variables for deriviation, the
  // derived class may declare more if needed
 protected:

  // @Brief: Real iteration number
  uint64_t num_iteration_;

  // @Brief: Status code
  hsa_status_t err_;
};

#endif  //  ROC_BANDWIDTH_TEST_BASE_H_
