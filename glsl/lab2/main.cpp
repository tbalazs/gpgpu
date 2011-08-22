#include <GL/glew.h>
#include <GL/glut.h>

#include <iostream>

#include "framebuffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "quad.hpp"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

int windowWidth = 600;
int windowHeight = 600;

Quad* fullscreenQuad;
Texture2D* texture;
Shader* simpleShader;

int example = 1;

// Mandelbrot set
Shader* shaderMandelbrot;
float kReal = 0.353f;
float kIm = 0.288f;

void mandelbrot()
{
  shaderMandelbrot->enable();
  shaderMandelbrot->bindUniformFloat2("k", kReal, kIm);
  fullscreenQuad->render(shaderMandelbrot);
  shaderMandelbrot->disable();
}

// Threshold
Shader* shaderThreshold;
float thresholdValue = 0.5f;

void threshold()
{
  shaderThreshold->enable();
  shaderThreshold->bindUniformTexture("textureMap",
				      texture->getTextureHandle(), 0);
  shaderThreshold->bindUniformFloat("threshold", thresholdValue);
  fullscreenQuad->render(shaderThreshold);
  shaderThreshold->disable();
}

// Edge detection
Shader* shaderEdgeDetection;

void edgeDetection()
{
  shaderEdgeDetection->enable();
  shaderEdgeDetection->bindUniformTexture("textureMap", texture->getTextureHandle(), 0);
  shaderEdgeDetection->bindUniformFloat2("textureSize", (float)texture->getWidth(), (float)texture->getHeight());
  fullscreenQuad->render(shaderEdgeDetection);
  shaderEdgeDetection->disable();
}

// Convolution
Shader* shaderConvolution;

void convolution()
{
  shaderConvolution->enable();
  shaderConvolution->bindUniformTexture("textureMap", texture->getTextureHandle(), 0);
  shaderConvolution->bindUniformFloat2("textureSize", (float)texture->getWidth(), (float)texture->getHeight());
  fullscreenQuad->render(shaderConvolution);
  shaderConvolution->disable();
}

void init()
{
  glewInit();

  fullscreenQuad = new Quad();
  simpleShader = new Shader("shaders/passthrough.vert", "shaders/simple.frag");

  shaderMandelbrot = new Shader("shaders/passthrough.vert", "shaders/mandelbrot.frag");

  texture = new Texture2D();
  texture->loadFromFile(std::string("../common/images/lena.jpg"));

  shaderThreshold = new Shader("shaders/passthrough.vert", "shaders/threshold.frag");
  shaderEdgeDetection = new Shader("shaders/passthrough.vert", "shaders/edgeDetection.frag");
  shaderConvolution = new Shader("shaders/passthrough.vert", "shaders/convolution.frag");


  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
}

void display()
{
  glClearColor(0.17f, 0.4f, 0.6f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch(example)
    {
    case 1:
      glutReshapeWindow(600, 600);
      mandelbrot();
      break;

    case 2:
      glutReshapeWindow(texture->getWidth(),
			texture->getHeight());
      threshold();
      break;

    case 3:
      glutReshapeWindow(texture->getWidth(), texture->getHeight());
      edgeDetection();
      break;

    case 4:
      glutReshapeWindow(texture->getWidth(), texture->getHeight());
      convolution();
      break;
  }

  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
  switch(key)
    {

    case 27:
      exit(0);
      break;

    case '1':
      example = 1;
      break;

    case '2':
      example = 2;
      break;
    case '3':
      example = 3;
      break;
    case '4':
      example = 4;
      break;

    case 'W':
      kReal += 0.01f;
      break;
    case 'S':
      kReal -= 0.01f;
      break;
    case 'A':
      kIm -= 0.01f;
      break;
    case 'D':
      kIm += 0.01f;
      break;

    default:
      std::cout << "Unbinded key: " << (unsigned int)key << std::endl;
    }

  glutPostRedisplay();
}

void reshape(int width, int height)
{
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

int main(int argc, char* argv[])
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("GPGPU 2. labor: glsl");

  init();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  glutMainLoop();

  return(0);
}

