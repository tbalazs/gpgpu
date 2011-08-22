#ifndef _QUAD_
#define _QUAD_

#include <GL/glew.h>

#include "shader.hpp"

class Quad{
private:
  GLuint vertexArray;

  static GLfloat vertices[18];
  GLuint vertexBuffer;

  static GLfloat texCoords[12];
  GLuint texCoordBuffer;

public:
  Quad();
  ~Quad();

  void render(Shader* shader);
};

#endif
