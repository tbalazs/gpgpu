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
