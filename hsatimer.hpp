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

#ifndef ROC_BANDWIDTH_TEST_MYTIME_H_
#define ROC_BANDWIDTH_TEST_MYTIME_H_

// Will use AMD timer and general Linux timer based on users'
// need --> compilation flag. Support for windows platform is
// not currently available

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <string>

using namespace std;

#include <sys/time.h>

#define HSA_FAILURE 1
#define HSA_SUCCESS 0

class PerfTimer {

 private:

  struct Timer {
    string name;       /* < name name of time object*/
    long long _freq;   /* < _freq frequency*/
    long long _clocks; /* < _clocks number of ticks at end*/
    long long _start;  /* < _start start point ticks*/
  };

  std::vector<Timer*> _timers; /*< _timers vector to Timer objects */
  double freq_in_100mhz;

 public:

  PerfTimer();
  ~PerfTimer();

 private:

  // AMD timing method
  uint64_t CoarseTimestampUs();
  uint64_t MeasureTSCFreqHz();

  // General Linux timing method

 public:
  
  int CreateTimer();
  int StartTimer(int index);
  int StopTimer(int index);
  void ResetTimer(int index);

 public:
 
  // retrieve time
  double ReadTimer(int index);
  
  // write into a file
  double WriteTimer(int index);

 public:
  void Error(string str);
};

#endif    //  ROC_BANDWIDTH_TEST_MYTIME_H_
