#include "camera.hpp"
#include <string>
#include <stdio.h>
#include <GL/glew.h>

Camera::Camera(void):
	aspectRatio(1),
	pos(0.0f, 0.0f, -4.0f),
	forward(0,0,1),
	right(-1,0,0),
	up(0,1,0),
	speed(10)
{
  dragStartX = 0;
  dragStartY = 0;
}

void Camera::control(float deltaTime, bool* inputs)
{
  // Camera controls
  if(inputs['w']) { pos += forward * speed * deltaTime; }
  if(inputs['s']) { pos -= forward * speed * deltaTime; }
  if(inputs['a']) { pos += right * speed * deltaTime; }
  if(inputs['d']) { pos -= right * speed * deltaTime; }
  if(inputs['e']) { pos += up * speed * deltaTime; }
  if(inputs['q']) { pos -= up * speed * deltaTime; }
  if(inputs['p']) { printf("P: %f, %f, %f F: %f, %f, %f\n", pos.x, pos.y, pos.z, forward.x, forward.y, forward.z); }
}


void Camera::startDrag(int x, int y)
{
  dragStartX = x;
  dragStartY = y;
}

void Camera::drag(int x, int y)
{
  int dx = dragStartX - x;
  int dy = dragStartY - y;

  forward = forward.normalize();
  right = forward.cross(Vector(0, 1, 0));
  up = right.cross(forward);

  forward = forward.rotate(dx * 0.003, Vector(0,1,0));
  forward = forward.rotate(-dy * 0.003, right);

  dragStartX = x;
  dragStartY = y;
}

Matrix4x4 Camera::getViewDirMatrix()
{
  Matrix4x4 viewRotationMatrix, projMatrix;
  viewRotationMatrix.makeViewRotation(forward);
  projMatrix.makeProj(1.5, aspectRatio, 0.1, 1000);
  //  return (projMatrix * viewRotationMatrix).invert();
  return viewRotationMatrix.invert();
}
