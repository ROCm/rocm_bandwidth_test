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

// @brief: Helper method to iterate throught the memory pools of
// an agent and discover its properties
hsa_status_t MemPoolInfo(hsa_amd_memory_pool_t pool, void* data) {

  hsa_status_t status;
  RocmBandwidthTest* asyncDrvr = reinterpret_cast<RocmBandwidthTest*>(data);

  // Query pools' segment, report only pools from global segment
  hsa_amd_segment_t segment;
  status = hsa_amd_memory_pool_get_info(pool,
                   HSA_AMD_MEMORY_POOL_INFO_SEGMENT, &segment);
  ErrorCheck(status);
  if (HSA_AMD_SEGMENT_GLOBAL != segment) {
    return HSA_STATUS_SUCCESS;
  }

  // Determine if allocation is allowed in this pool
  // Report only pools that allow an alloction by user
  bool alloc = false;
  status = hsa_amd_memory_pool_get_info(pool,
                   HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED, &alloc);
  ErrorCheck(status);
  if (alloc != true) {
    return HSA_STATUS_SUCCESS;
  }

  // Query the max allocatable size
  size_t max_size = 0;
  status = hsa_amd_memory_pool_get_info(pool,
                   HSA_AMD_MEMORY_POOL_INFO_SIZE, &max_size);
  ErrorCheck(status);

  // Determine if the pools is accessible to all agents
  bool access_to_all = false;
  status = hsa_amd_memory_pool_get_info(pool,
                HSA_AMD_MEMORY_POOL_INFO_ACCESSIBLE_BY_ALL, &access_to_all);
  ErrorCheck(status);

  // Determine type of access to owner agent
  hsa_amd_memory_pool_access_t owner_access;
  hsa_agent_t agent = asyncDrvr->agent_list_.back().agent_;
  status = hsa_amd_agent_memory_pool_get_info(agent, pool,
                         HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, &owner_access);
  ErrorCheck(status);

  // Determine if the pool is fine-grained or coarse-grained
  uint32_t flag = 0;
  status = hsa_amd_memory_pool_get_info(pool,
                   HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS, &flag);
  ErrorCheck(status);
  bool is_kernarg = (HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_KERNARG_INIT & flag);
  bool is_fine_grained = (HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_FINE_GRAINED & flag);

  // Update the pool handle for system memory if kernarg is true
  if (is_kernarg) {
    asyncDrvr->sys_pool_ = pool;
  }

  // Consult user request and add either fine-grained or
  // coarse-grained memory pools if agent is CPU
  agent_info_t& agent_info = asyncDrvr->agent_list_.back();
  if (agent_info.device_type_ == HSA_DEVICE_TYPE_CPU) {
    if (asyncDrvr->skip_fine_grain_ != NULL) {
      if (is_fine_grained == true) {
        return HSA_STATUS_SUCCESS;
      }
    } else {
      if (is_fine_grained == false) {
        return HSA_STATUS_SUCCESS;
      }
    }
  }
  // hsa_device_type_t device_type;
  // status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
  // ErrorCheck(status);

  // Create an instance of agent_pool_info and add it to the list
  pool_info_t pool_info(agent, asyncDrvr->agent_index_, pool,
                        segment, max_size, asyncDrvr->pool_index_,
                        is_fine_grained, is_kernarg,
                        access_to_all, owner_access);
  asyncDrvr->pool_list_.push_back(pool_info);

  // Create an agent_pool_infot and add it to its list
  asyncDrvr->agent_pool_list_[asyncDrvr->agent_index_].pool_list.push_back(pool_info);
  asyncDrvr->pool_index_++;

  return HSA_STATUS_SUCCESS;
}

// @brief: Helper method to iterate throught the agents of
// a system and discover its properties
hsa_status_t AgentInfo(hsa_agent_t agent, void* data) {

  RocmBandwidthTest* asyncDrvr = reinterpret_cast<RocmBandwidthTest*>(data);

  // Get the name of the agent
  char agent_name[64];
  hsa_status_t status;
  status = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, agent_name);
  ErrorCheck(status);

  // Get device type
  hsa_device_type_t device_type;
  status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
  ErrorCheck(status);

  // Capture the handle of Cpu agent
  if (device_type == HSA_DEVICE_TYPE_CPU) {
    asyncDrvr->cpu_agent_ = agent;
    asyncDrvr->cpu_index_ = asyncDrvr->agent_index_;
  }

  // Instantiate an instance of agent_info_t and populate its name
  // field before adding it to the list of agent_info_t objects
  agent_info_t agent_info(agent, asyncDrvr->agent_index_, device_type);
  status = hsa_agent_get_info(agent,
                      (hsa_agent_info_t)HSA_AMD_AGENT_INFO_PRODUCT_NAME,
                      (void *)&agent_info.name_[0]);
  asyncDrvr->agent_list_.push_back(agent_info);

  // Contruct an new agent_pool_info structure and add it to the list
  agent_pool_info node;
  node.agent = asyncDrvr->agent_list_.back();
  asyncDrvr->agent_pool_list_.push_back(node);

  status = hsa_amd_agent_iterate_memory_pools(agent, MemPoolInfo, asyncDrvr);
  asyncDrvr->agent_index_++;

  return HSA_STATUS_SUCCESS;
}

void RocmBandwidthTest::PopulateAccessMatrix() {

  // Allocate memory to hold access lists
  access_matrix_ = new uint32_t[agent_index_ * agent_index_]();

  hsa_status_t status;
  uint32_t size = pool_list_.size();
  for (uint32_t src_idx = 0; src_idx < size; src_idx++) {

    // Determine if the pool belongs to Cpu and is coarse-grained
    uint32_t src_dev_idx = pool_list_[src_idx].agent_index_;
    hsa_device_type_t src_dev_type = agent_list_[src_dev_idx].device_type_;

    /*
    * This block of code makes sense only if both Fine and Coarse
    * grained memory pools are captured. This does not make sense
    * if only of them is captured
    if (src_dev_type == HSA_DEVICE_TYPE_CPU) {
      bool src_fine_grained =  pool_list_[src_idx].is_fine_grained_;
      if (src_fine_grained == false) {
        continue;
      }
    }
    */

    hsa_agent_t src_agent = pool_list_[src_idx].owner_agent_;
    hsa_amd_memory_pool_t src_pool = pool_list_[src_idx].pool_;

    for (uint32_t dst_idx = 0; dst_idx < size; dst_idx++) {

      // Determine if the pool belongs to Cpu and is coarse-grained
      uint32_t dst_dev_idx = pool_list_[dst_idx].agent_index_;
      hsa_device_type_t dst_dev_type = agent_list_[dst_dev_idx].device_type_;

      /*
       * This block of code makes sense only if both Fine and Coarse
       * grained memory pools are captured. This does not make sense
       * if only of them is captured
      if (dst_dev_type == HSA_DEVICE_TYPE_CPU) {
        bool dst_fine_grained =  pool_list_[dst_idx].is_fine_grained_;
        if (dst_fine_grained == false) {
          continue;
        }
      }
      */
      hsa_agent_t dst_agent = pool_list_[dst_idx].owner_agent_;
      hsa_amd_memory_pool_t dst_pool = pool_list_[dst_idx].pool_;

      // Determine if accessibility to dst pool for src agent is not denied
      hsa_amd_memory_pool_access_t access1;
      status = hsa_amd_agent_memory_pool_get_info(src_agent, dst_pool,
                             HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, &access1);
      ErrorCheck(status);

      // Determine if accessibility to src pool for dst agent is not denied
      hsa_amd_memory_pool_access_t access2;
      status = hsa_amd_agent_memory_pool_get_info(dst_agent, src_pool,
                             HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, &access2);

      // Access between the two agents is Non-Existent
      if ((access1 == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED) &&
          (access2 == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED)) {
        access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = 0;
      }

      // Access between the two agents is Unidirectional
      if ((access1 == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED) ||
          (access2 == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED)) {
        if ((src_dev_type == HSA_DEVICE_TYPE_GPU) &&
            (dst_dev_type == HSA_DEVICE_TYPE_GPU)) {
          access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = 0;
        } else {
          access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = 1;
        }
      }

      // Access between the two agents is Bidirectional
      if ((access1 != HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED) &&
          (access2 != HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED)) {
        access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = 2;
      }
    }
  }
}

void RocmBandwidthTest::DiscoverTopology() {

  // Populate the lists of agents and pools
  err_ = hsa_iterate_agents(AgentInfo, this);

  // Populate the access matrix
  PopulateAccessMatrix();
  DiscoverLinkWeight();
}

void RocmBandwidthTest::DiscoverLinkWeight() {

  // Allocate space if it is first time
  if (link_matrix_ == NULL) {
    link_matrix_ = new uint32_t[agent_index_ * agent_index_]();
  }

  agent_info_t agent_info;
  hsa_agent_t agent1;
  hsa_agent_t agent2;
  hsa_amd_memory_pool_link_info_t link_info = {0};
  for (uint32_t idx1 = 0; idx1 < agent_index_; idx1++) {
      agent1 = agent_list_[idx1].agent_;
    for (uint32_t idx2 = 0; idx2 < agent_index_; idx2++) {
      if (idx1 == idx2) {
        link_matrix_[(idx1 *agent_index_) + idx2] = 0;
        continue;
      }
      if (agent_pool_list_[idx2].pool_list.size() != 0) {
        hsa_amd_memory_pool_t& pool = agent_pool_list_[idx2].pool_list[0].pool_;
        agent2 = agent_pool_list_[idx2].agent.agent_;
        err_ = hsa_amd_agent_memory_pool_get_info(agent1, pool,
                         HSA_AMD_AGENT_MEMORY_POOL_INFO_LINK_INFO, &link_info);
        link_matrix_[(idx1 *agent_index_) + idx2] = link_info.numa_distance;
      }
    }
  }
}

