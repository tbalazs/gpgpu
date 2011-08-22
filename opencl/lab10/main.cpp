#include <iostream>
#include <fstream>

#include <cmath>

#include <CL/opencl.h>

#include "clwrapper.hpp"

ClWrapper cl;

// OpenCL program
cl_program program;

void testUniform1DArray(size_t maxWorkGroupSize, int randomNums, cl_mem randomsGPU){
  cl_kernel testUniform1DKernel = cl.createKernel(program, "testUniform1D");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(testUniform1DKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL) );
  workGroupSize = workGroupSize > maxWorkGroupSize ? maxWorkGroupSize : workGroupSize;

  const int bucketNum = 16;
  int* buckets = new int[bucketNum * workGroupSize];
  cl_mem bucketsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(int) * workGroupSize * bucketNum, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(testUniform1DKernel, 0, sizeof(int), &randomNums) );
  CL_SAFE_CALL( clSetKernelArg(testUniform1DKernel, 1, sizeof(cl_mem), &randomsGPU) );
  CL_SAFE_CALL( clSetKernelArg(testUniform1DKernel, 2, sizeof(int), &bucketNum) );
  CL_SAFE_CALL( clSetKernelArg(testUniform1DKernel, 3, sizeof(cl_mem), &bucketsGPU) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), testUniform1DKernel,
				       1, NULL, &workGroupSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), bucketsGPU, CL_TRUE, 0, sizeof(int) * workGroupSize * bucketNum,
				    buckets, 0, NULL, NULL) );

  for(int i = 0; i < bucketNum; ++i){
    float e = 0;
    float e2 = 0;
    for(int j = 0; j < workGroupSize; ++j){
      e += buckets[j + i * workGroupSize];
      e2 += buckets[j + i * workGroupSize] * buckets[j + i * workGroupSize];
    }
    e = e / workGroupSize;
    e2 = e2 / workGroupSize;
    std::cout << i << " e: " << e << " d: " << sqrt(e2 - (e*e)) << std::endl;
  }
  std::cout << std::endl;

  clReleaseKernel(testUniform1DKernel);
  delete buckets;
}

// #define WRITE_OUT_RANDOMS
const int randomNumbers = 1024;

void simpleLCG(){
  cl_kernel randomLCGKernel = cl.createKernel(program, "randomLCG");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(randomLCGKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL) );

  float* seedCPU = new float[workGroupSize];
  for(int i = 0; i < workGroupSize; ++i){
    seedCPU[i] = rand();
  }
  cl_mem seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * workGroupSize, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
				     CL_FALSE, 0, sizeof(float) * workGroupSize,
				     seedCPU, 0, NULL, NULL) );

  float* randomsCPU = new float[workGroupSize * randomNumbers];
  cl_mem randomsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize * randomNumbers, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(randomLCGKernel, 0, sizeof(int), &randomNumbers) );
  CL_SAFE_CALL( clSetKernelArg(randomLCGKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(randomLCGKernel, 2, sizeof(cl_mem), &randomsGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), randomLCGKernel,
				       1, NULL, &workGroupSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), randomsGPU, CL_TRUE, 0, sizeof(float) * workGroupSize * randomNumbers,
				    randomsCPU, 0, NULL, NULL) );

  testUniform1DArray(workGroupSize, randomNumbers, randomsGPU);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("simpleLCG.txt", "w");
  for(int i = 0; i < workGroupSize; ++i){
    for(int j = 0; j < randomNumbers; ++j){
      fprintf(outFile, "%d %f\n", j, randomsCPU[i + j * workGroupSize]);
    }
    fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(randomLCGKernel);
  delete randomsCPU;
}

void simpleLFG(){
  cl_kernel randomLFGKernel = cl.createKernel(program, "randomLFG");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(randomLFGKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL) );

  float* seedCPU = new float[workGroupSize];
  for(int i = 0; i < workGroupSize; ++i){
    seedCPU[i] = rand();
  }
  cl_mem seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * workGroupSize, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
				     CL_FALSE, 0, sizeof(float) * workGroupSize,
				     seedCPU, 0, NULL, NULL) );

  const int randomsStateSize = 1000;
  cl_mem randomStateGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_uint) * workGroupSize * randomsStateSize, NULL, NULL);

  float* randomsCPU = new float[workGroupSize * randomNumbers];
  cl_mem randomsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize * randomNumbers, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(randomLFGKernel, 0, sizeof(int), &randomNumbers) );
  CL_SAFE_CALL( clSetKernelArg(randomLFGKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(randomLFGKernel, 2, sizeof(int), &randomsStateSize) );
  CL_SAFE_CALL( clSetKernelArg(randomLFGKernel, 3, sizeof(cl_mem), &randomStateGPU) );
  CL_SAFE_CALL( clSetKernelArg(randomLFGKernel, 4, sizeof(cl_mem), &randomsGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), randomLFGKernel,
				       1, NULL, &workGroupSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), randomsGPU, CL_TRUE, 0, sizeof(float) * workGroupSize * randomNumbers,
				    randomsCPU, 0, NULL, NULL) );

  testUniform1DArray(workGroupSize, randomNumbers, randomsGPU);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("simpleLFG.txt", "w");
  for(int i = 0; i < workGroupSize; ++i){
    for(int j = 0; j < randomNumbers; ++j){
      fprintf(outFile, "%d %f\n", j, randomsCPU[i + j * workGroupSize]);
    }
    fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(randomLFGKernel);
  delete randomsCPU;
}

void simpleCTG(){
  cl_kernel randomCTGKernel = cl.createKernel(program, "randomCTG");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(randomCTGKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL) );

  cl_float2* seedCPU = new cl_float2[workGroupSize];
  for(int i = 0; i < workGroupSize; ++i){
    seedCPU[i].s[0] = rand();
    seedCPU[i].s[1] = rand();
  }
  cl_mem seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(cl_float2) * workGroupSize, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
				     CL_FALSE, 0, sizeof(cl_float2) * workGroupSize,
				     seedCPU, 0, NULL, NULL) );

  float* randomsCPU = new float[workGroupSize * randomNumbers];
  cl_mem randomsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize * randomNumbers, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(randomCTGKernel, 0, sizeof(int), &randomNumbers) );
  CL_SAFE_CALL( clSetKernelArg(randomCTGKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(randomCTGKernel, 2, sizeof(cl_mem), &randomsGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), randomCTGKernel,
				       1, NULL, &workGroupSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), randomsGPU, CL_TRUE, 0, sizeof(float) * workGroupSize * randomNumbers,
				    randomsCPU, 0, NULL, NULL) );

  testUniform1DArray(workGroupSize, randomNumbers, randomsGPU);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("simpleCTG.txt", "w");
  for(int i = 0; i < workGroupSize; ++i){
    for(int j = 0; j < randomNumbers; ++j){
      fprintf(outFile, "%d %f\n", j, randomsCPU[i + j * workGroupSize]);
    }
    fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(randomCTGKernel);
  delete randomsCPU;
}

void hybridRNG(){
  cl_kernel hybridRNGKernel = cl.createKernel(program, "hybridRNG");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(hybridRNGKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL) );

  float* seedCPU = new float[workGroupSize * 4];
  for(int i = 0; i < workGroupSize * 4; ++i){
    seedCPU[i] = rand();
  }
  cl_mem seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * workGroupSize * 4, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
				     CL_FALSE, 0, sizeof(float) * workGroupSize * 4,
				     seedCPU, 0, NULL, NULL) );

  float* randomsCPU = new float[workGroupSize * randomNumbers];
  cl_mem randomsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize * randomNumbers, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(hybridRNGKernel, 0, sizeof(int), &randomNumbers) );
  CL_SAFE_CALL( clSetKernelArg(hybridRNGKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(hybridRNGKernel, 2, sizeof(cl_mem), &randomsGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), hybridRNGKernel,
				       1, NULL, &workGroupSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), randomsGPU, CL_TRUE, 0, sizeof(float) * workGroupSize * randomNumbers,
				    randomsCPU, 0, NULL, NULL) );

  testUniform1DArray(workGroupSize, randomNumbers, randomsGPU);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("hybridRNG.txt", "w");
  for(int i = 0; i < workGroupSize; ++i){
    for(int j = 0; j < randomNumbers; ++j){
      fprintf(outFile, "%d %f\n", j, randomsCPU[i + j * workGroupSize]);
    }
    fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(hybridRNGKernel);
  delete randomsCPU;
}

// Mersene Twister
#define      DCMT_SEED 4172

typedef struct{
  unsigned int matrix_a;
  unsigned int mask_b;
  unsigned int mask_c;
  unsigned int seed;
} mt_struct_stripped;

#define   MT_RNG_COUNT 4096
#define          MT_MM 9
#define          MT_NN 19
#define       MT_WMASK 0xFFFFFFFFU
#define       MT_UMASK 0xFFFFFFFEU
#define       MT_LMASK 0x1U
#define      MT_SHIFT0 12
#define      MT_SHIFTB 7
#define      MT_SHIFTC 15
#define      MT_SHIFT1 18
#define PI 3.14159265358979f

void loadMTGPU(const char *fname,
               const unsigned int seed,
               mt_struct_stripped *h_MT,
               const size_t size){
  FILE* fd = 0;
#ifdef _WIN32
  // open the file for binary read
  errno_t err;
  if ((err = fopen_s(&fd, fname, "rb")) != 0)
#else
    // open the file for binary read
    if ((fd = fopen(fname, "rb")) == 0)
#endif
      {
	if(fd)
	  {
	    fclose (fd);
	  }
      }

  for (unsigned int i = 0; i < size; i++)
    fread(&h_MT[i], sizeof(mt_struct_stripped), 1, fd);
  fclose(fd);

  for(unsigned int i = 0; i < size; i++)
    h_MT[i].seed = seed;
}

cl_program mtProgram;
cl_kernel mtKernel;
void mersenneTwister(){
  mtProgram = cl.createProgram("kernels/mersenneTwister.cl");
  mtKernel = cl.createKernel(mtProgram, "MersenneTwister");

  const int seed = 777;
  const int nPerRng = randomNumbers; // 5860
  const int nRand = MT_RNG_COUNT * nPerRng;

  mt_struct_stripped *h_MT = (mt_struct_stripped*)malloc(sizeof(mt_struct_stripped)*MT_RNG_COUNT);
  const char* mtDataPath = "kernels/mersenneTwister.dat";
  loadMTGPU(mtDataPath, seed, h_MT, MT_RNG_COUNT);

  cl_mem d_MT = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(mt_struct_stripped) * MT_RNG_COUNT, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), d_MT, CL_TRUE, 0, sizeof(mt_struct_stripped) * MT_RNG_COUNT, h_MT, 0, NULL, NULL) );

  cl_mem d_Rand = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float) * nRand, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(mtKernel, 0, sizeof(cl_mem), &d_Rand) );
  CL_SAFE_CALL( clSetKernelArg(mtKernel, 1, sizeof(cl_mem), &d_MT) );
  CL_SAFE_CALL( clSetKernelArg(mtKernel, 2, sizeof(int), &nPerRng) );

  size_t globalWorkSize[1] = {MT_RNG_COUNT};
  size_t localWorksSize[1] = {128};
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), mtKernel,
				       1, NULL, globalWorkSize, localWorksSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  float* h_RandGPU = new float[nRand];
  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), d_Rand, CL_TRUE, 0,
				    sizeof(cl_float) * nRand, h_RandGPU, 0, NULL, NULL) );

  testUniform1DArray(MT_RNG_COUNT, nPerRng, d_Rand);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("mersenneTwisterRNG.txt", "w");
  for(int i = 0; i < MT_RNG_COUNT; ++i){
   for(int j = 0; j < nPerRng; ++j){
    fprintf(outFile, "%d %f\n", j, h_RandGPU[i + j * MT_RNG_COUNT]);
   }
   fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(mtKernel);
}

void haltonSequence(){
  cl_kernel haltonKernel = cl.createKernel(program, "haltonSequence");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(haltonKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(workGroupSize), &workGroupSize, NULL) );

  float* randomsCPU = new float[workGroupSize * randomNumbers];
  cl_mem randomsGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize * randomNumbers, NULL, NULL);

  const int base = 2;

  CL_SAFE_CALL( clSetKernelArg(haltonKernel, 0, sizeof(int), &randomNumbers) );
  CL_SAFE_CALL( clSetKernelArg(haltonKernel, 1, sizeof(int), &base) );
  CL_SAFE_CALL( clSetKernelArg(haltonKernel, 2, sizeof(cl_mem), &randomsGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), haltonKernel,
    1, NULL, &workGroupSize, NULL,
    0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), randomsGPU, CL_TRUE, 0, sizeof(float) * workGroupSize * randomNumbers,
    randomsCPU, 0, NULL, NULL) );

  testUniform1DArray(workGroupSize, randomNumbers, randomsGPU);

#ifdef WRITE_OUT_RANDOMS
  FILE* outFile = fopen("haltonSequence.txt", "w");
  for(int i = 0; i < workGroupSize; ++i){
   for(int j = 0; j < randomNumbers; ++j){
    fprintf(outFile, "%d %f\n", j, randomsCPU[i + j * workGroupSize]);
   }
   fprintf(outFile, "\n");
  }
  fclose(outFile);
#endif

  clReleaseKernel(haltonKernel);
  delete randomsCPU;
}

void mcInt1D(){
  cl_kernel mcInt1DKernel = cl.createKernel(program, "mcInt1D");

  size_t workGroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(mcInt1DKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
    sizeof(workGroupSize), &workGroupSize, NULL) );

  cl_float4* seedCPU = new cl_float4[workGroupSize];
  for(int i = 0; i < workGroupSize; ++i){
   seedCPU[i].s[0] = rand();
   seedCPU[i].s[1] = rand();
   seedCPU[i].s[2] = rand();
   seedCPU[i].s[3] = rand();
  }
  cl_mem seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(cl_float4) * workGroupSize, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
    CL_FALSE, 0, sizeof(cl_float4) * workGroupSize,
    seedCPU, 0, NULL, NULL) );

  float* partialIntCPU = new float[workGroupSize];
  cl_mem partialIntGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * workGroupSize, NULL, NULL);

  const int sampleNumber = 1000;

  CL_SAFE_CALL( clSetKernelArg(mcInt1DKernel, 0, sizeof(int), &sampleNumber) );
  CL_SAFE_CALL( clSetKernelArg(mcInt1DKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(mcInt1DKernel, 2, sizeof(cl_mem), &partialIntGPU) );

  clEnqueueBarrier(cl.cqueue());

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), mcInt1DKernel,
    1, NULL, &workGroupSize, NULL,
    0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), partialIntGPU, CL_TRUE, 0, sizeof(float) * workGroupSize,
    partialIntCPU, 0, NULL, NULL) );

  float integral = 0.0f;
  for(int i = 0; i < workGroupSize; ++i){
   integral += partialIntCPU[i];
  }
  integral = integral / workGroupSize;

  std::cout << "Estimated integral: " << integral << std::endl;

  clReleaseKernel(mcInt1DKernel);
  delete partialIntCPU;
}

int main(int arc, char* argv[]){
  // OpenCL init
  program = cl.createProgram("kernels/programs.cl");

  // OpenCL processing
  simpleLCG();
  simpleLFG();
  simpleCTG();
  hybridRNG();
  mersenneTwister();
  haltonSequence();
  mcInt1D();

  // OpenCL cleanup
  clReleaseProgram(program);
  return 0;
}
