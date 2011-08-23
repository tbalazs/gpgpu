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

#include "quad.hpp"

GLfloat Quad::vertices[18] = {-1.0f, 1.0f, 0.0f,
			     1.0f, 1.0f, 0.0f,
			     -1.0f, -1.0f, 0.0f,
			     -1.0f, -1.0f, 0.0f,
			     1.0f, 1.0f, 0.0f,
			     1.0f, -1.0f, 0.0f};

GLfloat Quad::texCoords[12] = {0.0f, 0.0f,
			       1.0f, 0.0f,
			       0.0f, 1.0f,
			       0.0f, 1.0f,
			       1.0f, 0.0f,
			       1.0f, 1.0f};

Quad::Quad(){
  glGenVertexArrays(1, &vertexArray);
  glBindVertexArray(vertexArray);

  glGenBuffers(1, &vertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 18, vertices, GL_STATIC_DRAW);
  glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &texCoordBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, texCoords, GL_STATIC_DRAW);
  glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glBindVertexArray(0);
}

void Quad::render(Shader* shader){
  glBindVertexArray(vertexArray);

  shader->bindAttribLocation(0, "position");
  shader->bindAttribLocation(1, "texCoord");

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
}
