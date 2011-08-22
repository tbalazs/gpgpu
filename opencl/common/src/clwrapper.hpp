#ifndef _CL_WRAPPER_
#define _CL_WRAPPER_

#include <iostream>
#include <fstream>

#include <CL/opencl.h>
#include <CL/cl_platform.h>

#define CL_SAFE_CALL( call ) {						\
    cl_int err = call;							\
    if(CL_SUCCESS != err){						\
      std::cout << "Unsuccesful OpenCL call " << __FILE__ << " : " << __LINE__ << std::endl; \
      exit(EXIT_FAILURE);						\
    } }

class ClWrapper {
public:
  ClWrapper(cl_device_type _device_type = CL_DEVICE_TYPE_GPU);
  ~ClWrapper();

  cl_device_id device_id() { return _device_id; }
  cl_context context() { return _context; }
  cl_command_queue cqueue() { return _cqueue; }

  char* getPlatformInfo(cl_platform_info paramName);
  void* getDeviceInfo(cl_device_info paramName);

  cl_program createProgram(const char* fileName);
  cl_kernel createKernel(cl_program program, const char* kernelName);

  void printOpenCLInfo();

private:
  cl_device_type _device_type;
  cl_platform_id _platform;
  cl_device_id _device_id;
  cl_context _context;
  cl_command_queue _cqueue;

  void createPlatform();
  void createDevice();
  void createContext();
  void createCommandQueue();

  bool fileToString(const char* path, char*& out, int& len);
};

#endif
