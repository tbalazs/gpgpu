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
