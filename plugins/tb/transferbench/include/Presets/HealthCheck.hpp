/*
Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

enum {
  HBM_READ      = 0,
  HBM_WRITE     = 1,
  HBM_COPY      = 2,
  HBM_ADD       = 3,
  NUM_HBM_TESTS = 4
} HbmTests;

struct HbmTestConfig
{
  std::string name;
  int numInputs;
  int numOutputs;
};

HbmTestConfig HbmTestConfigs[NUM_HBM_TESTS] =
{ {"READ",  1, 0},
  {"WRITE", 0, 1},
  {"COPY",  1, 1},
  {"ADD",   2, 1}
};

typedef struct
{
  double unidirHostToDeviceCopyLimit;
  double unidirDeviceToHostCopyLimit;
  double bidirDmaCopyLimit;
  int    a2aUnrollFactor;
  int    a2aNumSubExecs;
  double a2aCopyLimit;

  int    hbmBlockSize   [NUM_HBM_TESTS];
  int    hbmUnrollFactor[NUM_HBM_TESTS];
  int    hbmTemporalMode[NUM_HBM_TESTS];
  double hbmLimit       [NUM_HBM_TESTS];
} TestConfig;

typedef enum
{
  MODEL_08_GFX0942_304 = 0,
  MODEL_08_GFX0942_064 = 1,
  NUM_SUPPORTED_MODELS = 2
} ModelEnum;

// All limits are scaled by this factor
#define SFACTOR 0.97

TestConfig Config_08_GFX0942_304 = {
  .unidirHostToDeviceCopyLimit   = 50,
  .unidirDeviceToHostCopyLimit   = 50,
  .bidirDmaCopyLimit             = 90,
  .a2aUnrollFactor               = 2,
  .a2aNumSubExecs                = 8,
  .a2aCopyLimit                  = 45,
  .hbmBlockSize                  = { 384,  256,  320,  256},
  .hbmUnrollFactor               = {   7,    4,    8,    7},
  .hbmTemporalMode               = {   3,    3,    3,    3},
  .hbmLimit                      = {4980, 4850, 2045, 1405},
};

TestConfig Config_08_GFX0942_064 = {
  .unidirHostToDeviceCopyLimit   = 50,
  .unidirDeviceToHostCopyLimit   = 50,
  .bidirDmaCopyLimit             = 90,
  .a2aUnrollFactor               = 2,
  .a2aNumSubExecs                = 8,
  .a2aCopyLimit                  = 45,
  .hbmBlockSize                  = { 448,  448,  448,  384},
  .hbmUnrollFactor               = {   8,    3,    8,    7},
  .hbmTemporalMode               = {   3,    3,    3,    3},
  .hbmLimit                      = {4180, 2800, 1400, 1055},
};

TestConfig TestConfigs[NUM_SUPPORTED_MODELS] =
{
  Config_08_GFX0942_304,
  Config_08_GFX0942_064,
};

int DetectModel()
{
  int numGpuDevices = TransferBench::GetNumExecutors(EXE_GPU_GFX);

  std::string archName = "";
  int numSubExecutors = 0;

  // Loop over all GPUs and determine if they are identical
  for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
    // Check that arch name is identical
    hipDeviceProp_t prop;
    HIP_CALL(hipGetDeviceProperties(&prop, gpuId));
    std::string fullName = prop.gcnArchName;
    std::string currArchName = fullName.substr(0, fullName.find(':'));
    if (archName != "" && archName != currArchName) {
      printf("[WARN] healthcheck preset is currently only supported when all GPUs are identical\n");
      printf("       Detected both %s and %s\n", archName.c_str(), currArchName.c_str());
      exit(1);
    }
    archName = currArchName;

    // Check number of subexecutors
    int currNumSubExecutors = TransferBench::GetNumSubExecutors({EXE_GPU_GFX, gpuId});
    if (numSubExecutors != 0 && numSubExecutors != currNumSubExecutors) {
      printf("[WARN] healthcheck preset is currently only supported when all GPUs are identical\n");
      printf("       Detected different subexecutor counts: %d and %d\n", numSubExecutors, currNumSubExecutors);
      exit(1);
    }
    numSubExecutors = currNumSubExecutors;
  }

  // Classify based on detected configuration
  if (numGpuDevices == 8) {
    if (archName == "gfx942") {
      switch (numSubExecutors) {
      case 304: return MODEL_08_GFX0942_304;
      case 64:  return MODEL_08_GFX0942_064;
      }
    }
  }

  printf("[WARN] healthcheck preset is currently not supported on this hardware\n");
  printf("       Detected %d x [%s] with [%d] subexecutors per GPU\n", numGpuDevices, archName.c_str(), numSubExecutors);
  exit(1);
}

int TestUnidir(int modelId, bool verbose)
{
  TestConfig const& testConfig = TestConfigs[modelId];
  TransferBench::ConfigOptions cfg;
  TransferBench::TestResults results;

  int hasFail = 0;
  int numGpuDevices = TransferBench::GetNumExecutors(EXE_GPU_GFX);
  cfg.dma.useHsaCopy = 1;

  // Run unidirectional host to device copy
  printf("Testing unidirectional host to device copy%c", verbose ? '\n' : ' ');
  {
    double limit = testConfig.unidirHostToDeviceCopyLimit * SFACTOR;

    std::vector<std::pair<int, double>> fails;
    for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
      if (!verbose) printf(".");
      fflush(stdout);

      int memIndex = TransferBench::GetClosestCpuNumaToGpu(gpuId);
      if (memIndex == -1) {
        printf("[ERROR] Unable to detect closest CPU NUMA node to GPU %d\n", gpuId);
        exit(1);
      }

      std::vector<Transfer> transfers(1);
      Transfer& t   = transfers[0];
      t.exeDevice   = {EXE_GPU_DMA, gpuId};
      t.numBytes    = 256*1024*1024;
      t.srcs        = {{MEM_CPU, memIndex}};
      t.dsts        = {{MEM_GPU, gpuId}};
      t.numSubExecs = 1;

      if (TransferBench::RunTransfers(cfg, transfers, results)) {
        double measuredBw = results.tfrResults[0].avgBandwidthGbPerSec;
        if (measuredBw < limit) {
          fails.push_back(std::make_pair(gpuId, measuredBw));
        }
        if (verbose) printf("   GPU %02d: Measured %6.2f Limit %6.2f\n", gpuId, measuredBw, limit);
      } else {
        PrintErrors(results.errResults);
      }
    }

    if (fails.size() == 0) {
      printf("PASS\n");
    } else {
      hasFail = 1;
      printf("FAIL (%lu test(s))\n", fails.size());
      for (auto p : fails) {
        printf(" GPU %02d: Measured: %6.2f GB/s      Criteria: %6.2f GB/s\n", p.first, p.second, limit);
      }
    }
  }

  // Run unidirectional device to host copy
  printf("Testing unidirectional device to host copy%c", verbose ? '\n' : ' ');
  {
    double limit = testConfig.unidirDeviceToHostCopyLimit * SFACTOR;

    std::vector<std::pair<int, double>> fails;
    for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
      if (!verbose) printf(".");
      fflush(stdout);

      int memIndex = TransferBench::GetClosestCpuNumaToGpu(gpuId);
      if (memIndex == -1) {
        printf("[ERROR] Unable to detect closest CPU NUMA node to GPU %d\n", gpuId);
        exit(1);
      }

      std::vector<Transfer> transfers(1);
      Transfer& t   = transfers[0];
      t.exeDevice   = {EXE_GPU_DMA, gpuId};
      t.numBytes    = 256*1024*1024;
      t.srcs        = {{MEM_GPU, gpuId}};
      t.dsts        = {{MEM_CPU, memIndex}};
      t.numSubExecs = 1;

      if (TransferBench::RunTransfers(cfg, transfers, results)) {
        double measuredBw = results.tfrResults[0].avgBandwidthGbPerSec;
        if (measuredBw < limit) {
          fails.push_back(std::make_pair(gpuId, measuredBw));
        }
        if (verbose) printf("   GPU %02d: Measured %6.2f Limit %6.2f\n", gpuId, measuredBw, limit);
      } else {
        PrintErrors(results.errResults);
      }
    }

    if (fails.size() == 0) {
      printf("PASS\n");
    } else {
      hasFail = 1;
      printf("FAIL (%lu test(s))\n", fails.size());
      for (auto p : fails) {
        printf(" GPU %02d: Measured: %6.2f GB/s      Criteria: %6.2f GB/s\n", p.first, p.second, limit);
      }
    }
  }
  return hasFail;
}

int TestBidir(int modelId, bool verbose)
{
  TestConfig const& testConfig = TestConfigs[modelId];
  TransferBench::ConfigOptions cfg;


  int hasFail = 0;
  int numGpuDevices = TransferBench::GetNumExecutors(EXE_GPU_GFX);

  printf("Testing bidirectional host<->device copies%c", verbose ? '\n' : ' ');
  {
    double limit = testConfig.bidirDmaCopyLimit * SFACTOR;

    std::vector<std::pair<int, double>> fails;
    for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
      if (!verbose)  printf(".");
      fflush(stdout);

      int memIndex = TransferBench::GetClosestCpuNumaToGpu(gpuId);
      if (memIndex == -1) {
        printf("[ERROR] Unable to detect closest CPU NUMA node to GPU %d\n", gpuId);
        exit(1);
      }

      std::vector<Transfer> transfers(2);
      Transfer& t0 = transfers[0];
      Transfer& t1 = transfers[1];

      t0.exeDevice   = {EXE_GPU_DMA, gpuId};
      t0.numBytes    = 256*1024*1024;
      t0.srcs        = {{MEM_GPU, gpuId}};
      t0.dsts        = {{MEM_CPU, memIndex}};
      t0.numSubExecs = 1;

      t1.exeDevice   = {EXE_GPU_DMA, gpuId};
      t1.numBytes    = 256*1024*1024;
      t1.srcs        = {{MEM_CPU, memIndex}};
      t1.dsts        = {{MEM_GPU, gpuId}};
      t1.numSubExecs = 1;

      TransferBench::TestResults results;
      if (TransferBench::RunTransfers(cfg, transfers, results)) {
        double measuredBw = (results.tfrResults[0].avgBandwidthGbPerSec +
                             results.tfrResults[1].avgBandwidthGbPerSec);
        if (measuredBw < limit) {
          fails.push_back(std::make_pair(gpuId, measuredBw));
        }
        if (verbose) printf("   GPU %02d: Measured %6.2f Limit %6.2f\n", gpuId, measuredBw, limit);
      } else {
        PrintErrors(results.errResults);
      }
    }

    if (fails.size() == 0) {
      printf("PASS\n");
    } else {
      hasFail = 1;
      printf("FAIL (%lu test(s))\n", fails.size());
      for (auto p : fails) {
        printf(" GPU %02d: Measured: %6.2f GB/s      Criteria: %6.2f GB/s\n", p.first, p.second, limit);
      }
    }
  }
  return hasFail;
}

int TestAllToAll(int modelId, bool verbose)
{
  TestConfig const& testConfig = TestConfigs[modelId];
  TransferBench::ConfigOptions cfg;
  cfg.gfx.unrollFactor = testConfig.a2aUnrollFactor;

  int numSubExecs = testConfig.a2aNumSubExecs;
  int hasFail = 0;
  int numGpuDevices = TransferBench::GetNumExecutors(EXE_GPU_GFX);

  printf("Testing all-to-all XGMI copies            %c", verbose ? '\n' : ' '); fflush(stdout);
  {
    double limit = testConfig.a2aCopyLimit * SFACTOR;

    std::vector<Transfer> transfers;
    for (int i = 0; i < numGpuDevices; i++) {
      for (int j = 0; j < numGpuDevices; j++) {
        if (i == j) continue;
        Transfer t;
        t.numBytes    = 256*1024*1024;
        t.numSubExecs = numSubExecs;
        t.exeDevice   = {EXE_GPU_GFX, i};
        t.srcs        = {{MEM_GPU_FINE, i}};
        t.dsts        = {{MEM_GPU_FINE, j}};
        transfers.push_back(t);
      }
    }
    std::vector<std::pair<std::pair<int,int>, double>> fails;
    TransferBench::TestResults results;
    if (TransferBench::RunTransfers(cfg, transfers, results)) {
      int transferIdx = 0;
      for (int i = 0; i < numGpuDevices; i++) {
        if (!verbose) printf("."); fflush(stdout);
        for (int j = 0; j < numGpuDevices; j++) {
          if (i == j) continue;
          double bw = results.tfrResults[transferIdx].avgBandwidthGbPerSec;
          if (bw < limit) {
            fails.push_back(std::make_pair(std::make_pair(i,j), bw));
          }
          if (verbose) printf("   GPU %02d to GPU %02d: : Measured %6.2f Limit %6.2f\n", i, j, bw, limit);
          transferIdx++;
        }
      }
    }
    if (fails.size() == 0) {
      printf("PASS\n");
    } else {
      hasFail = 1;
      printf("FAIL (%lu test(s))\n", fails.size());
      for (auto p : fails) {
        printf(" GPU %02d to GPU %02d: %6.2f GB/s      Criteria: %6.2f GB/s\n", p.first.first, p.first.second, p.second, limit);
      }
    }
  }
  return hasFail;
}

int TestHbmPerformance(int modelId, bool verbose)
{
  TestConfig const& testConfig = TestConfigs[modelId];

  int hasFail = 0;
  int numGpuDevices = TransferBench::GetNumExecutors(EXE_GPU_GFX);
  char testname[50];

  for (int testId = 0; testId < NUM_HBM_TESTS; testId++) {
    TransferBench::ConfigOptions cfg;
    cfg.general.numIterations = 1000;
    cfg.general.numWarmups    = 50;
    cfg.gfx.blockSize         = testConfig.hbmBlockSize[testId];
    cfg.gfx.unrollFactor      = testConfig.hbmUnrollFactor[testId];
    cfg.gfx.temporalMode      = testConfig.hbmTemporalMode[testId];

    sprintf(testname, "Testing HBM performance [%s]", HbmTestConfigs[testId].name.c_str());
    if (verbose) printf("[Blocksize: %d Unroll: %d TemporalMode: %d]\n", cfg.gfx.blockSize, cfg.gfx.unrollFactor, cfg.gfx.temporalMode);
    printf("%-42s%c", testname, verbose ? '\n' : ' ');
    fflush(stdout);

    int numInputs = HbmTestConfigs[testId].numInputs;
    int numOutputs = HbmTestConfigs[testId].numOutputs;

    double limit = testConfig.hbmLimit[testId] * SFACTOR;

    std::vector<std::pair<int, double>> fails;
    TransferBench::TestResults results;
    std::vector<Transfer> transfers;

    for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
      Transfer t;
      t.numSubExecs = TransferBench::GetNumSubExecutors({EXE_GPU_GFX, gpuId});
      t.numBytes    = 16777216ULL * t.numSubExecs;
      t.exeDevice   = {EXE_GPU_GFX, gpuId};
      for (int i = 0; i < numInputs; i++) t.srcs.push_back({MEM_GPU, gpuId});
      for (int i = 0; i < numOutputs; i++) t.dsts.push_back({MEM_GPU, gpuId});
      transfers.push_back(t);
    }

    if (TransferBench::RunTransfers(cfg, transfers, results)) {
      for (int gpuId = 0; gpuId < numGpuDevices; gpuId++) {
        if (!verbose) printf(".");
        fflush(stdout);
        double measuredBw = results.tfrResults[gpuId].avgBandwidthGbPerSec;
        if (measuredBw < limit) {
          fails.push_back(std::make_pair(gpuId, measuredBw));
        }
        if (verbose) printf("   GPU %02d: Measured %6.2f Limit %6.2f\n", gpuId, measuredBw, limit);
      }
    } else {
      PrintErrors(results.errResults);
    }

    if (fails.size() == 0) {
      printf("PASS\n");
    } else {
      hasFail = 1;
      printf("FAIL (%lu test(s))\n", fails.size());
      for (auto p : fails) {
        printf(" GPU %02d: Measured: %6.2f GB/s      Criteria: %6.2f GB/s\n", p.first, p.second, limit);
      }
    }
  }
  return hasFail;
}

void HealthCheckPreset(EnvVars&           ev,
                       size_t      const  numBytesPerTransfer,
                       std::string const  presetName)
{
  // Check for supported platforms
#if defined(__NVCC__)
  printf("[WARN] healthcheck preset not supported on NVIDIA hardware\n");
  return;
#endif

  printf("Disclaimer:\n");
  printf("==================================================================\n");
  printf("NOTE: This is an experimental feature and may be subject to change\n");
  printf("      Failures do not necessarily indicate hardware issues, as other factors\n");
  printf("      such as simultaneous workloads may influence results\n");
  printf("\n");

  // Collect custom env vars for this preset
  int verbose = EnvVars::GetEnvVar("VERBOSE", 0);

  // Determine if this is a supported model
  int modelId = DetectModel();

  // Run through all tests
  int numFails = 0;
  numFails += TestHbmPerformance(modelId, verbose);
  numFails += TestUnidir(modelId, verbose);
  numFails += TestBidir(modelId, verbose);
  numFails += TestAllToAll(modelId, verbose);
  exit(numFails ? 1 : 0);
}
