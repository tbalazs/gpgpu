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
