#ifndef _VECTOR_
#define _VECTOR_

#include <math.h>

class Vector
{
public:
	union{
		float v[3];
		struct{ float x; float y; float z; };
		struct{ float r; float g; float b; };
		};

	Vector():x(0), y(0), z(0){}
	Vector(	float x, float y, float z):x(x), y(y), z(z){}

	//dot product
	float dot(const Vector& dotProductOperand) const
	{
		return	v[0] * dotProductOperand.v[0] + 
				v[1] * dotProductOperand.v[1] +
				v[2] * dotProductOperand.v[2];
	}

	//cross product
	Vector cross(const Vector& crossProductOperand) const
	{
		return Vector(
			v[1] * crossProductOperand.v[2] - v[2] * crossProductOperand.v[1],
			v[2] * crossProductOperand.v[0] - v[0] * crossProductOperand.v[2],
			v[0] * crossProductOperand.v[1] - v[1] * crossProductOperand.v[0]);
	}

	Vector normalize () const
	{
		double length = sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		if(length <= 0.0)
			return Vector(1, 0, 0);
		double lengthInv = 1.0f / length;
		return Vector(
			v[0] * lengthInv,
			v[1] * lengthInv,
			v[2] * lengthInv);
	}

	Vector operator-() const
	{
		return Vector( -v[0], -v[1], -v[2]);
	}

	Vector operator*(float s) const
	{
		return Vector( v[0] * s, v[1] * s, -v[2] * s);
	}

	void operator+=(const Vector& o)
	{
		v[0] += o.v[0];
		v[1] += o.v[1];
		v[2] += o.v[2];
	}

	void operator-=(const Vector& o)
	{
		v[0] -= o.v[0];
		v[1] -= o.v[1];
		v[2] -= o.v[2];
	}

	Vector rotate(float angle, const Vector& axis)
	{
		float c = cos(angle);
		float s = sin(angle);
		return Vector(
			(c + axis.x * axis.x * (1 - c)			) * v[0] +
			(axis.y * axis.x * (1 - c) - axis.z * s	) * v[1] +
			(axis.z * axis.x * (1 - c) - axis.y * s	) * v[2],

			(axis.x * axis.y * (1 - c) - axis.z * s	) * v[0] +
			(c + axis.y * axis.y * (1 - c)			) * v[1] +
			(axis.z * axis.y * (1 - c) + axis.x * s	) * v[2],

			(axis.x * axis.z * (1 - c) + axis.y * s	) * v[0] +
			(axis.y * axis.z * (1 - c) + axis.x * s	) * v[1] +
			(c + axis.z * axis.z * (1 - c)			) * v[2]
			);
	}
};

#endif
