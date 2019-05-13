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
#include <sstream>
#include <limits>

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
  } else if (dst_dev_type == HSA_DEVICE_TYPE_GPU) {
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
    memset(init_src_, init_val_, size);
    err_ = hsa_signal_create(0, 0, NULL, &init_signal_);
    ErrorCheck(err_);
  }

  // If Copy device is a Gpu setup buffer access
  hsa_device_type_t cpy_dev_type = agent_list_[cpy_dev_idx].device_type_;
  if (cpy_dev_type == HSA_DEVICE_TYPE_GPU) {
    AcquireAccess(cpy_agent, init_src_);
    hsa_signal_store_relaxed(init_signal_, 1);
    copy_buffer(buf_cpy, cpy_agent,
                init_src_, cpu_agent_,
                size, init_signal_);
    return;
  }

  // Copy initialization buffer into copy buffer
  memcpy(buf_cpy, init_src_, size);
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
  memset(validate_dst_, ~init_val_, curr_size);
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

  // Copy initialization buffer into copy buffer
  err_ = (hsa_status_t)memcmp(init_src_, validate_dst_, curr_size);
  if (err_ != HSA_STATUS_SUCCESS) {
    exit_value_ = err_;
  }
  return (err_ == HSA_STATUS_SUCCESS);
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
    // Wait for copy operation to complete
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
  if ((init_) || (validate_)) {
    InitializeSrcBuffer(max_size, buf_src_fwd,
                        src_dev_idx_fwd, src_agent_fwd);

    if (bidir) {
      InitializeSrcBuffer(max_size, buf_src_rev,
                          src_dev_idx_rev, src_agent_rev);
    }
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

    // Get Cpu min copy time
    trans.cpu_min_time_.push_back(GetMinTime(cpu_time));
    // Get Cpu mean copy time and store to the array
    trans.cpu_avg_time_.push_back(GetMeanTime(cpu_time));

    if (print_cpu_time_ == false) {
      if (trans.copy.uses_gpu_) {
        // Get Gpu min and mean copy times
        double min_time = (verify) ? GetMinTime(gpu_time) : std::numeric_limits<double>::max();
        double mean_time = (verify) ? GetMeanTime(gpu_time) : std::numeric_limits<double>::max();
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
  std::cout << std::endl;

  // Disable profiling of Async Copy Activity
  if (print_cpu_time_ == false) {
    err_ = hsa_amd_profiling_async_copy_enable(false);
    ErrorCheck(err_);
  }

}

void RocmBandwidthTest::Close() {
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
  req_copy_bidir_ = REQ_INVALID;
  req_copy_unidir_ = REQ_INVALID;
  req_copy_all_bidir_ = REQ_INVALID;
  req_copy_all_unidir_ = REQ_INVALID;
  
  access_matrix_ = NULL;
  link_type_matrix_ = NULL;
  active_agents_list_ = NULL;
  link_weight_matrix_ = NULL;
  
  init_ = false;
  latency_ = false;
  validate_ = false;
  print_cpu_time_ = false;
  
  // Set initial value to 0x23 in case
  // user does not have a preference
  init_val_ = 0x23;
  init_src_ = NULL;
  validate_dst_ = NULL;

  // Initialize version of the test
  version_.major_id = 2;
  version_.minor_id = 2;
  version_.step_id = 0;
  version_.reserved = 0;

  bw_iter_cnt_ = getenv("ROCM_BW_ITER_CNT");
  bw_default_run_ = getenv("ROCM_BW_DEFAULT_RUN");
  bw_blocking_run_ = getenv("ROCR_BW_RUN_BLOCKING");
  skip_fine_grain_ = getenv("ROCM_SKIP_FINE_GRAINED_POOL");

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
  if (init_) {
    hsa_signal_destroy(init_signal_);
    hsa_amd_memory_pool_free(init_src_);
    if (validate_) {
      hsa_amd_memory_pool_free(validate_dst_);
    }
  }
}

std::string RocmBandwidthTest::GetVersion() const {

  std::stringstream stream;
  stream << version_.major_id << ".";
  stream << version_.minor_id << ".";
  stream << version_.step_id;
  return stream.str();
}

