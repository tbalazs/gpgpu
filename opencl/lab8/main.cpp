#include <iostream>
#include <fstream>

#include <CL/opencl.h>
#include <CL/cl_platform.h>

#include "clwrapper.hpp"

ClWrapper cl;

// OpenCL program
cl_program program;

// simple global address
void globalAddress(){
  cl_kernel globalAddressKernel = cl.createKernel(program, "globalAddress");

  const int data_size = 1024;
  float* data = (float*)malloc(sizeof(float)*data_size);
  cl_mem clData = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * data_size, NULL, NULL);
  CL_SAFE_CALL( clSetKernelArg(globalAddressKernel, 0, sizeof(cl_mem), &clData) );

  size_t workgroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(globalAddressKernel, cl.device_id(),
					 CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workgroupSize), &workgroupSize, NULL) );
  size_t workSize = data_size;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), globalAddressKernel,
				       1, NULL, &workSize, &workgroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), clData, CL_TRUE, 0, sizeof(float) * data_size,
				    data, 0, NULL, NULL) );

  FILE* outFile = fopen("globalAddress.txt", "w");
  for(int i = 0; i < data_size; ++i){
    fprintf(outFile, "%f\n", data[i]);
  }
  fclose(outFile);

  clReleaseKernel(globalAddressKernel);
  free(data);
}

// local address
void localAddress(){
  cl_kernel localAddressKernel = cl.createKernel(program, "localAddress");

  const int data_size = 1024;
  float* data = (float*)malloc(sizeof(float)*data_size);
  cl_mem clData = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * data_size, NULL, NULL);
  CL_SAFE_CALL( clSetKernelArg(localAddressKernel, 0, sizeof(cl_mem), &clData) );

  size_t workgroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(localAddressKernel, cl.device_id(),
					 CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workgroupSize), &workgroupSize, NULL) );
  workgroupSize = workgroupSize / 4;

  size_t workSize = data_size;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), localAddressKernel,
				       1, NULL, &workSize, &workgroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), clData, CL_TRUE, 0, sizeof(float) * data_size,
				    data, 0, NULL, NULL) );

  FILE* outFile = fopen("localAddress.txt", "w");
  for(int i = 0; i < data_size; ++i){
    fprintf(outFile, "%f\n", data[i]);
  }
  fclose(outFile);

  clReleaseKernel(localAddressKernel);
  free(data);
}

// 2D address
void address2D(){
  cl_kernel address2DKernel = cl.createKernel(program, "address2D");

  const int data_size[2] = {1024, 1024};
  cl_float4* data = (cl_float4*)malloc(sizeof(cl_float4)*data_size[0] * data_size[1]);
  cl_mem clData = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(cl_float4) * data_size[0] * data_size[1], NULL, NULL);
  CL_SAFE_CALL( clSetKernelArg(address2DKernel, 0, sizeof(cl_mem), &clData) );

  size_t workgroupSize[2] = {8, 8};
  size_t workSize[2] = { data_size[0], data_size[1] };
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), address2DKernel,
				       2, NULL, workSize, workgroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), clData, CL_TRUE, 0, sizeof(cl_float4) * data_size[0] * data_size[1],
				    data, 0, NULL, NULL) );

  FILE* outFile = fopen("2DAddress.txt", "w");
  for(int i = 0; i < data_size[0] * data_size[1]; ++i){
    fprintf(outFile, "G: [%f, %f]  L: [%f, %f]\n", data[i].s[0], data[i].s[1], data[i].s[2], data[i].s[3]);
  }
  fclose(outFile);

  clReleaseKernel(address2DKernel);
  free(data);
}

// square
void square(){
  cl_kernel squareKernel = cl.createKernel(program, "square");

  const int data_size = 1024;

  float* inputData = (float*)malloc(sizeof(float) * data_size);
  for(int i = 0; i < data_size; ++i){
    inputData[i] = i;
  }
  cl_mem clInputData = clCreateBuffer(cl.context(), CL_MEM_READ_ONLY, sizeof(float) * data_size, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), clInputData,
				     CL_TRUE, 0, sizeof(float) * data_size,
				     inputData, 0, NULL, NULL) );

  float* data = (float*)malloc(sizeof(float)*data_size);
  cl_mem clData = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(float) * data_size, NULL, NULL);

  CL_SAFE_CALL( clSetKernelArg(squareKernel, 0, sizeof(cl_mem), &clInputData) );
  CL_SAFE_CALL( clSetKernelArg(squareKernel, 1, sizeof(cl_mem), &clData) );
  CL_SAFE_CALL( clSetKernelArg(squareKernel, 2, sizeof(int), &data_size) );

  size_t workgroupSize = 0;
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(squareKernel, cl.device_id(),
					 CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workgroupSize), &workgroupSize, NULL) );
  size_t workSize = data_size;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), squareKernel,
				       1, NULL, &workSize, &workgroupSize,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), clData, CL_TRUE, 0, sizeof(float) * data_size,
				    data, 0, NULL, NULL) );

  int wrong = 0;
  for(int i = 0; i < data_size; ++i){
    if(data[i] != inputData[i] * inputData[i]){
      wrong++;
    }
  }
  std::cout << "Wrong squares: " << wrong << std::endl;

  clReleaseKernel(squareKernel);
  free(data);
  free(inputData);
}

// 2D function
void function2D(){
  cl_kernel function2DKernel = cl.createKernel(program, "function2D");

  const int data_size[2] = {1024, 1024};
  cl_float4* data = (cl_float4*)malloc(sizeof(cl_float4) * data_size[0] * data_size[1]);
  cl_mem clData = clCreateBuffer(cl.context(), CL_MEM_WRITE_ONLY, sizeof(cl_float4) * data_size[0] * data_size[1], NULL, NULL);
  CL_SAFE_CALL( clSetKernelArg(function2DKernel, 0, sizeof(cl_mem), &clData) );

  size_t workSize[2] = { data_size[0], data_size[1] };
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), function2DKernel,
				       2, NULL, workSize, NULL,
				       0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), clData, CL_TRUE, 0, sizeof(cl_float4) * data_size[0] * data_size[1],
				    data, 0, NULL, NULL) );

  FILE* outFile = fopen("function2D.txt", "w");
  for(int i = 0; i < data_size[0] * data_size[1]; ++i){
    fprintf(outFile, "%f %f %f\n", data[i].s[0], data[i].s[1], data[i].s[2]);
  }
  fclose(outFile);

  clReleaseKernel(function2DKernel);
  free(data);
}

int main(int arc, char* argv[]){
  program = cl.createProgram("kernels/programs.cl");

  // OpenCL processing
  globalAddress();
  localAddress();
  address2D();
  square();
  function2D();

  // OpenCL cleanup
  clReleaseProgram(program);
  return 0;
}
