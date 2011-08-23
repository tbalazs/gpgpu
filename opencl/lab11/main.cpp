/*
 *
 * Copyright © 2010-2011 Balázs Tóth <tbalazs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <CL/opencl.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "camera.hpp"
#include "matrix4x4.hpp"
#include "clwrapper.hpp"

ClWrapper cl;

// *****************
// OpenCL processing
// *****************

/* temporary functions */
void saveSimulation(unsigned int photonNum, int resolution, cl_mem simulationBufferGPU){
  float* simulationBufferCPU = new float[resolution * resolution * resolution];
  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), simulationBufferGPU, CL_TRUE, 0, sizeof(float) * resolution * resolution * resolution,
				    simulationBufferCPU, 0, NULL, NULL));

  // for(int i = 0; i < resolution * resolution * resolution; ++i){
  //   simulationBufferCPU[i] = simulationBufferCPU[i] / photonNum;
  // }

  FILE* f = fopen("simulation.vox", "wb");
  char magic[3] = "VF";
  fwrite(magic, sizeof(char), 2, f);
  fwrite(&resolution, sizeof(unsigned int), 1, f);
  fwrite(&resolution, sizeof(unsigned int), 1, f);
  fwrite(&resolution, sizeof(unsigned int), 1, f);
  fwrite(simulationBufferCPU, sizeof(float), resolution * resolution * resolution, f);
}

void saveTGA(int pictureNumber, int width, int height, cl_float4* buffer){
  char* fname = (char*)malloc(sizeof(char)*32);
  sprintf(fname, "out_%03d.tga", pictureNumber);
  FILE *f = fopen(fname, "wb");
  if(!f) {
    fprintf(stderr, "Unable to create output targa image `%s'\n", "out.tga");
    exit(EXIT_FAILURE);
  }

  fputc(0x00, f); /* ID Length, 0 => No ID        */
  fputc(0x00, f); /* Color Map Type, 0 => No color map included   */
  fputc(0x02, f); /* Image Type, 2 => Uncompressed, True-color Image */
  fputc(0x00, f); /* Next five bytes are about the color map entries */
  fputc(0x00, f); /* 2 bytes Index, 2 bytes length, 1 byte size */
  fputc(0x00, f);
  fputc(0x00, f);
  fputc(0x00, f);
  fputc(0x00, f); /* X-origin of Image    */
  fputc(0x00, f);
  fputc(0x00, f); /* Y-origin of Image    */
  fputc(0x00, f);
  fputc(width & 0xff, f); /* Image Width      */
  fputc((width >> 8) & 0xff, f);
  fputc(height & 0xff, f); /* Image Height     */
  fputc((height >> 8) & 0xff, f);
  fputc(0x18, f); /* Pixel Depth, 0x18 => 24 Bits */
  fputc(0x20, f); /* Image Descriptor     */

  for(int y = 0; y < height; ++y) {
    for(size_t x = 0 ; x < width; ++x) {
      fputc(buffer[x + y * width].s[2] * 255, f); /* write blue */
      fputc(buffer[x + y * width].s[1] * 255, f); /* write green */
      fputc(buffer[x + y * width].s[0] * 255, f); /* write red */
    }
  }
  fclose(f);
  free(fname);
}

/* temporary functions */

struct photon {
  cl_float4 origin;
  cl_float4 direction;
  cl_float energy;
};

// OpenCL program
cl_program photonProgram;

// OpenCL kernels
cl_kernel simulationKernel;
cl_kernel visualizationKernel;

// Problem set size
size_t workGroupSize = 0;
int maxComputeUnits = 0;
size_t problemSize = 0;

// Random Generator Seed
cl_mem seedGPU;

// Photon store
cl_mem photonBufferGPU;

// Energy store
const int resolution = 64;
cl_mem simulationBufferGPU;

// Visualization buffer
int width = 600;
int height = 600;
cl_mem visualizationBufferGPU;
cl_float4* visualizationBufferCPU;
Camera camera;

// Simulation parameters
int iteration = 0;
cl_float4 lightSourcePosition;

void initSimulation(){
  photonProgram = cl.createProgram("kernels/programs.cl");
  simulationKernel = cl.createKernel(photonProgram, "simulation");
  visualizationKernel = cl.createKernel(photonProgram, "visualization");

  // working set size
  CL_SAFE_CALL( clGetKernelWorkGroupInfo(simulationKernel, cl.device_id(), CL_KERNEL_WORK_GROUP_SIZE,
					 sizeof(workGroupSize), &workGroupSize, NULL));
  maxComputeUnits = *(int*)cl.getDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS);
  problemSize = workGroupSize * maxComputeUnits;

  std::cout << "Working set: " << workGroupSize << " * " << maxComputeUnits << " = " << problemSize << std::endl;

  // init random number generator
  cl_uint4* seedCPU = new cl_uint4[workGroupSize * maxComputeUnits];
  for(int i=0; i < workGroupSize * maxComputeUnits; ++i){
    seedCPU[i].s[0] = rand();
    seedCPU[i].s[1] = rand();
    seedCPU[i].s[2] = rand();
    seedCPU[i].s[3] = rand();
  }
  seedGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_uint4) * workGroupSize * maxComputeUnits, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), seedGPU,
				     CL_TRUE, 0, sizeof(cl_uint4) * workGroupSize * maxComputeUnits,
				     seedCPU, 0, NULL, NULL));

  // photon buffer
  photonBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(struct photon) * workGroupSize * maxComputeUnits, NULL, NULL);

  // simulation buffer
  simulationBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * resolution * resolution * resolution, NULL, NULL);

  // light source parameters
  lightSourcePosition.s[0] = 0.6f;
  lightSourcePosition.s[1] = 0.5f;
  lightSourcePosition.s[2] = 0.5f;
  lightSourcePosition.s[3] = 0.0f;
}

void resetSimulation(int resolution, cl_mem simulationBufferGPU){
  cl_kernel resetSimulationKernel = cl.createKernel(photonProgram, "resetSimulation");

  CL_SAFE_CALL(clSetKernelArg(resetSimulationKernel, 0, sizeof(int), &resolution));
  CL_SAFE_CALL(clSetKernelArg(resetSimulationKernel, 1, sizeof(cl_mem), &simulationBufferGPU));

  size_t workSize[3];
  workSize[0] = resolution;
  workSize[1] = resolution;
  workSize[2] = resolution;

  CL_SAFE_CALL(clEnqueueNDRangeKernel(cl.cqueue(), resetSimulationKernel,
				      3, NULL, workSize, NULL,
				      0, NULL, NULL));
  clFinish(cl.cqueue());

  clReleaseKernel(resetSimulationKernel);
}

void simulationStep(){
  if(1 == iteration){
    resetSimulation(resolution, simulationBufferGPU);
  }

  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 0, sizeof(int), &iteration) );
  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 1, sizeof(cl_mem), &seedGPU) );
  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 2, sizeof(cl_mem), &photonBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 3, sizeof(int), &resolution) );
  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 4, sizeof(cl_mem), &simulationBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(simulationKernel, 5, sizeof(cl_float4), &lightSourcePosition) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), simulationKernel,
				       1, NULL, &problemSize, &workGroupSize,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());
}

void visualizationStep(){
  cl_float16 clViewDir;
  float* camMatrix = camera.getViewDirMatrix().getPointer();
  for(int i = 0; i < 16; ++i){
    clViewDir.s[i] = camMatrix[i];
  }

  Vector eye = camera.getEye();
  clViewDir.s[12] = eye.x;
  clViewDir.s[13] = eye.y;
  clViewDir.s[14] = eye.z;
  clViewDir.s[15] = 1.0f;

  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 0, sizeof(int), &width) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 1, sizeof(int), &height) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 3, sizeof(int), &resolution) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 4, sizeof(cl_mem), &simulationBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 5, sizeof(int), &iteration) );
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 6, sizeof(cl_float16), &clViewDir));

  size_t visualizationBufferSize[2];
  visualizationBufferSize[0] = width;
  visualizationBufferSize[1] = height;

  CL_SAFE_CALL(clEnqueueNDRangeKernel(cl.cqueue(), visualizationKernel,
				      2, NULL, visualizationBufferSize, NULL,
				      0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), visualizationBufferGPU, CL_TRUE, 0, sizeof(cl_float4) * width * height,
				    visualizationBufferCPU, 0, NULL, NULL));

  //saveTGA(iteration, width, height, visualizationBufferCPU);
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, visualizationBufferCPU);
}

// Iso-surface raycasting
cl_program visualizationProgram;
cl_kernel isosurfaceRaycastingKernel;
cl_kernel alphaBlendedKernel;

float* volumeData;
int volumeSize[3];

cl_mem volumeDataGPU;

float isoValue = 0.5f;
float alphaExponent = 2.0f;
float alphaCenter = 0.5f;

void loadVolume(char* fileName){
  FILE* dataFile = fopen(fileName, "rb");
  char* magicNum = new char[2];
  fread(magicNum, sizeof(char), 2, dataFile);
  if('V' == magicNum[0] && 'F' == magicNum[1]){
    fread(volumeSize, sizeof(int), 3, dataFile);
    volumeData = new float[volumeSize[0] * volumeSize[1] * volumeSize[2]];
    fread(volumeData, sizeof(float), volumeSize[0] * volumeSize[1] * volumeSize[2], dataFile);
  } else {
    std::cout << "Can't open volume file %s\n" << fileName << std::endl;
  }
}

void initIsosurface(){
  visualizationProgram = cl.createProgram("kernels/visualization.cl");
  isosurfaceRaycastingKernel = cl.createKernel(visualizationProgram, "isosurface");
  alphaBlendedKernel = cl.createKernel(visualizationProgram, "alphaBlended");

  loadVolume("../common/volumes/head.vox");
  if(NULL == volumeData){
    exit(-1);
  }

  volumeDataGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * volumeSize[0] * volumeSize[1] * volumeSize[2], NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), volumeDataGPU,
				     CL_TRUE, 0, sizeof(float) * volumeSize[0] * volumeSize[1] * volumeSize[2],
				     volumeData, 0, NULL, NULL));
}

// Iso surface raycasting
void isosurface(){
  cl_float16 clViewDir;
  float* camMatrix = camera.getViewDirMatrix().getPointer();
  for(int i = 0; i < 16; ++i){
    clViewDir.s[i] = camMatrix[i];
  }

  Vector eye = camera.getEye();
  clViewDir.s[12] = eye.x;
  clViewDir.s[13] = eye.y;
  clViewDir.s[14] = eye.z;
  clViewDir.s[15] = 1.0f;

  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 0, sizeof(int), &width) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 1, sizeof(int), &height) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 3, sizeof(int), &volumeSize[0]) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 4, sizeof(cl_mem), &volumeDataGPU) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 5, sizeof(float), &isoValue) );
  CL_SAFE_CALL( clSetKernelArg(isosurfaceRaycastingKernel, 6, sizeof(cl_float16), &clViewDir) );

  size_t visualizationBufferSize[2];
  visualizationBufferSize[0] = width;
  visualizationBufferSize[1] = height;

  CL_SAFE_CALL(clEnqueueNDRangeKernel(cl.cqueue(), isosurfaceRaycastingKernel,
				      2, NULL, visualizationBufferSize, NULL,
				      0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), visualizationBufferGPU, CL_TRUE, 0, sizeof(cl_float4) * width * height,
				    visualizationBufferCPU, 0, NULL, NULL));

  //saveTGA(iteration, width, height, visualizationBufferCPU);
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, visualizationBufferCPU);
}

// Alpha blended volume visualization
void alphaBlended(){
  cl_float16 clViewDir;
  float* camMatrix = camera.getViewDirMatrix().getPointer();
  for(int i = 0; i < 16; ++i){
    clViewDir.s[i] = camMatrix[i];
  }

  Vector eye = camera.getEye();
  clViewDir.s[12] = eye.x;
  clViewDir.s[13] = eye.y;
  clViewDir.s[14] = eye.z;
  clViewDir.s[15] = 1.0f;

  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 0, sizeof(int), &width) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 1, sizeof(int), &height) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 3, sizeof(int), &volumeSize[0]) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 4, sizeof(cl_mem), &volumeDataGPU) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 5, sizeof(float), &alphaExponent) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 6, sizeof(float), &alphaCenter) );
  CL_SAFE_CALL( clSetKernelArg(alphaBlendedKernel, 7, sizeof(cl_float16), &clViewDir) );

  size_t visualizationBufferSize[2];
  visualizationBufferSize[0] = width;
  visualizationBufferSize[1] = height;

  CL_SAFE_CALL(clEnqueueNDRangeKernel(cl.cqueue(), alphaBlendedKernel,
				      2, NULL, visualizationBufferSize, NULL,
				      0, NULL, NULL) );

  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), visualizationBufferGPU, CL_TRUE, 0, sizeof(cl_float4) * width * height,
				    visualizationBufferCPU, 0, NULL, NULL));

  //saveTGA(iteration, width, height, visualizationBufferCPU);
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, visualizationBufferCPU);
}

// Common data
void initCommon(){
  // visualization buffer
  visualizationBufferCPU = new cl_float4[width*height];
  visualizationBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float4) * width * height, NULL, NULL);
}

// OpenGL
int method = 1;
bool keysPressed[256];

void initOpenGL(){
  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (GLEW_OK != err)     {
    std::cerr << "Error: "<< glewGetErrorString(err) << std::endl;
  } else {
    if (GLEW_VERSION_3_0)
      {
	std::cout<< "Driver supports OpenGL 3.0\nDetails:"<< std::endl;
	std::cout << "  Using GLEW " << glewGetString(GLEW_VERSION)<< std::endl;
	std::cout << "  Vendor: " << glGetString (GL_VENDOR)<< std::endl;
	std::cout << "  Renderer: " << glGetString (GL_RENDERER)<< std::endl;
	std::cout << "  Version: " << glGetString (GL_VERSION)<< std::endl;
	std::cout << "  GLSL: " << glGetString (GL_SHADING_LANGUAGE_VERSION)<< std::endl;
      }
  }

  glClearColor(0.17f, 0.4f, 0.6f, 1.0f);
}

void display(){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);

  switch(method){
  case 1:
    isosurface();
    break;

  case 2:
    alphaBlended();
    break;

  case 3:
    iteration++;
    simulationStep();
    visualizationStep();
    break;
  }

  glEnable(GL_DEPTH_TEST);
  glutSwapBuffers();
}

void animate(){
        static float lastTime = 0.0;
        long timeInMilliSecs = glutGet(GLUT_ELAPSED_TIME);
        float timeNow = timeInMilliSecs / 1000.0f;
        float deltaTime = timeNow - lastTime;
        lastTime = timeNow;

        camera.control(deltaTime, keysPressed);

	glutPostRedisplay();
}

void keyDown(unsigned char key, int x, int y){
  keysPressed[key] = true;
  switch(key){
  case 'h':
	  lightSourcePosition.s[0] -= 0.05f;
	  iteration = 0;
	  break;
  case 'k':
	  lightSourcePosition.s[0] += 0.05f;
	  iteration = 0;
	  break;
  case 'u':
	  lightSourcePosition.s[1] += 0.05f;
	  iteration = 0;
	  break;
  case 'j':
	  lightSourcePosition.s[1] -= 0.05f;
	  iteration = 0;
	  break;
  case 'y':
	  lightSourcePosition.s[2] += 0.05f;
	  iteration = 0;
	  break;
  case 'i':
	  lightSourcePosition.s[2] -= 0.05f;
	  iteration = 0;
	  break;
  case 'r':
	  iteration = 0;
	  break;

  case '+':
    isoValue += 0.01f;
    break;
  case '-':
    isoValue -= 0.01f;
    break;

  case '[':
    alphaExponent *= 0.99f;
    break;
  case ']':
    alphaExponent *= 1.01f;
    break;

  case '{':
    alphaCenter -= 0.01f;
    break;
  case '}':
    alphaCenter += 0.01f;
    break;

  case '1':
    method = 1;
    break;
  case '2':
    method = 2;
    break;
  case '3':
    method = 3;
    break;
  }
}

void keyUp(unsigned char key, int x, int y){
  keysPressed[key] = false;
  switch(key){
  case 27:
    exit(0);
    break;
  }
}

void mouseClick(int button, int state, int x, int y){
  if(button == GLUT_LEFT_BUTTON)
    if(state == GLUT_DOWN)
      camera.startDrag(x, y);
}

void mouseMove(int x, int y){
        camera.drag(x, y);
}

void reshape(int newWidth, int newHeight){
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
        camera.setAspectRatio((float)width/height);
}

int main(int argc, char* argv[]){
  glutInit(&argc, argv);
  glutInitContextVersion (3, 0);
  glutInitContextFlags (GLUT_CORE_PROFILE | GLUT_DEBUG);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(width, height);
  glutCreateWindow("GPGPU 11. labor: Monte Carlo simulation");

  initOpenGL();

  glutDisplayFunc(display);
  glutIdleFunc(animate);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyDown);
  glutKeyboardUpFunc(keyUp);
  glutMouseFunc(mouseClick);
  glutMotionFunc(mouseMove);

  // OpenCL processing
  initCommon();
  initIsosurface();
  initSimulation();

  glutMainLoop();
  return(0);
}
