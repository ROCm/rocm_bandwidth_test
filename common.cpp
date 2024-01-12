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

void error_check(hsa_status_t hsa_error_code, int line_num, const char* str) {
    if (hsa_error_code != HSA_STATUS_SUCCESS && hsa_error_code != HSA_STATUS_INFO_BREAK) {
        printf("HSA Error Found!  In file: %s;   At line: %d\n", str, line_num);
        const char* string = NULL;
        hsa_status_string(hsa_error_code, &string);
        printf("Error: %s\n", string);
        exit(EXIT_FAILURE);
    }
}

// So far, always find the first device
hsa_status_t FindGpuDevice(hsa_agent_t agent, void* data) {
    if (data == NULL) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }

    hsa_device_type_t hsa_device_type;
    hsa_status_t hsa_error_code =
        hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &hsa_device_type);
    if (hsa_error_code != HSA_STATUS_SUCCESS) {
        return hsa_error_code;
    }

    if (hsa_device_type == HSA_DEVICE_TYPE_GPU) {
        *((hsa_agent_t*)data) = agent;
        return HSA_STATUS_INFO_BREAK;
    }

    return HSA_STATUS_SUCCESS;
}

hsa_status_t FindCpuDevice(hsa_agent_t agent, void* data) {
    if (data == NULL) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }

    hsa_device_type_t hsa_device_type;
    hsa_status_t hsa_error_code =
        hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &hsa_device_type);
    if (hsa_error_code != HSA_STATUS_SUCCESS) {
        return hsa_error_code;
    }

    if (hsa_device_type == HSA_DEVICE_TYPE_CPU) {
        *((hsa_agent_t*)data) = agent;
        return HSA_STATUS_INFO_BREAK;
    }

    return HSA_STATUS_SUCCESS;
}

hsa_status_t FindGlobalPool(hsa_amd_memory_pool_t region, void* data) {
    if (NULL == data) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }

    hsa_status_t err;
    hsa_amd_segment_t segment;
    uint32_t flag;

    err = hsa_amd_memory_pool_get_info(region, HSA_AMD_MEMORY_POOL_INFO_SEGMENT, &segment);
    ErrorCheck(err);

    err = hsa_amd_memory_pool_get_info(region, HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS, &flag);
    ErrorCheck(err);

    if ((HSA_AMD_SEGMENT_GLOBAL == segment) &&
        (flag & HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_FINE_GRAINED)) {
        *((hsa_amd_memory_pool_t*)data) = region;
    }

    return HSA_STATUS_SUCCESS;
}

double CalcMedian(vector<double> scores) {
    double median;
    size_t size = scores.size();

    if (size % 2 == 0)
        median = (scores[size / 2 - 1] + scores[size / 2]) / 2;
    else
        median = scores[size / 2];

    return median;
}

double CalcMean(vector<double> scores) {
    double mean = 0;
    size_t size = scores.size();

    for (size_t i = 0; i < size; ++i) mean += scores[i];

    return mean / size;
}

double CalcStdDeviation(vector<double> scores, int score_mean) {
    double ret = 0.0;
    for (size_t i = 0; i < scores.size(); ++i) {
        ret += (scores[i] - score_mean) * (scores[i] - score_mean);
    }

    ret /= scores.size();

    return sqrt(ret);
}

int CalcConcurrentQueues(vector<double> scores) {
    int num_of_concurrent_queues = 0;
    vector<double> execpted_exec_time_array;

    for (size_t i = 0; i < scores.size(); ++i) {
        execpted_exec_time_array.push_back(scores[0] / (1 << i));
    }

    for (size_t i = 0; i < scores.size(); ++i) {
        cout << "expected exe time = " << execpted_exec_time_array[i] << endl;
    }

    for (size_t i = 1; i < scores.size(); ++i) {
        if ((execpted_exec_time_array[i] - scores[i]) < 0.1 * execpted_exec_time_array[i])
            ++num_of_concurrent_queues;
    }

    return num_of_concurrent_queues;
}

/**  hsa_status_t FindHostRegion(hsa_region_t region, void *data) {
  if (data == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  bool is_host_region = false;
  hsa_status_t hsa_error_code = hsa_region_get_info(
    region, (hsa_region_info_t)HSA_EXT_REGION_INFO_HOST_ACCESS, &is_host_region
  );
  if (hsa_error_code != HSA_STATUS_SUCCESS) {
    return hsa_error_code;
  }

  if (is_host_region) {
    *((hsa_region_t*)data) = region;
  }

  return HSA_STATUS_SUCCESS;
} */
