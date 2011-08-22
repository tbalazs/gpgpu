#include "framebuffer.hpp"

#include <string>
#include <iostream>

Framebuffer::Framebuffer(GLuint width, GLuint height, GLuint planes){
  this->width = width;
  this->height = height;
  this->planes = planes;

  buffers = new GLenum[planes];

  glGenFramebuffers(1, &handle);
  colorBuffer = new GLuint[planes];
  glGenTextures(planes, colorBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, handle);

  for(unsigned int i = 0; i < planes; ++i){
    glBindTexture(GL_TEXTURE_2D, colorBuffer[i]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffer[i], 0);
    buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    if(glGetError() != GL_NO_ERROR){
      std::cout << "Framebuffer: Error creating color attachment" << std::endl;
    }
  }

  glGenRenderbuffers(1, &depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
  if(glGetError() != GL_NO_ERROR){
    std::cout << "Framebuffer: Error creating depth attachment" << std::endl;
  }

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(status != GL_FRAMEBUFFER_COMPLETE){
    std::cout << "Framebuffer: Incomplete framebuffer (";
    switch(status){
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      std::cout << "GL_FRAMEBUFFER_UNSUPPORTED";
      break;
    }
    std::cout << ")" << std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer(){
  glDeleteFramebuffers(1, &handle);
  glDeleteRenderbuffers(1, &depthBuffer);
}

void Framebuffer::setRenderTarget(){
  glBindFramebuffer(GL_FRAMEBUFFER, handle);
  glDrawBuffers(planes, buffers);
  glViewport(0,0, width, height);
}

void Framebuffer::disableRenderTarget(){
  GLenum tmpBuff[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(1, tmpBuff);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::clear(){
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearDepth(1.0f);
  setRenderTarget();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  disableRenderTarget();
}
