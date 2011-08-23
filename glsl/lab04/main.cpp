#include <GL/glew.h>
#include <GL/freeglut.h>

#include <iostream>
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

int windowWidth = 600;
int windowHeight = 600;

int example = 1;

Shader* histogramShader;
Texture2D* texture;
PointGrid* grid;
Framebuffer* histogramBuffer;

void histogram(){
  histogramBuffer->clear();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  histogramBuffer->setRenderTarget();
  histogramShader->enable();
  histogramShader->bindUniformTexture("inputBuffer", texture->getTextureHandle(), 0);
  grid->render(histogramShader);
  histogramShader->disable();
  histogramBuffer->disableRenderTarget();
  glDisable(GL_BLEND);

  glFinish();

  float histogram[255] = {0.0f};
  glBindFramebuffer(GL_FRAMEBUFFER, histogramBuffer->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, 255, 1, GL_RED, GL_FLOAT, histogram);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // std::cout << "Histogram:\n----------" << std::endl;
  int maxValue = 0;
  for(int i = 0; i < 255; ++i){
    maxValue = maxValue > histogram[i] ? maxValue : histogram[i];
    // std::cout << i << " : " << histogram[i] << std::endl;
  }
  // std::cout << "-----------------\nMaximum: " << maxValue << std::endl;

  glViewport(0, 0, windowWidth, windowHeight);
  simpleShader->enable();
  simpleShader->bindUniformTexture("inputBuffer", histogramBuffer->getColorBuffer(0), 0);
  simpleShader->bindUniformTexture("original", texture->getTextureHandle(), 1);
  simpleShader->bindUniformInt("maxValue", maxValue);
  fullscreenQuad->render(simpleShader);
  simpleShader->disable();

  glFinish();
}

Framebuffer* U_histogramBuffer = NULL;
Framebuffer* U_cumulatedHistogramBuffer[2] = {NULL};
Framebuffer* U_equalizedBuffer = NULL;
Shader* computeHistogram = NULL;
Shader* computeCumulativeHistogram = NULL;
Shader* computeEqualizedHistogram = NULL;

Shader* U_histogramShader = NULL;
Shader* U_visualizeShader = NULL;

const int histogramLevels = 256;

void histogramEqualization(){
  if(computeHistogram == NULL){
    computeHistogram = new Shader("shaders/computeHistogram.vert", "shaders/computeHistogram.frag");
    computeCumulativeHistogram = new Shader("shaders/passthrough.vert", "shaders/computeCumulativeHistogram.frag");
    computeEqualizedHistogram = new Shader("shaders/passthrough.vert", "shaders/computeEqualizedHistogram.frag");

    U_histogramShader = new Shader("shaders/uhistogram.vert", "shaders/computeHistogram.frag");
    U_visualizeShader = new Shader("shaders/passthrough.vert", "shaders/uvisualize.frag");
  }
  if(U_histogramBuffer == NULL){
    U_histogramBuffer = new Framebuffer(histogramLevels, 1, 1);
    U_equalizedBuffer = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);
  }
  if(U_cumulatedHistogramBuffer[0] == NULL){
    U_cumulatedHistogramBuffer[0] = new Framebuffer(histogramLevels, 1, 1);
    U_cumulatedHistogramBuffer[1] = new Framebuffer(histogramLevels, 1, 1);
  }

  U_histogramBuffer->clear();
  U_cumulatedHistogramBuffer[0]->clear();
  U_cumulatedHistogramBuffer[1]->clear();

  // generate histogram
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  U_histogramBuffer->setRenderTarget();
  computeHistogram->enable();
  computeHistogram->bindUniformTexture("inputMap", texture->getTextureHandle(), 0);
  computeHistogram->bindUniformFloat("incr", 1.0 );
  computeHistogram->bindUniformFloat("histogramLevels", histogramLevels);
  grid->render(computeHistogram);
  computeHistogram->disable();
  U_histogramBuffer->disableRenderTarget();
  glDisable(GL_BLEND);

  // compute cumulative histogram
  float offset = 1.0f / (float)histogramLevels;
  int inputBuffer = 0;
  for(int i=0; i < 8; ++i){
    U_cumulatedHistogramBuffer[(inputBuffer + 1) % 2]->setRenderTarget();
    computeCumulativeHistogram->enable();
    computeCumulativeHistogram->bindUniformTexture("inputMap",
    						   0 == i ? U_histogramBuffer->getColorBuffer(0) : U_cumulatedHistogramBuffer[inputBuffer]->getColorBuffer(0), 0);
    computeCumulativeHistogram->bindUniformFloat("offset", -offset);
    fullscreenQuad->render(computeCumulativeHistogram);
    computeCumulativeHistogram->disable();
    U_cumulatedHistogramBuffer[(inputBuffer + 1) % 2]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;
    offset *= 2.0f;
  }

  float sat[histogramLevels] = {0.0f};
  glBindFramebuffer(GL_FRAMEBUFFER, U_cumulatedHistogramBuffer[inputBuffer]->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, histogramLevels, 1, GL_RED, GL_FLOAT, sat);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  U_equalizedBuffer->clear();
  U_equalizedBuffer->setRenderTarget();
  computeEqualizedHistogram->enable();
  computeEqualizedHistogram->bindUniformTexture("inputMap", texture->getTextureHandle(), 0);
  computeEqualizedHistogram->bindUniformTexture("histogram", U_cumulatedHistogramBuffer[inputBuffer]->getColorBuffer(0), 1);
  computeEqualizedHistogram->bindUniformFloat("level", (texture->getWidth() * texture->getHeight() / ((float)histogramLevels / 8.0f)));
  computeEqualizedHistogram->bindUniformFloat("numLevel", histogramLevels / 8.0f);
  fullscreenQuad->render(computeEqualizedHistogram);
  computeEqualizedHistogram->disable();
  U_equalizedBuffer->disableRenderTarget();

  // -------------------------------------------------------------------------------------
  U_histogramBuffer->clear();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  U_histogramBuffer->setRenderTarget();
  computeHistogram->enable();
  computeHistogram->bindUniformTexture("inputMap",U_equalizedBuffer->getColorBuffer(0), 0);
  computeHistogram->bindUniformFloat("incr", 1.0 );
  computeHistogram->bindUniformFloat("histogramLevels", histogramLevels);
  grid->render(computeHistogram);
  computeHistogram->disable();
  U_histogramBuffer->disableRenderTarget();
  glDisable(GL_BLEND);

  glFinish();

  float histogram[255] = {0.0f};
  glBindFramebuffer(GL_FRAMEBUFFER, U_histogramBuffer->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, 255, 1, GL_RED, GL_FLOAT, histogram);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // std::cout << "Histogram:\n----------" << std::endl;
  int maxValue = 0;
  for(int i = 0; i < 255; ++i){
    maxValue = maxValue > histogram[i] ? maxValue : histogram[i];
    // std::cout << i << " : " << histogram[i] << std::endl;
  }
  // std::cout << "-----------------\nMaximum: " << maxValue << std::endl;

  glViewport(0, 0, windowWidth, windowHeight);
  U_visualizeShader->enable();
  U_visualizeShader->bindUniformTexture("inputBuffer", U_histogramBuffer->getColorBuffer(0), 0);
  U_visualizeShader->bindUniformTexture("original", U_equalizedBuffer->getColorBuffer(0), 1);
  U_visualizeShader->bindUniformInt("maxValue", maxValue);
  fullscreenQuad->render(U_visualizeShader);
  U_visualizeShader->disable();

  glFinish();
}

Shader* quantizeShader;
Framebuffer* quantizedBuffer;
int levels = 16;

void quantizeAndHistogram(){
  quantizedBuffer->setRenderTarget();
  quantizeShader->enable();
  quantizeShader->bindUniformTexture("inputBuffer", texture->getTextureHandle(), 0);
  quantizeShader->bindUniformInt("levels", levels);
  fullscreenQuad->render(quantizeShader);
  quantizeShader->disable();
  quantizedBuffer->disableRenderTarget();

  histogramBuffer->clear();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  histogramBuffer->setRenderTarget();
  histogramShader->enable();
  histogramShader->bindUniformTexture("inputBuffer", quantizedBuffer->getColorBuffer(0), 0);
  grid->render(histogramShader);
  histogramShader->disable();
  histogramBuffer->disableRenderTarget();
  glDisable(GL_BLEND);

  glFinish();

  float histogram[255] = {0.0f};
  glBindFramebuffer(GL_FRAMEBUFFER, histogramBuffer->getHandle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0, 255, 1, GL_RED, GL_FLOAT, histogram);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // std::cout << "Histogram:\n----------" << std::endl;
  int maxValue = 0;
  for(int i = 0; i < 255; ++i){
    maxValue = maxValue > histogram[i] ? maxValue : histogram[i];
    // std::cout << i << " : " << histogram[i] << std::endl;
  }
  // std::cout << "-----------------\nMaximum: " << maxValue << std::endl;

  glViewport(0, 0, windowWidth, windowHeight);
  simpleShader->enable();
  simpleShader->bindUniformTexture("inputBuffer", histogramBuffer->getColorBuffer(0), 0);
  simpleShader->bindUniformTexture("original", quantizedBuffer->getColorBuffer(0), 1);
  simpleShader->bindUniformInt("maxValue", maxValue);
  fullscreenQuad->render(simpleShader);
  simpleShader->disable();

  glFinish();
}

Shader* satXShader;
Shader* satYShader;
Shader* satVisShader;
Framebuffer* satBuffer[2];
void sat(){
  satXShader = new Shader("shaders/passthrough.vert", "shaders/satx.frag");
  satYShader = new Shader("shaders/passthrough.vert", "shaders/saty.frag");
  satVisShader = new Shader("shaders/passthrough.vert", "shaders/satvis.frag");
  satBuffer[0] = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);
  satBuffer[1] = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);
  satBuffer[0]->clear();
  satBuffer[1]->clear();

  int inputBuffer = 0;
  float offset = 1.0f / (float)texture->getWidth();
  int passes = ceil(log((float)texture->getWidth()) / log(2.0f));

  for(int i = 0; i < passes; ++i){
    satBuffer[(inputBuffer + 1) % 2 ]->setRenderTarget();
    satXShader->enable();
    satXShader->bindUniformTexture("inputMap", i == 0 ? texture->getTextureHandle() : satBuffer[inputBuffer]->getColorBuffer(0), 0);
    satXShader->bindUniformFloat("offset", -offset);
    fullscreenQuad->render(satXShader);
    satXShader->disable();
    satBuffer[(inputBuffer + 1) % 2 ]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;

    satBuffer[(inputBuffer + 1) % 2 ]->setRenderTarget();
    satYShader->enable();
    satYShader->bindUniformTexture("inputMap", satBuffer[inputBuffer]->getColorBuffer(0), 0);
    satYShader->bindUniformFloat("offset", -offset);
    fullscreenQuad->render(satYShader);
    satYShader->disable();
    satBuffer[(inputBuffer + 1) % 2 ]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;

    offset *= 2.0f;
  }

  glViewport(0, 0, texture->getWidth(), texture->getHeight());
  satVisShader->enable();
  satVisShader->bindUniformTexture("inputMap", satBuffer[inputBuffer]->getColorBuffer(0), 0);
  fullscreenQuad->render(satVisShader);
  satVisShader->disable();
}

Texture2D* hdrTexture;
Framebuffer* satHDRBuffer[2] = {NULL, NULL};
Framebuffer* hdrLumBuffer = NULL;
Framebuffer* hdrRelLumBuffer = NULL;
Shader* hdrLuminanceShader = NULL;
Shader* hdrSatXShader = NULL;
Shader* hdrSatYShader = NULL;
Shader* hdrRelLumShader = NULL;
Shader* hdrToneMapShader = NULL;

float W = 0.9f;
float alpha = 0.5f;

void toneMap(){
  if(satHDRBuffer[0] == NULL){
    satHDRBuffer[0] = new Framebuffer(hdrTexture->getWidth(), hdrTexture->getHeight(), 1);
    satHDRBuffer[1] = new Framebuffer(hdrTexture->getWidth(), hdrTexture->getHeight(), 1);
    hdrLumBuffer = new Framebuffer(hdrTexture->getWidth(), hdrTexture->getHeight(), 1);
    hdrRelLumBuffer = new Framebuffer(hdrTexture->getWidth(), hdrTexture->getHeight(), 1);
  }

  if(hdrSatXShader == NULL){
    hdrSatXShader = new Shader("shaders/passthrough.vert", "shaders/satx.frag");
    hdrSatYShader = new Shader("shaders/passthrough.vert", "shaders/saty.frag");
  }

  if(hdrToneMapShader == NULL){
    hdrToneMapShader = new Shader("shaders/passthrough.vert", "shaders/tonemap.frag");
    hdrLuminanceShader = new Shader("shaders/passthrough.vert", "shaders/luminance.frag");
    hdrRelLumShader = new Shader("shaders/passthrough.vert", "shaders/rellum.frag");
  }

  // generate luminance
  hdrLumBuffer->clear();
  hdrLumBuffer->setRenderTarget();
  hdrLuminanceShader->enable();
  hdrLuminanceShader->bindUniformTexture("hdr", hdrTexture->getTextureHandle(), 0);
  fullscreenQuad->render(hdrLuminanceShader);
  hdrLuminanceShader->disable();
  hdrLumBuffer->disableRenderTarget();

  // generate SAT
  float offset = 1.0f / (float)hdrTexture->getWidth();
  int passes = ceil(log((float)hdrTexture->getWidth()) / log(2.0f));
  int inputBuffer = 0;

  for(int i = 0; i < passes; ++i){
    satHDRBuffer[(inputBuffer + 1) % 2 ]->setRenderTarget();
    hdrSatXShader->enable();
    hdrSatXShader->bindUniformTexture("inputMap", i == 0 ? hdrLumBuffer->getColorBuffer(0) : satHDRBuffer[inputBuffer]->getColorBuffer(0), 0);
    hdrSatXShader->bindUniformFloat("offset", -offset);
    fullscreenQuad->render(hdrSatXShader);
    hdrSatXShader->disable();
    satHDRBuffer[(inputBuffer + 1) % 2 ]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;

    satHDRBuffer[(inputBuffer + 1) % 2 ]->setRenderTarget();
    hdrSatYShader->enable();
    hdrSatYShader->bindUniformTexture("inputMap", satHDRBuffer[inputBuffer]->getColorBuffer(0), 0);
    hdrSatYShader->bindUniformFloat("offset", -offset);
    fullscreenQuad->render(hdrSatYShader);
    hdrSatYShader->disable();
    satHDRBuffer[(inputBuffer + 1) % 2 ]->disableRenderTarget();
    inputBuffer = (inputBuffer + 1) % 2;

    offset *= 2.0f;
  }

  hdrRelLumBuffer->clear();
  hdrRelLumBuffer->setRenderTarget();
  hdrRelLumShader->enable();
  hdrRelLumShader->bindUniformTexture("luminanceMap", hdrLumBuffer->getColorBuffer(0), 0);
  hdrRelLumShader->bindUniformTexture("sat", satHDRBuffer[inputBuffer]->getColorBuffer(0), 1);
  hdrRelLumShader->bindUniformFloat("alpha", alpha);
  fullscreenQuad->render(hdrRelLumShader);
  hdrRelLumShader->disable();
  hdrRelLumBuffer->disableRenderTarget();

  hdrToneMapShader->enable();
  hdrToneMapShader->bindUniformTexture("hdr", hdrTexture->getTextureHandle(), 0);
  hdrToneMapShader->bindUniformTexture("rellum", hdrRelLumBuffer->getColorBuffer(0), 1);
  hdrToneMapShader->bindUniformFloat("W", W);
  fullscreenQuad->render(hdrToneMapShader);
  hdrToneMapShader->disable();

}

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
  histogramShader = new Shader("shaders/histogram.vert", "shaders/histogram.frag");
  texture = new Texture2D();
  texture->loadFromFile(std::string("../common/images/lena.jpg"));

  grid = new PointGrid(texture->getWidth(), texture->getHeight());

  histogramBuffer = new Framebuffer(255, 1, 1);

  quantizedBuffer = new Framebuffer(texture->getWidth(), texture->getHeight(), 1);
  quantizeShader = new Shader("shaders/passthrough.vert", "shaders/quantize.frag");

  hdrTexture = new Texture2D();
  hdrTexture->loadFromFile(std::string("../common/images/free_005.hdr"));

  glutReshapeWindow(texture->getWidth(), texture->getHeight());
}

void display(){
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch(example){
  case 1:
    histogram();
    break;

  case 2:
    quantizeAndHistogram();
    break;

  case 3:
    histogramEqualization();
    break;

  case 4:
    sat();
    break;

  case 5:
    toneMap();
    break;
  }

  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y){
  switch(key){

  case 27:
    exit(0);
    break;

  case '1':
    windowWidth = texture->getWidth();
    windowHeight = texture->getHeight();
    glutReshapeWindow(windowWidth, windowHeight);
    example = 1;
    break;

  case '2':
    windowWidth = texture->getWidth();
    windowHeight = texture->getHeight();
    glutReshapeWindow(windowWidth, windowHeight);
    example = 2;
    break;

  case '3':
    windowWidth = texture->getWidth();
    windowHeight = texture->getHeight();
    glutReshapeWindow(windowWidth, windowHeight);
    example = 3;
    break;

  case '4':
    windowWidth = texture->getWidth();
    windowHeight = texture->getHeight();
    glutReshapeWindow(windowWidth, windowHeight);
    example = 4;
    break;

  case '5':
    windowWidth = hdrTexture->getWidth();
    windowHeight = hdrTexture->getHeight();
    glutReshapeWindow(windowWidth, windowHeight);
    example = 5;
    break;


  case ']':
    levels += 1;
    break;

  case '[':
    levels = (levels - 1) > 1 ? levels-1 : 1;
    break;

  case 'w':
    W -= 0.01f;
    break;
  case 'W':
    W += 0.01f;
    break;
  case 'a':
    alpha -= 0.01f;
    break;
  case 'A':
    alpha += 0.01f;
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

}

int main(int argc, char* argv[]){
  glutInit(&argc, argv);
  glutInitContextVersion(3, 3);
  glutInitContextProfile(GLUT_CORE_PROFILE | GLUT_DEBUG);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(windowWidth, windowHeight);
  glutCreateWindow("GPGPU 4. labor: glsl");

  init();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  glutMainLoop();

  return(0);
}
