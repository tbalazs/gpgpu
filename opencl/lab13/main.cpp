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

#include "clwrapper.hpp"

ClWrapper cl;

// *****************
// OpenCL processing
// *****************

cl_program simulationProgram;

// simulation
int gridResolution = 512;
int inputVelocityBuffer = 0;
cl_mem velocityBuffer[2];

int inputDensityBuffer = 0;
cl_mem densityBuffer[2];
cl_float4 densityColor;

int inputPressureBuffer = 0;
cl_mem pressureBuffer[2];
cl_mem divergenceBuffer;

cl_mem vorticityBuffer;

cl_kernel advectionKernel;
cl_kernel advectionDensityKernel;
cl_kernel diffusionKernel;
cl_kernel divergenceKernel;
cl_kernel pressureJacobiKernel;
cl_kernel projectionKernel;
cl_kernel vorticityKernel;
cl_kernel addVorticityForceKernel;
cl_kernel addForceKernel;
cl_kernel resetSimulationKernel;

size_t problemSize[2];

cl_float2 force;

// visualization
int width = 512;
int height = 512;

cl_mem visualizationBufferGPU;
cl_float4* visualizationBufferCPU;

int visualizationMethod = 0;

size_t visualizationSize[2];
cl_kernel visualizationDensityKernel;
cl_kernel visualizationVelocityKernel;
cl_kernel visualizationPressureKernel;

void initSimulation(){
  simulationProgram = cl.createProgram("kernels/programs.cl");

  // simulation
  problemSize[0] = gridResolution;
  problemSize[1] = gridResolution;

  advectionKernel = cl.createKernel(simulationProgram, "advection");
  velocityBuffer[0] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float2) * gridResolution * gridResolution, NULL, NULL);
  velocityBuffer[1] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float2) * gridResolution * gridResolution, NULL, NULL);

  advectionDensityKernel = cl.createKernel(simulationProgram, "advectionDensity");
  densityBuffer[0] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float4) * gridResolution * gridResolution, NULL, NULL);
  densityBuffer[1] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float4) * gridResolution * gridResolution, NULL, NULL);

  diffusionKernel = cl.createKernel(simulationProgram, "diffusion");

  divergenceKernel = cl.createKernel(simulationProgram, "divergence");
  pressureJacobiKernel = cl.createKernel(simulationProgram, "pressureJacobi");
  pressureBuffer[0] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * gridResolution * gridResolution, NULL, NULL);
  pressureBuffer[1] = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * gridResolution * gridResolution, NULL, NULL);
  divergenceBuffer = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * gridResolution * gridResolution, NULL, NULL);
  projectionKernel = cl.createKernel(simulationProgram, "projection");

  vorticityKernel = cl.createKernel(simulationProgram, "vorticity");
  addVorticityForceKernel = cl.createKernel(simulationProgram, "addVorticity");
  vorticityBuffer = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(float) * gridResolution * gridResolution, NULL, NULL);

  densityColor.s[0] = densityColor.s[1] = densityColor.s[2] = densityColor.s[3] = 1.0f;
  addForceKernel = cl.createKernel(simulationProgram, "addForce");

  resetSimulationKernel = cl.createKernel(simulationProgram, "resetSimulation");

  // visualization
  visualizationSize[0] = width;
  visualizationSize[1] = height;

  visualizationBufferCPU = new cl_float4[width * height];
  visualizationBufferGPU = clCreateBuffer(cl.context(), CL_MEM_READ_WRITE, sizeof(cl_float4) * width * height, NULL, NULL);
  visualizationDensityKernel = cl.createKernel(simulationProgram, "visualizationDensity");
  visualizationVelocityKernel = cl.createKernel(simulationProgram, "visualizationVelocity");
  visualizationPressureKernel = cl.createKernel(simulationProgram, "visualizationPressure");
}

void resetSimulation(){
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 2, sizeof(cl_mem), &pressureBuffer[inputPressureBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 3, sizeof(cl_mem), &densityBuffer[inputDensityBuffer]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), resetSimulationKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());
}

void resetPressure(){
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 1, sizeof(cl_mem), &velocityBuffer[(inputVelocityBuffer+1)%2]) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 2, sizeof(cl_mem), &pressureBuffer[inputPressureBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(resetSimulationKernel, 3, sizeof(cl_mem), &densityBuffer[(inputDensityBuffer+1)%2]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), resetSimulationKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());
}

void simulateAdvection(){
  CL_SAFE_CALL( clSetKernelArg(advectionKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(advectionKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(advectionKernel, 2, sizeof(cl_mem), &velocityBuffer[(inputVelocityBuffer + 1) % 2]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), advectionKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());

  inputVelocityBuffer = (inputVelocityBuffer + 1) % 2;
}

void simulateVorticity(){
  CL_SAFE_CALL( clSetKernelArg(vorticityKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(vorticityKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(vorticityKernel, 2, sizeof(cl_mem), &vorticityBuffer) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), vorticityKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());

  CL_SAFE_CALL( clSetKernelArg(addVorticityForceKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(addVorticityForceKernel, 1, sizeof(cl_mem), &vorticityBuffer) );
  CL_SAFE_CALL( clSetKernelArg(addVorticityForceKernel, 2, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), addVorticityForceKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());
}

void simulateDiffusion(){
  for(int i = 0; i < 10; ++i){
    CL_SAFE_CALL( clSetKernelArg(diffusionKernel, 0, sizeof(int), &gridResolution) );
    CL_SAFE_CALL( clSetKernelArg(diffusionKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
    CL_SAFE_CALL( clSetKernelArg(diffusionKernel, 2, sizeof(cl_mem), &velocityBuffer[(inputVelocityBuffer + 1) % 2]) );

    CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), diffusionKernel,
					 2, NULL, problemSize, NULL,
					 0, NULL, NULL) );
    clFinish(cl.cqueue());

    inputVelocityBuffer = (inputVelocityBuffer + 1) % 2;
  }
}

void projection(){
  CL_SAFE_CALL( clSetKernelArg(divergenceKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(divergenceKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(divergenceKernel, 2, sizeof(cl_mem), &divergenceBuffer) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), divergenceKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());

  resetPressure();

  for(int i = 0; i < 10; ++i){
    CL_SAFE_CALL( clSetKernelArg(pressureJacobiKernel, 0, sizeof(int), &gridResolution) );
    CL_SAFE_CALL( clSetKernelArg(pressureJacobiKernel, 1, sizeof(cl_mem), &pressureBuffer[inputPressureBuffer]) );
    CL_SAFE_CALL( clSetKernelArg(pressureJacobiKernel, 2, sizeof(cl_mem), &pressureBuffer[(inputPressureBuffer + 1) % 2]) );
    CL_SAFE_CALL( clSetKernelArg(pressureJacobiKernel, 3, sizeof(cl_mem), &divergenceBuffer) );

    CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), pressureJacobiKernel,
					 2, NULL, problemSize, NULL,
					 0, NULL, NULL) );
    clFinish(cl.cqueue());

    inputPressureBuffer = (inputPressureBuffer + 1) % 2;
  }

  CL_SAFE_CALL( clSetKernelArg(projectionKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(projectionKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(projectionKernel, 2, sizeof(cl_mem), &pressureBuffer[inputPressureBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(projectionKernel, 3, sizeof(cl_mem), &velocityBuffer[(inputVelocityBuffer + 1) % 2]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), projectionKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());

  inputVelocityBuffer = (inputVelocityBuffer + 1) % 2;
}

void simulateDensityAdvection(){
  CL_SAFE_CALL( clSetKernelArg(advectionDensityKernel, 0, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(advectionDensityKernel, 1, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(advectionDensityKernel, 2, sizeof(cl_mem), &densityBuffer[inputDensityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(advectionDensityKernel, 3, sizeof(cl_mem), &densityBuffer[(inputDensityBuffer + 1) % 2]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), advectionDensityKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());

  inputDensityBuffer = (inputDensityBuffer + 1) % 2;
}

void addForce(int x, int y, cl_float2 force){
  float fx = (float)x / width;
  float fy = (float)y / height;

  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 0, sizeof(float), &fx) );
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 1, sizeof(float), &fy) );
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 2, sizeof(cl_float2), &force))
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 3, sizeof(int), &gridResolution) );
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 4, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 5, sizeof(cl_float4), &densityColor) );
  CL_SAFE_CALL( clSetKernelArg(addForceKernel, 6, sizeof(cl_mem), &densityBuffer[inputDensityBuffer]) );

  CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), addForceKernel,
				       2, NULL, problemSize, NULL,
				       0, NULL, NULL) );
  clFinish(cl.cqueue());
}

void simulationStep(){
  simulateAdvection();
  simulateDiffusion();
  simulateVorticity();
  projection();
  simulateDensityAdvection();
}

void visualizationStep(){
  switch(visualizationMethod){
  case 0:
    CL_SAFE_CALL( clSetKernelArg(visualizationDensityKernel, 0, sizeof(int), &width) );
    CL_SAFE_CALL( clSetKernelArg(visualizationDensityKernel, 1, sizeof(int), &height) );
    CL_SAFE_CALL( clSetKernelArg(visualizationDensityKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
    CL_SAFE_CALL( clSetKernelArg(visualizationDensityKernel, 3, sizeof(int), &gridResolution) );
    CL_SAFE_CALL( clSetKernelArg(visualizationDensityKernel, 4, sizeof(cl_mem), &densityBuffer[inputDensityBuffer]) );
    CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), visualizationDensityKernel,
					 2, NULL, visualizationSize, NULL,
					 0, NULL, NULL) );
    break;

  case 1:
    CL_SAFE_CALL( clSetKernelArg(visualizationVelocityKernel, 0, sizeof(int), &width) );
    CL_SAFE_CALL( clSetKernelArg(visualizationVelocityKernel, 1, sizeof(int), &height) );
    CL_SAFE_CALL( clSetKernelArg(visualizationVelocityKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
    CL_SAFE_CALL( clSetKernelArg(visualizationVelocityKernel, 3, sizeof(int), &gridResolution) );
    CL_SAFE_CALL( clSetKernelArg(visualizationVelocityKernel, 4, sizeof(cl_mem), &velocityBuffer[inputVelocityBuffer]) );
    CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), visualizationVelocityKernel,
					 2, NULL, visualizationSize, NULL,
					 0, NULL, NULL) );
    break;

  case 2:
    CL_SAFE_CALL( clSetKernelArg(visualizationPressureKernel, 0, sizeof(int), &width) );
    CL_SAFE_CALL( clSetKernelArg(visualizationPressureKernel, 1, sizeof(int), &height) );
    CL_SAFE_CALL( clSetKernelArg(visualizationPressureKernel, 2, sizeof(cl_mem), &visualizationBufferGPU) );
    CL_SAFE_CALL( clSetKernelArg(visualizationPressureKernel, 3, sizeof(int), &gridResolution) );
    CL_SAFE_CALL( clSetKernelArg(visualizationPressureKernel, 4, sizeof(cl_mem), &pressureBuffer[inputPressureBuffer]) );

    CL_SAFE_CALL( clEnqueueNDRangeKernel(cl.cqueue(), visualizationPressureKernel,
					 2, NULL, visualizationSize, NULL,
					 0, NULL, NULL) );
    break;

  }
  clFinish(cl.cqueue());

  CL_SAFE_CALL( clEnqueueReadBuffer(cl.cqueue(), visualizationBufferGPU, CL_TRUE,
				    0, sizeof(cl_float4) * width * height, visualizationBufferCPU,
				    0, NULL, NULL) );
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, visualizationBufferCPU);
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

  simulationStep();
  visualizationStep();

  glEnable(GL_DEPTH_TEST);
  glutSwapBuffers();
}

void idle(){
  glutPostRedisplay();
}

void keyDown(unsigned char key, int x, int y){
  keysPressed[key] = true;
}

void keyUp(unsigned char key, int x, int y){
  keysPressed[key] = false;
  switch(key){
  case 'r':
    resetSimulation();
    break;

  case 'd':
    visualizationMethod = 0;
    break;
  case 'v':
    visualizationMethod = 1;
    break;
  case 'p':
    visualizationMethod = 2;
    break;

  case '1':
    densityColor.s[0] = densityColor.s[1] = densityColor.s[2] = densityColor.s[3] = 1.0f;
    break;

  case '2':
    densityColor.s[0] = 1.0f;
    densityColor.s[1] = densityColor.s[2] = densityColor.s[3] = 0.0f;
    break;

  case '3':
    densityColor.s[1] = 1.0f;
    densityColor.s[0] = densityColor.s[2] = densityColor.s[3] = 0.0f;
    break;

  case '4':
    densityColor.s[2] = 1.0f;
    densityColor.s[0] = densityColor.s[1] = densityColor.s[3] = 0.0f;
    break;

  case 27:
    exit(0);
    break;
  }
}

int mX, mY;

void mouseClick(int button, int state, int x, int y){
  if(button == GLUT_LEFT_BUTTON)
    if(state == GLUT_DOWN){
      mX = x;
      mY = y;
    }
}

void mouseMove(int x, int y){
  force.s[0] = (float)(x - mX);
  force.s[1] = - (float)(y - mY);
  //addForce(mX, height - mY, force);
  addForce(256, 256, force);
  mX = x;
  mY = y;
}

void reshape(int newWidth, int newHeight){
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
}

int main(int argc, char* argv[]){
  glutInit(&argc, argv);
  glutInitContextVersion (3, 0);
  glutInitContextFlags (GLUT_CORE_PROFILE | GLUT_DEBUG);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(width, height);
  glutCreateWindow("GPGPU 13. labor: Incompressible fluid simulation");

  initOpenGL();

  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyDown);
  glutKeyboardUpFunc(keyUp);
  glutMouseFunc(mouseClick);
  glutMotionFunc(mouseMove);

  // OpenCL processing
  initSimulation();

  glutMainLoop();
  return(0);
}
