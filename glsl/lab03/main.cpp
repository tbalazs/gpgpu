#include <GL/glew.h>
#include <GL/freeglut.h>

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

Shader* gaussShader;
/*void gauss()
  {
  gaussShader->enable();
  gaussShader->bindUniformTexture("colorMap",texture->getTextureHandle(),0);
  fullscreenQuad->render(gaussShader);
  gaussShader->disable();
  }*/

Framebuffer* computeBuffer[2];
int inputBuffer = 0;
int npasses = 15;

/*
  void gauss()
  {
  for(int i = 0; i < npasses; i++)
  {
  if(i!=npasses -1)
  computeBuffer[(inputBuffer + 1) % 2]->setRenderTarget();
  gaussShader->enable();
  gaussShader->bindUniformTexture("colorMap",
  i == 0 ? texture->getTextureHandle() : computeBuffer[inputBuffer]->getColorBuffer(0),
  0);
  fullscreenQuad->render(gaussShader);
  gaussShader->disable();
  if(i!=npasses -1)
  computeBuffer[(inputBuffer + 1) % 2]->disableRenderTarget();
  inputBuffer = (inputBuffer + 1) % 2;
  }
  }*/

Shader* gaussShaderV;
Shader* gaussShaderH;
void gauss()
{
  for(int i = 0; i < npasses; i++)
    {
      computeBuffer[1]->setRenderTarget();
      gaussShaderH->enable();
      gaussShaderH->bindUniformTexture("colorMap",
				       i == 0 ? texture->getTextureHandle() : computeBuffer[0]->getColorBuffer(0),
				       0);
      fullscreenQuad->render(gaussShaderH);
      gaussShaderH->disable();
      computeBuffer[1]->disableRenderTarget();

      if(i!=npasses -1)
	computeBuffer[0]->setRenderTarget();
      gaussShaderV->enable();
      gaussShaderV->bindUniformTexture("colorMap",
				       computeBuffer[1]->getColorBuffer(0),
				       0);
      fullscreenQuad->render(gaussShaderV);
      gaussShaderV->disable();
      if(i!=npasses -1)
	computeBuffer[0]->disableRenderTarget();
    }
}

Shader* WE_addForce;
Shader* WE_iteration;
Shader* WE_visualize;

Framebuffer* WE_computeBuffer[2];
void waveEquation()
{
  WE_computeBuffer[(inputBuffer + 1) % 2]->setRenderTarget();
  WE_iteration->enable();
  WE_iteration->bindUniformTexture("inputMap", WE_computeBuffer[inputBuffer]->getColorBuffer(0), 0);
  fullscreenQuad->render(WE_iteration);
  WE_iteration->disable();
  WE_computeBuffer[(inputBuffer + 1) % 2]->disableRenderTarget();

  inputBuffer = (inputBuffer + 1) % 2;

  WE_visualize->enable();
  WE_visualize->bindUniformTexture("inputMap", WE_computeBuffer[inputBuffer]->getColorBuffer(0), 0);
  fullscreenQuad->render(WE_visualize);
  WE_visualize->disable();
}
/*
  void waveEquation()
  {
  WE_computeBuffer[1]->setRenderTarget();
  WE_iteration->enable();
  WE_iteration->bindUniformTexture("inputMap", WE_computeBuffer[0]->getColorBuffer(0), 0);
  WE_iteration->bindUniformInt("pass", 0);
  fullscreenQuad->render(WE_iteration);
  WE_iteration->disable();
  WE_computeBuffer[1]->disableRenderTarget();

  WE_computeBuffer[0]->setRenderTarget();
  WE_iteration->enable();
  WE_iteration->bindUniformTexture("inputMap", WE_computeBuffer[1]->getColorBuffer(0), 0);
  WE_iteration->bindUniformInt("pass", 1);
  fullscreenQuad->render(WE_iteration);
  WE_iteration->disable();
  WE_computeBuffer[0]->disableRenderTarget();

  WE_visualize->enable();
  WE_visualize->bindUniformTexture("inputMap", WE_computeBuffer[0]->getColorBuffer(0), 0);
  fullscreenQuad->render(WE_visualize);
  WE_visualize->disable();
  }
*/
void resetWave()
{
  WE_computeBuffer[0]->clear();
  WE_computeBuffer[1]->clear();
}

void addForce(int x, int y){
  WE_computeBuffer[inputBuffer]->setRenderTarget();
  //WE_computeBuffer[0]->setRenderTarget();
  WE_addForce->enable();
  WE_addForce->bindUniformInt2("center", x, 600 - y);
  fullscreenQuad->render(WE_addForce);
  WE_addForce->disable();
  WE_computeBuffer[inputBuffer]->disableRenderTarget();
  //WE_computeBuffer[0]->disableRenderTarget();
}




int example = 1;


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

  if(glGetError() != GL_NO_ERROR){
    std::cout << "Framebuffer: Error creating color attachment0" << std::endl;
  }

  fullscreenQuad = new Quad();
  simpleShader = new Shader("shaders/passthrough.vert", "shaders/simple.frag");
  gaussShader = new Shader("shaders/passthrough.vert", "shaders/gauss.frag");
  gaussShaderH = new Shader("shaders/passthrough.vert", "shaders/gaussH.frag");
  gaussShaderV = new Shader("shaders/passthrough.vert", "shaders/gaussV.frag");
  WE_addForce = new Shader("shaders/passthrough.vert", "shaders/addForce.frag");
  WE_iteration = new Shader("shaders/passthrough.vert", "shaders/iteration3.frag");
  WE_visualize = new Shader("shaders/passthrough.vert", "shaders/visualize.frag");

  texture = new Texture2D();
  texture->loadFromFile(std::string("../common/images/lena.jpg"));

  computeBuffer[0] = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);
  computeBuffer[1] = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);

  WE_computeBuffer[0] = new Framebuffer(600, 600, 1);
  WE_computeBuffer[1] = new Framebuffer(600, 600, 1);

  resetWave();
  addForce(300,300);
}

void display(){
  glClearColor(0.17f, 0.4f, 0.6f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch(example)
    {
    case 1:
      glutReshapeWindow(texture->getWidth(),
			texture->getHeight());
      gauss();
      break;
    case 2:
      glutReshapeWindow(600,600);
      waveEquation();
      break;
    }

  glutSwapBuffers();
}

void mouse(int button, int state, int x, int y){
  if(GLUT_LEFT_BUTTON == button && GLUT_UP == state){
    addForce(x, y);
  }
}

void motion(int x, int y){
  addForce(x, y);
}

void keyboard(unsigned char key, int x, int y){
  switch(key){

  case 27:
    exit(0);
    break;

  case '1':
    example = 1;
    break;

  case '2':
    example = 2;
    break;

  case ' ':
    resetWave();
    break;

  default:
    std::cout << "Unbinded key: " << (unsigned int)key << std::endl;
  }
  inputBuffer = 0;
  glutPostRedisplay();
}

void reshape(int width, int height){
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

int main(int argc, char* argv[]){
  glutInit(&argc, argv);
  glutInitContextVersion (3, 3);
  glutInitContextFlags (GLUT_CORE_PROFILE | GLUT_DEBUG);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("GPGPU 3. labor: glsl");

  init();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glutMainLoop();

  return(0);
}
