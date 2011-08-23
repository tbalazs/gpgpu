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

