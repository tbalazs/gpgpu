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

#ifndef _FRAMEBUFFER_
#define _FRAMEBUFFER_

#include <GL/glew.h>

class Framebuffer{
private:
  GLuint handle;
  GLuint* colorBuffer;
  GLuint depthBuffer;

  GLuint width, height;
  GLuint planes;

  GLenum* buffers;

  bool hasMipMaps;
  int numMips;

public:
  Framebuffer(GLuint width, GLuint height, GLuint planes, bool genMipMaps = false, bool floatingpoint = true);
  ~Framebuffer();

  void setRenderTarget(int mipLevel = -1);
  void disableRenderTarget();

  GLuint getColorBuffer(unsigned int plane){
    return colorBuffer[plane];
  }

  void clear();

  int getWidth(){
    return width;
  }

  int getHeight(){
    return height;
  }

  int getLevels(){
	  return numMips;
  }

  GLuint getHandle(){
        return handle;
  }
};

#endif

