#ifndef _CAMERA_
#define _CAMERA_

#include "matrix4x4.hpp"

class Camera
{
	float aspectRatio;
	Vector pos;
	Vector forward;
	Vector right;
	Vector up;
	
	float speed;

	int dragStartX;
	int dragStartY;

public:
	void setAspectRatio(float ar) {aspectRatio = ar;}

	void control(float deltaTime, bool* inputs);
	void startDrag(int x, int y);
	void drag(int x, int y);

	Camera(void);

	Matrix4x4 getViewDirMatrix();
	Vector getEye() {return pos;}
};

#endif
