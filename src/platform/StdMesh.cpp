/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2010  Armin Burgmeier
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include "C4Include.h"
#include <StdMesh.h>

#ifdef _MSC_VER
# define _USE_MATH_DEFINES
# include <math.h>
#endif

#include <algorithm>

std::vector<StdMeshInstance::SerializableValueProvider::IDBase*>* StdMeshInstance::SerializableValueProvider::IDs = NULL;

namespace
{
	// Helper to sort faces for FaceOrdering
	struct StdMeshInstanceFaceOrderingCmpPred
	{
		const StdMeshInstance& m_inst;
		const StdMeshVertex* m_vertices;
		const StdMeshMatrix& m_global_trans;

		StdMeshInstanceFaceOrderingCmpPred(const StdMeshInstance& inst, unsigned int submesh, const StdMeshMatrix& global_trans):
				m_inst(inst), m_vertices(m_inst.GetSubMesh(submesh).GetVertices()), m_global_trans(global_trans) {}

		bool operator()(const StdMeshFace& face1, const StdMeshFace& face2) const
		{
			// TODO: Need to apply attach matrix in case of attached meshes
			switch (m_inst.GetFaceOrdering())
			{
			case StdMeshInstance::FO_Fixed:
				assert(false);
				return false;
			case StdMeshInstance::FO_FarthestToNearest:
			case StdMeshInstance::FO_NearestToFarthest:
			{
				float z11 = (m_global_trans*m_vertices[face1.Vertices[0]]).z;
				float z12 = (m_global_trans*m_vertices[face1.Vertices[1]]).z;
				float z13 = (m_global_trans*m_vertices[face1.Vertices[2]]).z;
				float z21 = (m_global_trans*m_vertices[face2.Vertices[0]]).z;
				float z22 = (m_global_trans*m_vertices[face2.Vertices[1]]).z;
				float z23 = (m_global_trans*m_vertices[face2.Vertices[2]]).z;

				float z1 = std::max(std::max(z11, z12), z13);
				float z2 = std::max(std::max(z21, z22), z23);

				if (m_inst.GetFaceOrdering() == StdMeshInstance::FO_FarthestToNearest)
					return z1 < z2;
				else
					return z2 < z1;
			}
			default:
				assert(false);
				return false;
			}
		}
	};

	// Seralize a ValueProvider with StdCompiler
	struct ValueProviderAdapt
	{
		ValueProviderAdapt(StdMeshInstance::ValueProvider** Provider):
				ValueProvider(Provider) {}

		StdMeshInstance::ValueProvider** ValueProvider;

		void CompileFunc(StdCompiler* pComp)
		{
			const StdMeshInstance::SerializableValueProvider::IDBase* id;
			StdMeshInstance::SerializableValueProvider* svp = NULL;

			if(pComp->isCompiler())
			{
				StdCopyStrBuf id_str;
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));

				id = StdMeshInstance::SerializableValueProvider::Lookup(id_str.getData());
				if(!id) pComp->excCorrupt("No value provider for ID \"%s\"", id_str.getData());
			}
			else
			{
				svp = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(*ValueProvider);
				if(!svp) pComp->excCorrupt("Value provider cannot be compiled");
				id = StdMeshInstance::SerializableValueProvider::Lookup(typeid(*svp));
				if(!id) pComp->excCorrupt("No ID for value provider registered");

				StdCopyStrBuf id_str(id->name);
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));
			}

			pComp->Separator(StdCompiler::SEP_START);
			pComp->Value(mkContextPtrAdapt(svp, *id, false));
			pComp->Separator(StdCompiler::SEP_END);

			if(pComp->isCompiler())
				*ValueProvider = svp;
		}

		ALLOW_TEMP_TO_REF(ValueProviderAdapt)
	};
	
	ValueProviderAdapt mkValueProviderAdapt(StdMeshInstance::ValueProvider** ValueProvider) { return ValueProviderAdapt(ValueProvider); }

	// Serialize a bone index by name with StdCompiler
	struct TransformAdapt
	{
		StdMeshMatrix& Matrix;
		TransformAdapt(StdMeshMatrix& matrix): Matrix(matrix) {}

		void CompileFunc(StdCompiler* pComp)
		{
			pComp->Separator(StdCompiler::SEP_START);
			for(unsigned int i = 0; i < 3; ++i)
			{
				for(unsigned int j = 0; j < 4; ++j)
				{
					if(i != 0 || j != 0) pComp->Separator();
					// TODO: Teach StdCompiler how to handle float
//					pComp->Value(Matrix(i, j));
					
					if(pComp->isCompiler())
					{
						C4Real f;
						pComp->Value(f);
						Matrix(i,j) = fixtof(f);
					}
					else
					{
						C4Real f = ftofix(Matrix(i,j));
						pComp->Value(f);
					}
				}
			}

			pComp->Separator(StdCompiler::SEP_END);
		}

		ALLOW_TEMP_TO_REF(TransformAdapt)
	};
	
	TransformAdapt mkTransformAdapt(StdMeshMatrix& Matrix) { return TransformAdapt(Matrix); }

	// Reset all animation list entries corresponding to node or its children
	void ClearAnimationListRecursively(std::vector<StdMeshInstance::AnimationNode*>& list, StdMeshInstance::AnimationNode* node)
	{
		list[node->GetNumber()] = NULL;

		if (node->GetType() == StdMeshInstance::AnimationNode::LinearInterpolationNode)
		{
			ClearAnimationListRecursively(list, node->GetLeftChild());
			ClearAnimationListRecursively(list, node->GetRightChild());
		}
	}
}

/* Boring Math stuff begins here */

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
	//t.rotate.v = -transform.rotate.v; // Someone set us up the union!?!??
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

float StdMeshMatrix::Determinant() const
{
	return a[0][0]*a[1][1]*a[2][2] + a[0][1]*a[1][2]*a[2][0] + a[0][2]*a[1][0]*a[2][1]
	       - a[0][0]*a[1][2]*a[2][1] - a[0][1]*a[1][0]*a[2][2] - a[0][2]*a[1][1]*a[2][0];
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

/* Boring math stuff ends here */

StdMeshTransformation StdMeshTrack::GetTransformAt(float time) const
{
	std::map<float, StdMeshKeyFrame>::const_iterator iter = Frames.lower_bound(time);

	// If this points to end(), then either
	// a) time > animation length
	// b) The track does not include a frame for the very end of the animation
	// Both is considered an error
	assert(iter != Frames.end());

	if (iter == Frames.begin())
		return iter->second.Transformation;

	std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
	-- prev_iter;

	float dt = iter->first - prev_iter->first;
	float weight1 = (time - prev_iter->first) / dt;
	float weight2 = (iter->first - time) / dt;

	assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
	assert(fabs(weight1 + weight2 - 1) < 1e-6);

	/*StdMeshTransformation transformation;
	transformation.scale = weight1 * iter->second.Transformation.scale + weight2 * prev_iter->second.Transformation.scale;
	transformation.rotate = weight1 * iter->second.Transformation.rotate + weight2 * prev_iter->second.Transformation.rotate; // TODO: slerp or renormalize
	transformation.translate = weight1 * iter->second.Transformation.translate + weight2 * prev_iter->second.Transformation.translate;
	return transformation;*/
	return StdMeshTransformation::Nlerp(prev_iter->second.Transformation, iter->second.Transformation, weight1);
}

StdMeshAnimation::StdMeshAnimation(const StdMeshAnimation& other):
		Name(other.Name), Length(other.Length), Tracks(other.Tracks.size())
{
	// Note that all Tracks are already default-initialized to zero
	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);
}

StdMeshAnimation::~StdMeshAnimation()
{
	for (unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];
}

StdMeshAnimation& StdMeshAnimation::operator=(const StdMeshAnimation& other)
{
	if (this == &other) return *this;

	Name = other.Name;
	Length = other.Length;

	for (unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];

	Tracks.resize(other.Tracks.size());

	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);

	return *this;
}

StdSubMesh::StdSubMesh():
		Material(NULL)
{
}

StdMesh::StdMesh()
{
	BoundingBox.x1 = BoundingBox.y1 = BoundingBox.z1 = 0.0f;
	BoundingBox.x2 = BoundingBox.y2 = BoundingBox.z2 = 0.0f;
	BoundingRadius = 0.0f;
}

StdMesh::~StdMesh()
{
	for (unsigned int i = 0; i < Bones.size(); ++i)
		delete Bones[i];
}

void StdMesh::AddMasterBone(StdMeshBone* bone)
{
	bone->Index = Bones.size(); // Remember index in master bone table
	Bones.push_back(bone);
	for (unsigned int i = 0; i < bone->Children.size(); ++i)
		AddMasterBone(bone->Children[i]);
}

const StdMeshBone* StdMesh::GetBoneByName(const StdStrBuf& name) const
{
	// Lookup parent bone
	for (unsigned int i = 0; i < Bones.size(); ++i)
		if (Bones[i]->Name == name)
			return Bones[i];

	return NULL;
}

const StdMeshAnimation* StdMesh::GetAnimationByName(const StdStrBuf& name) const
{
	StdCopyStrBuf name2(name);
	std::map<StdCopyStrBuf, StdMeshAnimation>::const_iterator iter = Animations.find(name2);
	if (iter == Animations.end()) return NULL;
	return &iter->second;
}

StdSubMeshInstance::StdSubMeshInstance(const StdSubMesh& submesh):
		Vertices(submesh.GetNumVertices()), Faces(submesh.GetNumFaces()),
		Material(NULL)
{
	// Copy initial Vertices/Faces
	for (unsigned int i = 0; i < submesh.GetNumVertices(); ++i)
		Vertices[i] = submesh.GetVertex(i);
	for (unsigned int i = 0; i < submesh.GetNumFaces(); ++i)
		Faces[i] = submesh.GetFace(i);

	SetMaterial(submesh.GetMaterial());
}

void StdSubMeshInstance::SetMaterial(const StdMeshMaterial& material)
{
	Material = &material;

	// Setup initial texture animation data
	assert(Material->BestTechniqueIndex >= 0);
	const StdMeshMaterialTechnique& technique = Material->Techniques[Material->BestTechniqueIndex];
	PassData.resize(technique.Passes.size());
	for (unsigned int i = 0; i < PassData.size(); ++i)
	{
		const StdMeshMaterialPass& pass = technique.Passes[i];
		// Clear from previous material
		PassData[i].TexUnits.clear();

		for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
		{
			TexUnit unit;
			unit.Phase = 0;
			unit.PhaseDelay = 0.0f;
			unit.Position = 0.0;
			PassData[i].TexUnits.push_back(unit);
		}
	}
}

void StdMeshInstance::SerializableValueProvider::CompileFunc(StdCompiler* pComp)
{
	pComp->Value(Value);
}

StdMeshInstance::AnimationNode::AnimationNode():
		Type(LeafNode), Parent(NULL)
{
	Leaf.Animation = NULL;
	Leaf.Position = NULL;
}

StdMeshInstance::AnimationNode::AnimationNode(const StdMeshAnimation* animation, ValueProvider* position):
		Type(LeafNode), Parent(NULL)
{
	Leaf.Animation = animation;
	Leaf.Position = position;
}

StdMeshInstance::AnimationNode::AnimationNode(AnimationNode* child_left, AnimationNode* child_right, ValueProvider* weight):
		Type(LinearInterpolationNode), Parent(NULL)
{
	LinearInterpolation.ChildLeft = child_left;
	LinearInterpolation.ChildRight = child_right;
	LinearInterpolation.Weight = weight;
}

StdMeshInstance::AnimationNode::~AnimationNode()
{
	switch (Type)
	{
	case LeafNode:
		delete Leaf.Position;
		break;
	case LinearInterpolationNode:
		delete LinearInterpolation.ChildLeft;
		delete LinearInterpolation.ChildRight;
		delete LinearInterpolation.Weight;
		break;
	}
}

bool StdMeshInstance::AnimationNode::GetBoneTransform(unsigned int bone, StdMeshTransformation& transformation)
{
	StdMeshTransformation combine_with;
	StdMeshTrack* track;

	switch (Type)
	{
	case LeafNode:
		//if(!Leaf.Animation) return false;
		track = Leaf.Animation->Tracks[bone];
		if (!track) return false;
		transformation = track->GetTransformAt(fixtof(Leaf.Position->Value));
		return true;
	case LinearInterpolationNode:
		if (!LinearInterpolation.ChildLeft->GetBoneTransform(bone, transformation))
			return LinearInterpolation.ChildRight->GetBoneTransform(bone, transformation);
		if (!LinearInterpolation.ChildRight->GetBoneTransform(bone, combine_with))
			return true; // First Child affects bone

		transformation = StdMeshTransformation::Nlerp(transformation, combine_with, fixtof(LinearInterpolation.Weight->Value));
		return true;
	default:
		assert(false);
		return false;
	}
}

void StdMeshInstance::AnimationNode::CompileFunc(StdCompiler* pComp, const StdMesh* Mesh)
{
	static const StdEnumEntry<NodeType> NodeTypes[] =
	{
		{ "Leaf",                  LeafNode                      },
		{ "LinearInterpolation",   LinearInterpolationNode       },

		{ NULL,     static_cast<NodeType>(0)  }
	};

	pComp->Value(mkNamingAdapt(Slot, "Slot"));
	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(Type, NodeTypes), "Type"));

	switch(Type)
	{
	case LeafNode:
		if(pComp->isCompiler())
		{
			StdCopyStrBuf anim_name;
			pComp->Value(mkNamingAdapt(toC4CStrBuf(anim_name), "Animation"));
			Leaf.Animation = Mesh->GetAnimationByName(anim_name);
			if(!Leaf.Animation) pComp->excCorrupt("No such animation: \"%s\"", anim_name.getData());
		}
		else
		{
			pComp->Value(mkNamingAdapt(mkParAdapt(mkDecompileAdapt(Leaf.Animation->Name), StdCompiler::RCT_All), "Animation"));
		}

		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&Leaf.Position), "Position"));
		break;
	case LinearInterpolationNode:
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildLeft, "ChildLeft"), Mesh));
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildRight, "ChildRight"), Mesh));
		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&LinearInterpolation.Weight), "Weight"));
		if(pComp->isCompiler())
		{
			if(LinearInterpolation.ChildLeft->Slot != Slot)
				pComp->excCorrupt("Slot of left child does not match parent slot");
			if(LinearInterpolation.ChildRight->Slot != Slot)
				pComp->excCorrupt("Slof of right child does not match parent slot");
			LinearInterpolation.ChildRight->Parent = this;
			LinearInterpolation.ChildRight->Parent = this;
		}
		break;
	default:
		pComp->excCorrupt("Invalid animation node type");
		break;
	}
}

void StdMeshInstance::AnimationNode::EnumeratePointers()
{
	SerializableValueProvider* value_provider = NULL;
	switch(Type)
	{
	case LeafNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(Leaf.Position);
		break;
	case LinearInterpolationNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(LinearInterpolation.Weight);
		break;
	}

	if(value_provider) value_provider->EnumeratePointers();
}

void StdMeshInstance::AnimationNode::DenumeratePointers()
{
	SerializableValueProvider* value_provider = NULL;
	switch(Type)
	{
	case LeafNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(Leaf.Position);
		break;
	case LinearInterpolationNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(LinearInterpolation.Weight);
		break;
	}

	if(value_provider) value_provider->DenumeratePointers();
}

StdMeshInstance::AttachedMesh::AttachedMesh():
	Number(0), Parent(NULL), Child(NULL), OwnChild(true), ChildDenumerator(NULL), ParentBone(0), ChildBone(0), FinalTransformDirty(false)
{
}

StdMeshInstance::AttachedMesh::AttachedMesh(unsigned int number, StdMeshInstance* parent, StdMeshInstance* child, bool own_child, Denumerator* denumerator,
		unsigned int parent_bone, unsigned int child_bone, const StdMeshMatrix& transform):
		Number(number), Parent(parent), Child(child), OwnChild(own_child), ChildDenumerator(denumerator),
		ParentBone(parent_bone), ChildBone(child_bone), AttachTrans(transform),
		FinalTransformDirty(true)
{
}

StdMeshInstance::AttachedMesh::~AttachedMesh()
{
	if (OwnChild)
		delete Child;
	delete ChildDenumerator;
}

bool StdMeshInstance::AttachedMesh::SetParentBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Parent->Mesh.GetBoneByName(bone);
	if (!bone_obj) return false;
	ParentBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

bool StdMeshInstance::AttachedMesh::SetChildBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Child->Mesh.GetBoneByName(bone);
	if (!bone_obj) return false;
	ChildBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

void StdMeshInstance::AttachedMesh::SetAttachTransformation(const StdMeshMatrix& transformation)
{
	AttachTrans = transformation;
	FinalTransformDirty = true;
}

void StdMeshInstance::AttachedMesh::CompileFunc(StdCompiler* pComp, DenumeratorFactoryFunc Factory)
{
	if(pComp->isCompiler())
	{
		FinalTransformDirty = true;
		ChildDenumerator = Factory();
	}

	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(ParentBone, "ParentBone")); // TODO: Save as string
	pComp->Value(mkNamingAdapt(ChildBone, "ChildBone")); // TODO: Save as string (note we can only resolve this in DenumeratePointers then!)
	pComp->Value(mkNamingAdapt(mkTransformAdapt(AttachTrans), "AttachTransformation"));

	pComp->Value(mkParAdapt(*ChildDenumerator, this));
}

void StdMeshInstance::AttachedMesh::EnumeratePointers()
{
	ChildDenumerator->EnumeratePointers(this);
}

void StdMeshInstance::AttachedMesh::DenumeratePointers()
{
	ChildDenumerator->DenumeratePointers(this);
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh):
		Mesh(mesh), CurrentFaceOrdering(FO_Fixed),
		BoneTransforms(Mesh.GetNumBones(), StdMeshMatrix::Identity()),
		SubMeshInstances(Mesh.GetNumSubMeshes()), AttachParent(NULL),
		BoneTransformsDirty(false)
{
	for (unsigned int i = 0; i < Mesh.GetNumSubMeshes(); ++i)
	{
		const StdSubMesh& submesh = Mesh.GetSubMesh(i);
		SubMeshInstances[i] = new StdSubMeshInstance(submesh);
	}
}

StdMeshInstance::~StdMeshInstance()
{
	// If we are attached then detach from parent
	if (AttachParent)
		AttachParent->Parent->DetachMesh(AttachParent->Number);

	// Remove all attach children
	while (!AttachChildren.empty())
		DetachMesh(AttachChildren.back()->Number);

	while (!AnimationStack.empty())
		StopAnimation(AnimationStack.front());
	assert(AnimationNodes.empty());

	// Delete submeshes
	for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		delete SubMeshInstances[i];
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
	if (CurrentFaceOrdering != ordering)
	{
		CurrentFaceOrdering = ordering;
		if (ordering == FO_Fixed)
		{
			// Copy original face ordering from StdMesh
			for (unsigned int i = 0; i < Mesh.GetNumSubMeshes(); ++i)
			{
				const StdSubMesh& submesh = Mesh.GetSubMesh(i);
				//SubMeshInstances[i]->Faces = submesh.GetFaces();
				for (unsigned int j = 0; j < submesh.GetNumFaces(); ++j)
					SubMeshInstances[i]->Faces[j] = submesh.GetFace(j);
			}
		}

		//BoneTransformsDirty = true;

		// Update attachments (only own meshes for now... others might be displayed both attached and non-attached...)
		// still not optimal.
		for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
			if ((*iter)->OwnChild)
				(*iter)->Child->SetFaceOrdering(ordering);
	}
}

void StdMeshInstance::SetFaceOrderingForClrModulation(uint32_t clrmod)
{
	// TODO: This could do face ordering only for non-opaque submeshes

	bool opaque = true;
	for(unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		if(!SubMeshInstances[i]->Material->IsOpaque())
			{ opaque = false; break; }

	if(!opaque)
		SetFaceOrdering(FO_FarthestToNearest);
	else if( ((clrmod >> 24) & 0xff) != 0xff)
		SetFaceOrdering(FO_NearestToFarthest);
	else
		SetFaceOrdering(FO_Fixed);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	const StdMeshAnimation* animation = Mesh.GetAnimationByName(animation_name);
	if (!animation) { delete position; delete weight; return NULL; }

	return PlayAnimation(*animation, slot, sibling, position, weight);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	// Default
	if (!sibling) sibling = GetRootAnimationForSlot(slot);
	assert(!sibling || sibling->Slot == slot);

	// Find two subsequent numbers in case we need to create two nodes, so
	// script can deduce the second node.
	unsigned int Number1, Number2;
	for (Number1 = 0; Number1 < AnimationNodes.size(); ++Number1)
		if (AnimationNodes[Number1] == NULL && (!sibling || Number1+1 == AnimationNodes.size() || AnimationNodes[Number1+1] == NULL))
			break;
	/*  for(Number2 = Number1+1; Number2 < AnimationNodes.size(); ++Number2)
	    if(AnimationNodes[Number2] == NULL)
	      break;*/
	Number2 = Number1 + 1;

	position->Value = BoundBy(position->Value, Fix0, ftofix(animation.Length));
	weight->Value = BoundBy(weight->Value, Fix0, itofix(1));

	if (Number1 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) NULL);
	if (sibling && Number2 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) NULL);

	AnimationNode* child = new AnimationNode(&animation, position);
	AnimationNodes[Number1] = child;
	child->Number = Number1;
	child->Slot = slot;

	if (sibling)
	{
		AnimationNode* parent = new AnimationNode(child, sibling, weight);
		AnimationNodes[Number2] = parent;
		parent->Number = Number2;
		parent->Slot = slot;

		child->Parent = parent;
		parent->Parent = sibling->Parent;
		parent->LinearInterpolation.ChildLeft = sibling;
		parent->LinearInterpolation.ChildRight = child;
		if (sibling->Parent)
		{
			if (sibling->Parent->LinearInterpolation.ChildLeft == sibling)
				sibling->Parent->LinearInterpolation.ChildLeft = parent;
			else
				sibling->Parent->LinearInterpolation.ChildRight = parent;
		}
		else
		{
			// set new parent
			AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
			// slot must not be empty, since sibling uses same slot
			assert(iter != AnimationStack.end() && *iter != NULL);
			*iter = parent;
		}

		sibling->Parent = parent;
	}
	else
	{
		delete weight;
		AnimationNodeList::iterator iter = GetStackIterForSlot(slot, true);
		assert(!*iter); // we have a sibling if slot is not empty
		*iter = child;
	}

	BoneTransformsDirty = true;
	return child;
}

void StdMeshInstance::StopAnimation(AnimationNode* node)
{
	ClearAnimationListRecursively(AnimationNodes, node);

	AnimationNode* parent = node->Parent;
	if (parent == NULL)
	{
		AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
		assert(iter != AnimationStack.end() && *iter == node);
		AnimationStack.erase(iter);
		delete node;
	}
	else
	{
		assert(parent->Type == AnimationNode::LinearInterpolationNode);

		// Remove parent interpolation node and re-link
		AnimationNode* other_child;
		if (parent->LinearInterpolation.ChildLeft == node)
		{
			other_child = parent->LinearInterpolation.ChildRight;
			parent->LinearInterpolation.ChildRight = NULL;
		}
		else
		{
			other_child = parent->LinearInterpolation.ChildLeft;
			parent->LinearInterpolation.ChildLeft = NULL;
		}

		if (parent->Parent)
		{
			assert(parent->Parent->Type == AnimationNode::LinearInterpolationNode);
			if (parent->Parent->LinearInterpolation.ChildLeft == parent)
				parent->Parent->LinearInterpolation.ChildLeft = other_child;
			else
				parent->Parent->LinearInterpolation.ChildRight = other_child;
			other_child->Parent = parent->Parent;
		}
		else
		{
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
			assert(iter != AnimationStack.end() && *iter == parent);
			*iter = other_child;

			other_child->Parent = NULL;
		}

		AnimationNodes[parent->Number] = NULL;
		// Recursively deletes parent and its descendants
		delete parent;
	}

	while (!AnimationNodes.empty() && AnimationNodes.back() == NULL)
		AnimationNodes.erase(AnimationNodes.end()-1);
	BoneTransformsDirty = true;
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetAnimationNodeByNumber(unsigned int number)
{
	if (number >= AnimationNodes.size()) return NULL;
	return AnimationNodes[number];
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetRootAnimationForSlot(int slot)
{
	AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
	if (iter == AnimationStack.end()) return NULL;
	return *iter;
}

void StdMeshInstance::SetAnimationPosition(AnimationNode* node, ValueProvider* position)
{
	assert(node->GetType() == AnimationNode::LeafNode);
	delete node->Leaf.Position;
	node->Leaf.Position = position;

	position->Value = BoundBy(position->Value, Fix0, ftofix(node->Leaf.Animation->Length));

	BoneTransformsDirty = true;
}

void StdMeshInstance::SetAnimationWeight(AnimationNode* node, ValueProvider* weight)
{
	assert(node->GetType() == AnimationNode::LinearInterpolationNode);
	delete node->LinearInterpolation.Weight; node->LinearInterpolation.Weight = weight;

	weight->Value = BoundBy(weight->Value, Fix0, itofix(1));

	BoneTransformsDirty = true;
}

void StdMeshInstance::ExecuteAnimation(float dt)
{
	// Iterate from the back since slots might get removed
	for (unsigned int i = AnimationStack.size(); i > 0; --i)
		ExecuteAnimationNode(AnimationStack[i-1]);

	// Update animated textures
	for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
	{
		StdSubMeshInstance& submesh = *SubMeshInstances[i];
		const StdMeshMaterial& material = submesh.GetMaterial();
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];
		for (unsigned int j = 0; j < submesh.PassData.size(); ++j)
		{
			StdSubMeshInstance::Pass& pass = submesh.PassData[j];
			for (unsigned int k = 0; k < pass.TexUnits.size(); ++k)
			{
				const StdMeshMaterialTextureUnit& texunit = technique.Passes[j].TextureUnits[k];
				StdSubMeshInstance::TexUnit& texunit_instance = submesh.PassData[j].TexUnits[k];
				if (texunit.HasFrameAnimation())
				{
					const unsigned int NumPhases = texunit.GetNumTextures();
					const float PhaseDuration = texunit.Duration / NumPhases;

					const float Position = texunit_instance.PhaseDelay + dt;
					const unsigned int AddPhases = static_cast<unsigned int>(Position / PhaseDuration);

					texunit_instance.Phase = (texunit_instance.Phase + AddPhases) % NumPhases;
					texunit_instance.PhaseDelay = Position - AddPhases * PhaseDuration;
				}

				if (texunit.HasTexCoordAnimation())
					texunit_instance.Position += dt;
			}
		}
	}

	// Update animation for attached meshes
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		(*iter)->Child->ExecuteAnimation(dt);
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(const StdMesh& mesh, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation)
{
	StdMeshInstance* instance = new StdMeshInstance(mesh);
	instance->SetFaceOrdering(CurrentFaceOrdering);
	AttachedMesh* attach = AttachMesh(*instance, denumerator, parent_bone, child_bone, transformation, true);
	if (!attach) { delete instance; delete denumerator; return NULL; }
	return attach;
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, bool own_child)
{
	std::auto_ptr<AttachedMesh::Denumerator> auto_denumerator(denumerator);

	// We don't allow an instance to be attached to multiple parent instances for now
	if (instance.AttachParent) return NULL;

	// Make sure there are no cyclic attachments
	for (StdMeshInstance* Parent = this; Parent->AttachParent != NULL; Parent = Parent->AttachParent->Parent)
		if (Parent == &instance)
			return NULL;

	AttachedMesh* attach = NULL;
	unsigned int number = 1;

	// Find free index.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->Number >= number)
			number = (*iter)->Number + 1;

	const StdMeshBone* parent_bone_obj = Mesh.GetBoneByName(parent_bone);
	const StdMeshBone* child_bone_obj = instance.Mesh.GetBoneByName(child_bone);
	if (!parent_bone_obj || !child_bone_obj) return NULL;

	// TODO: Face Ordering is not lined up... can't do that properly here
	attach = new AttachedMesh(number, this, &instance, own_child, auto_denumerator.release(), parent_bone_obj->Index, child_bone_obj->Index, transformation);
	instance.AttachParent = attach;
	AttachChildren.push_back(attach);

	return attach;
}

bool StdMeshInstance::DetachMesh(unsigned int number)
{
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		if ((*iter)->Number == number)
		{
			// Reset attach parent of child so it does not try
			// to detach itself on destruction.
			(*iter)->Child->AttachParent = NULL;

			delete *iter;
			AttachChildren.erase(iter);
			return true;
		}
	}

	return false;
}

StdMeshInstance::AttachedMesh* StdMeshInstance::GetAttachedMeshByNumber(unsigned int number) const
{
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->Number == number)
			return *iter;
	return NULL;
}

bool StdMeshInstance::UpdateBoneTransforms()
{
	bool was_dirty = BoneTransformsDirty;

	// Nothing changed since last time
	if (BoneTransformsDirty)
	{
		// Compute transformation matrix for each bone.
		for (unsigned int i = 0; i < BoneTransforms.size(); ++i)
		{
			StdMeshTransformation Transformation;

			const StdMeshBone& bone = Mesh.GetBone(i);
			const StdMeshBone* parent = bone.GetParent();
			assert(!parent || parent->Index < i);

			bool have_transform = false;
			for (unsigned int j = 0; j < AnimationStack.size(); ++j)
			{
				if (have_transform)
				{
					StdMeshTransformation other;
					if (AnimationStack[j]->GetBoneTransform(i, other))
						Transformation = StdMeshTransformation::Nlerp(Transformation, other, 1.0f); // TODO: Allow custom weighing for slot combination
				}
				else
				{
					have_transform = AnimationStack[j]->GetBoneTransform(i, Transformation);
				}
			}

			if (!have_transform)
			{
				if (parent)
					BoneTransforms[i] = BoneTransforms[parent->Index];
				else
					BoneTransforms[i] = StdMeshMatrix::Identity();
			}
			else
			{
				BoneTransforms[i] = StdMeshMatrix::Transform(bone.Transformation * Transformation * bone.InverseTransformation);
				if (parent) BoneTransforms[i] = BoneTransforms[parent->Index] * BoneTransforms[i];
			}
		}

		// Compute transformation for each vertex. We could later think about
		// doing this on the GPU using a vertex shader. This would then probably
		// need to go to CStdGL::PerformMesh and CStdD3D::PerformMesh.
		// (can only work for fixed face ordering though)
		for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		{
			const StdSubMesh& submesh = Mesh.GetSubMesh(i);
			std::vector<StdMeshVertex>& instance_vertices = SubMeshInstances[i]->Vertices;
			assert(submesh.GetNumVertices() == instance_vertices.size());
			for (unsigned int j = 0; j < instance_vertices.size(); ++j)
			{
				const StdSubMesh::Vertex& vertex = submesh.GetVertex(j);
				StdMeshVertex& instance_vertex = instance_vertices[j];
				if (!vertex.BoneAssignments.empty())
				{
					instance_vertex.x = instance_vertex.y = instance_vertex.z = 0.0f;
					instance_vertex.nx = instance_vertex.ny = instance_vertex.nz = 0.0f;
					instance_vertex.u = vertex.u; instance_vertex.v = vertex.v;

					for (unsigned int k = 0; k < vertex.BoneAssignments.size(); ++k)
					{
						const StdMeshVertexBoneAssignment& assignment = vertex.BoneAssignments[k];

						instance_vertex += assignment.Weight * (BoneTransforms[assignment.BoneIndex] * vertex);
					}
				}
				else
				{
					instance_vertex = vertex;
				}
			}
		}
	}

	// Update attachment's attach transformations. Note this is done recursively.
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		AttachedMesh* attach = *iter;
		const bool ChildBoneTransformsDirty = attach->Child->BoneTransformsDirty;
		attach->Child->UpdateBoneTransforms();

		if (BoneTransformsDirty || ChildBoneTransformsDirty || attach->FinalTransformDirty)
		{
			was_dirty = true;

			// Compute matrix to change the coordinate system to the one of the attached bone:
			// The idea is that a vertex at the child bone's position transforms to the parent bone's position.
			// Therefore (read from right to left) we first apply the inverse of the child bone transformation,
			// then an optional scaling matrix, and finally the parent bone transformation

			// TODO: we can cache the three matrices in the middle since they don't change over time,
			// reducing this to two matrix multiplications instead of four each frame.
			// Might even be worth to compute the complete transformation directly when rendering then
			// (saves per-instance memory, but requires recomputation if the animation does not change).
			// TODO: We might also be able to cache child inverse, and only recomupte it if
			// child bone transforms are dirty (saves matrix inversion for unanimated attach children).
			attach->FinalTrans = BoneTransforms[attach->ParentBone]
			                     * StdMeshMatrix::Transform(Mesh.GetBone(attach->ParentBone).Transformation)
			                     * attach->AttachTrans
			                     * StdMeshMatrix::Transform(attach->Child->Mesh.GetBone(attach->ChildBone).InverseTransformation)
			                     * StdMeshMatrix::Inverse(attach->Child->BoneTransforms[attach->ChildBone]);

			attach->FinalTransformDirty = false;
		}
	}

	BoneTransformsDirty = false;
	return was_dirty;
}

void StdMeshInstance::ReorderFaces(StdMeshMatrix* global_trans)
{
	if(CurrentFaceOrdering != FO_Fixed)
	{
		for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		{
			StdMeshInstanceFaceOrderingCmpPred pred(*this, i, global_trans ? *global_trans : StdMeshMatrix::Identity());
			std::sort(SubMeshInstances[i]->Faces.begin(), SubMeshInstances[i]->Faces.end(), pred);
		}
	}

	// TODO: Also reorder submeshes, attached meshes and include AttachTransformation for attached meshes...
}

void StdMeshInstance::CompileFunc(StdCompiler* pComp, AttachedMesh::DenumeratorFactoryFunc Factory)
{
	if(pComp->isCompiler())
	{
		// Only initially created instances can be compiled
		assert(!AttachParent);
		assert(AttachChildren.empty());
		assert(AnimationStack.empty());
		BoneTransformsDirty = true;

		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(int32_t i = 0; i < iAnimCnt; ++i)
		{
			AnimationNode* node = NULL;
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(node, "AnimationNode"), &Mesh));
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, true);
			if(*iter != NULL) { delete node; pComp->excCorrupt("Duplicate animation slot index"); }
			*iter = node;

			// Add nodes into lookup table
			std::vector<AnimationNode*> nodes(1, node);
			while(!nodes.empty())
			{
				node = nodes.back();
				nodes.erase(nodes.end()-1);

				AnimationNodes.resize(node->Number+1);
				if(AnimationNodes[node->Number] != NULL) pComp->excCorrupt("Duplicate animation node number");
				AnimationNodes[node->Number] = node;

				if(node->Type == AnimationNode::LinearInterpolationNode)
				{
					nodes.push_back(node->LinearInterpolation.ChildLeft);
					nodes.push_back(node->LinearInterpolation.ChildRight);
				}
			}
		}

		int32_t iAttachedCnt;
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));

		for(int32_t i = 0; i < iAttachedCnt; ++i)
		{
			AttachChildren.push_back(new AttachedMesh);
			AttachedMesh* attach = AttachChildren.back();

			attach->Parent = this;
			pComp->Value(mkNamingAdapt(mkParAdapt(*attach, Factory), "Attached"));
		}
	}
	else
	{
		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(*iter, "AnimationNode"), &Mesh));

		int32_t iAttachedCnt = AttachChildren.size();
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));
		
		for(unsigned int i = 0; i < AttachChildren.size(); ++i)
			pComp->Value(mkNamingAdapt(mkParAdapt(*AttachChildren[i], Factory), "Attached"));
	}
}

void StdMeshInstance::EnumeratePointers()
{
	for(unsigned int i = 0; i < AnimationNodes.size(); ++i)
		if(AnimationNodes[i])
			AnimationNodes[i]->EnumeratePointers();

	for(unsigned int i = 0; i < AttachChildren.size(); ++i)
		AttachChildren[i]->EnumeratePointers();
}

void StdMeshInstance::DenumeratePointers()
{
	for(unsigned int i = 0; i < AnimationNodes.size(); ++i)
		if(AnimationNodes[i])
			AnimationNodes[i]->DenumeratePointers();

	for(unsigned int i = 0; i < AttachChildren.size(); ++i)
		AttachChildren[i]->DenumeratePointers();
}

StdMeshInstance::AnimationNodeList::iterator StdMeshInstance::GetStackIterForSlot(int slot, bool create)
{
	// TODO: bsearch
	for (AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
	{
		if ((*iter)->Slot == slot)
		{
			return iter;
		}
		else if ((*iter)->Slot > slot)
		{
			if (!create)
				return AnimationStack.end();
			else
				return AnimationStack.insert(iter, NULL);
		}
	}

	if (!create)
		return AnimationStack.end();
	else
		return AnimationStack.insert(AnimationStack.end(), NULL);
}

bool StdMeshInstance::ExecuteAnimationNode(AnimationNode* node)
{
	ValueProvider* provider = NULL;
	C4Real min;
	C4Real max;

	switch (node->GetType())
	{
	case AnimationNode::LeafNode:
		provider = node->GetPositionProvider();
		min = Fix0;
		max = ftofix(node->GetAnimation()->Length);
		break;
	case AnimationNode::LinearInterpolationNode:
		provider = node->GetWeightProvider();
		min = Fix0;
		max = itofix(1);
		break;
	}

	const C4Real old_value = provider->Value;
	if (!provider->Execute())
	{
		if (node->GetType() == AnimationNode::LeafNode) return false;

		// Remove the child with less weight (normally weight reaches 0.0 or 1.0)
		if (node->GetWeight() > itofix(1, 2))
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetRightChild())) return false;
			// Remove left child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildLeft);
		}
		else
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetLeftChild())) return false;
			// Remove right child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildRight);
		}


	}
	else
	{
		if (provider->Value != old_value)
		{
			provider->Value = BoundBy(provider->Value, min, max);
			BoneTransformsDirty = true;
		}

		if (node->GetType() == AnimationNode::LinearInterpolationNode)
		{
			const bool left_result = ExecuteAnimationNode(node->GetLeftChild());
			const bool right_result = ExecuteAnimationNode(node->GetRightChild());

			// Remove this node completely
			if (!left_result && !right_result)
				return false;

			// Note that either of this also removes node
			if (!left_result)
				StopAnimation(node->GetLeftChild());
			if (!right_result)
				StopAnimation(node->GetRightChild());
		}
	}

	return true;
}

