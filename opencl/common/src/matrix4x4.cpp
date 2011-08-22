#include "matrix4x4.hpp"
#include <string>
#include <GL/glew.h>

Matrix4x4::Matrix4x4(void)
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}


Matrix4x4::~Matrix4x4(void)
{
}

Matrix4x4 Matrix4x4::operator* (const Matrix4x4& other) const
{
	Matrix4x4 result;
	for (int i=0;i<4;i++)
	{
		for (int j=0;j<4;j++)
		{
			result.m[i][j] = m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j]
			+ m[i][2] * other.m[2][j] + m[i][3] * other.m[3][j];
		}
	}
	return result;
}

Matrix4x4 Matrix4x4::invert () const
{
    float a0 = mf[ 0]*mf[ 5] - mf[ 1]*mf[ 4];
    float a1 = mf[ 0]*mf[ 6] - mf[ 2]*mf[ 4];
    float a2 = mf[ 0]*mf[ 7] - mf[ 3]*mf[ 4];
    float a3 = mf[ 1]*mf[ 6] - mf[ 2]*mf[ 5];
    float a4 = mf[ 1]*mf[ 7] - mf[ 3]*mf[ 5];
    float a5 = mf[ 2]*mf[ 7] - mf[ 3]*mf[ 6];
    float b0 = mf[ 8]*mf[13] - mf[ 9]*mf[12];
    float b1 = mf[ 8]*mf[14] - mf[10]*mf[12];
    float b2 = mf[ 8]*mf[15] - mf[11]*mf[12];
    float b3 = mf[ 9]*mf[14] - mf[10]*mf[13];
    float b4 = mf[ 9]*mf[15] - mf[11]*mf[13];
    float b5 = mf[10]*mf[15] - mf[11]*mf[14];

    float det = a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;
    Matrix4x4 inverse;
    inverse.mf[ 0] = + mf[ 5]*b5 - mf[ 6]*b4 + mf[ 7]*b3;
    inverse.mf[ 4] = - mf[ 4]*b5 + mf[ 6]*b2 - mf[ 7]*b1;
    inverse.mf[ 8] = + mf[ 4]*b4 - mf[ 5]*b2 + mf[ 7]*b0;
    inverse.mf[12] = - mf[ 4]*b3 + mf[ 5]*b1 - mf[ 6]*b0;
    inverse.mf[ 1] = - mf[ 1]*b5 + mf[ 2]*b4 - mf[ 3]*b3;
    inverse.mf[ 5] = + mf[ 0]*b5 - mf[ 2]*b2 + mf[ 3]*b1;
    inverse.mf[ 9] = - mf[ 0]*b4 + mf[ 1]*b2 - mf[ 3]*b0;
    inverse.mf[13] = + mf[ 0]*b3 - mf[ 1]*b1 + mf[ 2]*b0;
    inverse.mf[ 2] = + mf[13]*a5 - mf[14]*a4 + mf[15]*a3;
    inverse.mf[ 6] = - mf[12]*a5 + mf[14]*a2 - mf[15]*a1;
    inverse.mf[10] = + mf[12]*a4 - mf[13]*a2 + mf[15]*a0;
    inverse.mf[14] = - mf[12]*a3 + mf[13]*a1 - mf[14]*a0;
    inverse.mf[ 3] = - mf[ 9]*a5 + mf[10]*a4 - mf[11]*a3;
    inverse.mf[ 7] = + mf[ 8]*a5 - mf[10]*a2 + mf[11]*a1;
    inverse.mf[11] = - mf[ 8]*a4 + mf[ 9]*a2 - mf[11]*a0;
    inverse.mf[15] = + mf[ 8]*a3 - mf[ 9]*a1 + mf[10]*a0;

    float invDet = ((float)1)/det;
    inverse.mf[ 0] *= invDet;
    inverse.mf[ 1] *= invDet;
    inverse.mf[ 2] *= invDet;
    inverse.mf[ 3] *= invDet;
    inverse.mf[ 4] *= invDet;
    inverse.mf[ 5] *= invDet;
    inverse.mf[ 6] *= invDet;
    inverse.mf[ 7] *= invDet;
    inverse.mf[ 8] *= invDet;
    inverse.mf[ 9] *= invDet;
    inverse.mf[10] *= invDet;
    inverse.mf[11] *= invDet;
    inverse.mf[12] *= invDet;
    inverse.mf[13] *= invDet;
    inverse.mf[14] *= invDet;
    inverse.mf[15] *= invDet;

    return inverse;
}

Matrix4x4 Matrix4x4::transpose () const
{
	Matrix4x4 transposed;
	for(int u=0; u<4; u++)
		for(int v=0; v<4; v++)
			transposed.m[u][v] = m[v][u];
	return transposed;
}

void Matrix4x4::makeView(const Vector& eye, const Vector& ahead)
{
	static Vector worldUp(0, 1, 0);
	Vector unitAhead = ahead.normalize();
	Vector right = unitAhead.cross(worldUp).normalize();
	Vector up = right.cross(unitAhead);

	m[0][0] = right.x;
	m[0][1] = right.y;
	m[0][2] = right.z;
	m[0][3] = eye.dot(Vector(right.x, up.x, unitAhead.x));
	m[1][0] = up.x;
	m[1][1] = up.y;
	m[1][2] = up.z;
	m[1][3] = eye.dot(Vector(right.y, up.y, unitAhead.y));
	m[2][0] = unitAhead.x;
	m[2][1] = unitAhead.y;
	m[2][2] = unitAhead.z;
	m[2][3] = eye.dot(Vector(right.y, up.y, unitAhead.y));
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
}

void Matrix4x4::makeViewRotation(const Vector& ahead)
{
	static Vector worldUp(0, 1, 0);
	Vector backward = -ahead.normalize();
	Vector right = backward.cross(worldUp).normalize();
	Vector up = right.cross(backward);

	m[0][0] = right.x;
	m[0][1] = right.y;
	m[0][2] = right.z;
	m[0][3] = 0;
	m[1][0] = up.x;
	m[1][1] = up.y;
	m[1][2] = up.z;
	m[1][3] = 0;
	m[2][0] = backward.x;
	m[2][1] = backward.y;
	m[2][2] = backward.z;
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;
}

void Matrix4x4::makeProj(float fov, float aspect, float fp, float bp)
{
	m[0][0] = 1 / (tan(fov * 0.5 ) * aspect);
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;
	m[1][0] = 0;
	m[1][1] = 1 / (tan(fov * 0.5 ) );
	m[1][2] = 0;
	m[1][3] = 0;
	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = -(fp+bp)/(bp-fp);
	m[2][3] = -1;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = -2*fp*bp/(bp-fp);
	m[3][3] = 0;
}

void Matrix4x4::makeScaling(const Vector& factors)
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = factors.x;
	m[1][1] = factors.y;
	m[2][2] = factors.z;
	m[3][3] = 1.0f;
}

void Matrix4x4::makeRotation(const Vector& angles)
{
	float c1 = cos(angles.x); float s1 = sin(angles.x);
	float c2 = cos(angles.z); float s2 = sin(angles.z);
	float c3 = cos(angles.y); float s3 = sin(angles.y);
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = c2*c3;
	m[0][1] = -s2;
	m[0][2] = c2*s3;

	m[1][0] = s1*s3 + c3*c1*s2;
	m[1][1] = c1*c2;
	m[1][2] = c1*s2*s3 - c3*s1;

	m[2][0] = c3*s1*s2 - c1 * s3;
	m[2][1] = c2*s1;
	m[2][2] = s1*s2*s3 + c1*c3;

	m[3][3] = 1.0f;
}

void Matrix4x4::makeTranslation(const Vector& position)
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = 1.0f;
	m[1][1] = 1.0f;
	m[2][2] = 1.0f;
	m[3][0] = position.x;
	m[3][1] = position.y;
	m[3][2] = position.z;
	m[3][3] = 1.0f;
}


Matrix4x4& Matrix4x4::makeQuadricSphere()
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = m[1][1] = m[2][2] = 1.0f;
	m[3][3] = -1.0f;
	return *this;
}

Matrix4x4& Matrix4x4::makeQuadricCylinder()
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = 1.0f;
	m[2][2] = 1.0f;
	m[3][3] = -1.0f;
	return *this;
}

Matrix4x4& Matrix4x4::makeQuadricCone()
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = 1.0f;
	m[1][1] = -1.0f;
	m[2][2] = 1.0f;
	m[3][3] = 0.0f;
	return *this;
}

Matrix4x4& Matrix4x4::makeQuadricParaboloid()
{
	memset(m, 0, sizeof(float) * 16);
	m[0][0] = 1.0f;
	m[1][3] = -1.0f;
	m[2][2] = 1.0f;
	m[3][3] = -1.0f;
	return *this;
}

Matrix4x4& Matrix4x4::quadricScale(const Vector& factors)
{
	Matrix4x4 scaleMatrix;
	scaleMatrix.makeScaling(factors);
	Matrix4x4 scaleMatrixInverse = scaleMatrix.invert();
	(*this) = scaleMatrixInverse * (*this) * scaleMatrixInverse.transpose();
	return *this;
}

Matrix4x4& Matrix4x4::quadricRotate(const Vector& angles)
{
	Matrix4x4 rotMatrix;
	rotMatrix.makeRotation(angles);
	Matrix4x4 rotMatrixInverse = rotMatrix.invert();
	(*this) = rotMatrixInverse * (*this) * rotMatrixInverse.transpose();
	return *this;
}

Matrix4x4& Matrix4x4::quadricTranslate(const Vector& position)
{
	Matrix4x4 trMatrix;
	trMatrix.makeTranslation(position);
	Matrix4x4 trMatrixInverse = trMatrix.invert();
	(*this) = trMatrixInverse * (*this) * trMatrixInverse.transpose();
	return *this;
}
