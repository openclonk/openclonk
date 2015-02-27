/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2015, The OpenClonk Team and contributors
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

#ifndef INC_StdMeshMath
#define INC_StdMeshMath

// OGRE mesh

struct StdMeshVector
{
	float x, y, z;

	static StdMeshVector Zero();
	static StdMeshVector UnitScale();
	static StdMeshVector Translate(float dx, float dy, float dz);
	static StdMeshVector Cross(const StdMeshVector& lhs, const StdMeshVector& rhs);
};


struct StdMeshVertex
{
	static const size_t MaxBoneWeightCount = 8;

	// Match GL_T2F_N3F_V3F
	float u, v;
	float nx, ny, nz;
	float x, y, z;

	float bone_weight[MaxBoneWeightCount];
	uint16_t bone_index[MaxBoneWeightCount];

	char _padding[16];

	StdMeshVertex() : u(0), v(0), nx(0), ny(0), nz(0), x(0), y(0), z(0) 
	{
		std::uninitialized_fill(std::begin(bone_weight), std::end(bone_weight), 0);
		std::uninitialized_fill(std::begin(bone_index), std::end(bone_index), 0);
		std::uninitialized_fill(std::begin(_padding), std::end(_padding), 0);
	}
	//void Normalize();
};
static_assert((sizeof(StdMeshVertex) & 31) == 0, "StdMeshVertex should be a multiple of 32 bytes");

struct StdMeshQuaternion
{
	float w;
	// union
	// {
	//   struct { float x, y, z; };
	//   StdMeshVector v;
	// };
	float x, y, z;

	static StdMeshQuaternion Zero();
	static StdMeshQuaternion AngleAxis(float theta, const StdMeshVector& axis);

	float LenSqr() const { return w*w+x*x+y*y+z*z; }
	void Normalize();

	static StdMeshQuaternion Nlerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w);
	//static StdMeshQuaternion Slerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w);
};

struct StdMeshTransformation
{
	StdMeshVector scale;
	StdMeshQuaternion rotate;
	StdMeshVector translate;

	static StdMeshTransformation Zero();
	static StdMeshTransformation Identity();
	static StdMeshTransformation Inverse(const StdMeshTransformation& transform);
	static StdMeshTransformation Translate(float dx, float dy, float dz);
	static StdMeshTransformation Scale(float sx, float sy, float sz);
	static StdMeshTransformation Rotate(float angle, float rx, float ry, float rz);

	// TODO: Might add path parameter if necessary
	static StdMeshTransformation Nlerp(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs, float w);
	//static  StdMeshQuaternion Slerp(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs, float w);
};

class StdMeshMatrix
{
public:
	static StdMeshMatrix Zero();
	static StdMeshMatrix Identity();
	static StdMeshMatrix Inverse(const StdMeshMatrix& mat);
	static StdMeshMatrix Translate(float dx, float dy, float dz);
	static StdMeshMatrix Scale(float sx, float sy, float sz);
	static StdMeshMatrix Rotate(float angle, float rx, float ry, float rz);
	static StdMeshMatrix Transform(const StdMeshTransformation& transform);
	static StdMeshMatrix TransformInverse(const StdMeshTransformation& transform);

	float& operator()(int i, int j) { return a[i][j]; }
	float operator()(int i, int j) const { return a[i][j]; }

	float Determinant() const;
	StdMeshTransformation Decompose() const;

private:
	// 3x3 orthogonal + translation in last column
	float a[3][4];
};

StdMeshMatrix operator*(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(float lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(const StdMeshMatrix& lhs, float rhs);
StdMeshMatrix& operator*=(StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator+(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshQuaternion operator-(const StdMeshQuaternion& rhs);
StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion& operator*=(StdMeshQuaternion& lhs, float rhs);
StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, float rhs);
StdMeshQuaternion operator*(float lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion& operator+=(StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion operator+(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion operator-(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshTransformation operator*(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs);

StdMeshVector operator-(const StdMeshVector& rhs);
StdMeshVector& operator+=(StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator+(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator*(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector& operator*=(StdMeshVector& lhs, float rhs);
StdMeshVector operator*(const StdMeshVector& lhs, float rhs);
StdMeshVector operator*(float lhs, const StdMeshVector& rhs);
StdMeshVector operator/(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator/(float lhs, const StdMeshVector& rhs);
StdMeshVector operator/(const StdMeshVector& lhs, float rhs);

StdMeshVector operator*(const StdMeshMatrix& lhs, const StdMeshVector& rhs); // does not apply translation part
StdMeshVector operator*(const StdMeshQuaternion& lhs, const StdMeshVector& rhs);

StdMeshVertex& operator+=(StdMeshVertex& lhs, const StdMeshVertex& rhs);
StdMeshVertex operator+(const StdMeshVertex& lhs, const StdMeshVertex& rhs);
StdMeshVertex operator*(float lhs, const StdMeshVertex& rhs);
StdMeshVertex operator*(const StdMeshVertex& lhs, float rhs);
StdMeshVertex operator*(const StdMeshMatrix& lhs, const StdMeshVertex& rhs);

#endif
