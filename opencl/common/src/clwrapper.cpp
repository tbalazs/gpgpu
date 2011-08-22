#include "clwrapper.hpp"

ClWrapper::ClWrapper(cl_device_type device_type) : _device_type(device_type)
{
  createPlatform();
  createDevice();
  createContext();
  createCommandQueue();

  printOpenCLInfo();
}

ClWrapper::~ClWrapper()
{
  clReleaseCommandQueue(_cqueue);
  clReleaseContext(_context);
}

char* ClWrapper::getPlatformInfo(cl_platform_info paramName)
{
  size_t infoSize = 0;
  CL_SAFE_CALL( clGetPlatformInfo(_platform, paramName, 0, NULL, &infoSize) );
  char* info = (char*)malloc(infoSize);
  CL_SAFE_CALL( clGetPlatformInfo(_platform, paramName, infoSize, info, NULL) );
  return info;
}

void ClWrapper::createPlatform()
{
  CL_SAFE_CALL(clGetPlatformIDs(1, &_platform, NULL));
}

void* ClWrapper::getDeviceInfo(cl_device_info paramName)
{
  size_t infoSize = 0;
  CL_SAFE_CALL( clGetDeviceInfo(_device_id, paramName, 0, NULL, &infoSize) );
  char* info = (char*)malloc(infoSize);
  CL_SAFE_CALL( clGetDeviceInfo(_device_id, paramName, infoSize, info, NULL) );
  return info;
}

void ClWrapper::createDevice()
{
  CL_SAFE_CALL( clGetDeviceIDs(_platform, _device_type, 1, &_device_id, NULL) );
}

void ClWrapper::createContext()
{
  _context = clCreateContext(0, 1, &_device_id, NULL, NULL, NULL);
  if(!_context)
    {
      std::cerr << "Context creation failed!" << std::endl;
      exit(EXIT_FAILURE);
    }
}

void ClWrapper::createCommandQueue()
{
  _cqueue = clCreateCommandQueue(_context, _device_id, 0, NULL);
  if(!_cqueue)
    {
      std::cerr << "Command queue creation failed!" << std::endl;
    }
}

void ClWrapper::printOpenCLInfo()
{
  std::cout << getPlatformInfo(CL_PLATFORM_VERSION) << std::endl;

  cl_uint* max_compute_units = (cl_uint*)getDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS);
  std::cout << "Max compute units: " << *max_compute_units << std::endl;
}

bool ClWrapper::fileToString(const char* path, char*& out, int& len) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if(!file.is_open())
    {
      return false;
    }
  len = file.tellg();
  out = new char[ len+1 ];
  file.seekg (0, std::ios::beg);
  file.read(out, len);
  file.close();
  out[len] = 0;
  return true;
}

cl_program ClWrapper::createProgram(const char* fileName){
  char* programSource = NULL;
  int len = 0;
  int errorFlag = -1;
  if(!fileToString(fileName, programSource, len)){
    std::cerr << "Error loading program: " << fileName << std::endl;
    exit(EXIT_FAILURE);
  }
  cl_program program = 0;
  program = clCreateProgramWithSource(_context, 1, (const char**)&programSource, NULL, NULL);
  if (!program) {
    std::cerr << "Error: Failed to create compute program!" << std::endl;
    exit(EXIT_FAILURE);
  }

  cl_int err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS) {
    size_t len;
    char buffer[2048];

    std::cerr << "Error: Failed to build program executable!" << std::endl;
    clGetProgramBuildInfo(program, _device_id, CL_PROGRAM_BUILD_LOG,
			  sizeof(buffer), buffer, &len);
    std::cerr << buffer << std::endl;
    exit(1);
  }
  return program;
}

cl_kernel ClWrapper::createKernel(cl_program program, const char* kernelName){
  cl_kernel kernel;
  cl_int err;
  kernel = clCreateKernel(program, kernelName, &err);
  if (!kernel || err != CL_SUCCESS) {
    std::cerr << "Error: Failed to create compute kernel!" << std::endl;
    exit(1);
  }
  return kernel;
}

