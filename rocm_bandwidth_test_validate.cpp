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
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <sstream>

bool RocmBandwidthTest::PoolIsPresent(vector<size_t>& in_list) {
    bool is_present;
    uint32_t idx1 = 0;
    uint32_t idx2 = 0;
    uint32_t count = in_list.size();
    uint32_t pool_count = pool_list_.size();
    for (idx1 = 0; idx1 < count; idx1++) {
        is_present = false;
        for (idx2 = 0; idx2 < pool_count; idx2++) {
            if (in_list[idx1] == pool_list_[idx2].index_) {
                is_present = true;
                break;
            }
        }
        if (is_present == false) {
            return false;
        }
    }

    return true;
}

bool RocmBandwidthTest::PoolIsDuplicated(vector<size_t>& in_list) {
    uint32_t idx1 = 0;
    uint32_t idx2 = 0;
    uint32_t count = in_list.size();
    for (idx1 = 0; idx1 < count; idx1++) {
        for (idx2 = 0; idx2 < count; idx2++) {
            if ((in_list[idx1] == in_list[idx2]) && (idx1 != idx2)) {
                return false;
            }
        }
    }
    return true;
}

bool RocmBandwidthTest::ValidateReadOrWriteReq(vector<size_t>& in_list) {
    // Determine read / write request is even
    // Request is specified as a list of memory
    // pool, agent tuples - first element identifies
    // memory pool while the second element denotes
    // an agent
    uint32_t list_size = in_list.size();
    if ((list_size % 2) != 0) {
        return false;
    }

    // Validate the list of pool-agent tuples
    for (uint32_t idx = 0; idx < list_size; idx += 2) {
        uint32_t pool_idx = in_list[idx];
        uint32_t exec_idx = in_list[idx + 1];
        // Determine the pool and agent exist in system
        if ((pool_idx >= pool_index_) || (exec_idx >= agent_index_)) {
            return false;
        }
    }
    return true;
}

bool RocmBandwidthTest::ValidateReadReq() { return ValidateReadOrWriteReq(read_list_); }

bool RocmBandwidthTest::ValidateWriteReq() { return ValidateReadOrWriteReq(write_list_); }

bool RocmBandwidthTest::ValidateCopyReq(vector<size_t>& in_list) {
    // Determine pool list length is valid
    uint32_t count = in_list.size();
    uint32_t pool_count = pool_list_.size();
    if (count > pool_count) {
        return false;
    }

    // Determine no pool is duplicated
    bool status = PoolIsDuplicated(in_list);
    if (status == false) {
        return false;
    }

    // Determine every pool is present in system
    return PoolIsPresent(in_list);
}

bool RocmBandwidthTest::ValidateBidirCopyReq() { return ValidateCopyReq(bidir_list_); }

bool RocmBandwidthTest::ValidateUnidirCopyReq() {
    return ((ValidateCopyReq(src_list_)) && (ValidateCopyReq(dst_list_)));
}

bool RocmBandwidthTest::ValidateConcurrentCopyReq() {
    // Determine every pool is present in system
    return PoolIsPresent(bidir_list_);
}

bool RocmBandwidthTest::ValidateArguments() {
    // Determine if user has requested a READ
    // operation and gave valid inputs
    if (req_read_ == REQ_READ) {
        return ValidateReadReq();
    }

    // Determine if user has requested a WRITE
    // operation and gave valid inputs
    if (req_write_ == REQ_WRITE) {
        return ValidateWriteReq();
    }

    // Determine if user has requested a Copy
    // operation that is bidirectional and gave
    // valid inputs. Same validation is applied
    // for all-to-all unidirectional copy operation
    if ((req_copy_bidir_ == REQ_COPY_BIDIR) || (req_copy_all_bidir_ == REQ_COPY_ALL_BIDIR)) {
        return ValidateBidirCopyReq();
    }

    // Determine if user has requested a Copy
    // operation that is unidirectional and gave
    // valid inputs. Same validation is applied
    // for all-to-all bidirectional copy operation
    if ((req_copy_unidir_ == REQ_COPY_UNIDIR) || (req_copy_all_unidir_ == REQ_COPY_ALL_UNIDIR)) {
        return ValidateUnidirCopyReq();
    }

    // Determine if user has requested a Concurrent
    // Copy operation that is unidirectional or bidirectional
    // and gave valid inputs.
    if ((req_concurrent_copy_bidir_ == REQ_CONCURRENT_COPY_BIDIR) ||
        (req_concurrent_copy_unidir_ == REQ_CONCURRENT_COPY_UNIDIR)) {
        return ValidateConcurrentCopyReq();
    }

    // All of the request are well formed
    return true;
}
