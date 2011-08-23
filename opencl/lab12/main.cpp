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
#include <sstream>
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
void saveTGA(int pictureNumber, int width, int height, float* buffer){
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

  float vmax = 0;
  for(int y = 0; y < height; ++y) {
    for(size_t x = 0 ; x < width; ++x) {
        vmax = std::max(buffer[x + y * width],vmax);
    }
  }
  for(int y = 0; y < height; ++y) {
    for(size_t x = 0 ; x < width; ++x) {
        //buffer[x + y * width] *= (height*width) / vmax;
        buffer[x + y * width] /= vmax;
    }
  }


  for(int y = 0; y < height; ++y) {
    for(size_t x = 0 ; x < width; ++x) {
      fputc(buffer[x + y * width] * 255, f); /* write blue */
      fputc(buffer[x + y * width] * 255, f); /* write green */
      fputc(buffer[x + y * width] * 255, f); /* write red */
    }
  }
  fclose(f);
  free(fname);
}

std::ostream& operator<< ( std::ostream& os, cl_float4& v )
{
   os << "(" << v.s[0] << "," << v.s[1] << "," << v.s[2] << "," << v.s[3] << ")";
   return os;
}

/* temporary functions */

struct module
{
    cl_float4 origin;
    cl_float4 axial;
    cl_float4 transAxial;
    cl_float4 n;
};

const int nModules = 4;
const int coincidence = 1;
const int nPairs = nModules*coincidence / 2;

const int nAxial = 32;
const int nTransAxial = 32;
const int lorNumberPerPair = nAxial*nTransAxial*nAxial*nTransAxial;
const int lorNumber = lorNumberPerPair*nPairs;
const float detectorArea = 2.0f*2.0f / nAxial / nTransAxial;
cl_float4 volumeMin;
cl_float4 volumeMax;

cl_mem estimatedLorBufferGPU;
cl_mem measuredLorBufferGPU;
float estimatedLorBufferCPU[lorNumber];
float measuredLorBufferCPU[lorNumber];

cl_mem parallelProjectionBufferGPU;
float parallelProjectionBufferCPU[nPairs*nAxial*nTransAxial];

cl_mem moduleBufferGPU;
module moduleBufferCPU[nModules];

cl_mem volumeBufferGPU;
float* volumeData;
const int resolution = 32;

cl_program petProgram;

cl_kernel forwardProjectionKernel;
cl_kernel backProjectionKernel;
cl_kernel parallelProjectionKernel;

// Visualization buffer
cl_kernel visualizationKernel;

int width = 600;
int height = 600;
cl_mem visualizationBufferGPU;
cl_float4* visualizationBufferCPU;
Camera camera;

// Simulation parameters
int iteration = 0;
int measurementGeneration = 0;

void saveLorTGA(){
  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), estimatedLorBufferGPU, CL_TRUE, 0, sizeof(float) * nPairs*nAxial*nTransAxial*nAxial*nTransAxial,
				    estimatedLorBufferCPU, 0, NULL, NULL));
  saveTGA(100+iteration,nAxial*nTransAxial,nTransAxial*nAxial*nPairs,estimatedLorBufferCPU);
}

void rotate90xy( cl_float4 &vector )
{
    std::swap( vector.s[0], vector.s[1] );
    vector.s[0] *= -1.0f;
}

void initSimulation(){
  petProgram = cl.createProgram("kernels/programs.cl");
  forwardProjectionKernel = cl.createKernel(petProgram, "forwardProjection");
  backProjectionKernel = cl.createKernel(petProgram, "backProjection");
  parallelProjectionKernel = cl.createKernel(petProgram, "parallelProjection");
  visualizationKernel = cl.createKernel(petProgram, "visualization");

  // lor buffer
  estimatedLorBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * lorNumber, NULL, NULL);
  measuredLorBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * lorNumber, NULL, NULL);

  // module buffer
  cl_float4 origin, axial, transAxial, n;
  origin.s[3] = axial.s[3] = transAxial.s[3] = n.s[3] = 0.0f;
  origin.s[0] = origin.s[1] = origin.s[2] = -1.0f;                      // (-1,-1,-1)
  axial.s[2] = 2.0f; axial.s[0] = axial.s[1] = 0.0f;                    // (0,0,2)
  transAxial.s[0] = 2.0f; transAxial.s[1] = transAxial.s[2] = 0.0f;     // (2,0,0)
  n.s[1] = 1.0f; n.s[0] = n.s[2] = 0.0f;                                // (0,1,0)
  // module buffer
  moduleBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(module)*nModules, NULL, NULL);
  for ( int i = 0; i < nModules; ++i )
  {
    moduleBufferCPU[i].origin = origin;
    moduleBufferCPU[i].axial = axial;
    moduleBufferCPU[i].transAxial = transAxial;
    moduleBufferCPU[i].n = n;

    rotate90xy(origin);
    rotate90xy(axial);
    rotate90xy(transAxial);
    rotate90xy(n);
  }
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), moduleBufferGPU,
                                     CL_TRUE, 0, sizeof(module)*nModules,
                                     moduleBufferCPU, 0, NULL, NULL));

  for ( unsigned int i = 0; i < nModules; ++i )
  {
    std::cout << i << ": a=" << moduleBufferCPU[i].axial << ", t=" << moduleBufferCPU[i].transAxial << ", o=" << moduleBufferCPU[i].origin << ", n=" << moduleBufferCPU[i].n << std::endl;
  }

  volumeMin.s[0] = volumeMin.s[1] = volumeMin.s[2] = -0.5f;
  volumeMax.s[0] = volumeMax.s[1] = volumeMax.s[2] = 0.5f;

  volumeMin.s[3] = volumeMax.s[3] = 0.0f;

  volumeData = new float[resolution * resolution * resolution];
  for ( unsigned int x = 0; x < resolution; ++x )
   for ( unsigned int y = 0; y < resolution; ++y )
      for ( unsigned int z = 0; z < resolution; ++z ) {
         if ( (x-resolution/2)*(x-resolution/2)+(y-resolution/2)*(y-resolution/2)+(z-resolution/2)*(z-
               resolution/2) < 3*(resolution/5)*(resolution/5) )
            volumeData[ x + resolution*y + resolution*resolution*z ] = 1.0f;
         else
            volumeData[ x + resolution*y + resolution*resolution*z ] = 0.0f;
      }
  // volume buffer
  volumeBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * resolution * resolution * resolution, NULL, NULL);
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), volumeBufferGPU,
                                     CL_TRUE, 0, sizeof(float) * resolution * resolution * resolution,
                                     volumeData, 0, NULL, NULL));
   // forward projection for generating measurement
  measurementGeneration = 1;
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 0, sizeof(int), &nAxial) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 1, sizeof(int), &nTransAxial) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 2, sizeof(int), &resolution) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 3, sizeof(float), &detectorArea) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 4, sizeof(cl_float4), &volumeMin) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 5, sizeof(cl_float4), &volumeMax) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 6, sizeof(cl_mem), &moduleBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 7, sizeof(cl_mem), &measuredLorBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 8, sizeof(cl_mem), &estimatedLorBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 9, sizeof(cl_mem), &volumeBufferGPU) );
  CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 10, sizeof(int), &measurementGeneration) );
  size_t forwardProjectionSize[3];
  forwardProjectionSize[0] = nPairs;
  forwardProjectionSize[1] = forwardProjectionSize[2] = nTransAxial*nAxial;
  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), forwardProjectionKernel,
  				       3, NULL, forwardProjectionSize, NULL,
  				       0, NULL, NULL) );
  clFinish(cl.cqueue());
  measurementGeneration = 0;

  volumeData = new float[resolution * resolution * resolution];
  for ( unsigned int x = 0; x < resolution; ++x )
    for ( unsigned int y = 0; y < resolution; ++y )
      for ( unsigned int z = 0; z < resolution; ++z )
      {
        volumeData[ x +resolution*y +resolution*resolution*z ] = 1.0f;
      }

  // volume buffer
  CL_SAFE_CALL( clEnqueueWriteBuffer(cl.cqueue(), volumeBufferGPU,
                                     CL_TRUE, 0, sizeof(float) * resolution * resolution * resolution,
                                     volumeData, 0, NULL, NULL));

  // parallel projection buffer
  parallelProjectionBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * nPairs*nAxial*nTransAxial, NULL, NULL);
}

void resetSimulation(int resolution, cl_mem volumeBufferGPU){
  cl_kernel resetSimulationKernel = cl.createKernel(petProgram, "resetSimulation");

  CL_SAFE_CALL(clSetKernelArg(resetSimulationKernel, 0, sizeof(int), &resolution));
  CL_SAFE_CALL(clSetKernelArg(resetSimulationKernel, 1, sizeof(cl_mem), &volumeBufferGPU));

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

    std::cout << "Computing forward projection...";

   // forward projection
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 0, sizeof(int), &nAxial) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 1, sizeof(int), &nTransAxial) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 2, sizeof(int), &resolution) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 3, sizeof(float), &detectorArea) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 4, sizeof(cl_float4), &volumeMin) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 5, sizeof(cl_float4), &volumeMax) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 6, sizeof(cl_mem), &moduleBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 7, sizeof(cl_mem), &measuredLorBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 8, sizeof(cl_mem), &estimatedLorBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 9, sizeof(cl_mem), &volumeBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(forwardProjectionKernel, 10, sizeof(int), &measurementGeneration) );

   size_t forwardProjectionSize[3];
   forwardProjectionSize[0] = nPairs;
   forwardProjectionSize[1] = forwardProjectionSize[2] = nTransAxial*nAxial;
   CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), forwardProjectionKernel,
   				       3, NULL, forwardProjectionSize, NULL,
   				       0, NULL, NULL) );
   clFinish(cl.cqueue());

   std::cout << "Done." << std::endl << "Computing Back projection...";

   // backProjection
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 0, sizeof(int), &nPairs) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 1, sizeof(int), &nAxial) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 2, sizeof(int), &nTransAxial) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 3, sizeof(int), &resolution) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 4, sizeof(cl_float4), &volumeMin) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 5, sizeof(cl_float4), &volumeMax) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 6, sizeof(cl_mem), &moduleBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 7, sizeof(cl_mem), &estimatedLorBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(backProjectionKernel, 8, sizeof(cl_mem), &volumeBufferGPU) );

   size_t backProjectionSize[3];
   backProjectionSize[0] = backProjectionSize[1] = backProjectionSize[2] = resolution;
   CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), backProjectionKernel,
   				       3, NULL, backProjectionSize, NULL,
   				       0, NULL, NULL) );
   clFinish(cl.cqueue());

   std::cout << "Done." << std::endl << "Computing parallel projection...";

   // parallel
   CL_SAFE_CALL( clSetKernelArg(parallelProjectionKernel, 0, sizeof(int), &nAxial) );
   CL_SAFE_CALL( clSetKernelArg(parallelProjectionKernel, 1, sizeof(int), &nTransAxial) );
   CL_SAFE_CALL( clSetKernelArg(parallelProjectionKernel, 2, sizeof(cl_mem), &estimatedLorBufferGPU) );
   CL_SAFE_CALL( clSetKernelArg(parallelProjectionKernel, 3, sizeof(cl_mem), &parallelProjectionBufferGPU) );

   size_t parallelProjectionSize[3];
   parallelProjectionSize[0] = nPairs;
   parallelProjectionSize[1] = nAxial;
   parallelProjectionSize[2] = nTransAxial;
   CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), parallelProjectionKernel,
   				       3, NULL, parallelProjectionSize, NULL,
   				       0, NULL, NULL) );
   clFinish(cl.cqueue());

   std::cout << "Done." << std::endl;

   // save parallel projection
   CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), parallelProjectionBufferGPU, CL_TRUE, 0, sizeof(float) * nPairs*nAxial*nTransAxial,
				    parallelProjectionBufferCPU, 0, NULL, NULL));

    saveTGA(iteration,nAxial,nTransAxial*nPairs,parallelProjectionBufferCPU);
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
  CL_SAFE_CALL( clSetKernelArg(visualizationKernel, 4, sizeof(cl_mem), &volumeBufferGPU) );
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

  visualizationStep();

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
}

void keyUp(unsigned char key, int x, int y){
  keysPressed[key] = false;
  switch(key){
  case 'i':
  case 'I':
      ++iteration;
      simulationStep();
      break;
  case 'l':
  case 'L':
      saveLorTGA();
      break;
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
  glutCreateWindow("GPGPU 12. labor: PET reconstruction");

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
  initSimulation();

  glutMainLoop();
  return(0);
}
