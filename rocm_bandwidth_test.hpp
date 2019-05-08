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

#ifndef __ROC_BANDWIDTH_TEST_H__
#define __ROC_BANDWIDTH_TEST_H__

#include "hsa/hsa.h"
#include "base_test.hpp"
#include "hsatimer.hpp"
#include "common.hpp"

#include <vector>

using namespace std;

// Structure to encapsulate a RocR agent and its index in a list
typedef struct agent_info {

  agent_info(hsa_agent_t agent,
             uint32_t index, hsa_device_type_t device_type) {
    agent_ = agent;
    index_ = index;
    device_type_ = device_type;
  }

  agent_info() {}
  
  uint32_t index_;
  hsa_agent_t agent_;
  hsa_device_type_t device_type_;
  char name_[64];     // Size specified in public header file
  char bdf_id_[16];   // Bus (8-bits), Device (5-bits), Function (3-bits)

} agent_info_t;

typedef struct pool_info {

  pool_info(hsa_agent_t agent, uint32_t agent_index,
            hsa_amd_memory_pool_t pool, hsa_amd_segment_t segment,
            size_t size, uint32_t index, bool is_fine_grained,
            bool is_kernarg, bool access_to_all,
            hsa_amd_memory_pool_access_t owner_access) {

    pool_ = pool;
    index_ = index;
    segment_ = segment;
    owner_agent_ = agent;
    agent_index_ = agent_index;
    allocable_size_ = size;
    is_kernarg_ = is_kernarg;
    owner_access_ = owner_access;
    access_to_all_ = access_to_all;
    is_fine_grained_ = is_fine_grained;
  }

  pool_info() {}

  uint32_t index_;
  bool is_kernarg_;
  bool access_to_all_;
  bool is_fine_grained_;
  size_t allocable_size_;
  uint32_t agent_index_;
  hsa_agent_t owner_agent_;
  hsa_amd_segment_t segment_;
  hsa_amd_memory_pool_t pool_;
  hsa_amd_memory_pool_access_t owner_access_;

} pool_info_t;

// Used to print out topology info
typedef struct agent_pool_info {

  agent_pool_info() {}
  
  agent_info agent;
  
  vector<pool_info_t> pool_list;

} agent_pool_info_t;

typedef struct async_trans {

  uint32_t req_type_;
  union {
    struct {
      bool bidir_;
      bool uses_gpu_;
      uint32_t src_idx_;
      uint32_t dst_idx_;
      hsa_amd_memory_pool_t src_pool_;
      hsa_amd_memory_pool_t dst_pool_;
    } copy;
    struct {
      void* code_;
      uint32_t agent_idx_;
      hsa_agent_t agent_;
      uint32_t pool_idx_;
      hsa_amd_memory_pool_t pool_;
    } kernel;
  };

  // Cpu BenchMark average copy time
  vector<double> cpu_avg_time_;

  // Cpu Min time
  vector<double> cpu_min_time_;

  // Gpu BenchMark average copy time
  vector<double> gpu_avg_time_;

  // Gpu Min time
  vector<double> gpu_min_time_;

  // BenchMark's Average copy time and average bandwidth
  vector<double> avg_time_;
  vector<double> avg_bandwidth_;

  // BenchMark's Min copy time and peak bandwidth
  vector<double> min_time_;
  vector<double> peak_bandwidth_;

  async_trans(uint32_t req_type) { req_type_ = req_type; }
} async_trans_t;

typedef enum Request_Type {

  REQ_READ = 1,
  REQ_WRITE = 2,
  REQ_VERSION = 3,
  REQ_TOPOLOGY = 4,
  REQ_COPY_BIDIR = 5,
  REQ_COPY_UNIDIR = 6,
  REQ_COPY_ALL_BIDIR = 7,
  REQ_COPY_ALL_UNIDIR = 8,
  REQ_INVALID = 9,

} Request_Type;

class RocmBandwidthTest : public BaseTest {

 public:

  // @brief: Constructor for test case of RocmBandwidthTest
  RocmBandwidthTest(int argc, char** argv);

  // @brief: Destructor for test case of RocmBandwidthTest
  virtual ~RocmBandwidthTest();

  // @brief: Setup the environment for measurement
  virtual void SetUp();

  // @brief: Core measurement execution
  virtual void Run();

  // @brief: Clean up and retrive the resource
  virtual void Close();

  // @brief: Display the results
  virtual void Display() const;

  // @brief: Return exit value, useful in case of error
  int32_t GetExitValue() { return exit_value_; }

 private:

  // @brief: Print Help Menu Screen
  void PrintHelpScreen();

  // @brief: Discover the topology of pools on Rocm Platform
  void DiscoverTopology();

  // @brief: Populate link properties for the set of agents
  void DiscoverLinkProps();
  void BindLinkProps(uint32_t idx1, uint32_t idx2);

  // @brief: Populates the access matrix
  void PopulateAccessMatrix();

  // @brief: Print topology info
  void PrintTopology();

  // @brief: Print in matrix form various
  // properties such as access, link weight,
  // link type and number of hops, etc
  void PrintLinkPropsMatrix(uint32_t key) const;

  // @brief: Print info on agents in system
  void PrintAgentsList();

  // @brief: Print info on memory pools in system
  void PrintPoolsList();

  // @brief: Parse the arguments provided by user to
  // build list of transactions
  void ParseArguments();
  
  // @brief Validate user input of primary operations
  void ValidateInputFlags(uint32_t pf_cnt,
                          uint32_t copy_mask, uint32_t copy_ctrl_mask);
  
  // @brief: Print the list of transactions
  void PrintTransList();

  // @brief: Run read/write requests of users
  void RunIOBenchmark(async_trans_t& trans);

  // @brief: Run copy requests of users
  void RunCopyBenchmark(async_trans_t& trans);

  // @brief: Get iteration number
  uint32_t GetIterationNum();

  // @brief: Get the mean copy time
  double GetMeanTime(std::vector<double>& vec);

  // @brief: Get the min copy time
  double GetMinTime(std::vector<double>& vec);

  // @brief: Dispaly Benchmark result
  void PopulatePerfMatrix(bool peak, double* perf_matrix) const;
  void PrintPerfMatrix(bool validate, bool peak, double* perf_matrix) const;
  void DisplayDevInfo() const;
  void DisplayIOTime(async_trans_t& trans) const;
  void DisplayCopyTime(async_trans_t& trans) const;
  void DisplayCopyTimeMatrix(bool peak) const;
  void DisplayValidationMatrix() const;
 
 private:

  // @brief: Validate the arguments passed in by user
  bool ValidateArguments();
  bool ValidateReadReq();
  bool ValidateWriteReq();
  bool ValidateReadOrWriteReq(vector<size_t>& in_list);

  void ValidateCopyBidirFlags(uint32_t copy_ctrl_mask);
  void ValidateCopyAllBidirFlags(uint32_t copy_ctrl_mask);
  void ValidateCopyAllUnidirFlags(uint32_t copy_ctrl_mask);
  void ValidateCopyUnidirFlags(uint32_t copy_mask, uint32_t copy_ctrl_mask);
  
  bool ValidateBidirCopyReq();
  bool ValidateUnidirCopyReq();
  bool ValidateCopyReq(vector<size_t>& in_list);
  void PrintIOAccessError(uint32_t agent_idx, uint32_t pool_idx);
  void PrintCopyAccessError(uint32_t src_pool_idx, uint32_t dst_pool_idx);
  
  bool PoolIsPresent(vector<size_t>& in_list);
  bool PoolIsDuplicated(vector<size_t>& in_list);

  // @brief: Builds a list of transaction per user request
  void ComputeCopyTime(async_trans_t& trans);
  void BuildDeviceList();
  void BuildBufferList();
  bool BuildTransList();
  bool BuildReadTrans();
  bool BuildWriteTrans();
  bool BuildBidirCopyTrans();
  bool BuildUnidirCopyTrans();
  bool BuildAllPoolsBidirCopyTrans();
  bool BuildAllPoolsUnidirCopyTrans();
  bool BuildReadOrWriteTrans(uint32_t req_type,
                             vector<size_t>& in_list);
  bool BuildCopyTrans(uint32_t req_type,
                      vector<size_t>& src_list,
                      vector<size_t>& dst_list);

  void WaitForCopyCompletion(vector<hsa_signal_t>& signal_list);

  void AllocateCopyBuffers(size_t size,
                           void*& src, hsa_amd_memory_pool_t src_pool,
                           void*& dst, hsa_amd_memory_pool_t dst_pool);
  
  void ReleaseBuffers(std::vector<void*>& buffer_list);
  void ReleaseSignals(std::vector<hsa_signal_t>& signal_list);
  
  double GetGpuCopyTime(bool bidir, hsa_signal_t signal_fwd, hsa_signal_t signal_rev);
  
  void InitializeSrcBuffer(size_t size, void* buf_cpy,
                           uint32_t cpy_dev_idx, hsa_agent_t cpy_agent);
  
  bool ValidateDstBuffer(size_t max_size, size_t curr_size,
                         void* buf_cpy, uint32_t cpy_dev_idx, hsa_agent_t cpy_agent);

  void copy_buffer(void* dst, hsa_agent_t dst_agent,
                   void* src, hsa_agent_t src_agent,
                   size_t size, hsa_signal_t signal);
  bool FilterCpuPool(uint32_t req_type,
                     hsa_device_type_t dev_type,
                     bool fine_grained);

  // Find the mirror transaction if present
  bool FindMirrorRequest(uint32_t src_idx, uint32_t dst_idx);

  // @brief: Check if agent and access memory pool, if so, set 
  // access to the agent, if not, exit
  void AcquireAccess(hsa_agent_t agent, void* ptr);
  void AcquirePoolAcceses(uint32_t src_dev_idx, hsa_agent_t src_agent, void* src,
                          uint32_t dst_dev_idx, hsa_agent_t dst_agent, void* dst);

  // Functions to find agents and memory pools and udpate
  // relevant data structures used to maintain system topology
  friend hsa_status_t AgentInfo(hsa_agent_t agent, void* data);
  friend hsa_status_t MemPoolInfo(hsa_amd_memory_pool_t pool, void* data);
  
  // Populate the Bus Device Function of Gpu device
  friend void PopulateBDF(uint32_t bdf_id, agent_info_t *agent_info);

  // Compute the type and weight of a link
  friend uint32_t GetLinkType(hsa_device_type_t src_dev_type,
                     hsa_device_type_t dst_dev_type,
                     hsa_amd_memory_pool_link_info_t* link_info, uint32_t hops);
  friend uint32_t GetLinkWeight(hsa_amd_memory_pool_link_info_t* link_info, uint32_t hops);

  // Return value of input key as string
  friend std::string GetValueAsString(uint32_t key, uint32_t value);

  // Structure of Version used to identify an instance of RocmBandwidthTest
  struct RocmBandwidthVersion {

    // Tracks changes such as re-design
    // re-factor, re-write, extend with
    // new major functionality, etc
    uint32_t major_id;

    // Tracks changes that affect Apis
    // being added, removed, modified
    uint32_t minor_id;

    // Tracks changes that affect Apis
    // being added, removed, modified
    uint32_t step_id;

    // Used to pack space for structure alignment
    uint32_t reserved;
  };

  RocmBandwidthVersion version_;
  void PrintVersion() const;
  std::string GetVersion() const;

  // More variables declared for testing
  // vector<transaction> tran_;

  // Used to help count agent_info
  uint32_t agent_index_;

  // List used to store agent info, indexed by agent_index_
  vector<agent_info_t> agent_list_;

  // Used to help count pool_info_t
  uint32_t pool_index_;

  // List used to store pool_info_t, indexed by pool_index_
  vector<pool_info_t> pool_list_;

  // List used to store agent_pool_info_t
  vector<agent_pool_info_t> agent_pool_list_;

  // List of agents involved in a bidrectional copy operation
  // Size of the list cannot exceed the number of agents
  // reported by the system
  vector<size_t> bidir_list_;

  // List of source agents in a unidrectional copy operation
  // Size of the list cannot exceed the number of agents
  // reported by the system
  vector<size_t> src_list_;

  // List of destination agents in a unidrectional copy operation
  // Size of the list cannot exceed the number of agents
  // reported by the system
  vector<size_t> dst_list_;

  // List of agents involved in read operation. Has
  // two agents, the first agent hosts the memory pool
  // while the second agent executes the read operation
  vector<size_t> read_list_;
  
  // List of agents involved in write operation. Has
  // two agents, the first agent hosts the memory pool
  // while the second agent executes the write operation
  vector<size_t> write_list_;
  
  // List of sizes to use in copy and read/write transactions
  // Size is specified in terms of Megabytes
  vector<size_t> size_list_;

  // Type of service requested by user
  uint32_t req_read_;
  uint32_t req_write_;
  uint32_t req_version_;
  uint32_t req_topology_;
  uint32_t req_copy_bidir_;
  uint32_t req_copy_unidir_;
  uint32_t req_copy_all_bidir_;
  uint32_t req_copy_all_unidir_;
  
  static const uint32_t USR_SRC_FLAG = 0x01;
  static const uint32_t USR_DST_FLAG = 0x02;

  static const uint32_t USR_BUFFER_SIZE = 0x01;
  static const uint32_t USR_BUFFER_INIT = 0x02;
  static const uint32_t CPU_VISIBLE_TIME = 0x04;
  static const uint32_t DEV_COPY_LATENCY = 0x08;
  static const uint32_t VALIDATE_COPY_OP = 0x010;

  static const uint32_t LINK_TYPE_SELF = 0x00;
  static const uint32_t LINK_TYPE_PCIE = 0x01;
  static const uint32_t LINK_TYPE_XGMI = 0x02;
  static const uint32_t LINK_TYPE_IGNORED = 0x03;
  static const uint32_t LINK_TYPE_NO_PATH = 0xFFFFFFFF;

  static const uint32_t LINK_PROP_HOPS = 0x00;
  static const uint32_t LINK_PROP_TYPE = 0x01;
  static const uint32_t LINK_PROP_WEIGHT = 0x02;
  static const uint32_t LINK_PROP_ACCESS = 0x03;

  // List used to store transactions per user request
  vector<async_trans_t> trans_list_;

  // List used to track agents involved in various transactions
  uint32_t* active_agents_list_;

  // Matrix used to track Access among agents
  uint32_t* access_matrix_;
  uint32_t* link_hops_matrix_;
  uint32_t* link_type_matrix_;
  uint32_t* link_weight_matrix_;
  uint32_t* direct_access_matrix_;
  
  // Env key to determine if Fine-grained or
  // Coarse-grained pool should be filtered out
  char* skip_fine_grain_;
  
  // Env key to determine if the run should block
  // or actively wait on completion signal
  char* bw_blocking_run_;
  
  // Env key to determine if the run is a default one
  char* bw_default_run_;
  
  // Env key to specify iteration count
  char* bw_iter_cnt_;

  // Variable to store argument number

  // Variable to store argument number

  // Variable to store argument number
  uint32_t usr_argc_;

  // Pointer to store address of argument text
  char** usr_argv_;

  // Flag to print Cpu time
  bool print_cpu_time_;

  // Determines if user has requested initialization
  bool init_;

  // Determines if user has requested validation
  bool validate_;
  uint8_t init_val_;

  // Handles to buffer used to initialize and validate
  void* init_src_;
  void* validate_dst_;
  hsa_signal_t init_signal_;
 
  // Determines the latency overhead of copy operations
  bool latency_;

  // CPU agent used for validation
  int32_t cpu_index_;
  hsa_agent_t cpu_agent_;

  // System region
  hsa_amd_memory_pool_t sys_pool_;
 
  static const size_t SIZE_LIST[20];
  static const size_t LATENCY_SIZE_LIST[20];

  // Exit value to return in case of error
  int32_t exit_value_;
};

#endif      //  __ROC_BANDWIDTH_TEST_H__
