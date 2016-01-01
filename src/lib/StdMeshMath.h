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

	void Normalize();
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
};
static_assert((sizeof(StdMeshVertex) & 31) == 0, "StdMeshVertex should be a multiple of 32 bytes");

struct StdMeshQuaternion
{
	float w;
	float x, y, z;

	static StdMeshQuaternion Zero();
	static StdMeshQuaternion AngleAxis(float theta, const StdMeshVector& axis);

	float LenSqr() const { return w*w+x*x+y*y+z*z; }
	void Normalize();

	static StdMeshQuaternion Nlerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w);
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
};

class StdMeshMatrix
{
public:
	static const int NColumns = 4;
	static const int NRows = 3;

	static StdMeshMatrix Zero();
	static StdMeshMatrix Identity();
	static StdMeshMatrix Inverse(const StdMeshMatrix& mat);
	static StdMeshMatrix Translate(float dx, float dy, float dz);
	static StdMeshMatrix Scale(float sx, float sy, float sz);
	static StdMeshMatrix Rotate(float angle, float rx, float ry, float rz);
	static StdMeshMatrix Transform(const StdMeshTransformation& transform);
	static StdMeshMatrix TransformInverse(const StdMeshTransformation& transform);
	static StdMeshMatrix LookAt(const StdMeshVector& eye, const StdMeshVector& center, const StdMeshVector& up);

	float& operator()(int i, int j) { return a[i][j]; }
	float operator()(int i, int j) const { return a[i][j]; }

	float Determinant() const;
	StdMeshTransformation Decompose() const;

	const float* data() const { return &a[0][0]; }

private:
	// 3x3 orthogonal + translation in last column
	float a[3][4];
};

// Full 4x4 matrix with projection components
class StdProjectionMatrix
{
public:
	static const int NColumns = 4;
	static const int NRows = 4;

	static StdProjectionMatrix Identity();
	static StdProjectionMatrix Translate(float dx, float dy, float dz);
	static StdProjectionMatrix Scale(float sx, float sy, float sz);
	static StdProjectionMatrix Rotate(float angle, float rx, float ry, float rz);

	static StdProjectionMatrix Orthographic(float left, float right, float bottom, float top);

	static StdMeshMatrix Upper3x4(const StdProjectionMatrix& matrix);

	float& operator()(int i, int j) { return a[i][j]; }
	float operator()(int i, int j) const { return a[i][j]; }

	const float* data() const { return &a[0][0]; }
private:
	float a[4][4];
};

StdMeshMatrix operator*(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(float lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(const StdMeshMatrix& lhs, float rhs);
StdMeshMatrix& operator*=(StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator+(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);

StdProjectionMatrix operator*(const StdProjectionMatrix& lhs, const StdProjectionMatrix& rhs);
StdProjectionMatrix& operator*=(StdProjectionMatrix& lhs, const StdProjectionMatrix& rhs);

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
StdMeshVector& operator-=(StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator-(const StdMeshVector& lhs, const StdMeshVector& rhs);
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

// Multiply in-place the given matrix with a translation matrix to the right
template<typename MatrixType>
void Translate(MatrixType& mat, float dx, float dy, float dz)
{
	static_assert(MatrixType::NColumns >= 4, "Matrix must have at least 4 columns");

	for (int i = 0; i < MatrixType::NRows; ++i)
		mat(i, 3) += mat(i,0)*dx + mat(i,1)*dy + mat(i,2)*dz;
}

// Multiply in-place the given matrix with a scale matrix to the right
template<typename MatrixType>
void Scale(MatrixType& mat, float sx, float sy, float sz)
{
	static_assert(MatrixType::NColumns >= 3, "Matrix must have at least 3 columns");

	for (int i = 0; i < MatrixType::NRows; ++i)
	{
		mat(i, 0) *= sx;
		mat(i, 1) *= sy;
		mat(i, 2) *= sz;
	}
}

// Multiply in-place the given matrix with a rotation matrix to the right
template<typename MatrixType>
void Rotate(MatrixType& mat, float angle, float x, float y, float z)
{
	mat *= MatrixType::Rotate(angle, x, y, z);
}

// Multiply in-place the given matrix with a perspective projection matrix to the right
template<typename MatrixType>
void Perspective(MatrixType& mat, float cot_fovy2, float aspect, float nearVal, float farVal)
{
	static_assert(MatrixType::NColumns >= 4, "Matrix must have at least 4 columns");

	const float fa = cot_fovy2 / aspect;
	const float fb = cot_fovy2;
	const float z1 = (nearVal + farVal) / (nearVal - farVal);
	const float z2 = 2 * nearVal * farVal / (nearVal - farVal);

	for (int i = 0; i < MatrixType::NRows; ++i)
	{
		const float mat2 = mat(i, 2);
		mat(i, 0) *= fa;
		mat(i, 1) *= fb;
		mat(i, 2) = mat2 * z1 - mat(i, 3);
		mat(i, 3) = mat2 * z2;
	}
}

#endif
