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

#include <iomanip>
#include <sstream>
#include <algorithm>

static void printRecord(size_t size, double avg_time,
                        double avg_bandwidth, double min_time,
                        double peak_bandwidth) {

  std::stringstream size_str;
  if (size < 1024) {
    size_str << size << " Bytes";
  } else if (size < 1024 * 1024) {
    size_str << size / 1024 << " KB";
  } else {
    size_str << size / (1024 * 1024) << " MB";
  }

  uint32_t format = 15;
  std::cout.precision(6);
  std::cout << std::fixed;
  std::cout.width(format);
  std::cout << size_str.str();
  std::cout.width(format);
  std::cout << (avg_time * 1e6);
  std::cout.width(format);
  std::cout << avg_bandwidth;
  std::cout.width(format);
  std::cout << (min_time * 1e6);
  std::cout.width(format);
  std::cout << peak_bandwidth;
  std::cout << std::endl;
}

static void printCopyBanner(uint32_t src_pool_id, uint32_t src_agent_type,
                            uint32_t dst_pool_id, uint32_t dst_agent_type) {

  std::stringstream src_type;
  std::stringstream dst_type;
  (src_agent_type == 0) ? src_type <<  "Cpu" : src_type << "Gpu";
  (dst_agent_type == 0) ? dst_type <<  "Cpu" : dst_type << "Gpu";

  std::cout << std::endl;
  std::cout << "================";
  std::cout << "           Benchmark Result";
  std::cout << "         ================";
  std::cout << std::endl;
  std::cout << "================";
  std::cout << " Src Device Id: " << src_pool_id;
  std::cout << " Src Device Type: " << src_type.str();
  std::cout << " ================";
  std::cout << std::endl;
  std::cout << "================";
  std::cout << " Dst Device Id: " << dst_pool_id;
  std::cout << " Dst Device Type: " << dst_type.str();
  std::cout << " ================";
  std::cout << std::endl;
  std::cout << std::endl;

  uint32_t format = 15;
  std::cout.setf(ios::left);
  std::cout.width(format);
  std::cout << "Data Size";
  std::cout.width(format);
  std::cout << "Avg Time(us)";
  std::cout.width(format);
  std::cout << "Avg BW(GB/s)";
  std::cout.width(format);
  std::cout << "Min Time(us)";
  std::cout.width(format);
  std::cout << "Peak BW(GB/s)";
  std::cout << std::endl;
}

double RocmBandwidthTest::GetMinTime(std::vector<double>& vec) {

  std::sort(vec.begin(), vec.end());
  return vec.at(0);
}

double RocmBandwidthTest::GetMeanTime(std::vector<double>& vec) {

  std::sort(vec.begin(), vec.end());
  vec.erase(vec.begin());
  vec.erase(vec.begin(), vec.begin() + num_iteration_ * 0.1);
  vec.erase(vec.begin() + num_iteration_, vec.end());

  double mean = 0.0;
  int num = vec.size();
  for (int it = 0; it < num; it++) {
    mean += vec[it];
  }
  mean /= num;
  return mean;
}

void RocmBandwidthTest::Display() const {

  // Iterate through list of transactions and display its timing data
  uint32_t trans_size = trans_list_.size();
  if (trans_size == 0) {
    std::cout << std::endl;
    std::cout << "  Invalid Request" << std::endl;
    std::cout << std::endl;
    return;
  }
  
  if (validate_) {
    PrintVersion();
    DisplayDevInfo();
    PrintLinkPropsMatrix(LINK_PROP_ACCESS);
    DisplayValidationMatrix();
    return;
  }

  if (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR) {
    PrintVersion();
    DisplayDevInfo();
    PrintLinkPropsMatrix(LINK_PROP_ACCESS);
    PrintLinkPropsMatrix(LINK_PROP_WEIGHT);
    DisplayCopyTimeMatrix(true);
    return;
  }

  if (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR) {
    if (bw_default_run_ == NULL) {
      PrintVersion();
      DisplayDevInfo();
      PrintLinkPropsMatrix(LINK_PROP_ACCESS);
      PrintLinkPropsMatrix(LINK_PROP_WEIGHT);
    }
    DisplayCopyTimeMatrix(true);
    return;
  }

  if ((req_copy_bidir_ == REQ_COPY_BIDIR) ||
      (req_copy_unidir_ == REQ_COPY_UNIDIR)) {
      PrintVersion();
  }

  for (uint32_t idx = 0; idx < trans_size; idx++) {
    async_trans_t trans = trans_list_[idx];
    if ((trans.req_type_ == REQ_COPY_BIDIR) ||
        (trans.req_type_ == REQ_COPY_UNIDIR)) {
      DisplayCopyTime(trans);
    }
    if ((trans.req_type_ == REQ_READ) ||
        (trans.req_type_ == REQ_WRITE)) {
      DisplayIOTime(trans);
    }
  }
  std::cout << std::endl;
}

void RocmBandwidthTest::DisplayIOTime(async_trans_t& trans) const {

}

void RocmBandwidthTest::DisplayCopyTime(async_trans_t& trans) const {

  // Print Benchmark Header
  uint32_t src_idx = trans.copy.src_idx_;
  uint32_t dst_idx = trans.copy.dst_idx_;
  uint32_t src_dev_idx = pool_list_[src_idx].agent_index_;
  hsa_device_type_t src_dev_type = agent_list_[src_dev_idx].device_type_;
  uint32_t dst_dev_idx = pool_list_[dst_idx].agent_index_;
  hsa_device_type_t dst_dev_type = agent_list_[dst_dev_idx].device_type_;
  printCopyBanner(src_idx, src_dev_type, dst_idx, dst_dev_type);

  uint32_t size_len = size_list_.size();
  for (uint32_t idx = 0; idx < size_len; idx++) {
    printRecord(size_list_[idx], trans.avg_time_[idx],
                trans.avg_bandwidth_[idx], trans.min_time_[idx],
                trans.peak_bandwidth_[idx]);
  }
}

void RocmBandwidthTest::PopulatePerfMatrix(bool peak, double* perf_matrix) const {

  uint32_t trans_size = trans_list_.size();
  for (uint32_t idx = 0; idx < trans_size; idx++) {
    async_trans_t trans = trans_list_[idx];
    uint32_t src_idx = trans.copy.src_idx_;
    uint32_t dst_idx = trans.copy.dst_idx_;
    uint32_t src_dev_idx = pool_list_[src_idx].agent_index_;
    uint32_t dst_dev_idx = pool_list_[dst_idx].agent_index_;

    // For COPY_ALL_UNIDIR and COPY_ALL_BIDIR we use only one copy size
    double bandwidth = (peak) ? trans.peak_bandwidth_[0] : trans.avg_bandwidth_[0];
    perf_matrix[(src_dev_idx * agent_index_) + dst_dev_idx] = bandwidth;
    if (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR) {
      perf_matrix[(dst_dev_idx * agent_index_) + src_dev_idx] = bandwidth;
    }
  }

}

void RocmBandwidthTest::PrintPerfMatrix(bool validate, bool peak, double* perf_matrix) const {

  uint32_t format = 10;
  std::cout.setf(ios::left);

  std::cout.width(format);
  std::cout << "";
  std::cout.width(format);
  
  if (validate == false) { 
    if ((peak) && (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR)) {
      std::cout << "Unidirectional copy peak bandwidth GB/s";
    }
    
    if ((peak == false) && (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR)) {
      std::cout << "Unidirectional copy average bandwidth GB/s";
    }
    
    if ((peak) && (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR)) {
      std::cout << "Bdirectional copy peak bandwidth GB/s";
    }
    
    if ((peak == false) && (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR)) {
      std::cout << "Bidirectional copy average bandwidth GB/s";
    }
  } else {
    std::cout << "Data Path Validation";
  }

  std::cout << std::endl;
  std::cout << std::endl;
  std::cout.precision(6);
  std::cout << std::fixed;

  std::cout.width(format);
  std::cout << "";
  std::cout.width(format);
  std::cout << "D/D";
  format = 12;
  for (uint32_t idx0 = 0; idx0 < agent_index_; idx0++) {
    std::cout.width(format);
    std::stringstream agent_id;
    agent_id << idx0;
    std::cout << agent_id.str();
  }
  std::cout << std::endl;
  std::cout << std::endl;
  for (uint32_t idx0 = 0; idx0 < agent_index_; idx0++) {
    format = 10;
    std::cout.width(format);
    std::cout << "";
    std::stringstream agent_id;
    agent_id << idx0;
    std::cout.width(format);
    std::cout << agent_id.str();
    for (uint32_t idx1 = 0; idx1 < agent_index_; idx1++) {
      format = 12;
      std::cout.width(format);
      double value = perf_matrix[(idx0 * agent_index_) + idx1];
      if (validate) {
        if (value == 0) {
          std::cout << "N/A";
        } else if (value < 1) {
          std::cout << "FAIL";
        } else {
          std::cout << "PASS";
        }
      } else {
        if (value == 0) {
          std::cout << "N/A";
        } else {
          std::cout << perf_matrix[(idx0 * agent_index_) + idx1];
        }
      }
    }
    std::cout << std::endl;
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void RocmBandwidthTest::DisplayCopyTimeMatrix(bool peak) const {

  double* perf_matrix = new double[agent_index_ * agent_index_]();
  PopulatePerfMatrix(peak, perf_matrix);
  PrintPerfMatrix(false, peak, perf_matrix);
  free(perf_matrix);
}

void RocmBandwidthTest::DisplayValidationMatrix() const {

  double* perf_matrix = new double[agent_index_ * agent_index_]();
  PopulatePerfMatrix(true, perf_matrix);
  PrintPerfMatrix(true, true, perf_matrix);
  free(perf_matrix);
}

void RocmBandwidthTest::DisplayDevInfo() const {

  uint32_t format = 10;
  std::cout.setf(ios::left);

  std::cout << std::endl;
  for (uint32_t idx = 0; idx < agent_index_; idx++) {
    uint32_t active = active_agents_list_[idx];
    if (active == 1) {
      std::cout.width(format);
      std::cout << "";
      std::cout << "Device: " << idx;
      std::cout << ",  " << agent_list_[idx].name_;
      bool gpuDevice = (agent_list_[idx].device_type_ == HSA_DEVICE_TYPE_GPU);
      if (gpuDevice) {
        std::cout << ",  " << agent_list_[idx].bdf_id_;
      }
      std::cout << std::endl;
    }
  }
  std::cout << std::endl;
}


