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

#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <cctype>
#include <cmath>
#include <sstream>
#include <limits>

// Initialize the variable used to capture validation failure
const double RocmBandwidthTest::VALIDATE_COPY_OP_FAILURE = std::numeric_limits<double>::max();

// The values are in megabytes at allocation time
const size_t RocmBandwidthTest::SIZE_LIST[] = { 1 * 1024,
                                2 * 1024, 4 * 1024, 8 * 1024,
                                16 * 1024, 32 * 1024, 64 * 1024,
                                128 * 1024, 256 * 1024, 512 * 1024,
                                1 * 1024 * 1024, 2 * 1024 * 1024,
                                4 * 1024 * 1024, 8 * 1024 * 1024,
                                16 * 1024 * 1024, 32 * 1024 * 1024,
                                64 * 1024 * 1024, 128 * 1024 * 1024,
                                256 * 1024 * 1024, 512 * 1024  * 1024};

const size_t RocmBandwidthTest::LATENCY_SIZE_LIST[] = { 1,
                                2, 4, 8,
                                16, 32, 64,
                                128, 256, 512,
                                1 * 1024, 2 * 1024,
                                4 * 1024, 8 * 1024,
                                16 * 1024, 32 * 1024,
                                64 * 1024, 128 * 1024,
                                256 * 1024, 512 * 1024 };

uint32_t RocmBandwidthTest::GetIterationNum() {
  return (validate_) ? 1 : (num_iteration_ * 1.2 + 1);
}

void RocmBandwidthTest::AcquireAccess(hsa_agent_t agent, void* ptr) {
  err_ = hsa_amd_agents_allow_access(1, &agent, NULL, ptr);
  ErrorCheck(err_);
}

void RocmBandwidthTest::AcquirePoolAcceses(uint32_t src_dev_idx,
                                   hsa_agent_t src_agent, void* src,
                                   uint32_t dst_dev_idx,
                                   hsa_agent_t dst_agent, void* dst) {

  // determine which one is a cpu and call acquire on the other agent
  hsa_device_type_t src_dev_type = agent_list_[src_dev_idx].device_type_;
  hsa_device_type_t dst_dev_type = agent_list_[dst_dev_idx].device_type_;
  if (src_dev_type == HSA_DEVICE_TYPE_GPU) {
    AcquireAccess(src_agent, dst);
  }

  if (dst_dev_type == HSA_DEVICE_TYPE_GPU) {
    AcquireAccess(dst_agent, src);
  }

  return;
}
  
void RocmBandwidthTest::InitializeSrcBuffer(size_t size, void* buf_cpy,
                                     uint32_t cpy_dev_idx, hsa_agent_t cpy_agent) {

  // Allocate host buffers and setup accessibility for copy operation
  if (init_src_ == NULL) {
    err_ = hsa_amd_memory_pool_allocate(sys_pool_, size, 0, (void**)&init_src_);
    ErrorCheck(err_);
    long double* src_buf = (long double*) init_src_;
    uint32_t count = (size / sizeof(long double));
    for (uint32_t idx = 0; idx < count; idx++) {
      src_buf[idx] = (init_) ? init_val_ : sin(idx);
    }
    err_ = hsa_signal_create(0, 0, NULL, &init_signal_);
    ErrorCheck(err_);
  }

  // If copying agent is a CPU, use memcpy to initialize copy buffer
  hsa_device_type_t cpy_dev_type = agent_list_[cpy_dev_idx].device_type_;
  if (cpy_dev_type == HSA_DEVICE_TYPE_CPU) {
    memcpy(buf_cpy, init_src_, size);
    return;
  }

  // Copying device is a Gpu, setup buffer access
  // before copying initialization buffer
  AcquireAccess(cpy_agent, init_src_);
  hsa_signal_store_relaxed(init_signal_, 1);
  copy_buffer(buf_cpy, cpy_agent,
              init_src_, cpu_agent_,
              size, init_signal_);
  return;
}
  
bool RocmBandwidthTest::ValidateDstBuffer(size_t max_size, size_t curr_size, void* buf_cpy,
                                          uint32_t cpy_dev_idx, hsa_agent_t cpy_agent) {

  // Allocate host buffers and setup accessibility for copy operation
  if (validate_dst_ == NULL) {
    err_ = hsa_amd_memory_pool_allocate(sys_pool_, max_size, 0, (void**)&validate_dst_);
    ErrorCheck(err_);
  }

  // If Copy device is a Gpu setup buffer access
  memset(validate_dst_, ~(0x23), curr_size);
  hsa_device_type_t cpy_dev_type = agent_list_[cpy_dev_idx].device_type_;
  if (cpy_dev_type == HSA_DEVICE_TYPE_GPU) {
    AcquireAccess(cpy_agent, validate_dst_);
    hsa_signal_store_relaxed(init_signal_, 1);
    copy_buffer(validate_dst_, cpu_agent_,
                buf_cpy, cpy_agent,
                curr_size, init_signal_);
  } else {

    // Copying device is a CPU, copy dst buffer
    // into validation buffer
    memcpy(validate_dst_, buf_cpy, curr_size);
  }

  // Compare initialization buffer with validation buffer
  err_ = (hsa_status_t)memcmp(init_src_, validate_dst_, curr_size);
  if (err_ != HSA_STATUS_SUCCESS) {
    exit_value_ = err_;
  }
  return (err_ == HSA_STATUS_SUCCESS);
}

void RocmBandwidthTest::AllocateConcurrentCopyResources(bool bidir,
                                vector<async_trans_t>& trans_list,
                                vector<void*>& buf_list,
                                vector<hsa_agent_t>& dev_list,
                                vector<uint32_t>& dev_idx_list,
                                vector<hsa_signal_t>& sig_list,
                                vector<hsa_amd_memory_pool_t>& pool_list) {

  // Number of Unidirectional or Bidirectional
  // Concurrent Copy transactions in user request
  uint32_t trans_cnt = trans_list.size();
  size_t max_size = size_list_.back();

  // Common variables used in different loops
  void* buf_src;
  void* buf_dst;
  uint32_t src_idx;
  uint32_t dst_idx;
  hsa_signal_t signal;
  hsa_agent_t src_dev;
  hsa_agent_t dst_dev;
  uint32_t src_dev_idx;
  uint32_t dst_dev_idx;
  hsa_amd_memory_pool_t src_pool;
  hsa_amd_memory_pool_t dst_pool;

  // Allocate buffers for the various transactions
  for (uint32_t idx = 0; idx < trans_cnt; idx++) {
    async_trans_t& trans = trans_list[idx];
    src_idx = trans.copy.src_idx_;
    dst_idx = trans.copy.dst_idx_;
    src_pool = trans.copy.src_pool_;
    dst_pool = trans.copy.dst_pool_;
    src_dev = pool_list_[src_idx].owner_agent_;
    dst_dev = pool_list_[dst_idx].owner_agent_;
    src_dev_idx = pool_list_[src_idx].agent_index_;
    dst_dev_idx = pool_list_[dst_idx].agent_index_;
    
    // Allocate buffers and signal for forward copy operation
    AllocateCopyBuffers(max_size,
                        buf_src, src_pool,
                        buf_dst, dst_pool);
    
    err_ = hsa_signal_create(1, 0, NULL, &signal);
    ErrorCheck(err_);

    // Acquire access to destination buffers
    AcquirePoolAcceses(src_dev_idx, src_dev, buf_src,
                       dst_dev_idx, dst_dev, buf_dst);
    
    sig_list.push_back(signal);
    buf_list.push_back(buf_src);
    buf_list.push_back(buf_dst);
    dev_list.push_back(src_dev);
    dev_list.push_back(dst_dev);
    dev_idx_list.push_back(src_dev_idx);
    dev_idx_list.push_back(dst_dev_idx);
  
    // Initialize source buffers with data that could be verified
    InitializeSrcBuffer(max_size, buf_src, src_dev_idx, src_dev);
    
    // For bidirectional copies allocate buffers
    // and signal for reverse direction as well
    if (bidir) {
      AllocateCopyBuffers(max_size,
                          buf_src, dst_pool,
                          buf_dst, src_pool);
      err_ = hsa_signal_create(1, 0, NULL, &signal);
      ErrorCheck(err_);

      // Acquire access to destination buffers
      AcquirePoolAcceses(dst_dev_idx, dst_dev, buf_src,
                         src_dev_idx, src_dev, buf_dst);
      
      sig_list.push_back(signal);
      buf_list.push_back(buf_src);
      buf_list.push_back(buf_dst);
      dev_list.push_back(dst_dev);
      dev_list.push_back(src_dev);
      dev_idx_list.push_back(dst_dev_idx);
      dev_idx_list.push_back(src_dev_idx);
    
      // Initialize source buffers with data that could be verified
      InitializeSrcBuffer(max_size, buf_src, dst_dev_idx, dst_dev);
    }
  }
}

void RocmBandwidthTest::AllocateCopyBuffers(size_t size,
                        void*& src, hsa_amd_memory_pool_t src_pool,
                        void*& dst, hsa_amd_memory_pool_t dst_pool) {

  // Allocate buffers in src and dst pools for forward copy
  err_ = hsa_amd_memory_pool_allocate(src_pool, size, 0, &src);
  ErrorCheck(err_);
  err_ = hsa_amd_memory_pool_allocate(dst_pool, size, 0, &dst);
  ErrorCheck(err_);
}

void RocmBandwidthTest::ReleaseBuffers(std::vector<void*>& buffer_list) {

  for(uint32_t idx = 0; idx < buffer_list.size(); idx++) {
    void* buffer = buffer_list[idx];
    err_ = hsa_amd_memory_pool_free(buffer);
    ErrorCheck(err_);
  }
}

void RocmBandwidthTest::ReleaseSignals(std::vector<hsa_signal_t>& signal_list) {

  for(uint32_t idx = 0; idx < signal_list.size(); idx++) {
    hsa_signal_t signal = signal_list[idx];
    err_ = hsa_signal_destroy(signal);
    ErrorCheck(err_);
  }
}

double RocmBandwidthTest::GetGpuCopyTime(bool bidir,
                                 hsa_signal_t signal_fwd,
                                 hsa_signal_t signal_rev) {

  // Obtain time taken for forward copy
  hsa_amd_profiling_async_copy_time_t async_time_fwd = {0};
  err_= hsa_amd_profiling_get_async_copy_time(signal_fwd, &async_time_fwd);
  ErrorCheck(err_);
  if (bidir == false) {
    return(async_time_fwd.end - async_time_fwd.start);
  }

  hsa_amd_profiling_async_copy_time_t async_time_rev = {0};
  err_= hsa_amd_profiling_get_async_copy_time(signal_rev, &async_time_rev);
  ErrorCheck(err_);
  
  // Compute time taken to copy
  double start = min(async_time_fwd.start, async_time_rev.start);
  double end = max(async_time_fwd.end, async_time_rev.end);
  double copy_time = end - start;

  // Forward copy completed before Reverse began
  if (async_time_fwd.end < async_time_rev.start) {
    return (copy_time - (async_time_rev.start - async_time_fwd.end));
  }

  // Reverse copy completed before Forward began
  if (async_time_rev.end < async_time_fwd.start) {
    return (copy_time - (async_time_fwd.start - async_time_rev.end));
  }

  // Forward and Reverse copies overlapped
  return copy_time;
}

void RocmBandwidthTest::WaitForCopyCompletion(vector<hsa_signal_t>& signal_list) {

  hsa_wait_state_t policy = (bw_blocking_run_ == NULL) ?
                             HSA_WAIT_STATE_ACTIVE : HSA_WAIT_STATE_BLOCKED;

  uint32_t size = signal_list.size();
  for (uint32_t idx = 0; idx < size; idx++) {
    hsa_signal_t signal = signal_list[idx];
    while (hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_LT,
                                   1, uint64_t(-1), policy));
  }
}

void RocmBandwidthTest::copy_buffer(void* dst, hsa_agent_t dst_agent,
                            void* src, hsa_agent_t src_agent,
                            size_t size, hsa_signal_t signal) {

  // Copy from src into dst buffer
  err_ = hsa_amd_memory_async_copy(dst, dst_agent,
                                   src, src_agent,
                                   size, 0, NULL, signal);
  ErrorCheck(err_);

  // Wait for the forward copy operation to complete
  while (hsa_signal_wait_acquire(signal, HSA_SIGNAL_CONDITION_LT, 1,
                                     uint64_t(-1), HSA_WAIT_STATE_ACTIVE));
}

void RocmBandwidthTest::RunConcurrentCopyBenchmark(bool bidir,
                                                   vector<async_trans_t>& trans_list) {

  // Number of Unidirectional or Bidirectional
  // Concurrent Copy transactions in user request
  uint32_t trans_cnt = trans_list.size();
  size_t max_size = size_list_.back();
  uint32_t size_len = size_list_.size();

  // Lists of buffers, pools, agents and signals
  // used to run copy requests
  vector<void*> buf_list;
  vector<hsa_agent_t> dev_list;
  vector<uint32_t> dev_idx_list;
  vector<hsa_signal_t> sig_list;
  vector<hsa_amd_memory_pool_t> pool_list;

  // Allocate resources for the various transactions
  AllocateConcurrentCopyResources(bidir, trans_list,
                                  buf_list, dev_list,
                                  dev_idx_list, sig_list, pool_list);

  // Common variables used in different loops
  void* buf_src;
  void* buf_dst;
  hsa_agent_t src_dev;
  hsa_agent_t dst_dev;
  hsa_signal_t signal;
  
  // Signa to trigger all copy requests to wait
  // until allowed to begin
  hsa_signal_t sig_grp_start;
  err_ = hsa_signal_create(1, 0, NULL, &sig_grp_start);
  ErrorCheck(err_);

  // Bind the number of iterations
  uint32_t iterations = GetIterationNum();

  // Iterate through the differnt buffer sizes to
  // compute the bandwidth as determined by copy
  for (uint32_t idx = 0; idx < size_len; idx++) {

    // This should not be happening
    size_t curr_size = size_list_[idx];
    if (curr_size > max_size) {
      break;
    }

    std::vector< std::vector<double> > gpu_time_list(trans_cnt, std::vector<double>());
    for (uint32_t it = 0; it < iterations; it++) {
      if (it % 2) {
        printf(".");
        fflush(stdout);
      }

      // Set group trigger signal
      hsa_signal_store_relaxed(sig_grp_start, 1);

      // Update signal value to one before submitting copy requests
      uint32_t sig_idx = 0;
      uint32_t sig_cnt = sig_list.size();
      for (sig_idx = 0; sig_idx < sig_cnt; sig_idx++) {
        signal = sig_list[sig_idx];
        hsa_signal_store_relaxed(signal, 1);
      }

      // Submit copy operations in batch mode
      uint32_t rsrc_idx = 0;
      uint32_t cpy_cnt = (bidir) ? (trans_cnt * 2) : trans_cnt;
      for (uint32_t cpy_idx = 0; cpy_idx < cpy_cnt; cpy_idx++) {

        sig_idx = cpy_idx;
        rsrc_idx = cpy_idx * 2;
        signal = sig_list[sig_idx + 0];
        buf_src = buf_list[rsrc_idx + 0];
        buf_dst = buf_list[rsrc_idx + 1];
        src_dev = dev_list[rsrc_idx + 0];
        dst_dev = dev_list[rsrc_idx + 1];

        err_ = hsa_amd_memory_async_copy(buf_dst, dst_dev,
                                         buf_src, src_dev, curr_size,
                                         1, &sig_grp_start, signal);
        ErrorCheck(err_);
      }
      
      // Set group trigger signal
      hsa_signal_store_relaxed(sig_grp_start, 0);

      // Wait for the copy operations to complete
      WaitForCopyCompletion(sig_list);

      // Retrieve times for each copy operation
      hsa_signal_t signal_rev;
      for (uint32_t tidx = 0; tidx < trans_cnt; tidx++) {
        sig_idx = (bidir) ? (tidx * 2) : (tidx);
        signal = sig_list[sig_idx + 0];
        signal_rev = (bidir) ? (sig_list[sig_idx + 1]) : signal;
        double temp = GetGpuCopyTime(bidir, signal, signal_rev);
        std::vector<double>& gpu_time = gpu_time_list[tidx];
        gpu_time.push_back(temp);
      }
    }
    
    // Update time taken to copy a particular size
    // Get Gpu min and mean copy times
    for (uint32_t tidx = 0; tidx < trans_cnt; tidx++) {
      async_trans_t& trans = trans_list[tidx];
      std::vector<double>& gpu_time = gpu_time_list[tidx];
      double min_time = GetMinTime(gpu_time);
      double mean_time = GetMeanTime(gpu_time);
      trans.gpu_min_time_.push_back(min_time);
      trans.gpu_avg_time_.push_back(mean_time);
      gpu_time.clear();
    }
  }

  // Free up buffers and signal objects used in copy operation
  sig_list.push_back(sig_grp_start);
  ReleaseSignals(sig_list);
  ReleaseBuffers(buf_list);
}

void RocmBandwidthTest::RunCopyBenchmark(async_trans_t& trans) {

  // Bind if this transaction is bidirectional
  bool bidir = trans.copy.bidir_;

  // Initialize size of buffer to equal the largest element of allocation
  size_t max_size = size_list_.back();
  uint32_t size_len = size_list_.size();

  // Bind to resources such as pool and agents that are involved
  // in both forward and reverse copy operations
  void* buf_src_fwd;
  void* buf_dst_fwd;
  void* buf_src_rev;
  void* buf_dst_rev;
  hsa_signal_t signal_fwd;
  hsa_signal_t signal_rev;
  hsa_signal_t signal_start_bidir;
  uint32_t src_idx = trans.copy.src_idx_;
  uint32_t dst_idx = trans.copy.dst_idx_;
  uint32_t src_dev_idx_fwd = pool_list_[src_idx].agent_index_;
  uint32_t dst_dev_idx_fwd = pool_list_[dst_idx].agent_index_;
  uint32_t src_dev_idx_rev = dst_dev_idx_fwd;
  uint32_t dst_dev_idx_rev = src_dev_idx_fwd;
  hsa_amd_memory_pool_t src_pool_fwd = trans.copy.src_pool_;
  hsa_amd_memory_pool_t dst_pool_fwd = trans.copy.dst_pool_;
  hsa_amd_memory_pool_t src_pool_rev = dst_pool_fwd;
  hsa_amd_memory_pool_t dst_pool_rev = src_pool_fwd;
  hsa_agent_t src_agent_fwd = pool_list_[src_idx].owner_agent_;
  hsa_agent_t dst_agent_fwd = pool_list_[dst_idx].owner_agent_;
  hsa_agent_t src_agent_rev = dst_agent_fwd;
  hsa_agent_t dst_agent_rev = src_agent_fwd;
  std::vector<void*> buffer_list;
  std::vector<hsa_signal_t> signal_list;

  // Allocate buffers for forward path of unidirectional
  // or bidirectional copy
  AllocateCopyBuffers(max_size,
                      buf_src_fwd, src_pool_fwd,
                      buf_dst_fwd, dst_pool_fwd);

  // Create a signal to wait on copy operation
  // @TODO: replace it with a signal pool call
  err_ = hsa_signal_create(1, 0, NULL, &signal_fwd);
  ErrorCheck(err_);

  // Collect resources to be released later
  signal_list.push_back(signal_fwd);
  buffer_list.push_back(buf_src_fwd);
  buffer_list.push_back(buf_dst_fwd);

  // Allocate buffers for reverse path of bidirectional copy
  if (bidir) {
    AllocateCopyBuffers(max_size,
                        buf_src_rev, src_pool_rev,
                        buf_dst_rev, dst_pool_rev);

    // Create a signal to begin bidir copy operations
    // @TODO: replace it with a signal pool call
    err_ = hsa_signal_create(1, 0, NULL, &signal_rev);
    ErrorCheck(err_);
    err_ = hsa_signal_create(1, 0, NULL, &signal_start_bidir);
    ErrorCheck(err_);
  
    signal_list.push_back(signal_rev);
    signal_list.push_back(signal_start_bidir);
    buffer_list.push_back(buf_src_rev);
    buffer_list.push_back(buf_dst_rev);
  }

  // Initialize source buffers with data that could be verified
  InitializeSrcBuffer(max_size, buf_src_fwd,
                      src_dev_idx_fwd, src_agent_fwd);
  if (bidir) {
    InitializeSrcBuffer(max_size, buf_src_rev,
                        src_dev_idx_rev, src_agent_rev);
  }

  // Setup access to destination buffers for
  // both unidirectional and bidirectional copies
  AcquirePoolAcceses(src_dev_idx_fwd, src_agent_fwd, buf_src_fwd,
                     dst_dev_idx_fwd, dst_agent_fwd, buf_dst_fwd);
  if (bidir) {
    AcquirePoolAcceses(src_dev_idx_rev, src_agent_rev, buf_src_rev,
                       dst_dev_idx_rev, dst_agent_rev, buf_dst_rev);
  }
    
  // Bind the number of iterations
  uint32_t iterations = GetIterationNum();

  // Iterate through the differnt buffer sizes to
  // compute the bandwidth as determined by copy
  for (uint32_t idx = 0; idx < size_len; idx++) {
    
    // This should not be happening
    size_t curr_size = size_list_[idx];
    if (curr_size > max_size) {
      break;
    }

    bool verify = true;
    std::vector<double> cpu_time;
    std::vector<double> gpu_time;
    for (uint32_t it = 0; it < iterations; it++) {
      if (it % 2) {
        printf(".");
        fflush(stdout);
      }

      hsa_signal_store_relaxed(signal_fwd, 1);
      if (bidir) {
        hsa_signal_store_relaxed(signal_rev, 1);
        hsa_signal_store_relaxed(signal_start_bidir, 1);
      }

      // Create a timer object and reset signals
      PerfTimer timer;
      uint32_t index = timer.CreateTimer();

      // Start the timer and launch forward copy operation
      timer.StartTimer(index);
      if (bidir == false) {
        err_ = hsa_amd_memory_async_copy(buf_dst_fwd, dst_agent_fwd,
                                         buf_src_fwd, src_agent_fwd,
                                         curr_size, 0, NULL, signal_fwd);
      } else {
        err_ = hsa_amd_memory_async_copy(buf_dst_fwd, dst_agent_fwd,
                                         buf_src_fwd, src_agent_fwd,
                                         curr_size, 1, &signal_start_bidir,
                                         signal_fwd);
      }
      ErrorCheck(err_);

      // Launch reverse copy operation if it is bidirectional
      if (bidir) {
        err_ = hsa_amd_memory_async_copy(buf_dst_rev, dst_agent_rev,
                                         buf_src_rev, src_agent_rev,
                                         curr_size, 1, &signal_start_bidir,
                                         signal_rev);
        ErrorCheck(err_);
      }
      
      // Signal the bidir copies to begin
      if (bidir) {
        hsa_signal_store_relaxed(signal_start_bidir, 0);
      }

      WaitForCopyCompletion(signal_list);

      // Stop the timer object
      timer.StopTimer(index);

      // Push the time taken for copy into a vector of copy times
      cpu_time.push_back(timer.ReadTimer(index));

      // Collect time from the signal(s)
      if (print_cpu_time_ == false) {
        if (trans.copy.uses_gpu_) {
          double temp = GetGpuCopyTime(bidir, signal_fwd, signal_rev);
          gpu_time.push_back(temp);
        }
      }

      if (validate_) {
        verify = ValidateDstBuffer(max_size, curr_size, buf_dst_fwd,
                                   dst_dev_idx_fwd, dst_agent_fwd);
      }
    }

    // Collecting Cpu time. Capture verify failures if any
    // Get min and mean copy times and collect them into Cpu
    // time list
    double min_time = 0;
    double mean_time = 0;
    if (print_cpu_time_) {
      min_time = (verify) ? GetMinTime(cpu_time) : VALIDATE_COPY_OP_FAILURE;
      mean_time = (verify) ? GetMeanTime(cpu_time) : VALIDATE_COPY_OP_FAILURE;
      trans.cpu_min_time_.push_back(min_time);
      trans.cpu_avg_time_.push_back(mean_time);
    }

    // Collecting Gpu time. Capture verify failures if any
    // Get min and mean copy times and collect them into Gpu
    // time list
    if (print_cpu_time_ == false) {
      if (trans.copy.uses_gpu_) {
        min_time = (verify) ? GetMinTime(gpu_time) : VALIDATE_COPY_OP_FAILURE;
        mean_time = (verify) ? GetMeanTime(gpu_time) : VALIDATE_COPY_OP_FAILURE;
        trans.gpu_min_time_.push_back(min_time);
        trans.gpu_avg_time_.push_back(mean_time);
      }
    }
    verify = true;

    // Clear the stack of cpu times
    cpu_time.clear();
    gpu_time.clear();
  }

  // Free up buffers and signal objects used in copy operation
  ReleaseSignals(signal_list);
  ReleaseBuffers(buffer_list);
}

void RocmBandwidthTest::Run() {

  // Enable profiling of Async Copy Activity
  if (print_cpu_time_ == false) {
    err_ = hsa_amd_profiling_async_copy_enable(true);
    ErrorCheck(err_);
  }

  if ((req_concurrent_copy_bidir_ == REQ_CONCURRENT_COPY_BIDIR) ||
      (req_concurrent_copy_unidir_ == REQ_CONCURRENT_COPY_UNIDIR)) {
    bool bidir = (req_concurrent_copy_bidir_ == REQ_CONCURRENT_COPY_BIDIR);
    RunConcurrentCopyBenchmark(bidir, trans_list_);
    ComputeCopyTime(trans_list_);
    err_ = hsa_amd_profiling_async_copy_enable(false);
    ErrorCheck(err_);
    return;
  }

  // Iterate through the list of transactions and execute them
  uint32_t trans_size = trans_list_.size();
  for (uint32_t idx = 0; idx < trans_size; idx++) {
    async_trans_t& trans = trans_list_[idx];
    if ((trans.req_type_ == REQ_COPY_BIDIR) ||
        (trans.req_type_ == REQ_COPY_UNIDIR) ||
        (trans.req_type_ == REQ_COPY_ALL_BIDIR) ||
        (trans.req_type_ == REQ_COPY_ALL_UNIDIR)) {
      RunCopyBenchmark(trans);
      ComputeCopyTime(trans);
    }
    if ((trans.req_type_ == REQ_READ) ||
        (trans.req_type_ == REQ_WRITE)) {
      RunIOBenchmark(trans);
    }
  }

  // Disable profiling of Async Copy Activity
  if (print_cpu_time_ == false) {
    err_ = hsa_amd_profiling_async_copy_enable(false);
    ErrorCheck(err_);
  }

}

void RocmBandwidthTest::Close() {

  if (init_src_ != NULL) {
    hsa_signal_destroy(init_signal_);
    hsa_amd_memory_pool_free(init_src_);
  }

  if (validate_) {
    hsa_amd_memory_pool_free(validate_dst_);
  }

  hsa_status_t status = hsa_shut_down();
  ErrorCheck(status);
  return;
}

// Sets up the bandwidth test object to enable running
// the various test scenarios requested by user. The
// things this proceedure takes care of are:
//    
//    Parse user arguments
//    Discover RocR Device Topology
//    Determine validity of requested test scenarios
//    Build the list of transactions to execute
//    Miscellaneous
//
void RocmBandwidthTest::SetUp() {

  // Parse user arguments
  ParseArguments();

  // Validate input parameters
  bool status = ValidateArguments();
  if (status == false) {
    PrintHelpScreen();
    exit(1);
  }

  // Build list of transactions (copy, read, write) to execute
  status = BuildTransList();
  if (status == false) {
    PrintHelpScreen();
    exit(1);
  }
}

RocmBandwidthTest::RocmBandwidthTest(int argc, char** argv) : BaseTest() {
  
  usr_argc_ = argc;
  usr_argv_ = argv;
  
  pool_index_ = 0;
  cpu_index_ = -1;
  agent_index_ = 0;
  
  req_read_ = REQ_INVALID;
  req_write_ = REQ_INVALID;
  req_version_ = REQ_INVALID;
  req_topology_ = REQ_INVALID;
  req_copy_bidir_ = REQ_INVALID;
  req_copy_unidir_ = REQ_INVALID;
  req_copy_all_bidir_ = REQ_INVALID;
  req_copy_all_unidir_ = REQ_INVALID;
  req_concurrent_copy_bidir_ = REQ_INVALID;
  req_concurrent_copy_unidir_ = REQ_INVALID;
  
  access_matrix_ = NULL;
  link_type_matrix_ = NULL;
  active_agents_list_ = NULL;
  link_weight_matrix_ = NULL;
  
  init_ = false;
  latency_ = false;
  validate_ = false;
  print_cpu_time_ = false;
  
  // Set initial value to 11.231926 in case
  // user does not have a preference
  init_val_ = 11.231926;
  init_src_ = NULL;
  validate_dst_ = NULL;

  // Initialize version of the test
  version_.major_id = 2;
  version_.minor_id = 3;
  version_.step_id = 9;
  version_.reserved = 0;

  bw_iter_cnt_ = getenv("ROCM_BW_ITER_CNT");
  bw_default_run_ = getenv("ROCM_BW_DEFAULT_RUN");
  bw_blocking_run_ = getenv("ROCR_BW_RUN_BLOCKING");
  skip_cpu_fine_grain_ = getenv("ROCM_SKIP_CPU_FINE_GRAINED_POOL");
  skip_gpu_coarse_grain_ = getenv("ROCM_SKIP_GPU_COARSE_GRAINED_POOL");

  if (bw_iter_cnt_ != NULL) {
    int32_t num = atoi(bw_iter_cnt_);
    if (num < 0) {
      std::cout << "Value of ROCM_BW_ITER_CNT can't be negative: " << num << std::endl;
    }
    set_num_iteration(num);
  }

  exit_value_ = 0;
}

RocmBandwidthTest::~RocmBandwidthTest() {

  delete access_matrix_;
  delete link_type_matrix_;
  delete link_weight_matrix_;
  delete active_agents_list_;
}

std::string RocmBandwidthTest::GetVersion() const {

  std::stringstream stream;
  stream << version_.major_id << ".";
  stream << version_.minor_id << ".";
  stream << version_.step_id;
  return stream.str();
}

