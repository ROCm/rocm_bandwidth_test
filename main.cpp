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

#include <unistd.h>
#include <iostream>
#include "hsatimer.hpp"
#include "rocm_bandwidth_test.hpp"

using namespace std;

int main(int argc, char** argv) {

  // Default behavior is implemented as two runs
  uint32_t arg_cnt = argc;
  if (argc == 1) {
    argc++;
    argv[1] = (char*)"-a";
    setenv("ROCM_BW_DEFAULT_RUN", "true", true);
  }

  // Create the Bandwidth test object
  RocmBandwidthTest bw_test1(argc, argv);

  // Initialize the Bandwidth test object
  bw_test1.SetUp();

  // Run the Bandwidth tests requested by user
  bw_test1.Run();

  // Return if user has not passed in any arguments
  // Display the time taken by various tests
  // Release the Bandwidth test object resources
  if (arg_cnt != 1) {
    bw_test1.Display();
    bw_test1.Close();
    return bw_test1.GetExitValue();
  }

  // Run the second iteration of copy requests
  if (arg_cnt == 1) {
    optind = 1;
    argv[1] = (char*)"-A";
  }

  // Create the Bandwidth test object
  RocmBandwidthTest bw_test2(argc, argv);

  // Initialize the Bandwidth test object
  bw_test2.SetUp();

  // Run the Bandwidth tests requested by user
  bw_test2.Run();

  // Display the time taken by various tests
  // and then release associated resources
  bw_test1.Display();
  bw_test1.Close();

  // Display the time taken by various tests
  // and then release associated resources
  bw_test2.Display();
  bw_test2.Close();

  return 0;
}
