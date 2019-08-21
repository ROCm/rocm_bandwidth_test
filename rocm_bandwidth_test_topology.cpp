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
#include <string>

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
  // coarse-grained memory pools if agent is CPU. Default
  // is to skip coarse-grained memory pools
  agent_info_t& agent_info = asyncDrvr->agent_list_.back();
  if (agent_info.device_type_ == HSA_DEVICE_TYPE_CPU) {
    if (asyncDrvr->skip_cpu_fine_grain_ != NULL) {
      if (is_fine_grained == true) {
        return HSA_STATUS_SUCCESS;
      }
    } else {
      if (is_fine_grained == false) {
        return HSA_STATUS_SUCCESS;
      }
    }
  }

  // Consult user request and add either fine-grained or
  // coarse-grained memory pools if agent is GPU. Default
  // is to skip fine-grained memory pools
  if (agent_info.device_type_ == HSA_DEVICE_TYPE_GPU) {
    if (asyncDrvr->skip_gpu_coarse_grain_ != NULL) {
      if (is_fine_grained == false) {
        return HSA_STATUS_SUCCESS;
      }
    } else {
      if (is_fine_grained == true) {
        return HSA_STATUS_SUCCESS;
      }
    }
  }

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

void PopulateBDF(uint32_t bdf_id, agent_info_t *agent_info) {

  uint8_t func_id = (bdf_id & 0x00000003);
  uint8_t dev_id = ((bdf_id & 0x000000F8) >> 3);
  uint8_t bus_id = ((bdf_id & 0x0000FF00) >> 8);
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(sizeof(uint8_t) * 2);
  stream << std::hex << +bus_id << ":" << +dev_id << "." << +func_id;
  strcpy(agent_info->bdf_id_, (stream.str()).c_str());
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
  // and BDF fields before adding it to the list of agent_info_t objects
  agent_info_t agent_info(agent, asyncDrvr->agent_index_, device_type);
  status = hsa_agent_get_info(agent,
                      (hsa_agent_info_t)HSA_AMD_AGENT_INFO_PRODUCT_NAME,
                      (void *)&agent_info.name_[0]);
  if (device_type == HSA_DEVICE_TYPE_GPU) {
    uint32_t bdf_id = 0;
    status = hsa_agent_get_info(agent,
                      (hsa_agent_info_t)HSA_AMD_AGENT_INFO_BDFID,
                      (void *)&bdf_id);
    PopulateBDF(bdf_id, &agent_info);
  }
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
  direct_access_matrix_ = new uint32_t[agent_index_ * agent_index_]();

  hsa_status_t status;
  uint32_t size = pool_list_.size();
  for (uint32_t src_idx = 0; src_idx < size; src_idx++) {

    // Get handle of Src agent of the pool
    uint32_t src_dev_idx = pool_list_[src_idx].agent_index_;
    hsa_agent_t src_agent = pool_list_[src_idx].owner_agent_;
    hsa_amd_memory_pool_t src_pool = pool_list_[src_idx].pool_;
    hsa_device_type_t src_dev_type = agent_list_[src_dev_idx].device_type_;

    for (uint32_t dst_idx = 0; dst_idx < size; dst_idx++) {

      // Get handle of Dst pool
      uint32_t dst_dev_idx = pool_list_[dst_idx].agent_index_;
      hsa_agent_t dst_agent = pool_list_[dst_idx].owner_agent_;
      hsa_amd_memory_pool_t dst_pool = pool_list_[dst_idx].pool_;
      hsa_device_type_t dst_dev_type = agent_list_[dst_dev_idx].device_type_;

      // Determine if src agent has access to dst pool
      hsa_amd_memory_pool_access_t access;
      status = hsa_amd_agent_memory_pool_get_info(src_agent, dst_pool,
                             HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, &access);
      ErrorCheck(status);
      
      // Record if Src device can access or not
      uint32_t path;
      path = (access == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED) ? 0 : 1;
      direct_access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = path;
      
      if ((src_dev_type == HSA_DEVICE_TYPE_CPU) &&
          (dst_dev_type == HSA_DEVICE_TYPE_GPU) &&
          (access == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED)) {
        status = hsa_amd_agent_memory_pool_get_info(dst_agent, src_pool,
                             HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, &access);
        ErrorCheck(status);
      }

      // Access between the two agents is Non-Existent
      path = (access == HSA_AMD_MEMORY_POOL_ACCESS_NEVER_ALLOWED) ? 0 : 1;
      access_matrix_[(src_dev_idx * agent_index_) + dst_dev_idx] = path;
    }
  }
}

void RocmBandwidthTest::DiscoverTopology() {

  // Populate the lists of agents and pools
  err_ = hsa_iterate_agents(AgentInfo, this);

  // Populate the access, link type and weight matrices
  // Access matrix must be populated first
  PopulateAccessMatrix();
  DiscoverLinkProps();
}

uint32_t GetLinkType(hsa_device_type_t src_dev_type,
                     hsa_device_type_t dst_dev_type,
                     hsa_amd_memory_pool_link_info_t* link_info, uint32_t hops) {
  
  // Link type is ignored, linkinfo is illegal
  // Currently Thunk collapses multi-hop paths into one
  // while accumulating their numa weight
  // @note: Thunk retains the original link type
  if (hops != 1) {
    return RocmBandwidthTest::LINK_TYPE_IGNORED;
  }
  
  // Return link type only if it specified as XGMI
  if ((link_info[0]).link_type == HSA_AMD_LINK_INFO_TYPE_XGMI) {
    return RocmBandwidthTest::LINK_TYPE_XGMI;
  }

  // In this case all we know is there is a path involving
  // one or more links. Since it binding either two GPU's or
  // one Gpu and one Cpu, we infer it to be of type PCIe
  if ((src_dev_type == HSA_DEVICE_TYPE_GPU) ||
      (dst_dev_type == HSA_DEVICE_TYPE_GPU)) {
    return RocmBandwidthTest::LINK_TYPE_PCIE;
  }

  // This occurs when both devices are CPU's
  return RocmBandwidthTest::LINK_TYPE_IGNORED;
}

uint32_t GetLinkWeight(hsa_amd_memory_pool_link_info_t* link_info, uint32_t hops) {

  uint32_t weight = 0;
  for(uint32_t hopIdx = 0; hopIdx < hops; hopIdx++) {
    weight += (link_info[hopIdx]).numa_distance;
  }
  return weight;
}

void RocmBandwidthTest::BindLinkProps(uint32_t idx1, uint32_t idx2) {
  
  // Agent has no pools so no need to look for numa distance
  if (agent_pool_list_[idx2].pool_list.size() == 0) {
    link_hops_matrix_[(idx1 * agent_index_) + idx2] = 0xFFFFFFFF;
    link_weight_matrix_[(idx1 * agent_index_) + idx2] = 0xFFFFFFFF;
    link_type_matrix_[(idx1 * agent_index_) + idx2] = LINK_TYPE_NO_PATH;
    return;
  }
  
  uint32_t hops = 0;
  hsa_agent_t agent1 = agent_list_[idx1].agent_;
  hsa_amd_memory_pool_t& pool = agent_pool_list_[idx2].pool_list[0].pool_;
  err_ = hsa_amd_agent_memory_pool_get_info(agent1, pool,
                   HSA_AMD_AGENT_MEMORY_POOL_INFO_NUM_LINK_HOPS, &hops);
  if (hops < 1) {
    link_hops_matrix_[(idx1 * agent_index_) + idx2] = 0xFFFFFFFF;
    link_weight_matrix_[(idx1 * agent_index_) + idx2] = 0xFFFFFFFF;
    link_type_matrix_[(idx1 * agent_index_) + idx2] = LINK_TYPE_NO_PATH;
    return;
  }

  hsa_amd_memory_pool_link_info_t *link_info;
  uint32_t link_info_sz = hops * sizeof(hsa_amd_memory_pool_link_info_t);
  link_info = (hsa_amd_memory_pool_link_info_t *)malloc(link_info_sz);
  memset(link_info, 0, (hops * sizeof(hsa_amd_memory_pool_link_info_t)));
  err_ = hsa_amd_agent_memory_pool_get_info(agent1, pool,
                 HSA_AMD_AGENT_MEMORY_POOL_INFO_LINK_INFO, link_info);


  link_hops_matrix_[(idx1 * agent_index_) + idx2] = hops;
  link_weight_matrix_[(idx1 * agent_index_) + idx2] = GetLinkWeight(link_info, hops);
  
  // Initialize link type based on Src and Dst devices plus link
  // type reported by ROCr library
  hsa_device_type_t src_dev_type = agent_list_[idx1].device_type_;
  hsa_device_type_t dst_dev_type = agent_list_[idx2].device_type_;
  link_type_matrix_[(idx1 * agent_index_) + idx2] = GetLinkType(src_dev_type,
                                                                dst_dev_type, link_info, hops);
  // Free the allocated link block
  free(link_info); 
}

void RocmBandwidthTest::DiscoverLinkProps() {

  // Allocate space if it is first time
  if (link_weight_matrix_ == NULL) {
    link_type_matrix_ = new uint32_t[agent_index_ * agent_index_]();
    link_hops_matrix_ = new uint32_t[agent_index_ * agent_index_]();
    link_weight_matrix_ = new uint32_t[agent_index_ * agent_index_]();
  }

  agent_info_t agent_info;
  for (uint32_t idx1 = 0; idx1 < agent_index_; idx1++) {
    for (uint32_t idx2 = 0; idx2 < agent_index_; idx2++) {
      if (idx1 == idx2) {
        link_hops_matrix_[(idx1 * agent_index_) + idx2] = 0;
        link_weight_matrix_[(idx1 *agent_index_) + idx2] = 0;
        link_type_matrix_[(idx1 * agent_index_) + idx2] = LINK_TYPE_SELF;
        continue;
      }
      BindLinkProps(idx1, idx2);
    }
  }
}

