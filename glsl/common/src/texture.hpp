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

#ifndef _TEXTURE_
#define _TEXTURE_

#include <string>

#include <GL/glew.h>

// DevIL
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

class Texture2D{
private:
  GLuint handle;
  GLuint width, height;
  GLuint bpp;

public:
  Texture2D();
  ~Texture2D();

  int getWidth(){ return width; }
  int getHeight(){ return height; }

  void initialize(GLuint width, GLuint height, GLuint bpp);
  void setData(float* data);
  void loadFromFile(std::string fileName);
  GLuint getTextureHandle();
};

#endif
