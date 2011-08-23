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

#ifndef _MATRIX4X4_
#define _MATRIX4X4_

#include <cstring>

#include "vector.hpp"

class Matrix4x4
{
	union{
		float m[4][4];
		float mf[16];
	};
public:
	Matrix4x4(void);
	~Matrix4x4(void);


	Matrix4x4 operator* (const Matrix4x4& other) const;
	Matrix4x4 invert () const;
	Matrix4x4 transpose () const;

	void makeView(const Vector& eye, const Vector& ahead);
	void makeViewRotation(const Vector& ahead);
	void makeProj(float fov, float aspect, float fp, float bp);

	void makeScaling(const Vector& factors);
	void makeRotation(const Vector& angles);
	void makeTranslation(const Vector& position);

	Matrix4x4& makeQuadricSphere();
	Matrix4x4& makeQuadricCylinder();
	Matrix4x4& makeQuadricCone();
	Matrix4x4& makeQuadricParaboloid();
	Matrix4x4& quadricScale(const Vector& factors);
	Matrix4x4& quadricRotate(const Vector& angles);
	Matrix4x4& quadricTranslate(const Vector& position);

	float* getPointer() {return mf;}
};

#endif
