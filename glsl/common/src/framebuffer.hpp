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

public:
  Framebuffer(GLuint width, GLuint height, GLuint planes);
  ~Framebuffer();

  void setRenderTarget();
  void disableRenderTarget();

  GLuint getColorBuffer(unsigned int plane){
    return colorBuffer[plane];
  }

  int getWidth(){
    return width;
  }

  int getHeight(){
    return height;
  }
};

#endif

