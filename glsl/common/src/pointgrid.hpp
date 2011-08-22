#ifndef _POINTGRID_
#define _POINTGRID_

#include <GL/glew.h>

#include "shader.hpp"

class PointGrid{
private:
  int width;
  int height;

  GLuint vertexArray;

public:
  PointGrid(int width, int height);
  ~PointGrid();

  void render(Shader* shader);
};

#endif
