#include "framebuffer.hpp"

#include <string>
#include <iostream>
#include <cmath>

// Calculates log2 of number.  
float Log2( float n )  
{  
    // log(n)/log(2) is log2.  
    return log( n ) / log( 2.0f );  
}

Framebuffer::Framebuffer(GLuint width, GLuint height, GLuint planes, bool genMipMaps, bool floatingpoint){
  this->width = width;
  this->height = height;
  this->planes = planes;
  hasMipMaps = genMipMaps;

  if(width != height)
	  hasMipMaps = false;
  else  {
	float fnumMips = Log2((float)width);
	if(fmod(fnumMips, 1.0f) != 0)
		hasMipMaps = false;
	else 
		numMips = fnumMips + 1;
  }

  if(genMipMaps && !hasMipMaps)
	  std::cout << "Framebuffer: Generation of mipmaps failed. The texture is not square, or not power of 2!" << std::endl;

  buffers = new GLenum[planes];

  glGenFramebuffers(1, &handle);
  colorBuffer = new GLuint[planes];
  glGenTextures(planes, colorBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, handle);

  for(unsigned int i = 0; i < planes; ++i){
    glBindTexture(GL_TEXTURE_2D, colorBuffer[i]);
	if(hasMipMaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	
	}
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if(floatingpoint)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGetError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffer[i], 0);
    buffers[i] = GL_COLOR_ATTACHMENT0 + i;
    if(glGetError() != GL_NO_ERROR){
      std::cout << "Framebuffer: Error creating color attachment" << std::endl;
    }

	if(hasMipMaps)
	{
		int size = width;
		for(int l = 1; l < numMips; l++)
		{
			size = size / 2;
			if(floatingpoint)
				glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA32F, size, size, 0, GL_RGBA, GL_FLOAT, NULL);
			else
				glTexImage2D(GL_TEXTURE_2D, l, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}
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

void Framebuffer::setRenderTarget(int mipLevel)
{
  glBindFramebuffer(GL_FRAMEBUFFER, handle);
  
  if(mipLevel >= 0 && hasMipMaps)
  {
	  for(int i = 0; i < planes; i++)
	  {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffer[i], mipLevel);
	  }
	  int texsize = pow(2.0f, numMips - 1 - mipLevel);
	  glViewport(0,0, texsize, texsize);
  }
  else
	  glViewport(0,0, width, height);
  
  glDrawBuffers(planes, buffers);  
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
