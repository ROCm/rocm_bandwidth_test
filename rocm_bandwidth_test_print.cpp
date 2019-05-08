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
#include <iostream>
#include <string>
#include <sstream>

// @Brief: Print Help Menu Screen
void RocmBandwidthTest::PrintHelpScreen() {

  std::cout << std::endl;
  std::cout << "Supported arguments:" << std::endl;
  std::cout << std::endl;
  std::cout << "\t -h    Prints the help screen" << std::endl;
  std::cout << "\t -q    Query version of the test" << std::endl;
  std::cout << "\t -v    Run the test in validation mode" << std::endl;
  std::cout << "\t -l    Run test to collect Latency data" << std::endl;
  std::cout << "\t -c    Time the operation using CPU Timers" << std::endl;
  std::cout << "\t -i    Initialize copy buffer with specified byte pattern" << std::endl;
  std::cout << "\t -t    Prints system topology and allocatable memory info" << std::endl;
  std::cout << "\t -m    List of buffer sizes to use, specified in Megabytes" << std::endl;
  std::cout << "\t -b    List devices to use in bidirectional copy operations" << std::endl;
  std::cout << "\t -s    List of source devices to use in copy unidirectional operations" << std::endl;
  std::cout << "\t -d    List of destination devices to use in unidirectional copy operations" << std::endl;
  std::cout << "\t -a    Perform Unidirectional Copy involving all device combinations" << std::endl;
  std::cout << "\t -A    Perform Bidirectional Copy involving all device combinations" << std::endl;
  std::cout << std::endl;
  
  std::cout << "\t NOTE: Mixing following options is illegal/unsupported" << std::endl;
  std::cout << "\t\t Case 1: rocm_bandwidth_test -a or -A with -c" << std::endl;
  std::cout << "\t\t Case 2: rocm_bandwidth_test -b or -A with -m" << std::endl;
  std::cout << "\t\t Case 3: rocm_bandwidth_test -b or -A with -l" << std::endl;
  std::cout << "\t\t Case 4: rocm_bandwidth_test -b or -A with -v" << std::endl;
  std::cout << "\t\t Case 5: rocm_bandwidth_test -a or -s x -d y with -l and -c" << std::endl;
  std::cout << "\t\t Case 6: rocm_bandwidth_test -a or -s x -d y with -l and -m" << std::endl;
  std::cout << "\t\t Case 7: rocm_bandwidth_test -a or -s x -d y with -l and -v" << std::endl;
  std::cout << std::endl;


  std::cout << std::endl;

}

// @brief: Print the version of the test
void RocmBandwidthTest::PrintVersion() const {

  uint32_t format = 10;
  std::cout.setf(ios::left);

  std::cout << std::endl;
  std::cout.width(format);
  std::cout << "";
  std::cout << "RocmBandwidthTest Version: " << GetVersion() << std::endl;
}

// @brief: Print the topology of Memory Pools and Devices present in system
void RocmBandwidthTest::PrintTopology() {

  uint32_t format = 10;
  size_t count = agent_pool_list_.size();
  std::cout << std::endl;
  for (uint32_t idx = 0; idx < count; idx++) {
    agent_pool_info_t node = agent_pool_list_.at(idx);
    
    std::cout.width(format);
    std::cout << "";
    std::cout.width(format);

    // Print device info
    std::cout << "Device Index:                             "
              << node.agent.index_ << std::endl;
    
    std::cout.width(format);
    std::cout << "";
    std::cout.width(format);
    
    if (HSA_DEVICE_TYPE_CPU == node.agent.device_type_) {
      std::cout << "  Device Type:                            CPU" << std::endl;
    } else if (HSA_DEVICE_TYPE_GPU == node.agent.device_type_) {
      std::cout << "  Device Type:                            GPU" << std::endl;
      std::cout.width(format);
      std::cout << "";
      std::cout.width(format);
      std::cout << "  Device  BDF:                            " << node.agent.bdf_id_ << std::endl;
    }

    // Print pool info
    size_t pool_count = node.pool_list.size();
    for (uint32_t jdx = 0; jdx < pool_count; jdx++) {
      
      std::cout.width(format);
      std::cout << "";
      std::cout.width(format);
      
      std::cout << "    Allocatable Memory Size (KB):         "
           << node.pool_list.at(jdx).allocable_size_ / 1024 << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

std::string GetValueAsString(uint32_t key, uint32_t value) {

  std::stringstream ss;

  switch(key) {
    case RocmBandwidthTest::LINK_PROP_ACCESS:
      ss << value;
      return ss.str();
      break;
    case RocmBandwidthTest::LINK_PROP_HOPS:
    case RocmBandwidthTest::LINK_PROP_WEIGHT:
      ss << value;
      return (value == 0xFFFFFFFF) ? std::string("N/A") :  ss.str();
      break;
    case RocmBandwidthTest::LINK_PROP_TYPE:
      if ((value == RocmBandwidthTest::LINK_TYPE_SELF) ||
          (value == RocmBandwidthTest::LINK_TYPE_NO_PATH) ||
          (value == RocmBandwidthTest::LINK_TYPE_IGNORED)) {
        return std::string("N/A");
      } else if (value == RocmBandwidthTest::LINK_TYPE_XGMI) {
        return std::string("X");
      } else if (value == RocmBandwidthTest::LINK_TYPE_PCIE) {
        return std::string("P");
      }
      break;
  }
  std::cout << "An illegal key to get value for" << std::endl;
  assert(false);
}

void RocmBandwidthTest::PrintLinkPropsMatrix(uint32_t key) const {

  uint32_t format = 10;
  std::cout.setf(ios::left);

  std::cout.width(format);
  std::cout << "";
  std::cout.width(format);
  
  switch(key) {
    case LINK_PROP_ACCESS:
      std::cout << "Inter-Device Access";
      break;
    case LINK_PROP_TYPE:
      std::cout << "Inter-Device Link Type: P = PCIe, X = xGMI, N/A = Not Applicable";
      break;
    case LINK_PROP_HOPS:
      std::cout << "Inter-Device Link Hops";
      break;
    case LINK_PROP_WEIGHT:
      std::cout << "Inter-Device Numa Distance";
      break;
    default:
      std::cout << "An illegal key to print matrix" << std::endl;
      assert(false);
  }
  std::cout << std::endl;
  std::cout << std::endl;

  std::cout.width(format);
  std::cout << "";
  std::cout.width(format);
  std::cout << "D/D";
  for (uint32_t idx0 = 0; idx0 < agent_index_; idx0++) {
    std::cout.width(format);
    std::cout << idx0;
  }
  std::cout << std::endl;
  std::cout << std::endl;

  for (uint32_t src_idx = 0; src_idx < agent_index_; src_idx++) {
    std::cout.width(format);
    std::cout << "";
    std::cout.width(format);
    std::cout << src_idx;
    for (uint32_t dst_idx = 0; dst_idx < agent_index_; dst_idx++) {
      uint32_t value = 0x00;
      switch(key) {
        case LINK_PROP_ACCESS:
          value = direct_access_matrix_[(src_idx * agent_index_) + dst_idx];
          break;
        case LINK_PROP_TYPE:
          value = link_type_matrix_[(src_idx * agent_index_) + dst_idx];
          break;
        case LINK_PROP_HOPS:
          value = link_hops_matrix_[(src_idx * agent_index_) + dst_idx];
          break;
        case LINK_PROP_WEIGHT:
          value = link_weight_matrix_[(src_idx * agent_index_) + dst_idx];
          break;
      }
      std::cout.width(format);
      std::cout << GetValueAsString(key, value);
    }
    std::cout << std::endl;
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

// @brief: Print info on Devices in system
void RocmBandwidthTest::PrintAgentsList() {

  size_t count = agent_pool_list_.size();
  for (uint32_t idx = 0; idx < count; idx++) {
    std::cout << std::endl;
    agent_pool_info_t node = agent_pool_list_.at(idx);
    std::cout << "Device Index:                             "
              << node.agent.index_ << std::endl;
    if (HSA_DEVICE_TYPE_CPU == node.agent.device_type_) {
      std::cout << "  Device Type:                            CPU" << std::endl;
    } else if (HSA_DEVICE_TYPE_GPU == node.agent.device_type_) {
      std::cout << "  Device Type:                            GPU" << std::endl;
      std::cout << "  Device  BDF:                            " << node.agent.bdf_id_ << std::endl;
    }
  }
  std::cout << std::endl;
}

// @brief: Print info on memory pools in system
void RocmBandwidthTest::PrintPoolsList() {

  size_t pool_count = pool_list_.size();
  for (uint32_t jdx = 0; jdx < pool_count; jdx++) {
    std::cout << std::endl;
    std::cout << "Memory Pool Idx:                          "
         << pool_list_.at(jdx).index_ << std::endl;
    std::cout << "  max allocable size in KB:               "
         << pool_list_.at(jdx).allocable_size_ / 1024 << std::endl;
    std::cout << "  segment id:                             "
         << pool_list_.at(jdx).segment_ << std::endl;
    std::cout << "  is kernarg:                             "
         << pool_list_.at(jdx).is_kernarg_ << std::endl;
    std::cout << "  is fine-grained:                        "
         << pool_list_.at(jdx).is_fine_grained_ << std::endl;
    std::cout << "  accessible to owner:                    "
         << pool_list_.at(jdx).owner_access_ << std::endl;
    std::cout << "  accessible to all by default:           "
         << pool_list_.at(jdx).access_to_all_ << std::endl;
  }
  std::cout << std::endl;

}

// @brief: Print the list of transactions that will be executed
void RocmBandwidthTest::PrintTransList() {

  size_t count = trans_list_.size();
  for (uint32_t idx = 0; idx < count; idx++) {
    async_trans_t trans = trans_list_.at(idx);
    std::cout << std::endl;
    std::cout << "                 Transaction Id: " << idx << std::endl;
    std::cout << "               Transaction Type: " << trans.req_type_ << std::endl;
    if ((trans.req_type_ == REQ_READ) || (trans.req_type_ == REQ_WRITE)) {
      std::cout << "Rocm Kernel used by Transaction: " << trans.kernel.code_ << std::endl;
      std::cout << "Rocm Buffer index Used by Kernel: " << trans.kernel.pool_idx_ << std::endl;
      std::cout << "  Rocm Device used for Execution: " << trans.kernel.agent_idx_ << std::endl;
    }
    if ((trans.req_type_ == REQ_COPY_BIDIR) || (trans.req_type_ == REQ_COPY_UNIDIR)) {
      std::cout << "   Src Buffer used in Copy: " << trans.copy.src_idx_ << std::endl;
      std::cout << "   Dst Buffer used in Copy: " << trans.copy.dst_idx_ << std::endl;
    }
    if ((trans.req_type_ == REQ_COPY_ALL_BIDIR) || (trans.req_type_ == REQ_COPY_ALL_UNIDIR)) {
      std::cout << "   Src Memory Pool used in Copy: " << trans.copy.src_idx_ << std::endl;
      std::cout << "   Dst Memory Pool used in Copy: " << trans.copy.dst_idx_ << std::endl;
    }

  }
  std::cout << std::endl;
}

// @brief: Prints error message when a request to copy between
// source buffer and destination buffer is not possible
void RocmBandwidthTest::PrintCopyAccessError(uint32_t src_idx, uint32_t dst_idx) {

  // Retrieve Roc runtime handles for Src memory pool and devices
  uint32_t src_dev_idx = pool_list_[src_idx].agent_index_;
  hsa_device_type_t src_dev_type = agent_list_[src_dev_idx].device_type_;
    
  // Retrieve Roc runtime handles for Dst memory pool and devices
  uint32_t dst_dev_idx = pool_list_[dst_idx].agent_index_;
  hsa_device_type_t dst_dev_type = agent_list_[dst_dev_idx].device_type_;

  std::cout << std::endl;
  std::cout << "Src Device: Index "
            << src_dev_idx
            << ", Type: "
            << ((src_dev_type == HSA_DEVICE_TYPE_CPU) ? "CPU" : "GPU") << std::endl;
  std::cout << "Dst Device: Index "
            << dst_dev_idx
            << ", Type: "
            << ((dst_dev_type == HSA_DEVICE_TYPE_CPU) ? "CPU" : "GPU") << std::endl;
  std::cout << "Rocm Device hosting Src Memory cannot ACCESS Dst Memory" << std::endl;
  std::cout << std::endl;
}

// @brief: Prints error message when a request to read / write from
// a buffer of a device is not possible
void RocmBandwidthTest::PrintIOAccessError(uint32_t exec_idx, uint32_t pool_idx) {

  // Retrieve device type of executing device
  hsa_device_type_t exec_dev_type = agent_list_[exec_idx].device_type_;
    
  // Retrieve device type of memory pool's device
  uint32_t pool_dev_idx = pool_list_[pool_idx].agent_index_;
  hsa_device_type_t pool_dev_type = agent_list_[pool_dev_idx].device_type_;

  std::cout << std::endl;
  std::cout << "Index of Executing Device: " << exec_idx << std::endl;
  std::cout << "Device Type of Executing Device: " << exec_dev_type << std::endl;

  std::cout << "Index of Buffer: " << pool_idx << std::endl;
  std::cout << "Index of Buffer's Device: " << pool_dev_idx << std::endl;
  std::cout << "Device Type Hosting Buffer: " << pool_dev_type << std::endl;
  std::cout << "Rocm Device executing Read / Write request cannot ACCESS Buffer" << std::endl;
  std::cout << std::endl;
}
