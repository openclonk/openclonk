/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include <C4Include.h>

#ifdef _MSC_VER
# define _USE_MATH_DEFINES
# include <math.h>
#endif

#include <StdMeshMath.h>

StdMeshVector StdMeshVector::Zero()
{
	StdMeshVector v;
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 0.0f;
	return v;
}

StdMeshVector StdMeshVector::UnitScale()
{
	StdMeshVector v;
	v.x = 1.0f;
	v.y = 1.0f;
	v.z = 1.0f;
	return v;
}

StdMeshVector StdMeshVector::Translate(float dx, float dy, float dz)
{
	StdMeshVector v;
	v.x = dx;
	v.y = dy;
	v.z = dz;
	return v;
}

StdMeshVector StdMeshVector::Cross(const StdMeshVector& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v;
	v.x = lhs.y*rhs.z - lhs.z*rhs.y;
	v.y = lhs.z*rhs.x - lhs.x*rhs.z;
	v.z = lhs.x*rhs.y - lhs.y*rhs.x;
	return v;
}

void StdMeshVector::Normalize()
{
	const float len = sqrt(x*x + y*y + z*z);
	x /= len; y /= len; z /= len;
}

StdMeshQuaternion StdMeshQuaternion::Zero()
{
	StdMeshQuaternion q;
	q.w = 0.0f;
	q.x = 0.0f;
	q.y = 0.0f;
	q.z = 0.0f;
	return q;
}

StdMeshQuaternion StdMeshQuaternion::AngleAxis(float theta, const StdMeshVector& axis)
{
	StdMeshQuaternion q;
	const float theta2 = theta/2.0f;
	const float s = sin(theta2);
	q.w = cos(theta2);
	q.x = s*axis.x;
	q.y = s*axis.y;
	q.z = s*axis.z;
	return q;
}

void StdMeshQuaternion::Normalize()
{
	float length = sqrt(LenSqr());
	w /= length;
	x /= length;
	y /= length;
	z /= length;
}

StdMeshQuaternion StdMeshQuaternion::Nlerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w)
{
	StdMeshQuaternion q;
	float c = lhs.w * rhs.w + lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	if (c < 0.0f)
		q = lhs + w * (-rhs - lhs);
	else
		q = lhs + w * ( rhs - lhs);
	q.Normalize();
	return q;
}

StdMeshTransformation StdMeshTransformation::Zero()
{
	StdMeshTransformation t;
	t.scale = StdMeshVector::Zero();
	t.rotate = StdMeshQuaternion::Zero();
	t.translate = StdMeshVector::Zero();
	return t;
}

StdMeshTransformation StdMeshTransformation::Identity()
{
	StdMeshTransformation t;
	t.scale = StdMeshVector::UnitScale();
	t.rotate.w = 1.0f;
	t.rotate.x = t.rotate.y = t.rotate.z = 0.0f;
	t.translate = StdMeshVector::Zero();
	return t;
}

StdMeshTransformation StdMeshTransformation::Inverse(const StdMeshTransformation& transform)
{
	StdMeshTransformation t;
	t.scale = 1.0f/transform.scale;
	t.rotate.w = transform.rotate.w;
	t.rotate.x = -transform.rotate.x;
	t.rotate.y = -transform.rotate.y;
	t.rotate.z = -transform.rotate.z;
	t.translate = t.rotate * (t.scale * -transform.translate);
	return t;
}

StdMeshTransformation StdMeshTransformation::Translate(float dx, float dy, float dz)
{
	StdMeshTransformation t;
	t.scale = StdMeshVector::UnitScale();
	t.rotate.w = 1.0f;
	t.rotate.x = t.rotate.y = t.rotate.z = 0.0f;
	t.translate = StdMeshVector::Translate(dx, dy, dz);
	return t;
}

StdMeshTransformation StdMeshTransformation::Scale(float sx, float sy, float sz)
{
	StdMeshTransformation t;
	t.scale = StdMeshVector::Translate(sx, sy, sz);
	t.rotate.w = 1.0f;
	t.rotate.x = t.rotate.y = t.rotate.z = 0.0f;
	t.translate = StdMeshVector::Zero();
	return t;
}

StdMeshTransformation StdMeshTransformation::Rotate(float angle, float rx, float ry, float rz)
{
	StdMeshTransformation t;
	t.scale = StdMeshVector::UnitScale();
	t.rotate = StdMeshQuaternion::AngleAxis(angle, StdMeshVector::Translate(rx, ry, rz));
	t.translate = StdMeshVector::Zero();
	return t;
}

StdMeshTransformation StdMeshTransformation::Nlerp(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs, float w)
{
	StdMeshTransformation t;
	t.translate = (1 - w) * lhs.translate + w * rhs.translate;
	t.rotate = StdMeshQuaternion::Nlerp(lhs.rotate, rhs.rotate, w);
	t.scale = (1 - w) * lhs.scale + w * rhs.scale;
	return t;
}

StdMeshMatrix StdMeshMatrix::Zero()
{
	StdMeshMatrix m;
	m.a[0][0] = 0.0f; m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = 0.0f;
	m.a[1][0] = 0.0f; m.a[1][1] = 0.0f; m.a[1][2] = 0.0f; m.a[1][3] = 0.0f;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = 0.0f; m.a[2][3] = 0.0f;
	return m;
}

StdMeshMatrix StdMeshMatrix::Identity()
{
	StdMeshMatrix m;
	m.a[0][0] = 1.0f; m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = 0.0f;
	m.a[1][0] = 0.0f; m.a[1][1] = 1.0f; m.a[1][2] = 0.0f; m.a[1][3] = 0.0f;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = 1.0f; m.a[2][3] = 0.0f;
	return m;
}

StdMeshMatrix StdMeshMatrix::Inverse(const StdMeshMatrix& mat)
{
	StdMeshMatrix m;

	const float det = mat.Determinant();
	assert(det != 0.0f);

	m.a[0][0] = (mat.a[1][1]*mat.a[2][2] - mat.a[1][2]*mat.a[2][1]) / det;
	m.a[1][0] = (mat.a[1][2]*mat.a[2][0] - mat.a[1][0]*mat.a[2][2]) / det;
	m.a[2][0] = (mat.a[1][0]*mat.a[2][1] - mat.a[1][1]*mat.a[2][0]) / det;

	m.a[0][1] = (mat.a[0][2]*mat.a[2][1] - mat.a[0][1]*mat.a[2][2]) / det;
	m.a[1][1] = (mat.a[0][0]*mat.a[2][2] - mat.a[0][2]*mat.a[2][0]) / det;
	m.a[2][1] = (mat.a[0][1]*mat.a[2][0] - mat.a[0][0]*mat.a[2][1]) / det;

	m.a[0][2] = (mat.a[0][1]*mat.a[1][2] - mat.a[0][2]*mat.a[1][1]) / det;
	m.a[1][2] = (mat.a[0][2]*mat.a[1][0] - mat.a[0][0]*mat.a[1][2]) / det;
	m.a[2][2] = (mat.a[0][0]*mat.a[1][1] - mat.a[0][1]*mat.a[1][0]) / det;

	m.a[0][3] = (mat.a[0][1]*mat.a[1][3]*mat.a[2][2]
	             +  mat.a[0][2]*mat.a[1][1]*mat.a[2][3]
	             +  mat.a[0][3]*mat.a[1][2]*mat.a[2][1]
	             -  mat.a[0][1]*mat.a[1][2]*mat.a[2][3]
	             -  mat.a[0][2]*mat.a[1][3]*mat.a[2][1]
	             -  mat.a[0][3]*mat.a[1][1]*mat.a[2][2]) / det;

	m.a[1][3] = (mat.a[0][0]*mat.a[1][2]*mat.a[2][3]
	             +  mat.a[0][2]*mat.a[1][3]*mat.a[2][0]
	             +  mat.a[0][3]*mat.a[1][0]*mat.a[2][2]
	             -  mat.a[0][0]*mat.a[1][3]*mat.a[2][2]
	             -  mat.a[0][2]*mat.a[1][0]*mat.a[2][3]
	             -  mat.a[0][3]*mat.a[1][2]*mat.a[2][0]) / det;

	m.a[2][3] = (mat.a[0][0]*mat.a[1][3]*mat.a[2][1]
	             +  mat.a[0][1]*mat.a[1][0]*mat.a[2][3]
	             +  mat.a[0][3]*mat.a[1][1]*mat.a[2][0]
	             -  mat.a[0][0]*mat.a[1][1]*mat.a[2][3]
	             -  mat.a[0][1]*mat.a[1][3]*mat.a[2][0]
	             -  mat.a[0][3]*mat.a[1][0]*mat.a[2][1]) / det;

	return m;
}

StdMeshMatrix StdMeshMatrix::Translate(float dx, float dy, float dz)
{
	StdMeshMatrix m;
	m.a[0][0] = 1.0f; m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = dx;
	m.a[1][0] = 0.0f; m.a[1][1] = 1.0f; m.a[1][2] = 0.0f; m.a[1][3] = dy;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = 1.0f; m.a[2][3] = dz;
	return m;
}

StdMeshMatrix StdMeshMatrix::Scale(float sx, float sy, float sz)
{
	StdMeshMatrix m;
	m.a[0][0] = sx;   m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = 0.0f;
	m.a[1][0] = 0.0f; m.a[1][1] = sy;   m.a[1][2] = 0.0f; m.a[1][3] = 0.0f;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = sz;   m.a[2][3] = 0.0f;
	return m;
}

StdMeshMatrix StdMeshMatrix::Rotate(float angle, float rx, float ry, float rz)
{
	StdMeshMatrix m;

	// We do normalize the rx,ry,rz vector here: This is only required for
	// precalculations anyway, thus not time-critical.
	float abs = sqrt(rx*rx+ry*ry+rz*rz);
	rx/=abs; ry/=abs; rz/=abs;
	float c = cos(angle), s = sin(angle);

	m.a[0][0] = rx*rx*(1-c)+c;    m.a[0][1] = rx*ry*(1-c)-rz*s; m.a[0][2] = rx*rz*(1-c)+ry*s; m.a[0][3] = 0.0f;
	m.a[1][0] = ry*rx*(1-c)+rz*s; m.a[1][1] = ry*ry*(1-c)+c;    m.a[1][2] = ry*rz*(1-c)-rx*s; m.a[1][3] = 0.0f;
	m.a[2][0] = rz*rx*(1-c)-ry*s; m.a[2][1] = ry*rz*(1-c)+rx*s; m.a[2][2] = rz*rz*(1-c)+c;    m.a[2][3] = 0.0f;
	return m;
}

StdMeshMatrix StdMeshMatrix::Transform(const StdMeshTransformation& transform)
{
	StdMeshMatrix m;

	float tx = 2*transform.rotate.x;
	float ty = 2*transform.rotate.y;
	float tz = 2*transform.rotate.z;
	float twx = tx*transform.rotate.w;
	float twy = ty*transform.rotate.w;
	float twz = tz*transform.rotate.w;
	float txx = tx*transform.rotate.x;
	float txy = ty*transform.rotate.x;
	float txz = tz*transform.rotate.x;
	float tyy = ty*transform.rotate.y;
	float tyz = tz*transform.rotate.y;
	float tzz = tz*transform.rotate.z;

	m.a[0][0] = (1.0f - (tyy + tzz) ) * transform.scale.x;
	m.a[0][1] = (txy - twz)           * transform.scale.y;
	m.a[0][2] = (txz + twy)           * transform.scale.z;
	m.a[1][0] = (txy + twz)           * transform.scale.x;
	m.a[1][1] = (1.0f - (txx + tzz) ) * transform.scale.y;
	m.a[1][2] = (tyz - twx)           * transform.scale.z;
	m.a[2][0] = (txz - twy)           * transform.scale.x;
	m.a[2][1] = (tyz + twx)           * transform.scale.y;
	m.a[2][2] = (1.0f - (txx + tyy) ) * transform.scale.z;

	m.a[0][3] = transform.translate.x;
	m.a[1][3] = transform.translate.y;
	m.a[2][3] = transform.translate.z;

	return m;
}

StdMeshMatrix StdMeshMatrix::TransformInverse(const StdMeshTransformation& transform)
{
	StdMeshMatrix m;

	float tx = 2*transform.rotate.x;
	float ty = 2*transform.rotate.y;
	float tz = 2*transform.rotate.z;
	float twx = -tx*transform.rotate.w;
	float twy = -ty*transform.rotate.w;
	float twz = -tz*transform.rotate.w;
	float txx = tx*transform.rotate.x;
	float txy = ty*transform.rotate.x;
	float txz = tz*transform.rotate.x;
	float tyy = ty*transform.rotate.y;
	float tyz = tz*transform.rotate.y;
	float tzz = tz*transform.rotate.z;

	m.a[0][0] = (1.0f - (tyy + tzz) ) / transform.scale.x;
	m.a[0][1] = (txy - twz)           / transform.scale.x;
	m.a[0][2] = (txz + twy)           / transform.scale.x;
	m.a[1][0] = (txy + twz)           / transform.scale.y;
	m.a[1][1] = (1.0f - (txx + tzz) ) / transform.scale.y;
	m.a[1][2] = (tyz - twx)           / transform.scale.y;
	m.a[2][0] = (txz - twy)           / transform.scale.z;
	m.a[2][1] = (tyz + twx)           / transform.scale.z;
	m.a[2][2] = (1.0f - (txx + tyy) ) / transform.scale.z;

	// Signs do not cancel!
	StdMeshVector invtranslate = (-transform.rotate) * (-transform.translate/transform.scale);

	m.a[0][3] = invtranslate.x;
	m.a[1][3] = invtranslate.y;
	m.a[2][3] = invtranslate.z;

	return m;
}

StdMeshMatrix StdMeshMatrix::LookAt(const StdMeshVector& eye, const StdMeshVector& center, const StdMeshVector& up)
{
	// See http://stackoverflow.com/questions/349050/calculating-a-lookat-matrix
	StdMeshVector z = eye - center;
	z.Normalize();

	StdMeshVector x = StdMeshVector::Cross(up, z);
	x.Normalize();

	StdMeshVector y = StdMeshVector::Cross(z, x);

	StdMeshMatrix m;
	m.a[0][0] = x.x; m.a[0][1] = x.y; m.a[0][2] = x.z; m.a[0][3] = -x.x*eye.x - x.y*eye.y - x.z*eye.z;
	m.a[1][0] = y.x; m.a[1][1] = y.y; m.a[1][2] = y.z; m.a[1][3] = -y.x*eye.x - y.y*eye.y - y.z*eye.z;
	m.a[2][0] = z.x; m.a[2][1] = z.y; m.a[2][2] = z.z; m.a[2][3] = -z.x*eye.x - z.y*eye.y - z.z*eye.z;
	return m;
}

float StdMeshMatrix::Determinant() const
{
	return a[0][0]*a[1][1]*a[2][2] + a[0][1]*a[1][2]*a[2][0] + a[0][2]*a[1][0]*a[2][1]
	       - a[0][0]*a[1][2]*a[2][1] - a[0][1]*a[1][0]*a[2][2] - a[0][2]*a[1][1]*a[2][0];
}

StdProjectionMatrix StdProjectionMatrix::Identity()
{
	StdProjectionMatrix m;
	m.a[0][0] = 1.0f; m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = 0.0f;
	m.a[1][0] = 0.0f; m.a[1][1] = 1.0f; m.a[1][2] = 0.0f; m.a[1][3] = 0.0f;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = 1.0f; m.a[2][3] = 0.0f;
	m.a[3][0] = 0.0f; m.a[3][1] = 0.0f; m.a[3][2] = 0.0f; m.a[3][3] = 1.0f;
	return m;
}

StdProjectionMatrix StdProjectionMatrix::Translate(float dx, float dy, float dz)
{
	StdProjectionMatrix m;
	m.a[0][0] = 1.0f; m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = dx;
	m.a[1][0] = 0.0f; m.a[1][1] = 1.0f; m.a[1][2] = 0.0f; m.a[1][3] = dy;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = 1.0f; m.a[2][3] = dz;
	m.a[3][0] = 0.0f; m.a[3][1] = 0.0f; m.a[3][2] = 0.0f; m.a[3][3] = 1.0f;
	return m;
}

StdProjectionMatrix StdProjectionMatrix::Scale(float sx, float sy, float sz)
{
	StdProjectionMatrix m;
	m.a[0][0] = sx;   m.a[0][1] = 0.0f; m.a[0][2] = 0.0f; m.a[0][3] = 0.0f;
	m.a[1][0] = 0.0f; m.a[1][1] = sy;   m.a[1][2] = 0.0f; m.a[1][3] = 0.0f;
	m.a[2][0] = 0.0f; m.a[2][1] = 0.0f; m.a[2][2] = sz;   m.a[2][3] = 0.0f;
	m.a[3][0] = 0.0f; m.a[3][1] = 0.0f; m.a[3][2] = 0.0f; m.a[3][3] = 1.0f;
	return m;
}

StdProjectionMatrix StdProjectionMatrix::Rotate(float angle, float rx, float ry, float rz)
{
	StdProjectionMatrix m;

	// We do normalize the rx,ry,rz vector here: This is only required for
	// precalculations anyway, thus not time-critical.
	float abs = sqrt(rx*rx+ry*ry+rz*rz);
	rx/=abs; ry/=abs; rz/=abs;
	float c = cos(angle), s = sin(angle);

	m.a[0][0] = rx*rx*(1-c)+c;    m.a[0][1] = rx*ry*(1-c)-rz*s; m.a[0][2] = rx*rz*(1-c)+ry*s; m.a[0][3] = 0.0f;
	m.a[1][0] = ry*rx*(1-c)+rz*s; m.a[1][1] = ry*ry*(1-c)+c;    m.a[1][2] = ry*rz*(1-c)-rx*s; m.a[1][3] = 0.0f;
	m.a[2][0] = rz*rx*(1-c)-ry*s; m.a[2][1] = ry*rz*(1-c)+rx*s; m.a[2][2] = rz*rz*(1-c)+c;    m.a[2][3] = 0.0f;
	m.a[3][0] = 0.0f; m.a[3][1] = 0.0f; m.a[3][2] = 0.0f; m.a[3][3] = 1.0f;
	return m;
}

StdProjectionMatrix StdProjectionMatrix::Orthographic(float left, float right, float bottom, float top)
{
	StdProjectionMatrix matrix;
	matrix(0,0) = 2.0f / (right - left);
	matrix(0,1) = 0.0f;
	matrix(0,2) = 0.0f;
	matrix(0,3) = -(right + left) / (right - left);
	matrix(1,0) = 0.0f;
	matrix(1,1) = 2.0f / (top - bottom);
	matrix(1,2) = 0.0f;
	matrix(1,3) = -(top + bottom) / (top - bottom);
	matrix(2,0) = 0.0f;
	matrix(2,1) = 0.0f;
	matrix(2,2) = -1.0f;
	matrix(2,3) = 0.0f;
	matrix(3,0) = 0.0f;
	matrix(3,1) = 0.0f;
	matrix(3,2) = 0.0f;
	matrix(3,3) = 1.0f;
	return matrix;
}

StdMeshMatrix StdProjectionMatrix::Upper3x4(const StdProjectionMatrix& matrix)
{
	StdMeshMatrix m;
	m(0, 0) = matrix.a[0][0]; m(0, 1) = matrix.a[0][1]; m(0, 2) = matrix.a[0][2]; m(0, 3) = matrix.a[0][3];
	m(1, 0) = matrix.a[1][0]; m(1, 1) = matrix.a[1][1]; m(1, 2) = matrix.a[1][2]; m(1, 3) = matrix.a[1][3];
	m(2, 0) = matrix.a[2][0]; m(2, 1) = matrix.a[2][1]; m(2, 2) = matrix.a[2][2]; m(2, 3) = matrix.a[2][3];
	return m;
}

StdMeshTransformation StdMeshMatrix::Decompose() const
{
	// Extract the scale part of the matrix
	const float sx = sqrt(a[0][0]*a[0][0] + a[1][0]*a[1][0] + a[2][0]*a[2][0]);
	const float sy = sqrt(a[0][1]*a[0][1] + a[1][1]*a[1][1] + a[2][1]*a[2][1]);
	const float sz = sqrt(a[0][2]*a[0][2] + a[1][2]*a[1][2] + a[2][2]*a[2][2]);

	// What remains is the rotation part
	// TODO: This can be optimized by not doing the full matrix multiplication
	StdMeshMatrix rot = Scale(1.0f/sx, 1.0f/sy, 1.0f/sz) * *this;

	// Note that this does not work for skew matrices -- we cannot
	// represent skews in StdMeshTransformation
	const float cos_angle = 0.5f * (rot.a[0][0] + rot.a[1][1] + rot.a[2][2] - 1.0f);

	const float rdx = rot.a[2][1] - rot.a[1][2];
	const float rdy = rot.a[0][2] - rot.a[2][0];
	const float rdz = rot.a[1][0] - rot.a[0][1];
	const float det = sqrt(rdx*rdx + rdy*rdy + rdz*rdz);

	const float rx = (rot.a[2][1] - rot.a[1][2]) / det;
	const float ry = (rot.a[0][2] - rot.a[2][0]) / det;
	const float rz = (rot.a[1][0] - rot.a[0][1]) / det;

	StdMeshTransformation trans;
	trans.scale.x = sx;
	trans.scale.y = sy;
	trans.scale.z = sz;
	trans.rotate = StdMeshQuaternion::AngleAxis(acos(cos_angle), StdMeshVector::Translate(rx, ry, rz));
	trans.translate.x = a[0][3];
	trans.translate.y = a[1][3];
	trans.translate.z = a[2][3];

	return trans;
}

StdMeshMatrix operator*(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs)
{
	StdMeshMatrix m;

	m(0,0) = lhs(0,0)*rhs(0,0) + lhs(0,1)*rhs(1,0) + lhs(0,2)*rhs(2,0);
	m(1,0) = lhs(1,0)*rhs(0,0) + lhs(1,1)*rhs(1,0) + lhs(1,2)*rhs(2,0);
	m(2,0) = lhs(2,0)*rhs(0,0) + lhs(2,1)*rhs(1,0) + lhs(2,2)*rhs(2,0);

	m(0,1) = lhs(0,0)*rhs(0,1) + lhs(0,1)*rhs(1,1) + lhs(0,2)*rhs(2,1);
	m(1,1) = lhs(1,0)*rhs(0,1) + lhs(1,1)*rhs(1,1) + lhs(1,2)*rhs(2,1);
	m(2,1) = lhs(2,0)*rhs(0,1) + lhs(2,1)*rhs(1,1) + lhs(2,2)*rhs(2,1);

	m(0,2) = lhs(0,0)*rhs(0,2) + lhs(0,1)*rhs(1,2) + lhs(0,2)*rhs(2,2);
	m(1,2) = lhs(1,0)*rhs(0,2) + lhs(1,1)*rhs(1,2) + lhs(1,2)*rhs(2,2);
	m(2,2) = lhs(2,0)*rhs(0,2) + lhs(2,1)*rhs(1,2) + lhs(2,2)*rhs(2,2);

	m(0,3) = lhs(0,0)*rhs(0,3) + lhs(0,1)*rhs(1,3) + lhs(0,2)*rhs(2,3) + lhs(0,3);
	m(1,3) = lhs(1,0)*rhs(0,3) + lhs(1,1)*rhs(1,3) + lhs(1,2)*rhs(2,3) + lhs(1,3);
	m(2,3) = lhs(2,0)*rhs(0,3) + lhs(2,1)*rhs(1,3) + lhs(2,2)*rhs(2,3) + lhs(2,3);

	return m;
}

StdMeshMatrix operator*(float lhs, const StdMeshMatrix& rhs)
{
	StdMeshMatrix m;
	m(0,0) = lhs * rhs(0,0);
	m(1,0) = lhs * rhs(1,0);
	m(2,0) = lhs * rhs(2,0);
	m(0,1) = lhs * rhs(0,1);
	m(1,1) = lhs * rhs(1,1);
	m(2,1) = lhs * rhs(2,1);
	m(0,2) = lhs * rhs(0,2);
	m(1,2) = lhs * rhs(1,2);
	m(2,2) = lhs * rhs(2,2);
	m(0,3) = lhs * rhs(0,3);
	m(1,3) = lhs * rhs(1,3);
	m(2,3) = lhs * rhs(2,3);
	return m;
}

StdMeshMatrix operator*(const StdMeshMatrix& lhs, float rhs)
{
	return rhs * lhs;
}

StdMeshMatrix& operator*=(StdMeshMatrix& lhs, const StdMeshMatrix& rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

StdMeshMatrix operator+(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs)
{
	StdMeshMatrix m;
	m(0,0) = lhs(0,0) + rhs(0,0);
	m(1,0) = lhs(1,0) + rhs(1,0);
	m(2,0) = lhs(2,0) + rhs(2,0);
	m(0,1) = lhs(0,1) + rhs(0,1);
	m(1,1) = lhs(1,1) + rhs(1,1);
	m(2,1) = lhs(2,1) + rhs(2,1);
	m(0,2) = lhs(0,2) + rhs(0,2);
	m(1,2) = lhs(1,2) + rhs(1,2);
	m(2,2) = lhs(2,2) + rhs(2,2);
	m(0,3) = lhs(0,3) + rhs(0,3);
	m(1,3) = lhs(1,3) + rhs(1,3);
	m(2,3) = lhs(2,3) + rhs(2,3);
	return m;
}

StdProjectionMatrix operator*(const StdProjectionMatrix& lhs, const StdProjectionMatrix& rhs)
{
	StdProjectionMatrix m;

	m(0,0) = lhs(0,0)*rhs(0,0) + lhs(0,1)*rhs(1,0) + lhs(0,2)*rhs(2,0) + lhs(0,3)*rhs(3,0);
	m(1,0) = lhs(1,0)*rhs(0,0) + lhs(1,1)*rhs(1,0) + lhs(1,2)*rhs(2,0) + lhs(1,3)*rhs(3,0);
	m(2,0) = lhs(2,0)*rhs(0,0) + lhs(2,1)*rhs(1,0) + lhs(2,2)*rhs(2,0) + lhs(2,3)*rhs(3,0);
	m(3,0) = lhs(3,0)*rhs(0,0) + lhs(3,1)*rhs(1,0) + lhs(3,2)*rhs(2,0) + lhs(3,3)*rhs(3,0);

	m(0,1) = lhs(0,0)*rhs(0,1) + lhs(0,1)*rhs(1,1) + lhs(0,2)*rhs(2,1) + lhs(0,3)*rhs(3,1);
	m(1,1) = lhs(1,0)*rhs(0,1) + lhs(1,1)*rhs(1,1) + lhs(1,2)*rhs(2,1) + lhs(1,3)*rhs(3,1);
	m(2,1) = lhs(2,0)*rhs(0,1) + lhs(2,1)*rhs(1,1) + lhs(2,2)*rhs(2,1) + lhs(2,3)*rhs(3,1);
	m(3,1) = lhs(3,0)*rhs(0,1) + lhs(3,1)*rhs(1,1) + lhs(3,2)*rhs(2,1) + lhs(3,3)*rhs(3,1);

	m(0,2) = lhs(0,0)*rhs(0,2) + lhs(0,1)*rhs(1,2) + lhs(0,2)*rhs(2,2) + lhs(0,3)*rhs(3,2);
	m(1,2) = lhs(1,0)*rhs(0,2) + lhs(1,1)*rhs(1,2) + lhs(1,2)*rhs(2,2) + lhs(1,3)*rhs(3,2);
	m(2,2) = lhs(2,0)*rhs(0,2) + lhs(2,1)*rhs(1,2) + lhs(2,2)*rhs(2,2) + lhs(2,3)*rhs(3,2);
	m(3,2) = lhs(3,0)*rhs(0,2) + lhs(3,1)*rhs(1,2) + lhs(3,2)*rhs(2,2) + lhs(3,3)*rhs(3,2);

	m(0,3) = lhs(0,0)*rhs(0,3) + lhs(0,1)*rhs(1,3) + lhs(0,2)*rhs(2,3) + lhs(0,3)*rhs(3,3);
	m(1,3) = lhs(1,0)*rhs(0,3) + lhs(1,1)*rhs(1,3) + lhs(1,2)*rhs(2,3) + lhs(1,3)*rhs(3,3);
	m(2,3) = lhs(2,0)*rhs(0,3) + lhs(2,1)*rhs(1,3) + lhs(2,2)*rhs(2,3) + lhs(2,3)*rhs(3,3);
	m(3,3) = lhs(3,0)*rhs(0,3) + lhs(3,1)*rhs(1,3) + lhs(3,2)*rhs(2,3) + lhs(3,3)*rhs(3,3);

	return m;
}

StdProjectionMatrix& operator*=(StdProjectionMatrix& lhs, const StdProjectionMatrix& rhs)
{
	lhs = lhs * rhs;
	return lhs;
}

StdMeshQuaternion operator-(const StdMeshQuaternion& rhs)
{
	StdMeshQuaternion q;
	q.w = -rhs.w;
	q.x = -rhs.x;
	q.y = -rhs.y;
	q.z = -rhs.z;
	return q;
}

StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs)
{
	StdMeshQuaternion q;
	q.w = lhs.w*rhs.w - lhs.x*rhs.x - lhs.y*rhs.y - lhs.z*rhs.z;
	q.x = lhs.w*rhs.x + lhs.x*rhs.w + lhs.y*rhs.z - lhs.z*rhs.y;
	q.y = lhs.w*rhs.y + lhs.y*rhs.w + lhs.z*rhs.x - lhs.x*rhs.z;
	q.z = lhs.w*rhs.z + lhs.z*rhs.w + lhs.x*rhs.y - lhs.y*rhs.x;
	return q;
}

StdMeshQuaternion& operator*=(StdMeshQuaternion& lhs, float rhs)
{
	lhs.w *= rhs;
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, float rhs)
{
	StdMeshQuaternion q(lhs);
	q *= rhs;
	return q;
}

StdMeshQuaternion operator*(float lhs, const StdMeshQuaternion& rhs)
{
	return rhs * lhs;
}

StdMeshQuaternion& operator+=(StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs)
{
	lhs.w += rhs.w;
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

StdMeshQuaternion operator+(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs)
{
	StdMeshQuaternion q(lhs);
	q += rhs;
	return q;
}

StdMeshQuaternion operator-(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs)
{
	StdMeshQuaternion q;
	q.w = lhs.w - rhs.w;
	q.x = lhs.x - rhs.x;
	q.y = lhs.y - rhs.y;
	q.z = lhs.z - rhs.z;
	return q;
}

StdMeshTransformation operator*(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs)
{
	StdMeshTransformation t;
	t.rotate = lhs.rotate * rhs.rotate;
	t.scale = lhs.scale * rhs.scale;
	t.translate = lhs.translate + lhs.rotate * (lhs.scale * rhs.translate);
	return t;
}

StdMeshVector operator-(const StdMeshVector& rhs)
{
	StdMeshVector v;
	v.x = -rhs.x;
	v.y = -rhs.y;
	v.z = -rhs.z;
	return v;
}

StdMeshVector& operator+=(StdMeshVector& lhs, const StdMeshVector& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

StdMeshVector operator+(const StdMeshVector& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v(lhs);
	v += rhs;
	return v;
}

StdMeshVector& operator-=(StdMeshVector& lhs, const StdMeshVector& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}

StdMeshVector operator-(const StdMeshVector& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v(lhs);
	v -= rhs;
	return v;
}

StdMeshVector operator*(const StdMeshVector& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v;
	v.x = lhs.x * rhs.x;
	v.y = lhs.y * rhs.y;
	v.z = lhs.z * rhs.z;
	return v;
}

StdMeshVector& operator*=(StdMeshVector& lhs, float rhs)
{
	lhs.x *= rhs;
	lhs.y *= rhs;
	lhs.z *= rhs;
	return lhs;
}

StdMeshVector operator*(const StdMeshVector& lhs, float rhs)
{
	StdMeshVector v(lhs);
	v *= rhs;
	return v;
}

StdMeshVector operator*(float lhs, const StdMeshVector& rhs)
{
	return rhs * lhs;
}

StdMeshVector operator/(const StdMeshVector& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v;
	v.x = lhs.x/rhs.x;
	v.y = lhs.y/rhs.y;
	v.z = lhs.z/rhs.z;
	return v;
}

StdMeshVector operator/(float lhs, const StdMeshVector& rhs)
{
	StdMeshVector v;
	v.x = lhs/rhs.x;
	v.y = lhs/rhs.y;
	v.z = lhs/rhs.z;
	return v;
}

StdMeshVector operator/(const StdMeshVector& lhs, float rhs)
{
	StdMeshVector v;
	v.x = lhs.x/rhs;
	v.y = lhs.y/rhs;
	v.z = lhs.z/rhs;
	return v;
}

StdMeshVector operator*(const StdMeshMatrix& lhs, const StdMeshVector& rhs) // does not apply translation part
{
	StdMeshVector v;
	v.x = lhs(0,0)*rhs.x + lhs(0,1)*rhs.y + lhs(0,2)*rhs.z;
	v.y = lhs(1,0)*rhs.x + lhs(1,1)*rhs.y + lhs(1,2)*rhs.z;
	v.z = lhs(2,0)*rhs.x + lhs(2,1)*rhs.y + lhs(2,2)*rhs.z;
	return v;
}

StdMeshVector operator*(const StdMeshQuaternion& lhs, const StdMeshVector& rhs)
{
	StdMeshVector v = { lhs.x, lhs.y, lhs.z };
	StdMeshVector uv = 2.0f * StdMeshVector::Cross(v, rhs);
	StdMeshVector uuv = StdMeshVector::Cross(v, uv);
	return rhs + lhs.w * uv + uuv;
}

StdMeshVertex& operator+=(StdMeshVertex& lhs, const StdMeshVertex& rhs)
{
	lhs.nx += rhs.nx;
	lhs.ny += rhs.ny;
	lhs.nz += rhs.nz;
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}

StdMeshVertex operator+(const StdMeshVertex& lhs, const StdMeshVertex& rhs)
{
	StdMeshVertex vtx(lhs);
	vtx += rhs;
	return vtx;
}

StdMeshVertex operator*(float lhs, const StdMeshVertex& rhs)
{
	StdMeshVertex vtx;
	vtx.nx = lhs*rhs.nx;
	vtx.ny = lhs*rhs.ny;
	vtx.nz = lhs*rhs.nz;
	vtx.x = lhs*rhs.x;
	vtx.y = lhs*rhs.y;
	vtx.z = lhs*rhs.z;
	return vtx;
}

StdMeshVertex operator*(const StdMeshVertex& lhs, float rhs)
{
	return rhs * lhs;
}

StdMeshVertex operator*(const StdMeshMatrix& lhs, const StdMeshVertex& rhs)
{
	StdMeshVertex vtx;
	vtx.nx = lhs(0,0)*rhs.nx + lhs(0,1)*rhs.ny + lhs(0,2)*rhs.nz;
	vtx.ny = lhs(1,0)*rhs.nx + lhs(1,1)*rhs.ny + lhs(0,2)*rhs.nz;
	vtx.nz = lhs(2,0)*rhs.nx + lhs(2,1)*rhs.ny + lhs(2,2)*rhs.nz;
	vtx.x  = lhs(0,0)*rhs.x + lhs(0,1)*rhs.y + lhs(0,2)*rhs.z + lhs(0,3);
	vtx.y  = lhs(1,0)*rhs.x + lhs(1,1)*rhs.y + lhs(1,2)*rhs.z + lhs(1,3);
	vtx.z  = lhs(2,0)*rhs.x + lhs(2,1)*rhs.y + lhs(2,2)*rhs.z + lhs(2,3);
	vtx.u = rhs.u; vtx.v = rhs.v;
	return vtx;
}
