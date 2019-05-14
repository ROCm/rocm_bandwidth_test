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

#include "common.hpp"
#include "rocm_bandwidth_test.hpp"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <unistd.h>

// Parse option value string. The string has one decimal
// value as in example: -i 0x33
static bool ParseInitValue(char* value_str, uint8_t&value) {
 
  // Capture the option value string
  uint32_t value_read = strtoul(value_str, NULL, 0);
  return ((value = value_read) && (value_read > 255)) ? false : true;
}

// Parse option value string. The string has one more decimal
// values separated by comma - "3,6,9,12,15".
static bool ParseOptionValue(char* value, vector<size_t>&value_list) {
 
  // Capture the option value string
  std::stringstream stream;
  stream << value;
  
  uint32_t token = 0x11231926;
  do {
    
    // Read the option value
    stream >> token;
    if (stream.fail()) {
      exit(-1);
    }

    // Update output list with values
    value_list.push_back(token);

    // Ignore the delimiter
    if((stream.eof()) ||
       (stream.peek() == ',')) {
      stream.ignore();
    } else {
      return false;
    }

  } while (!stream.eof());

  return true;
}

void RocmBandwidthTest::ValidateCopyBidirFlags(uint32_t copy_ctrl_mask) {

  // It is illegal to specify following flags
  // secondary flag that affects a copy operation
  if ((copy_ctrl_mask & DEV_COPY_LATENCY) ||
       (copy_ctrl_mask & CPU_VISIBLE_TIME) ||
       (copy_ctrl_mask & VALIDATE_COPY_OP)) {
      PrintHelpScreen();
      exit(0);
  }

  return;
}

void RocmBandwidthTest::ValidateCopyUnidirFlags(uint32_t copy_mask,
                                                uint32_t copy_ctrl_mask) {

  if (copy_mask != (USR_SRC_FLAG | USR_DST_FLAG)) {
    PrintHelpScreen();
    exit(0);
  }

  if ((copy_ctrl_mask & DEV_COPY_LATENCY) &&
      (copy_ctrl_mask & USR_BUFFER_SIZE)) {
    PrintHelpScreen();
    exit(0);
  }

  // It is illegal to specify Latency and another
  // secondary flag that affects a copy operation
  if ((copy_ctrl_mask & DEV_COPY_LATENCY) &&
       ((copy_ctrl_mask & USR_BUFFER_INIT) ||
        (copy_ctrl_mask & CPU_VISIBLE_TIME) ||
        (copy_ctrl_mask & VALIDATE_COPY_OP))) {
    PrintHelpScreen();
    exit(0);
  }

  // It is illegal to specify user buffer sizes and another
  // secondary flag that affects a copy operation
  if ((copy_ctrl_mask & USR_BUFFER_SIZE) &&
        (copy_ctrl_mask & VALIDATE_COPY_OP)) {
    PrintHelpScreen();
    exit(0);
  }

  // Check of illegal flags is complete
  return;
}

void RocmBandwidthTest::ValidateCopyAllBidirFlags(uint32_t copy_ctrl_mask) {

  // It is illegal to specify following flags
  // secondary flag that affects a copy operation
  if ((copy_ctrl_mask & DEV_COPY_LATENCY) ||
       (copy_ctrl_mask & USR_BUFFER_SIZE) ||
       (copy_ctrl_mask & CPU_VISIBLE_TIME) ||
       (copy_ctrl_mask & VALIDATE_COPY_OP)) {
      PrintHelpScreen();
      exit(0);
  }

  // Check of illegal flags is complete
  return;
}

void RocmBandwidthTest::ValidateCopyAllUnidirFlags(uint32_t copy_ctrl_mask) {

  // It is illegal to specify following flags
  // secondary flag that affects a copy operation
  if ((copy_ctrl_mask & DEV_COPY_LATENCY) ||
      (copy_ctrl_mask & USR_BUFFER_SIZE)) {
      PrintHelpScreen();
      exit(0);
  }

  // Check of illegal flags is complete
  return;
}

void RocmBandwidthTest::ValidateInputFlags(uint32_t pf_cnt,
                                uint32_t copy_mask, uint32_t copy_ctrl_mask) {

  // Input can't have more than two Primary flags
  if ((pf_cnt == 0) || (pf_cnt > 2)) {
    PrintHelpScreen();
    exit(0);
  }

  // Input specifies unidirectional copy among subset of devices
  // rocm_bandwidth_test -s Di,Dj,Dk -d Dp,Dq,Dr
  if (pf_cnt == 2) {
    return ValidateCopyUnidirFlags(copy_mask, copy_ctrl_mask);
  }

  // Input is requesting to print RBT version
  // rocm_bandwidth_test -q
  if (req_version_ == REQ_VERSION) {
    PrintVersion();
    exit(0);
  }

  // Input is requesting to print ROCm topology
  // rocm_bandwidth_test -t
  if (req_topology_ == REQ_TOPOLOGY) {
    return;
  }

  // Input is for bidirectional bandwidth for some devices
  // rocm_bandwidth_test -b
  if (req_copy_bidir_ == REQ_COPY_BIDIR) {
    return ValidateCopyBidirFlags(copy_ctrl_mask);
  }

  // Input is for bidirectional bandwidth for all devices
  // rocm_bandwidth_test -A
  if (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR) {
    return ValidateCopyAllBidirFlags(copy_ctrl_mask);
  }

  // Input is for unidirectional bandwidth for all devices
  // rocm_bandwidth_test -a
  if (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR) {
    return ValidateCopyAllUnidirFlags(copy_ctrl_mask);
  }

  std::cout << "ValidateInputFlags: This should not be happening" << std::endl;
  assert(false);
  return;
}

void RocmBandwidthTest::BuildDeviceList() {

  // Initialize devices list if copying unidirectional
  // all or bidirectional all mode is enabled
  uint32_t size = pool_list_.size();
  for (uint32_t idx = 0; idx < size; idx++) {
    if (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR) {
      bidir_list_.push_back(idx);
    } else {
      src_list_.push_back(idx);
      dst_list_.push_back(idx);
    }
  }
}

void RocmBandwidthTest::BuildBufferList() {
  
  // User has specified buffer sizes to be used
  if (size_list_.size() != 0) {
    uint32_t size_len = size_list_.size();
    for (uint32_t idx = 0; idx < size_len; idx++) {
      size_list_[idx] = size_list_[idx] * 1024 * 1024;
    }
    return;
  }

  // User has NOT specified buffer sizes to be used
  // For All Copy operations use only one buffer size
  uint32_t size_len = sizeof(SIZE_LIST)/sizeof(size_t);
  for (uint32_t idx = 0; idx < size_len; idx++) {

    if ((req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR) ||
        (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR)) {
      if (idx == 16) {
        size_list_.push_back(SIZE_LIST[idx]);
      }
    }

    if (req_copy_unidir_ == REQ_COPY_UNIDIR) {
      if (latency_) {
        size_list_.push_back(LATENCY_SIZE_LIST[idx]);
      } else if (validate_) {
        if (idx == 16) {
          size_list_.push_back(SIZE_LIST[idx]);
        }
      } else {
        size_list_.push_back(SIZE_LIST[idx]);
      }
    }

    if (req_copy_bidir_ == REQ_COPY_BIDIR) {
      size_list_.push_back(SIZE_LIST[idx]);
    }
  }
}

void RocmBandwidthTest::ParseArguments() {

  bool print_help = 0;
  uint32_t copy_mask = 0;
  uint32_t copy_ctrl_mask = 0;
  uint32_t num_primary_flags = 0;

  // This will suppress prints from getopt implementation
  // In case of error, it will return the character '?' as
  // return value.
  opterr = 0;
  
  int opt;
  bool status;
  while ((opt = getopt(usr_argc_, usr_argv_, "hqtclvaAb:i:s:d:r:w:m:")) != -1) {
    switch (opt) {

      // Print help screen
      case 'h':
        print_help = true;
        break;

      // Print version of the test
      case 'q':
        num_primary_flags++;
        req_version_ = REQ_VERSION;
        break;

      // Print system topology
      case 't':
        num_primary_flags++;
        req_topology_ = REQ_TOPOLOGY;
        break;

      // Enable Unidirectional copy among all valid buffers
      case 'a':
        num_primary_flags++;
        req_copy_all_unidir_ = REQ_COPY_ALL_UNIDIR;
        break;

      // Enable Bidirectional copy among all valid buffers
      case 'A':
        num_primary_flags++;
        req_copy_all_bidir_ = REQ_COPY_ALL_BIDIR;
        break;

      // Collect list of source buffers involved in unidirectional copy operation
      case 's':
        status = ParseOptionValue(optarg, src_list_);
        if (status) {
          num_primary_flags++;
          copy_mask |= USR_SRC_FLAG;
          req_copy_unidir_ = REQ_COPY_UNIDIR;
          break;
        }
        print_help = true;
        break;

      // Collect list of destination buffers involved in unidirectional copy operation
      case 'd':
        status = ParseOptionValue(optarg, dst_list_);
        if (status) {
          num_primary_flags++;
          copy_mask |= USR_DST_FLAG;
          req_copy_unidir_ = REQ_COPY_UNIDIR;
          break;
        }
        print_help = true;
        break;

      // Collect list of agents involved in bidirectional copy operation
      case 'b':
        status = ParseOptionValue(optarg, bidir_list_);
        if (status) {
          num_primary_flags++;
          req_copy_bidir_ = REQ_COPY_BIDIR;
          break;
        }
        print_help = true;
        break;

      // Size of buffers to use in copy and read/write operations
      case 'm':
        status = ParseOptionValue(optarg, size_list_);
        if (status == false) {
          print_help = true;
        }
        copy_ctrl_mask |= USR_BUFFER_SIZE;
        break;

      // Print Cpu time
      case 'c':
        print_cpu_time_ = true;
        copy_ctrl_mask |= CPU_VISIBLE_TIME;
        break;

      // Set Latency mode flag to true
      case 'l':
        latency_ = true;
        copy_ctrl_mask |= DEV_COPY_LATENCY;
        break;

      // Set validation mode flag to true
      case 'v':
        validate_ = true;
        copy_ctrl_mask |= VALIDATE_COPY_OP;
        break;

      // Set initialization mode flag to true
      case 'i':
        init_ = true;
        status = ParseInitValue(optarg, init_val_);
        if (status == false) {
          print_help = true;
        }
        copy_ctrl_mask |= USR_BUFFER_INIT;
        break;

      // Collect request to read a buffer
      case 'r':
        req_read_ = REQ_READ;
        status = ParseOptionValue(optarg, read_list_);
        if (status == false) {
          print_help = true;
        }
        break;

      // Collect request to write a buffer
      case 'w':
        req_write_ = REQ_WRITE;
        status = ParseOptionValue(optarg, write_list_);
        if (status == false) {
          print_help = true;
        }
        break;

      // getopt implementation returns the value of the unknown
      // option or an option with missing operand in the variable
      // optopt
      case '?':
        std::cout << "Argument is illegal or needs value: " << '?' << std::endl;
        if ((optopt == 'b') || (optopt == 's') ||
            (optopt == 'd') || (optopt == 'm') || (optopt == 'i')) {
          std::cout << "Error: Options -b -s -d and -m -i require argument" << std::endl;
        }
        print_help = true;
        break;
      default:
        print_help = true;
        break;
    }
  }
  
  // Print help screen if user option has "-h"
  if (print_help) {
    PrintHelpScreen();
    exit(0);
  }
  
  // Determine input of primary flags is valid
  ValidateInputFlags(num_primary_flags, copy_mask, copy_ctrl_mask);
  
  // Initialize Roc Runtime
  err_ = hsa_init();
  ErrorCheck(err_);

  // Discover the topology of RocR agent in system
  DiscoverTopology();
  
  // Print system topology if user option has "-t"
  if (req_topology_ == REQ_TOPOLOGY) {
    PrintVersion();
    PrintTopology();
    PrintLinkPropsMatrix(LINK_PROP_ACCESS);
    PrintLinkPropsMatrix(LINK_PROP_TYPE);
    PrintLinkPropsMatrix(LINK_PROP_WEIGHT);
    exit(0);
  }

  // Initialize devices list if copying unidirectional
  // all or bidirectional all mode is enabled
  if ((req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR) ||
      (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR)) {
    BuildDeviceList();
  }
  
  // Initialize list of buffer sizes used in copy operations
  BuildBufferList();
  std::sort(size_list_.begin(), size_list_.end());
}

