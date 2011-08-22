#include <iostream>

#include "texture.hpp"

Texture2D::Texture2D(){
}

Texture2D::~Texture2D(){
}

void Texture2D::initialize(GLuint width, GLuint height, GLuint bpp){
  this->width = width;
  this->height = height;
  this->bpp = bpp;

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::setData(float* data){
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);
}

void Texture2D::loadFromFile(std::string fileName){
  ilInit();
  ILuint devilError = ilGetError();
  if(IL_NO_ERROR != devilError){
    std::cout << iluErrorString(devilError)<< std::endl;
  }

  ILuint imageHandle;
  ilGenImages(1, &imageHandle);
  ilBindImage(imageHandle);
  ilLoadImage(fileName.c_str());
  if(IL_NO_ERROR != devilError){
    std::cout << iluErrorString(devilError)<< std::endl;
  }

  width = (unsigned int)ilGetInteger(IL_IMAGE_WIDTH);
  height = (unsigned int)ilGetInteger(IL_IMAGE_HEIGHT);
  bpp = (unsigned int)ilGetInteger(IL_IMAGE_BITS_PER_PIXEL);

  ILuint dataSize = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);

  float* buffer = new float[width*height*4];
  ilCopyPixels(0,0,0, width, height, 1, IL_RGBA, IL_FLOAT, buffer);

  initialize(width, height, bpp);
  setData(buffer);

  ilDeleteImages(1, &imageHandle);
}

GLuint Texture2D::getTextureHandle(){
  return handle;
}

