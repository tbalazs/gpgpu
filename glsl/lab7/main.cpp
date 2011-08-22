#include <GL/glew.h>
#include <GL/freeglut.h>

#include <iostream>
#include <fstream>
#include <cmath>

#include "framebuffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "quad.hpp"
#include "pointgrid.hpp"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

Quad* fullscreenQuad;
Shader* simpleShader;

int trajectoryNum = 1024; // number of realizations
int trajectoryTimeSteps = 128; // number of simulation steps

Shader* trajectoryShader;
Framebuffer* trajectoriesBuffer;

Shader* sumShader;
Framebuffer* sumBuffer[2] = {NULL};

int windowWidth = 600;
int windowHeight = 600;

void init(){

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)	{
		std::cerr << "Error: "<< glewGetErrorString(err) << std::endl;
	} else {
		if (GLEW_VERSION_3_3)
		{
			std::cout<< "Driver supports OpenGL 3.3\nDetails:"<< std::endl;
			std::cout << "Using GLEW " << glewGetString(GLEW_VERSION)<< std::endl;
			std::cout << "Vendor: " << glGetString (GL_VENDOR)<< std::endl;
			std::cout << "Renderer: " << glGetString (GL_RENDERER)<< std::endl;
			std::cout << "Version: " << glGetString (GL_VERSION)<< std::endl;
			std::cout << "GLSL: " << glGetString (GL_SHADING_LANGUAGE_VERSION)<< std::endl;
		}
	}
	glGetError();

  fullscreenQuad = new Quad();
  simpleShader = new Shader("shaders/passthrough.vert", "shaders/simple.frag");

  trajectoryShader = new Shader("shaders/passthrough.vert", "shaders/trajectory.frag");
  trajectoriesBuffer = new Framebuffer(trajectoryNum, trajectoryTimeSteps, 1);

  sumShader = new Shader("shaders/passthrough.vert", "shaders/sum.frag");
  sumBuffer[0] = new Framebuffer(trajectoryNum, trajectoryTimeSteps, 1);
  sumBuffer[1] = new Framebuffer(trajectoryNum, trajectoryTimeSteps, 1);

  glutReshapeWindow(windowWidth, windowHeight);
}

void display(){
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  trajectoriesBuffer->setRenderTarget();
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  trajectoriesBuffer->disableRenderTarget();

  for(int i = 1; i < trajectoryTimeSteps; ++i){
    trajectoriesBuffer->setRenderTarget();
    trajectoryShader->enable();
    trajectoryShader->bindUniformTexture("trajectories", trajectoriesBuffer->getColorBuffer(0), 0);
    trajectoryShader->bindUniformInt("timeStep", i);
    fullscreenQuad->render(trajectoryShader);
    trajectoryShader->disable();
    trajectoriesBuffer->disableRenderTarget();
  }

  float* trajectories = new float[trajectoryNum * trajectoryTimeSteps];
  glBindFramebuffer(GL_FRAMEBUFFER, trajectoriesBuffer->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, trajectoryNum, trajectoryTimeSteps, GL_RED, GL_FLOAT, trajectories);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::ofstream dataFile;
  dataFile.open("simulation.txt");
  for(int x = 0; x < trajectoryNum; ++x){
    for(int y = 0; y < trajectoryTimeSteps; ++y){
      dataFile << y << " " << trajectories[x + y * trajectoryNum] << std::endl;
    }
    dataFile << std::endl;
  }
  dataFile.close();

  sumBuffer[0]->clear();
  sumBuffer[1]->clear();

  int inputBuffer = 0;
  int offset = 1;
  int passes = ceil(log((float)trajectoryNum) / log(2.0f));
  for(int pass = 0; pass < passes; ++pass){
    sumBuffer[(inputBuffer + 1) % 2]->setRenderTarget();
    sumShader->enable();
    sumShader->bindUniformTexture("inputMap", pass == 0 ? trajectoriesBuffer->getColorBuffer(0) : sumBuffer[inputBuffer]->getColorBuffer(0), 0);
    sumShader->bindUniformInt("offset", -offset);
    fullscreenQuad->render(sumShader);
    sumShader->disable();
    sumBuffer[(inputBuffer + 1) % 2]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;
    offset *= 2.0f;
  }

  float* e = new float[trajectoryTimeSteps];
  glBindFramebuffer(GL_FRAMEBUFFER, sumBuffer[inputBuffer]->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(trajectoryNum-1, 0, 1, trajectoryTimeSteps, GL_RED, GL_FLOAT, e);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  float* e2 = new float[trajectoryTimeSteps];
  glBindFramebuffer(GL_FRAMEBUFFER, sumBuffer[inputBuffer]->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(trajectoryNum-1, 0, 1, trajectoryTimeSteps, GL_GREEN, GL_FLOAT, e2);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::ofstream resultsFile;
  resultsFile.open("results.txt");
  for(int y = 0; y < trajectoryTimeSteps; ++y){
    float eValue = e[y] / trajectoryNum;
    float dValue = sqrt(e2[y]/trajectoryNum - (eValue * eValue));
    resultsFile << y << " " << eValue << " " << dValue << std::endl;
  }
  resultsFile.close();

  simpleShader->enable();
  simpleShader->bindUniformTexture("inputBuffer", trajectoriesBuffer->getColorBuffer(0), 0);
  //simpleShader->bindUniformTexture("inputBuffer", sumBuffer[inputBuffer]->getColorBuffer(0), 0);
  fullscreenQuad->render(simpleShader);
  simpleShader->disable();

  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y){
  switch(key){

  case 27:
    exit(0);
    break;

  default:
    std::cout << "Unbinded key: " << (unsigned int)key << std::endl;
  }

  glutPostRedisplay();
}

void reshape(int width, int height){
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

void idle(){
  //  glutPostRedisplay();
}

int main(int argc, char* argv[]){
  glutInit(&argc, argv);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("GPGPU 7. labor: stochastic process");

  init();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  glutMainLoop();

  return(0);
}
