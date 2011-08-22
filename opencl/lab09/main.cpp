#include <iostream>
#include <fstream>

#include <CL/opencl.h>

#include "clwrapper.hpp"

ClWrapper cl;

// OpenCL program
cl_program program;

void scalarMV(int n, int m, float* y, const float* A, const float* x, const float* b){
  for(int i=0; i<n; ++i){
    float yi = b[i];
    for(int j=0; j<m; ++j){
      yi += A[i * m + j] * x[j];
    }
    y[i] = yi;
  }
}

#define M 32
void reduceMV(int n, float* y, const float* A, const float* x, const float* b){
  cl_kernel reduceMVKernel = cl.createKernel(program, "reduceMV");

  cl_mem yGPU = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * M, NULL, NULL);
  cl_mem AGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * M * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), AGPU, CL_FALSE, 0, sizeof(float) * M * n, A, 0, NULL, NULL) );
  cl_mem xGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), xGPU, CL_FALSE, 0, sizeof(float) * n, x, 0, NULL, NULL) );
  cl_mem bGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * M, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), bGPU, CL_FALSE, 0, sizeof(float) * M, b, 0, NULL, NULL) );

  CL_SAFE_CALL( clSetKernelArg(reduceMVKernel, 0, sizeof(int), &n) );
  CL_SAFE_CALL( clSetKernelArg(reduceMVKernel, 1, sizeof(cl_mem), &yGPU) );
  CL_SAFE_CALL( clSetKernelArg(reduceMVKernel, 2, sizeof(cl_mem), &AGPU) );
  CL_SAFE_CALL( clSetKernelArg(reduceMVKernel, 3, sizeof(cl_mem), &xGPU) );
  CL_SAFE_CALL( clSetKernelArg(reduceMVKernel, 4, sizeof(cl_mem), &bGPU) );

  clEnqueueBarrier(cl.cqueue());

  size_t workSize = M * n;
  size_t workGroupSize = M;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), reduceMVKernel,
				       1, NULL, &workSize, &workGroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), yGPU, CL_TRUE, 0, sizeof(float) * M,
				    y, 0, NULL, NULL) );

  clReleaseMemObject(yGPU);
  clReleaseMemObject(AGPU);
  clReleaseMemObject(xGPU);
  clReleaseMemObject(bGPU);
  clReleaseKernel(reduceMVKernel);
}

#define T 8
#define Z 2
void largeMV(int n, int m, float* y, const float* A, const float* x, const float* b){
  cl_kernel largeMVKernel = cl.createKernel(program, "largeMV");

  cl_mem yGPU = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * m, NULL, NULL);
  cl_mem AGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * m * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), AGPU, CL_FALSE, 0, sizeof(float) * m * n, A, 0, NULL, NULL) );
  cl_mem xGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), xGPU, CL_FALSE, 0, sizeof(float) * n, x, 0, NULL, NULL) );
  cl_mem bGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * m, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), bGPU, CL_FALSE, 0, sizeof(float) * m, b, 0, NULL, NULL) );

  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 0, sizeof(int), &n) );
  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 1, sizeof(int), &m) );
  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 2, sizeof(cl_mem), &yGPU) );
  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 3, sizeof(cl_mem), &AGPU) );
  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 4, sizeof(cl_mem), &xGPU) );
  CL_SAFE_CALL( clSetKernelArg(largeMVKernel, 5, sizeof(cl_mem), &bGPU) );

  clEnqueueBarrier(cl.cqueue());

  size_t workSize = T * Z;
  size_t workGroupSize = T * Z;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), largeMVKernel,
				       1, NULL, &workSize, &workGroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), yGPU, CL_TRUE, 0, sizeof(float) * m,
				    y, 0, NULL, NULL) );

  clReleaseMemObject(yGPU);
  clReleaseMemObject(AGPU);
  clReleaseMemObject(xGPU);
  clReleaseMemObject(bGPU);
  clReleaseKernel(largeMVKernel);
}

void simpleMV(int n, int m, float* y, const float* A, const float* x, const float* b){
  cl_kernel simpleMVKernel = cl.createKernel(program, "simpleMV");

  cl_mem yGPU = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * m, NULL, NULL);
  cl_mem AGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * m * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), AGPU, CL_FALSE, 0, sizeof(float) * m * n, A, 0, NULL, NULL) );
  cl_mem xGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * n, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), xGPU, CL_FALSE, 0, sizeof(float) * n, x, 0, NULL, NULL) );
  cl_mem bGPU = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * m, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), bGPU, CL_FALSE, 0, sizeof(float) * m, b, 0, NULL, NULL) );

  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 0, sizeof(int), &n) );
  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 1, sizeof(int), &m) );
  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 2, sizeof(cl_mem), &yGPU) );
  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 3, sizeof(cl_mem), &AGPU) );
  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 4, sizeof(cl_mem), &xGPU) );
  CL_SAFE_CALL( clSetKernelArg(simpleMVKernel, 5, sizeof(cl_mem), &bGPU) );

  clEnqueueBarrier(cl.cqueue());

  size_t workSize = m;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), simpleMVKernel,
				       1, NULL, &workSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), yGPU, CL_TRUE, 0, sizeof(float) * m,
				    y, 0, NULL, NULL) );

  clReleaseMemObject(yGPU);
  clReleaseMemObject(AGPU);
  clReleaseMemObject(xGPU);
  clReleaseMemObject(bGPU);
  clReleaseKernel(simpleMVKernel);
}

void printResult(int m, const float* y, const char* type){
  std::cout << type << " y: [";
  for(int i = 0; i < m; ++i){
    std::cout << y[i];
    if(i < m - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
}

void MVmul(){
  int n = 32;
  int m = 32;

  float* yCPU = new float[m];
  float* ySimpleMV = new float[m];
  float* yReduceMV = new float[m];
  float* yLargeMV = new float[m];
  float* x = new float[n];
  float* A = new float[n * m];
  float* b = new float[m];

  for(int i = 0; i < m; ++i){
    yCPU[i] = 0.0f;
    ySimpleMV[i] = 0.0f;
    yReduceMV[i] = 0.0f;
    yLargeMV[i] = 0.0f;
  }

  for(int i = 0; i < n; ++i){
    x[i] = (float)i;
  }

  for(int i = 0; i < n; ++i){
    for(int j = 0; j < m; ++j){
      float v = 0.0f;
      if( i == j){
	v = 1.0f;
      }
      A[i + j * n] = v;
    }
  }

  for(int i = 0; i < m; ++i){
    b[i] = 1.0f;
  }

  scalarMV(n, m, yCPU, A, x, b);
  printResult(m, yCPU, "CPU");

  simpleMV(n, m, ySimpleMV, A, x, b);
  printResult(m, ySimpleMV, "SimpleMV");

  reduceMV(n, yReduceMV, A, x, b);
  printResult(m, yReduceMV, "ReduceMV");

  largeMV(n, m, yLargeMV, A, x, b);
  printResult(m, yLargeMV, "LargeMV");

  delete yCPU;
  delete ySimpleMV;
  delete yReduceMV;
  delete yLargeMV;
  delete x;
  delete A;
  delete b;
}

void jakobi(){
  int n = 8;

  float* x[2] = {NULL, NULL};
  x[0] = new float[n];
  x[1] = new float[n];
  for(int i = 0; i < n; ++i){
    x[0][i] = 0.0f;
    x[1][i] = 0.0f;
  }

  float* A = new float[n * n];
  for(int i = 0; i < n; ++i){
    for(int j = 0; j < n; ++j){
      float v = 0.0f;
      if( i == j){
	v = 0.5f;
      }
      A[i + j * n] = v;
    }
  }

  float* b = new float[n];
  for(int i = 0; i < n; ++i){
    b[i] = 1.0f;
  }

  int inputBuffer = 0;
  const int iterations = 20;
  for(int i = 0; i < iterations; ++i){
    largeMV(n, n, x[(inputBuffer + 1) % 2], A, x[inputBuffer], b);
    inputBuffer = (inputBuffer + 1) % 2;
    printResult(n, x[inputBuffer], "Jakobi");
  }

  delete x[0];
  delete x[1];
  delete A;
  delete b;
}

void gaussian(){
  int n = 4;
  int m = 3;
  float A[] = {2, 1, -1, 8,
  	       -3, -1, 2, -11,
  	       -2, 1, 2, -3};

  // int n = 6;
  // int m = 3;
  // float A[] = {  2, -1,  0,  1, 0, 0,
  // 		-1,  2, -1,  0, 1, 0,
  // 		 0, -1,  2,  0, 0, 1};


  cl_kernel gaussianKernel = cl.createKernel(program, "gaussian");

  cl_mem AGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) *  n * m, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), AGPU, CL_TRUE, 0, sizeof(float) * n * m, A, 0, NULL, NULL) );

  CL_SAFE_CALL( clSetKernelArg(gaussianKernel, 0, sizeof(int), &n) );
  CL_SAFE_CALL( clSetKernelArg(gaussianKernel, 1, sizeof(int), &m) );
  CL_SAFE_CALL( clSetKernelArg(gaussianKernel, 2, sizeof(cl_mem), &AGPU) );

  clEnqueueBarrier(cl.cqueue());

  size_t workSize = m;
  size_t workGroupSize = m;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), gaussianKernel,
				       1, NULL, &workSize, &workGroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), AGPU, CL_TRUE, 0, sizeof(float) * n * m,
				    A, 0, NULL, NULL) );

  for(int i = 0; i < m; ++i){
    for(int j = 0; j < n; ++j){
      std::cout << A[ j + i * n];
      if(j < n - 1) std::cout << ", ";
    }
    std::cout << std::endl;
  }

  clReleaseMemObject(AGPU);
  clReleaseKernel(gaussianKernel);
}

int main(int arc, char* argv[]){
  // OpenCL init
  program = cl.createProgram("kernels/programs.cl");

  // OpenCL processing
  MVmul();
  jakobi();
  gaussian();

  // OpenCL cleanup
  clReleaseProgram(program);
  return 0;
}
