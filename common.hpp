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

#ifndef ROC_BANDWIDTH_TEST_COMMON_HPP
#define ROC_BANDWIDTH_TEST_COMMON_HPP

#include <cstdlib>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdio.h>
#include "hsa/hsa.h"
#include "hsa/hsa_ext_amd.h"

using namespace std;

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__((aligned(x)))
#endif  // __GNUC__
#endif  // _MSC_VER

#define MULTILINE(...) #__VA_ARGS__

#define HSA_ARGUMENT_ALIGN_BYTES 16

#define ErrorCheck(x) error_check(x, __LINE__, __FILE__)

// @Brief: Check HSA API return value
void error_check(hsa_status_t hsa_error_code, int line_num, const char* str);

// @Brief: Find the first avaliable GPU device
hsa_status_t FindGpuDevice(hsa_agent_t agent, void* data);

// @Brief: Find the first avaliable CPU device
hsa_status_t FindCpuDevice(hsa_agent_t agent, void* data);

// @Brief: Find the agent's global region / pool
hsa_status_t FindGlobalPool(hsa_amd_memory_pool_t region, void* data);

// @Brief: Calculate the mean number of the vector
double CalcMean(vector<double> scores);

// @Brief: Calculate the Median valud of the vector
double CalcMedian(vector<double> scores);

// @Brief: Calculate the standard deviation of the vector
double CalcStdDeviation(vector<double> scores, int score_mean);

#endif  // ROC_BANDWIDTH_TEST_COMMON_HPP
