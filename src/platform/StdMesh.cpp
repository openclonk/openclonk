/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Armin Burgmeier
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
		StdMeshInstanceFaceOrderingCmpPred(const StdMeshInstance& inst):
			m_inst(inst) {}

		bool operator()(const StdMeshFace& face1, const StdMeshFace& face2) const
		{
			switch(m_inst.GetFaceOrdering())
			{
			case StdMeshInstance::FO_Fixed:
				assert(false);
				return false;
			case StdMeshInstance::FO_FarthestToNearest:
			case StdMeshInstance::FO_NearestToFarthest:
				{
					float z1 = m_inst.GetVertex(face1.Vertices[0]).z + m_inst.GetVertex(face1.Vertices[1]).z + m_inst.GetVertex(face1.Vertices[2]).z;
					float z2 = m_inst.GetVertex(face2.Vertices[0]).z + m_inst.GetVertex(face2.Vertices[1]).z + m_inst.GetVertex(face2.Vertices[2]).z;
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

	// Generate matrix to convert the mesh from Ogre coordinate system to Clonk coordinate system.
	StdMeshMatrix CoordCorrection = StdMeshMatrix::Scale(-1.0f, 1.0f, 1.0f) * StdMeshMatrix::Rotate(M_PI/2.0f, 1.0f, 0.0f, 0.0f) * StdMeshMatrix::Rotate(M_PI/2.0f, 0.0f, 0.0f, 1.0f);
	StdMeshMatrix CoordCorrectionInverse = StdMeshMatrix::Inverse(CoordCorrection);
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
	StdMeshTransformation inv;
	inv.scale = 1.0f/transform.scale;
	inv.rotate = -transform.rotate;
	inv.translate = inv.rotate * (inv.scale * -transform.translate);
	return inv;
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

StdMesh::StdMesh():
	Material(NULL)
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
	// Load first submesh only for now
	TiXmlElement* submesh_elem = mesh.RequireFirstChild(submeshes_elem, "submesh");
	TiXmlElement* geometry_elem = mesh.RequireFirstChild(submesh_elem, "geometry");

	const char* material = mesh.RequireStrAttribute(submesh_elem, "material");
	Material = manager.GetMaterial(material);
	if(!Material)
		mesh.Error(FormatString("There is no such material named '%s'", material), submesh_elem);

	int VertexCount = mesh.RequireIntAttribute(geometry_elem, "vertexcount");
	Vertices.resize(VertexCount);

	TiXmlElement* buffer_elem = mesh.RequireFirstChild(geometry_elem, "vertexbuffer");

	unsigned int i = 0;
	for(TiXmlElement* vertex_elem = buffer_elem->FirstChildElement("vertex"); vertex_elem != NULL && i < Vertices.size(); vertex_elem = vertex_elem->NextSiblingElement("vertex"), ++i)
	{
		TiXmlElement* position_elem = mesh.RequireFirstChild(vertex_elem, "position");
		TiXmlElement* normal_elem = mesh.RequireFirstChild(vertex_elem, "normal");
		TiXmlElement* texcoord_elem = mesh.RequireFirstChild(vertex_elem, "texcoord");

		Vertices[i].x = mesh.RequireFloatAttribute(position_elem, "x");
		Vertices[i].y = mesh.RequireFloatAttribute(position_elem, "y");
		Vertices[i].z = mesh.RequireFloatAttribute(position_elem, "z");
		Vertices[i].nx = mesh.RequireFloatAttribute(normal_elem, "x");
		Vertices[i].ny = mesh.RequireFloatAttribute(normal_elem, "y");
		Vertices[i].nz = mesh.RequireFloatAttribute(normal_elem, "z");
		Vertices[i].u = mesh.RequireFloatAttribute(texcoord_elem, "u");
		Vertices[i].v = mesh.RequireFloatAttribute(texcoord_elem, "v");

		// Convert to Clonk coordinate system (type does not match, StdMeshVertex vs. StdMesh::Vertex)
		StdMeshVertex Transformed = CoordCorrection * Vertices[i];
		Vertices[i].nx = Transformed.nx; Vertices[i].ny = Transformed.ny; Vertices[i].nz = Transformed.nz;
		Vertices[i].x  = Transformed.x;  Vertices[i].y  = Transformed.y;  Vertices[i].z  = Transformed.z;

		// Construct BoundingBox
		if(i == 0)
		{
			BoundingBox.x1 = BoundingBox.x2 = Vertices[i].x;
			BoundingBox.y1 = BoundingBox.y2 = Vertices[i].y;
			BoundingBox.z1 = BoundingBox.z2 = Vertices[i].z;
		}
		else
		{
			BoundingBox.x1 = Min(Vertices[i].x, BoundingBox.x1);
			BoundingBox.x2 = Max(Vertices[i].x, BoundingBox.x2);
			BoundingBox.y1 = Min(Vertices[i].y, BoundingBox.y1);
			BoundingBox.y2 = Max(Vertices[i].y, BoundingBox.y2);
			BoundingBox.z1 = Min(Vertices[i].z, BoundingBox.z1);
			BoundingBox.z2 = Max(Vertices[i].z, BoundingBox.z2);
		}
	}

	TiXmlElement* faces_elem = mesh.RequireFirstChild(submesh_elem, "faces");
	int FaceCount = mesh.RequireIntAttribute(faces_elem, "count");
	Faces.resize(FaceCount);

	i = 0;
	for(TiXmlElement* face_elem = faces_elem->FirstChildElement("face"); face_elem != NULL && i < Faces.size(); face_elem = face_elem->NextSiblingElement("face"), ++i)
	{
		int v[3];

		v[0] = mesh.RequireIntAttribute(face_elem, "v1");
		v[1] = mesh.RequireIntAttribute(face_elem, "v2");
		v[2] = mesh.RequireIntAttribute(face_elem, "v3");

		for(unsigned int j = 0; j < 3; ++j)
		{
			if(v[j] < 0 || static_cast<unsigned int>(v[j]) >= Vertices.size())
				mesh.Error(FormatString("Vertex index v%u (%d) is out of range", j+1, v[j]), face_elem);
			Faces[i].Vertices[j] = v[j];
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

			// TODO: The CoordCorrection transformation breaks here if it includes a scale part (|det| != 1)
			// Probably not trivial to fix, since transformation scale behaves different than matrix scale when transforming
			bone->Transformation.scale = StdMeshVector::UnitScale();
			bone->Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, -(CoordCorrection*r)); // negative sign because r is a pseudovector, and coordcorrection has negative determinant
			bone->Transformation.translate = CoordCorrection * d; // TODO: I think this should also apply translation part of coord correction matrix

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

		// Vertex<->Bone assignments
		TiXmlElement* boneassignments_elem = mesh.RequireFirstChild(submesh_elem, "boneassignments");
		for(TiXmlElement* vertexboneassignment_elem = boneassignments_elem->FirstChildElement("vertexboneassignment"); vertexboneassignment_elem != NULL; vertexboneassignment_elem = vertexboneassignment_elem->NextSiblingElement("vertexboneassignment"))
		{
			int BoneID = mesh.RequireIntAttribute(vertexboneassignment_elem, "boneindex");
			int VertexIndex = mesh.RequireIntAttribute(vertexboneassignment_elem, "vertexindex");
			float weight = mesh.RequireFloatAttribute(vertexboneassignment_elem, "weight");

			if(VertexIndex < 0 || static_cast<unsigned int>(VertexIndex) >= Vertices.size())
				mesh.Error(FormatString("Vertex index in bone assignment (%d) is out of range", VertexIndex), vertexboneassignment_elem);

			StdMeshBone* bone = NULL;
			for(unsigned int i = 0; !bone && i < bones.size(); ++i)
				if(bones[i]->ID == BoneID)
					bone = bones[i];

			if(!bone) mesh.Error(FormatString("There is no such bone with index %d", BoneID), vertexboneassignment_elem);

			Vertex& vertex = Vertices[VertexIndex];
			vertex.BoneAssignments.push_back(StdMeshVertexBoneAssignment());
			StdMeshVertexBoneAssignment& assignment = vertex.BoneAssignments.back();
			assignment.BoneIndex = bone->Index;
			assignment.Weight = weight;
		}

		// Normalize vertex bone assignment weights (this is not guaranteed in the
		// Ogre file format).
		for(unsigned int i = 0; i < Vertices.size(); ++i)
		{
			Vertex& vertex = Vertices[i];
			float sum = 0.0f;
			for(unsigned int j = 0; j < vertex.BoneAssignments.size(); ++j)
				sum += vertex.BoneAssignments[j].Weight;
			for(unsigned int j = 0; j < vertex.BoneAssignments.size(); ++j)
				vertex.BoneAssignments[j].Weight /= sum;
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

					// Apply inverse bone scale and rotation to translation part of transformation
					StdMeshVector scl = 1.0f/bone->Transformation.scale;
					StdMeshQuaternion quat = -bone->Transformation.rotate;
					StdMeshVector ddp = quat * (scl * d);

					// TODO: The CoordCorrection transformation breaks here if it includes a scale part (|det| != 1)
					// Probably not trivial to fix, since transformation scale behaves different than matrix scale when transforming.
					frame.Transformation.scale = StdMeshVector::UnitScale();
					frame.Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, -(CoordCorrection*r)); // negative sign because r is a pseudovector, and coordcorrection has negative determinant
					frame.Transformation.translate = ddp; // No CoordCorrection here, since it is applied already in ddp
				}
			}
		}
	}
	else
	{
		// Mesh has no skeleton
		TiXmlElement* boneassignments_elem = submesh_elem->FirstChildElement("boneassignments");
		if(boneassignments_elem)
		{
			// Bone assignements do not make sense then, as the
			// actual bones are defined in the skeleton file.
			mesh.Error(StdStrBuf("Mesh has bone assignments, but no skeleton"), boneassignments_elem);
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

StdMeshInstance::AnimationRef::AnimationRef(StdMeshInstance* instance, const StdStrBuf& animation_name):
	Instance(instance), Anim(NULL)
{
	const StdMeshAnimation* animation = instance->Mesh.GetAnimationByName(animation_name);
	if(animation)
	{
		for(unsigned int i = 0; i < instance->Animations.size(); ++i)
			if(instance->Animations[i].MeshAnimation == animation)
				{ Anim = &instance->Animations[i]; break; }
	}
}

StdMeshInstance::AnimationRef::AnimationRef(StdMeshInstance* instance, const StdMeshAnimation& animation):
	Instance(instance), Anim(NULL)
{
	for(unsigned int i = 0; i < instance->Animations.size(); ++i)
		if(instance->Animations[i].MeshAnimation == &animation)
			{ Anim = &instance->Animations[i]; break; }
}

const StdMeshAnimation& StdMeshInstance::AnimationRef::GetAnimation() const
{
	return *Anim->MeshAnimation;
}

void StdMeshInstance::AnimationRef::SetPosition(float position)
{
	assert(position <= Anim->MeshAnimation->Length);
	Anim->Position = position;
	Instance->BoneTransformsDirty = true;
}

void StdMeshInstance::AnimationRef::SetWeight(float weight)
{
	Anim->Weight = weight;
	Instance->BoneTransformsDirty = true;
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh):
	Mesh(mesh), CurrentFaceOrdering(FO_Fixed),
	BoneTransforms(Mesh.GetNumBones()), Vertices(Mesh.GetNumVertices()),
	Faces(Mesh.Faces), BoneTransformsDirty(false)
{
	for(unsigned int i = 0; i < Mesh.GetNumVertices(); ++i)
		Vertices[i] = Mesh.GetVertex(i);
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
	CurrentFaceOrdering = ordering;
	if(ordering == FO_Fixed)
		Faces = Mesh.Faces;
}

bool StdMeshInstance::PlayAnimation(const StdStrBuf& animation_name, float weight)
{
	const StdMeshAnimation* animation = Mesh.GetAnimationByName(animation_name);
	if(!animation) return false;
	return PlayAnimation(*animation, weight);
}

bool StdMeshInstance::PlayAnimation(const StdMeshAnimation& animation, float weight)
{
	for(unsigned int i = 0; i < Animations.size(); ++i)
		if(Animations[i].MeshAnimation == &animation)
			return false;

	Animation anim;
	anim.MeshAnimation = &animation;
	anim.Position = 0.0f;
	anim.Weight = weight;
	Animations.push_back(anim);

	BoneTransformsDirty = true;
	return true;
}

bool StdMeshInstance::StopAnimation(const StdStrBuf& animation_name)
{
	const StdMeshAnimation* animation = Mesh.GetAnimationByName(animation_name);
	if(!animation) return false;
	return StopAnimation(*animation);
}

bool StdMeshInstance::StopAnimation(const StdMeshAnimation& animation)
{
	for(std::vector<Animation>::iterator iter = Animations.begin();
	    iter != Animations.end(); ++iter)
	{
		if(iter->MeshAnimation == &animation)
		{
			Animations.erase(iter);
			BoneTransformsDirty = true;
			return true;
		}
	}

	return false;
}

void StdMeshInstance::UpdateBoneTransforms()
{
	// Nothing changed since last time
	if(!BoneTransformsDirty) return;

	// Compute transformation matrix for each bone.
	for(unsigned int i = 0; i < BoneTransforms.size(); ++i)
	{
		float accum_weight = 0.0f;
		StdMeshTransformation Transformation = StdMeshTransformation::Zero();

		for(unsigned int j = 0; j < Animations.size(); ++j)
		{
			StdMeshTrack* track = Animations[j].MeshAnimation->Tracks[i];
			if(track)
			{
				accum_weight += Animations[j].Weight;
				StdMeshTransformation Track = track->GetTransformAt(Animations[j].Position);
				
				Transformation.scale += Animations[j].Weight * Track.scale;
				Transformation.rotate += Animations[j].Weight * Track.rotate; // TODO: introduce animation stack, then slerp
				Transformation.translate += Animations[j].Weight * Track.translate;
			}
		}

		const StdMeshBone* bone = Mesh.Bones[i];
		const StdMeshBone* parent = bone->GetParent();
		assert(!parent || parent->Index < i);

		if(!accum_weight)
		{
			if(parent)
				BoneTransforms[i] = BoneTransforms[parent->Index];
			else
				BoneTransforms[i] = StdMeshMatrix::Identity();
		}
		else
		{
			Transformation.scale *= 1.0f/accum_weight;
			Transformation.rotate.Normalize(); // renormalize. We can skip this if we decide to use slerp
			Transformation.translate *= 1.0f/accum_weight;

			BoneTransforms[i] = StdMeshMatrix::Transform(bone->Transformation * Transformation * bone->InverseTransformation);

			if(parent)
				BoneTransforms[i] = BoneTransforms[parent->Index] * BoneTransforms[i];
		}
	}

	// Compute transformation for each vertex. We could later think about
	// doing this on the GPU using a vertex shader. This would then probably
	// need to go to CStdGL::PerformMesh and CStdD3D::PerformMesh.
	// (can only work for fixed face ordering though)
	for(unsigned int i = 0; i < Vertices.size(); ++i)
	{
		const StdMesh::Vertex& vertex = Mesh.Vertices[i];
		if(!vertex.BoneAssignments.empty())
		{
			Vertices[i].x = Vertices[i].y = Vertices[i].z = 0.0f;
			Vertices[i].nx = Vertices[i].ny = Vertices[i].nz = 0.0f;
			Vertices[i].u = vertex.u; Vertices[i].v = vertex.v;

			for(unsigned int j = 0; j < vertex.BoneAssignments.size(); ++j)
			{
				const StdMeshVertexBoneAssignment& assignment = vertex.BoneAssignments[j];
				
				Vertices[i] += assignment.Weight * (BoneTransforms[assignment.BoneIndex] * vertex);
			}
		}
		else
		{
			Vertices[i] = vertex;
		}
	}

	if(CurrentFaceOrdering != FO_Fixed)
		ReorderFaces();

	BoneTransformsDirty = false;
}

void StdMeshInstance::ReorderFaces()
{
	StdMeshInstanceFaceOrderingCmpPred pred(*this);
	std::sort(Faces.begin(), Faces.end(), pred);
}
