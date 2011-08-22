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

