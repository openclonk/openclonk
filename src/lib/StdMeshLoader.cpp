/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2015, The OpenClonk Team and contributors
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

// A loader for the OGRE .mesh binary file format

#include <C4Include.h>

#include <StdMeshLoader.h>

namespace
{
	// Transpose a StdMeshMatrix. The translate component of
	// the input matrix must be 0.
	StdMeshMatrix Transpose(const StdMeshMatrix& matrix)
	{
		assert(fabs(matrix(0,3)) < 1e-6);
		assert(fabs(matrix(1,3)) < 1e-6);
		assert(fabs(matrix(2,3)) < 1e-6);

		StdMeshMatrix result;

		result(0,0) = matrix(0,0);
		result(0,1) = matrix(1,0);
		result(0,2) = matrix(2,0);
		result(0,3) = 0.0f;
		result(1,0) = matrix(0,1);
		result(1,1) = matrix(1,1);
		result(1,2) = matrix(2,1);
		result(1,3) = 0.0f;
		result(2,0) = matrix(0,2);
		result(2,1) = matrix(1,2);
		result(2,2) = matrix(2,2);
		result(2,3) = 0.0f;

		return result;
	}

	// Transformation matrix to convert meshes from Ogre to Clonk coordinate system
	const StdMeshMatrix OgreToClonkMatrix = StdMeshMatrix::Scale(-1.0f, -1.0f, -1.0f) * StdMeshMatrix::Rotate(float(M_PI)/2.0f, 0.0f, 1.0f, 0.0f);

	const StdMeshMatrix OgreToClonkInverse = StdMeshMatrix::Inverse(OgreToClonkMatrix);
	const StdMeshMatrix OgreToClonkInverseTranspose = Transpose(OgreToClonkInverse);
	const float OgreToClonkDeterminant = OgreToClonkMatrix.Determinant();
}

namespace OgreToClonk
{

StdMeshVector TransformVector(const StdMeshVector& vector)
{
	return OgreToClonkMatrix * vector;
}

StdMeshVector TransformPseudoVector(const StdMeshVector& vector)
{
	// TODO: This works only for improper rotations... otherwise it might be better
	// to write vector as an antisymmetric tensor and do the matrix transform.
	return OgreToClonkDeterminant * (OgreToClonkMatrix * vector);
}

StdMeshVector TransformNormalVector(const StdMeshVector& vector)
{
	return OgreToClonkInverseTranspose * vector;
}

StdMeshVector TransformScaleVector(const StdMeshVector& vector)
{
	// TODO: Check we didn't introduce shear components
	StdMeshMatrix scale = StdMeshMatrix::Scale(vector.x, vector.y, vector.z);
	StdMeshMatrix transformed = OgreToClonkMatrix * scale * OgreToClonkInverse;

	StdMeshVector result;
	result.x = transformed(0,0);
	result.y = transformed(1,1);
	result.z = transformed(2,2);
	return result;
}

StdMeshQuaternion TransformQuaternion(const StdMeshQuaternion& quaternion)
{
	StdMeshVector axis;
	axis.x = quaternion.x;
	axis.y = quaternion.y;
	axis.z = quaternion.z;

	StdMeshVector transformed = TransformPseudoVector(axis);

	StdMeshQuaternion result;
	result.w = quaternion.w;
	result.x = transformed.x;
	result.y = transformed.y;
	result.z = transformed.z;

	return result;
}

StdSubMesh::Vertex TransformVertex(const StdSubMesh::Vertex& vertex)
{
	StdMeshVector pos, normal;
	pos.x = vertex.x;
	pos.y = vertex.y;
	pos.z = vertex.z;
	normal.x = vertex.nx;
	normal.y = vertex.ny;
	normal.z = vertex.nz;

	pos = TransformVector(pos);
	normal = TransformNormalVector(normal);

	StdSubMesh::Vertex result = vertex;
	result.x = pos.x;
	result.y = pos.y;
	result.z = pos.z;
	result.nx = normal.x;
	result.ny = normal.y;
	result.nz = normal.z;

	return result;
}

StdMeshTransformation TransformTransformation(const StdMeshTransformation& trans)
{
	StdMeshTransformation result;
	result.scale = TransformScaleVector(trans.scale);
	result.rotate = TransformQuaternion(trans.rotate);
	result.translate = TransformVector(trans.translate);

	// Consistency check:
	/*StdMeshMatrix matrix = StdMeshMatrix::Transform(trans);
	matrix = OgreToClonk * matrix * OgreToClonkInverse;
	
	StdMeshMatrix matrix2 = StdMeshMatrix::Transform(result);
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 4; ++j)
			assert(fabs(matrix(i,j) - matrix2(i,j)) < 1e-3);*/

	return result;
}

}
