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
#	define _USE_MATH_DEFINES
#	include <math.h>
#endif

#include <tinyxml/tinyxml.h>

#include <algorithm>

namespace
{
	// Helper to sort faces for FaceOrdering
	struct StdMeshInstanceFaceOrderingCmpPred
	{
		const StdMeshInstance& m_inst;
		const StdMeshVertex* m_vertices;

		StdMeshInstanceFaceOrderingCmpPred(const StdMeshInstance& inst, unsigned int submesh):
			m_inst(inst), m_vertices(m_inst.GetVertices(submesh)) {}

		bool operator()(const StdMeshFace& face1, const StdMeshFace& face2) const
		{
			// TODO: Need to apply attach matrix in case of attached meshes
			switch(m_inst.GetFaceOrdering())
			{
			case StdMeshInstance::FO_Fixed:
				assert(false);
				return false;
			case StdMeshInstance::FO_FarthestToNearest:
			case StdMeshInstance::FO_NearestToFarthest:
				{
					float z1 = m_vertices[face1.Vertices[0]].z + m_vertices[face1.Vertices[1]].z + m_vertices[face1.Vertices[2]].z;
					float z2 = m_vertices[face2.Vertices[0]].z + m_vertices[face2.Vertices[1]].z + m_vertices[face2.Vertices[2]].z;
					if(m_inst.GetFaceOrdering() == StdMeshInstance::FO_FarthestToNearest)
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

	// Reset all animation list entries corresponding to node or its children
	void ClearAnimationListRecursively(std::vector<StdMeshInstance::AnimationNode*>& list, StdMeshInstance::AnimationNode* node)
	{
		list[node->GetNumber()] = NULL;

		if(node->GetType() == StdMeshInstance::AnimationNode::LinearInterpolationNode)
		{
			ClearAnimationListRecursively(list, node->GetLeftChild());
			ClearAnimationListRecursively(list, node->GetRightChild());
		}
	}

	// Generate matrix to convert the mesh from Ogre coordinate system to Clonk coordinate system.
	StdMeshMatrix CoordCorrection = StdMeshMatrix::Scale(-1.0f, 1.0f, 1.0f) * StdMeshMatrix::Rotate(M_PI/2.0f, 1.0f, 0.0f, 0.0f) * StdMeshMatrix::Rotate(M_PI/2.0f, 0.0f, 0.0f, 1.0f);
	StdMeshMatrix CoordCorrectionInverse = StdMeshMatrix::Inverse(CoordCorrection);

	// Switch Transformation from one coordinate system to another, described by the given
	// change-of-basis matrix.
	StdMeshTransformation SwitchTransformation(const StdMeshTransformation& Transformation, const StdMeshMatrix& basis)
	{
		// TODO: The transformation breaks here if it includes a scale part (|det| != 1)
		// Probably not trivial to fix, since transformation scale behaves different than matrix scale when transforming
		// Remember also to normalize rotation axis again when fixing this
		// Note that we only use this to transform from OGRE to Clonk coordinate system currently,
		// which is described by an orthogonal matrix (det=-1).
		StdMeshTransformation new_transformation;
		new_transformation.scale = Transformation.scale;
		new_transformation.rotate.w = Transformation.rotate.w;
		new_transformation.rotate.v = -(basis*Transformation.rotate.v); // negative sign because v is a pseudovector, and basis has negative determinant (TODO: Determine determinant at run-time)
		new_transformation.translate = basis * Transformation.translate; // TODO: I think this should also apply translation part of change-of-basis matrix
		return new_transformation;
	}

	// Moves a transformation from OGRE to Clonk coordinate system	
	StdMeshTransformation CoordCorrectedTransformation(const StdMeshTransformation& Transformation)
	{
		return SwitchTransformation(Transformation, CoordCorrection);
	}
	
	StdMeshTransformation InverseCoordCorrectedTransformation(const StdMeshTransformation& Transformation)
	{
		return SwitchTransformation(Transformation, CoordCorrectionInverse);
	}
}

StdMeshError::StdMeshError(const StdStrBuf& message, const char* file, unsigned int line)
{
	Buf.Format("%s:%u: %s", file, line, message.getData());
}

// Helper class to load things from an XML file with error checking
class StdMeshXML
{
public:
	StdMeshXML(const char* filename, const char* xml_data);

	const char* RequireStrAttribute(TiXmlElement* element, const char* attribute) const;
	int RequireIntAttribute(TiXmlElement* element, const char* attribute) const;
	float RequireFloatAttribute(TiXmlElement* element, const char* attribute) const;

	TiXmlElement* RequireFirstChild(TiXmlElement* element, const char* child);

	void Error(const StdStrBuf& message, TiXmlElement* element) const;

private:
	TiXmlDocument Document;
	StdStrBuf FileName;
};

StdMeshXML::StdMeshXML(const char* filename, const char* xml_data):
	FileName(filename)
{
	Document.Parse(xml_data);
	if(Document.Error())
		throw StdMeshError(StdStrBuf(Document.ErrorDesc()), FileName.getData(), Document.ErrorRow());
}

const char* StdMeshXML::RequireStrAttribute(TiXmlElement* element, const char* attribute) const
{
	const char* value = element->Attribute(attribute);
	if(!value) Error(FormatString("Element '%s' does not have attribute '%s'", element->Value(), attribute), element);
	return value;
}

int StdMeshXML::RequireIntAttribute(TiXmlElement* element, const char* attribute) const
{
	int retval;
	if(element->QueryIntAttribute(attribute, &retval) != TIXML_SUCCESS)
		Error(FormatString("Element '%s' does not have integer attribute '%s'", element->Value(), attribute), element);
	return retval;
}

float StdMeshXML::RequireFloatAttribute(TiXmlElement* element, const char* attribute) const
{
	float retval;
	if(element->QueryFloatAttribute(attribute, &retval) != TIXML_SUCCESS)
		Error(FormatString("Element '%s' does not have floating point attribute '%s'", element->Value(), attribute), element);
	return retval;
}

TiXmlElement* StdMeshXML::RequireFirstChild(TiXmlElement* element, const char* child)
{
	TiXmlElement* retval;
	
	if(element)
	{
		retval = element->FirstChildElement(child);
		if(!retval)
			Error(FormatString("Element '%s' does not contain '%s' child", element->Value(), child), element);
	}
	else
	{
		retval = Document.RootElement();
		if(strcmp(retval->Value(), child) != 0)
			Error(FormatString("Root element is not '%s'", child), retval);
	}

	return retval;
}

void StdMeshXML::Error(const StdStrBuf& message, TiXmlElement* element) const
{
	throw StdMeshError(message, FileName.getData(), element->Row());
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
	if(c < 0.0f)
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
	t.rotate = -transform.rotate;
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

	const float det = mat.a[0][0]*mat.a[1][1]*mat.a[2][2] + mat.a[0][1]*mat.a[1][2]*mat.a[2][0] + mat.a[0][2]*mat.a[1][0]*mat.a[2][1]
	                - mat.a[0][0]*mat.a[1][2]*mat.a[2][1] - mat.a[0][1]*mat.a[1][0]*mat.a[2][2] - mat.a[0][2]*mat.a[1][1]*mat.a[2][0];
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

	m.a[0][0] = rx*rx*(1-c)+c;		m.a[0][1] = rx*ry*(1-c)-rz*s; m.a[0][2] = rx*rz*(1-c)+ry*s; m.a[0][3] = 0.0f;
	m.a[1][0] = ry*rx*(1-c)+rz*s; m.a[1][1] = ry*ry*(1-c)+c;		m.a[1][2] = ry*rz*(1-c)-rx*s; m.a[1][3] = 0.0f;
	m.a[2][0] = rz*rx*(1-c)-ry*s; m.a[2][1] = ry*rz*(1-c)+rx*s; m.a[2][2] = rz*rz*(1-c)+c;		m.a[2][3] = 0.0f;
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
	q.w = rhs.w;
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
	StdMeshVector uv = 2.0f * StdMeshVector::Cross(lhs.v, rhs);
	StdMeshVector uuv = StdMeshVector::Cross(lhs.v, uv);
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
	if(iter == Frames.end())
	{
		std::map<float, StdMeshKeyFrame>::const_iterator citer = iter; --citer;
		printf("Time: %f (%f-%f)\n", time, Frames.begin()->first, citer->first);
		if(time<=0.0) iter = Frames.begin();
	}
	assert(iter != Frames.end());

	if(iter == Frames.begin())
		return iter->second.Transformation;

	std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
	-- prev_iter;

	float dt = iter->first - prev_iter->first;
	float weight1 = (time - prev_iter->first) / dt;
	float weight2 = (iter->first - time) / dt;

	assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
	assert(fabs(weight1 + weight2 - 1) < 1e-6);

	StdMeshTransformation transformation;
	transformation.scale = weight1 * iter->second.Transformation.scale + weight2 * prev_iter->second.Transformation.scale;
	transformation.rotate = weight1 * iter->second.Transformation.rotate + weight2 * prev_iter->second.Transformation.rotate; // TODO: slerp or renormalize
	transformation.translate = weight1 * iter->second.Transformation.translate + weight2 * prev_iter->second.Transformation.translate;
	return transformation;
	//return StdMeshTransformation::Nlerp(prev_iter->second.Transformation, iter->second.Transformation, weight1);
}

StdMeshAnimation::StdMeshAnimation(const StdMeshAnimation& other):
	Name(other.Name), Length(other.Length), Tracks(other.Tracks.size())
{
	// Note that all Tracks are already default-initialized to zero
	for(unsigned int i = 0; i < Tracks.size(); ++i)
		if(other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);
}

StdMeshAnimation::~StdMeshAnimation()
{
	for(unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];
}

StdMeshAnimation& StdMeshAnimation::operator=(const StdMeshAnimation& other)
{
	if(this == &other) return *this;

	Name = other.Name;
	Length = other.Length;

	for(unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];

	Tracks.resize(other.Tracks.size());

	for(unsigned int i = 0; i < Tracks.size(); ++i)
		if(other.Tracks[i])
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
}

StdMesh::~StdMesh()
{
	for(unsigned int i = 0; i < Bones.size(); ++i)
		delete Bones[i];
}

void StdMesh::InitXML(const char* filename, const char* xml_data, StdMeshSkeletonLoader& skel_loader, const StdMeshMatManager& manager)
{
	StdMeshXML mesh(filename, xml_data);

	TiXmlElement* mesh_elem = mesh.RequireFirstChild(NULL, "mesh");
	TiXmlElement* submeshes_elem = mesh.RequireFirstChild(mesh_elem, "submeshes");

	TiXmlElement* submesh_elem_base = mesh.RequireFirstChild(submeshes_elem, "submesh");
	for(TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != NULL; submesh_elem = submesh_elem->NextSiblingElement("submesh"))
	{
		TiXmlElement* geometry_elem = mesh.RequireFirstChild(submesh_elem, "geometry");
		
		SubMeshes.push_back(StdSubMesh());
		StdSubMesh& submesh = SubMeshes.back();

		const char* material = mesh.RequireStrAttribute(submesh_elem, "material");
		submesh.Material = manager.GetMaterial(material);
		if(!submesh.Material)
			mesh.Error(FormatString("There is no such material named '%s'", material), submesh_elem);

		int VertexCount = mesh.RequireIntAttribute(geometry_elem, "vertexcount");
		submesh.Vertices.resize(VertexCount);

		TiXmlElement* buffer_elem = mesh.RequireFirstChild(geometry_elem, "vertexbuffer");

		unsigned int i = 0;
		for(TiXmlElement* vertex_elem = buffer_elem->FirstChildElement("vertex"); vertex_elem != NULL && i < submesh.Vertices.size(); vertex_elem = vertex_elem->NextSiblingElement("vertex"), ++i)
		{
			TiXmlElement* position_elem = mesh.RequireFirstChild(vertex_elem, "position");
			TiXmlElement* normal_elem = mesh.RequireFirstChild(vertex_elem, "normal");
			TiXmlElement* texcoord_elem = mesh.RequireFirstChild(vertex_elem, "texcoord");

			submesh.Vertices[i].x = mesh.RequireFloatAttribute(position_elem, "x");
			submesh.Vertices[i].y = mesh.RequireFloatAttribute(position_elem, "y");
			submesh.Vertices[i].z = mesh.RequireFloatAttribute(position_elem, "z");
			submesh.Vertices[i].nx = mesh.RequireFloatAttribute(normal_elem, "x");
			submesh.Vertices[i].ny = mesh.RequireFloatAttribute(normal_elem, "y");
			submesh.Vertices[i].nz = mesh.RequireFloatAttribute(normal_elem, "z");
			submesh.Vertices[i].u = mesh.RequireFloatAttribute(texcoord_elem, "u");
			submesh.Vertices[i].v = mesh.RequireFloatAttribute(texcoord_elem, "v");

			// Convert to Clonk coordinate system (type does not match, StdMeshVertex vs. StdMesh::Vertex)
			StdMeshVertex Transformed = CoordCorrection * submesh.Vertices[i];
			submesh.Vertices[i].nx = Transformed.nx; submesh.Vertices[i].ny = Transformed.ny; submesh.Vertices[i].nz = Transformed.nz;
			submesh.Vertices[i].x  = Transformed.x;  submesh.Vertices[i].y  = Transformed.y;  submesh.Vertices[i].z  = Transformed.z;

			// Construct BoundingBox
			if(i == 0 && SubMeshes.size() == 1)
			{
				// First vertex
				BoundingBox.x1 = BoundingBox.x2 = submesh.Vertices[i].x;
				BoundingBox.y1 = BoundingBox.y2 = submesh.Vertices[i].y;
				BoundingBox.z1 = BoundingBox.z2 = submesh.Vertices[i].z;
			}
			else
			{
				BoundingBox.x1 = Min(submesh.Vertices[i].x, BoundingBox.x1);
				BoundingBox.x2 = Max(submesh.Vertices[i].x, BoundingBox.x2);
				BoundingBox.y1 = Min(submesh.Vertices[i].y, BoundingBox.y1);
				BoundingBox.y2 = Max(submesh.Vertices[i].y, BoundingBox.y2);
				BoundingBox.z1 = Min(submesh.Vertices[i].z, BoundingBox.z1);
				BoundingBox.z2 = Max(submesh.Vertices[i].z, BoundingBox.z2);
			}
		}

		TiXmlElement* faces_elem = mesh.RequireFirstChild(submesh_elem, "faces");
		int FaceCount = mesh.RequireIntAttribute(faces_elem, "count");
		submesh.Faces.resize(FaceCount);

		i = 0;
		for(TiXmlElement* face_elem = faces_elem->FirstChildElement("face"); face_elem != NULL && i < submesh.Faces.size(); face_elem = face_elem->NextSiblingElement("face"), ++i)
		{
			int v[3];

			v[0] = mesh.RequireIntAttribute(face_elem, "v1");
			v[1] = mesh.RequireIntAttribute(face_elem, "v2");
			v[2] = mesh.RequireIntAttribute(face_elem, "v3");

			for(unsigned int j = 0; j < 3; ++j)
			{
				if(v[j] < 0 || static_cast<unsigned int>(v[j]) >= submesh.Vertices.size())
					mesh.Error(FormatString("Vertex index v%u (%d) is out of range", j+1, v[j]), face_elem);
				submesh.Faces[i].Vertices[j] = v[j];
			}
		}
	}

	// Read skeleton, if any
	TiXmlElement* skeletonlink_elem = mesh_elem->FirstChildElement("skeletonlink");
	if(skeletonlink_elem)
	{
		const char* name = mesh.RequireStrAttribute(skeletonlink_elem, "name");
		StdCopyStrBuf xml_filename(name); xml_filename.Append(".xml");

		StdStrBuf skeleton_xml_data = skel_loader.LoadSkeleton(xml_filename.getData());
		if(skeleton_xml_data.isNull()) mesh.Error(FormatString("Failed to load '%s'", xml_filename.getData()), skeletonlink_elem);

		StdMeshXML skeleton(xml_filename.getData(), skeleton_xml_data.getData());
		TiXmlElement* skeleton_elem = skeleton.RequireFirstChild(NULL, "skeleton");
		TiXmlElement* bones_elem = skeleton.RequireFirstChild(skeleton_elem, "bones");

		// Read bones. Don't insert into Master bone table yet, as the master bone
		// table is sorted hierarchically, and we will read the hierarchy only
		// afterwards.
		std::vector<StdMeshBone*> bones;
		for(TiXmlElement* bone_elem = bones_elem->FirstChildElement("bone"); bone_elem != NULL; bone_elem = bone_elem->NextSiblingElement("bone"))
		{
			StdMeshBone* bone = new StdMeshBone;
			bones.push_back(bone);

			bone->ID = skeleton.RequireIntAttribute(bone_elem, "id");
			bone->Name = skeleton.RequireStrAttribute(bone_elem, "name");
			// TODO: Make sure ID and name are unique

			bone->Parent = NULL;
			// Index of bone will be set when building Master Bone Table later

			TiXmlElement* position_elem = skeleton.RequireFirstChild(bone_elem, "position");
			TiXmlElement* rotation_elem = skeleton.RequireFirstChild(bone_elem, "rotation");
			TiXmlElement* axis_elem = skeleton.RequireFirstChild(rotation_elem, "axis");

			StdMeshVector d, r;
			d.x = skeleton.RequireFloatAttribute(position_elem, "x");
			d.y = skeleton.RequireFloatAttribute(position_elem, "y");
			d.z = skeleton.RequireFloatAttribute(position_elem, "z");
			float angle = skeleton.RequireFloatAttribute(rotation_elem, "angle");
			r.x = skeleton.RequireFloatAttribute(axis_elem, "x");
			r.y = skeleton.RequireFloatAttribute(axis_elem, "y");
			r.z = skeleton.RequireFloatAttribute(axis_elem, "z");

			bone->Transformation.scale = StdMeshVector::UnitScale();
			bone->Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, r);
			bone->Transformation.translate = d;

			// Convert into Clonk coordinate system
			bone->Transformation = CoordCorrectedTransformation(bone->Transformation);
			// We need this later for applying animations, therefore cache it here
			bone->InverseTransformation = StdMeshTransformation::Inverse(bone->Transformation);
		}

		// Bone hierarchy
		TiXmlElement* bonehierarchy_elem = skeleton.RequireFirstChild(skeleton_elem, "bonehierarchy");
		for(TiXmlElement* boneparent_elem = bonehierarchy_elem->FirstChildElement("boneparent"); boneparent_elem != NULL; boneparent_elem = boneparent_elem->NextSiblingElement("boneparent"))
		{
			const char* child_name = skeleton.RequireStrAttribute(boneparent_elem, "bone");
			const char* parent_name = skeleton.RequireStrAttribute(boneparent_elem, "parent");

			// Lookup the two bones
			StdMeshBone* child = NULL;
			StdMeshBone* parent = NULL;
			for(unsigned int i = 0; i < bones.size() && (!child || !parent); ++i)
			{
				if(!child && bones[i]->Name == child_name)
					child = bones[i];
				if(!parent && bones[i]->Name == parent_name)
					parent = bones[i];
			}

			if(!child) skeleton.Error(FormatString("There is no such bone with name '%s'", child_name), boneparent_elem);
			if(!parent) skeleton.Error(FormatString("There is no such bone with name '%s'", parent_name), boneparent_elem);

			child->Parent = parent;
			parent->Children.push_back(child);

			// Apply parent transformation			
			child->Transformation = parent->Transformation * child->Transformation;
			// Update inverse
			child->InverseTransformation = StdMeshTransformation::Inverse(child->Transformation);
		}

		// Fill master bone table in hierarchical order:
		for(unsigned int i = 0; i < bones.size(); ++i)
			if(bones[i]->Parent == NULL)
				AddMasterBone(bones[i]);

		// Vertex<->Bone assignments for all vertices (need to go through SubMeshes again...)
		unsigned int submesh_index = 0;
		for(TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != NULL; submesh_elem = submesh_elem->NextSiblingElement("submesh"), ++submesh_index)
		{
			StdSubMesh& submesh = SubMeshes[submesh_index];

			TiXmlElement* boneassignments_elem = mesh.RequireFirstChild(submesh_elem, "boneassignments");
			for(TiXmlElement* vertexboneassignment_elem = boneassignments_elem->FirstChildElement("vertexboneassignment"); vertexboneassignment_elem != NULL; vertexboneassignment_elem = vertexboneassignment_elem->NextSiblingElement("vertexboneassignment"))
			{
				int BoneID = mesh.RequireIntAttribute(vertexboneassignment_elem, "boneindex");
				int VertexIndex = mesh.RequireIntAttribute(vertexboneassignment_elem, "vertexindex");
				float weight = mesh.RequireFloatAttribute(vertexboneassignment_elem, "weight");

				if(VertexIndex < 0 || static_cast<unsigned int>(VertexIndex) >= submesh.Vertices.size())
					mesh.Error(FormatString("Vertex index in bone assignment (%d) is out of range", VertexIndex), vertexboneassignment_elem);

				StdMeshBone* bone = NULL;
				for(unsigned int i = 0; !bone && i < bones.size(); ++i)
					if(bones[i]->ID == BoneID)
						bone = bones[i];

				if(!bone) mesh.Error(FormatString("There is no such bone with index %d", BoneID), vertexboneassignment_elem);

				StdSubMesh::Vertex& vertex = submesh.Vertices[VertexIndex];
				vertex.BoneAssignments.push_back(StdMeshVertexBoneAssignment());
				StdMeshVertexBoneAssignment& assignment = vertex.BoneAssignments.back();
				assignment.BoneIndex = bone->Index;
				assignment.Weight = weight;
			}

			// Normalize vertex bone assignment weights (this is not guaranteed in the
			// Ogre file format).
			for(unsigned int i = 0; i < submesh.Vertices.size(); ++i)
			{
				StdSubMesh::Vertex& vertex = submesh.Vertices[i];
				float sum = 0.0f;
				for(unsigned int j = 0; j < vertex.BoneAssignments.size(); ++j)
					sum += vertex.BoneAssignments[j].Weight;
				for(unsigned int j = 0; j < vertex.BoneAssignments.size(); ++j)
					vertex.BoneAssignments[j].Weight /= sum;
			}
		}

		// Load Animations
		TiXmlElement* animations_elem = skeleton.RequireFirstChild(skeleton_elem, "animations");
		for(TiXmlElement* animation_elem = animations_elem->FirstChildElement("animation"); animation_elem != NULL; animation_elem = animation_elem->NextSiblingElement("animation"))
		{
			StdCopyStrBuf name(skeleton.RequireStrAttribute(animation_elem, "name"));
			if(Animations.find(name) != Animations.end())
			skeleton.Error(FormatString("There is already an animation with name '%s'", name.getData()), animation_elem);

			StdMeshAnimation& animation = Animations.insert(std::make_pair(name, StdMeshAnimation())).first->second;
			animation.Name = name;
			animation.Length = skeleton.RequireFloatAttribute(animation_elem, "length");
			animation.Tracks.resize(Bones.size());

			TiXmlElement* tracks_elem = skeleton.RequireFirstChild(animation_elem, "tracks");
			for(TiXmlElement* track_elem = tracks_elem->FirstChildElement("track"); track_elem != NULL; track_elem = track_elem->NextSiblingElement("track"))
			{
				const char* bone_name = skeleton.RequireStrAttribute(track_elem, "bone");
				StdMeshBone* bone = NULL;
				for(unsigned int i = 0; !bone && i < Bones.size(); ++i)
					if(Bones[i]->Name == bone_name)
						bone = Bones[i];
				if(!bone) skeleton.Error(FormatString("There is no such bone with name '%s'", bone_name), track_elem);

				if(animation.Tracks[bone->Index] != NULL) skeleton.Error(FormatString("There is already a track for bone '%s' in animation '%s'", bone_name, animation.Name.getData()), track_elem);

				StdMeshTrack* track = new StdMeshTrack;
				animation.Tracks[bone->Index] = track;

				// Get inverse bone transformation in OGRE coordiante system; we need it to apply
				// the translation part of the bone transformation.
				StdMeshTransformation BoneInverseTrans = InverseCoordCorrectedTransformation(bone->InverseTransformation);

				TiXmlElement* keyframes_elem = skeleton.RequireFirstChild(track_elem, "keyframes");
				for(TiXmlElement* keyframe_elem = keyframes_elem->FirstChildElement("keyframe"); keyframe_elem != NULL; keyframe_elem = keyframe_elem->NextSiblingElement("keyframe"))
				{
					float time = skeleton.RequireFloatAttribute(keyframe_elem, "time");
					StdMeshKeyFrame& frame = track->Frames[time];

					TiXmlElement* translate_elem = skeleton.RequireFirstChild(keyframe_elem, "translate");
					TiXmlElement* rotate_elem = skeleton.RequireFirstChild(keyframe_elem, "rotate");
					TiXmlElement* scale_elem = skeleton.RequireFirstChild(keyframe_elem, "scale");
					TiXmlElement* axis_elem = skeleton.RequireFirstChild(rotate_elem, "axis");

					StdMeshVector d, s, r;
					d.x = skeleton.RequireFloatAttribute(translate_elem, "x");
					d.y = skeleton.RequireFloatAttribute(translate_elem, "y");
					d.z = skeleton.RequireFloatAttribute(translate_elem, "z");
					s.x = skeleton.RequireFloatAttribute(scale_elem, "x");
					s.y = skeleton.RequireFloatAttribute(scale_elem, "y");
					s.z = skeleton.RequireFloatAttribute(scale_elem, "z");
					float angle = skeleton.RequireFloatAttribute(rotate_elem, "angle");
					r.x = skeleton.RequireFloatAttribute(axis_elem, "x");
					r.y = skeleton.RequireFloatAttribute(axis_elem, "y");
					r.z = skeleton.RequireFloatAttribute(axis_elem, "z");

					frame.Transformation.scale = StdMeshVector::UnitScale();
					frame.Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, r);
					frame.Transformation.translate = BoneInverseTrans.rotate * (BoneInverseTrans.scale * d);

					frame.Transformation = CoordCorrectedTransformation(frame.Transformation);
				}
			}
		}
	}
	else
	{
		// Mesh has no skeleton
		for(TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != NULL; submesh_elem = submesh_elem->NextSiblingElement("submesh"))
		{
			TiXmlElement* boneassignments_elem = submesh_elem->FirstChildElement("boneassignments");
			if(boneassignments_elem)
			{
				// Bone assignements do not make sense then, as the
				// actual bones are defined in the skeleton file.
				mesh.Error(StdStrBuf("Mesh has bone assignments, but no skeleton"), boneassignments_elem);
			}
		}
	}
}

void StdMesh::AddMasterBone(StdMeshBone* bone)
{
	bone->Index = Bones.size(); // Remember index in master bone table
	Bones.push_back(bone);
	for(unsigned int i = 0; i < bone->Children.size(); ++i)
		AddMasterBone(bone->Children[i]);
}

const StdMeshAnimation* StdMesh::GetAnimationByName(const StdStrBuf& name) const
{
	StdCopyStrBuf name2(name);
	std::map<StdCopyStrBuf, StdMeshAnimation>::const_iterator iter = Animations.find(name2);
	if(iter == Animations.end()) return NULL;
	return &iter->second;
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
	switch(Type)
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

	switch(Type)
	{
	case LeafNode:
		//if(!Leaf.Animation) return false;
		track = Leaf.Animation->Tracks[bone];
		if(!track) return false;
		transformation = track->GetTransformAt(Leaf.Position->Value);
		return true;
	case LinearInterpolationNode:
		if(!LinearInterpolation.ChildLeft->GetBoneTransform(bone, transformation))
			return LinearInterpolation.ChildRight->GetBoneTransform(bone, transformation);
		if(!LinearInterpolation.ChildRight->GetBoneTransform(bone, combine_with))
			return true; // First Child affects bone

		transformation = StdMeshTransformation::Nlerp(transformation, combine_with, LinearInterpolation.Weight->Value);
		return true;
	default:
		assert(false);
		return false;
	}
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh):
	Mesh(mesh), CurrentFaceOrdering(FO_Fixed),
	BoneTransforms(Mesh.GetNumBones(), StdMeshMatrix::Identity()),
	Vertices(Mesh.GetNumSubMeshes()), Faces(Mesh.GetNumSubMeshes()),
	AttachParent(NULL), BoneTransformsDirty(false)
{
	for(unsigned int i = 0; i < Mesh.GetNumSubMeshes(); ++i)
	{
		const StdSubMesh& submesh = Mesh.GetSubMesh(i);
		Vertices[i].resize(submesh.GetNumVertices());
		Faces[i].resize(submesh.GetNumFaces());

		for(unsigned int j = 0; j < submesh.GetNumVertices(); ++j)
			Vertices[i][j] = submesh.GetVertex(j);
		for(unsigned int j = 0; j < submesh.GetNumFaces(); ++j)
			Faces[i][j] = submesh.GetFace(j);
	}
}

StdMeshInstance::~StdMeshInstance()
{
	for(AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		delete iter->Child;
	while(!AnimationStack.empty())
		StopAnimation(AnimationStack.front());
	assert(AnimationNodes.empty());
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
	if(CurrentFaceOrdering != ordering)
	{
		CurrentFaceOrdering = ordering;
		if(ordering == FO_Fixed)
		{
			// Copy original face ordering from StdMesh
			for(unsigned int i = 0; i < Mesh.GetNumSubMeshes(); ++i)
			{
				const StdSubMesh& submesh = Mesh.GetSubMesh(i);
				for(unsigned int j = 0; j < submesh.GetNumFaces(); ++j)
					Faces[i][j] = submesh.GetFace(j);
			}
		}

		BoneTransformsDirty = true;

		// Update attachments
		for(AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
			iter->Child->SetFaceOrdering(ordering);
	}
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	const StdMeshAnimation* animation = Mesh.GetAnimationByName(animation_name);
	if(!animation) { delete position; delete weight; return NULL; }

	return PlayAnimation(*animation, slot, sibling, position, weight);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	// Default
	if(!sibling) sibling = GetRootAnimationForSlot(slot);
	assert(!sibling || sibling->Slot == slot);

	// Find two subsequent numbers in case we need to create two nodes, so
	// script can deduce the second node.
	unsigned int Number1, Number2;
	for(Number1 = 0; Number1 < AnimationNodes.size(); ++Number1)
		if(AnimationNodes[Number1] == NULL && (!sibling || Number1+1 == AnimationNodes.size() || AnimationNodes[Number1+1] == NULL))
			break;
/*	for(Number2 = Number1+1; Number2 < AnimationNodes.size(); ++Number2)
		if(AnimationNodes[Number2] == NULL)
			break;*/
	Number2 = Number1 + 1;

	if(Number1 == AnimationNodes.size()) AnimationNodes.push_back(NULL);
	if(sibling && Number2 == AnimationNodes.size()) AnimationNodes.push_back(NULL);

	AnimationNode* child = new AnimationNode(&animation, position);
	AnimationNodes[Number1] = child;
	child->Number = Number1;
	child->Slot = slot;

	if(sibling)
	{
		AnimationNode* parent = new AnimationNode(child, sibling, weight);
		AnimationNodes[Number2] = parent;
		parent->Number = Number2;
		parent->Slot = slot;

		child->Parent = parent;
		parent->Parent = sibling->Parent;
		parent->LinearInterpolation.ChildLeft = sibling;
		parent->LinearInterpolation.ChildRight = child;
		if(sibling->Parent)
		{
			if(sibling->Parent->LinearInterpolation.ChildLeft == sibling)
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
	if(parent == NULL)
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
		if(parent->LinearInterpolation.ChildLeft == node)
		{
			other_child = parent->LinearInterpolation.ChildRight;
			parent->LinearInterpolation.ChildRight = NULL;
		}
		else
		{
			other_child = parent->LinearInterpolation.ChildLeft;
			parent->LinearInterpolation.ChildLeft = NULL;
		}

		if(parent->Parent)
		{
			assert(parent->Parent->Type == AnimationNode::LinearInterpolationNode);
			if(parent->Parent->LinearInterpolation.ChildLeft == parent)
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

	while(AnimationNodes.back() == NULL)
		AnimationNodes.erase(AnimationNodes.end()-1);
	BoneTransformsDirty = true;
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetAnimationNodeByNumber(unsigned int number)
{
	if(number >= AnimationNodes.size()) return NULL;
	return AnimationNodes[number];
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetRootAnimationForSlot(int slot)
{
	AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
	if(iter == AnimationStack.end()) return NULL;
	return *iter;
}

void StdMeshInstance::SetAnimationPosition(AnimationNode* node, ValueProvider* position)
{
	assert(node->GetType() == AnimationNode::LeafNode);
	delete node->Leaf.Position;
	node->Leaf.Position = position;

	BoneTransformsDirty = true;
}

void StdMeshInstance::SetAnimationWeight(AnimationNode* node, ValueProvider* weight)
{
	assert(node->GetType() == AnimationNode::LinearInterpolationNode);
	delete node->LinearInterpolation.Weight; node->LinearInterpolation.Weight = weight;

	BoneTransformsDirty = true;
}

void StdMeshInstance::ExecuteAnimation()
{
	// Iterate from the back since slots might get removed
	for(unsigned int i = AnimationStack.size(); i > 0; --i)
		ExecuteAnimationNode(AnimationStack[i-1]);
}

const StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(const StdMesh& mesh, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, float scale)
{
	AttachedMesh attach;
	unsigned int i;

	// Find free index.
	attach.Number = 1;
	for(AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if(iter->Number >= attach.Number)
			attach.Number = iter->Number + 1;

	// Lookup parent bone
	for(i = 0; i < Mesh.GetNumBones(); ++i)
		if(Mesh.GetBone(i).Name == parent_bone)
			{ attach.ParentBone = i; break; }
	if(i == Mesh.GetNumBones()) return NULL;

	// Lookup child bone
	for(i = 0; i < mesh.GetNumBones(); ++i)
		if(mesh.GetBone(i).Name == child_bone)
			{ attach.ChildBone = i; break; }
	if(i == mesh.GetNumBones()) return NULL;

	attach.Scale = scale;
	attach.Parent = this;
	attach.Child = new StdMeshInstance(mesh);
	attach.Child->SetFaceOrdering(CurrentFaceOrdering);

	AttachChildren.push_back(attach);
	attach.Child->AttachParent = &AttachChildren.back();
	BoneTransformsDirty = true; // so that attach transformation is computed before rendering
	return &AttachChildren.back();
}

bool StdMeshInstance::DetachMesh(unsigned int number)
{
	for(AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		if(iter->Number == number)
		{
			delete iter->Child;
			AttachChildren.erase(iter);
			return true;
		}
	}

	return false;
}

const StdMeshInstance::AttachedMesh* StdMeshInstance::GetAttachedMeshByNumber(unsigned int number) const
{
	for(AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if(iter->Number == number)
			return &*iter;
	return NULL;
}

void StdMeshInstance::UpdateBoneTransforms()
{
	// Nothing changed since last time
	if(!BoneTransformsDirty) return;

	// Compute transformation matrix for each bone.
	for(unsigned int i = 0; i < BoneTransforms.size(); ++i)
	{
		StdMeshTransformation Transformation;

		const StdMeshBone& bone = Mesh.GetBone(i);
		const StdMeshBone* parent = bone.GetParent();
		assert(!parent || parent->Index < i);

		bool have_transform = false;
		for(unsigned int j = 0; j < AnimationStack.size(); ++j)
		{
			if(have_transform)
			{
				StdMeshTransformation other;
				if(AnimationStack[j]->GetBoneTransform(i, other))
					Transformation = StdMeshTransformation::Nlerp(Transformation, other, 1.0f); // TODO: Allow custom weighing for slot combination
			}
			else
			{
				have_transform = AnimationStack[j]->GetBoneTransform(i, Transformation);
			}
		}

		if(!have_transform)
		{
			if(parent)
				BoneTransforms[i] = BoneTransforms[parent->Index];
			else
				BoneTransforms[i] = StdMeshMatrix::Identity();
		}
		else
		{
			BoneTransforms[i] = StdMeshMatrix::Transform(bone.Transformation * Transformation * bone.InverseTransformation);
			if(parent) BoneTransforms[i] = BoneTransforms[parent->Index] * BoneTransforms[i];
		}
	}

	// Compute transformation for each vertex. We could later think about
	// doing this on the GPU using a vertex shader. This would then probably
	// need to go to CStdGL::PerformMesh and CStdD3D::PerformMesh.
	// (can only work for fixed face ordering though)
	for(unsigned int i = 0; i < Vertices.size(); ++i)
	{
		const StdSubMesh& submesh = Mesh.GetSubMesh(i);
		std::vector<StdMeshVertex>& instance_vertices = Vertices[i];
		assert(submesh.GetNumVertices() == instance_vertices.size());
		for(unsigned int j = 0; j < instance_vertices.size(); ++j)
		{
			const StdSubMesh::Vertex& vertex = submesh.GetVertex(j);
			StdMeshVertex& instance_vertex = instance_vertices[j];
			if(!vertex.BoneAssignments.empty())
			{
				instance_vertex.x = instance_vertex.y = instance_vertex.z = 0.0f;
				instance_vertex.nx = instance_vertex.ny = instance_vertex.nz = 0.0f;
				instance_vertex.u = vertex.u; instance_vertex.v = vertex.v;

				for(unsigned int k = 0; k < vertex.BoneAssignments.size(); ++k)
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

	// Update attachment's attach transformations. Note this is done recursively.
	for(AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		iter->Child->UpdateBoneTransforms();

		// Compute matrix to change the coordinate system to the one of the attached bone:
		// The idea is that a vertex at the child bone's position transforms to the parent bone's position.
		// Therefore (read from right to left) we first apply the inverse of the child bone transformation,
		// then an optional scaling matrix, and finally the parent bone transformation

		// TODO: we can cache the three matrices in the middle since they don't change over time,
		// reducing this to two matrix multiplications instead of four each frame.
		// Might even be worth to compute the complete transformation directly when rendering then
		// (saves per-instance memory, but requires recomputation if the animation does not change).
		iter->AttachTrans = BoneTransforms[iter->ParentBone]
		                  * StdMeshMatrix::Transform(Mesh.GetBone(iter->ParentBone).Transformation)
		                  * StdMeshMatrix::Scale(iter->Scale, iter->Scale, iter->Scale)
		                  * StdMeshMatrix::Transform(iter->Child->Mesh.GetBone(iter->ChildBone).InverseTransformation)
		                  * StdMeshMatrix::Inverse(iter->Child->BoneTransforms[iter->ChildBone]);
	}

	if(CurrentFaceOrdering != FO_Fixed)
		ReorderFaces();

	BoneTransformsDirty = false;
}

StdMeshInstance::AnimationNodeList::iterator StdMeshInstance::GetStackIterForSlot(int slot, bool create)
{
	// TODO: bsearch
	for(AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
	{
		if((*iter)->Slot == slot)
		{
			return iter;
		}
		else if((*iter)->Slot < slot)
		{
			if(!create)
				return AnimationStack.end();
			else
				return AnimationStack.insert(iter, NULL);
		}
	}
	
	if(!create)
		return AnimationStack.end();
	else
		return AnimationStack.insert(AnimationStack.end(), NULL);
}

bool StdMeshInstance::ExecuteAnimationNode(AnimationNode* node)
{
	ValueProvider* provider = NULL;
	switch(node->GetType())
	{
	case AnimationNode::LeafNode:
		provider = node->GetPositionProvider();
		break;
	case AnimationNode::LinearInterpolationNode:
		provider = node->GetWeightProvider();
		break;
	}

	const float old_value = provider->Value;
	if(!provider->Execute())
	{
		if(node->GetType() == AnimationNode::LeafNode) return false;

		// Remove the child with less weight (normally weight reaches 0.0 or 1.0)
		if(node->GetWeight() > 0.5)
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if(!ExecuteAnimationNode(node->GetRightChild())) return false;
			// Remove left child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildLeft);
		}
		else
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if(!ExecuteAnimationNode(node->GetLeftChild())) return false;
			// Remove right child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildRight);
		}

		
	}
	else
	{
		if(provider->Value != old_value) BoneTransformsDirty = true;

		if(node->GetType() == AnimationNode::LinearInterpolationNode)
		{
			const bool left_result = ExecuteAnimationNode(node->GetLeftChild());
			const bool right_result = ExecuteAnimationNode(node->GetRightChild());

			// Remove this node completely
			if(!left_result && !right_result)
				return false;

			// Note that either of this also removes node
			if(!left_result)
				StopAnimation(node->GetLeftChild());
			if(!right_result)
				StopAnimation(node->GetRightChild());
		}
	}

	return true;
}

void StdMeshInstance::ReorderFaces()
{
	for(unsigned int i = 0; i < Faces.size(); ++i)
	{
		StdMeshInstanceFaceOrderingCmpPred pred(*this, i);
		std::sort(Faces[i].begin(), Faces[i].end(), pred);
	}
	
	// TODO: Also reorder submeshes and attached meshes... maybe this face ordering
	// is not a good idea after all. Another possibility to obtain the effect would be
	// to first render to an offscreen texture, then render to screen (only for meshes
	// with halftransparent clrmod applied)
}
