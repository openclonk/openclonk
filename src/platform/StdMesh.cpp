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

		bool operator()(const StdMeshFace* face1, const StdMeshFace* face2) const
		{
			switch(m_inst.GetFaceOrdering())
			{
			case StdMeshInstance::FO_Fixed:
				// Faces are in a vector, thus contiuous in memory
				return face1 < face2; // TODO: face1 > face2?
			case StdMeshInstance::FO_FarthestToNearest:
			case StdMeshInstance::FO_NearestToFarthest:
				{
					float z1 = m_inst.GetVertex(face1->Vertices[0]).z + m_inst.GetVertex(face1->Vertices[1]).z + m_inst.GetVertex(face1->Vertices[2]).z;
					float z2 = m_inst.GetVertex(face2->Vertices[0]).z + m_inst.GetVertex(face2->Vertices[1]).z + m_inst.GetVertex(face2->Vertices[2]).z;
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

	// Generate matrix to convert the mesh from Ogre coordinate system to Clonk
	// coordinate system. When making changes here, don't forget to make
	// corresponding changes for the inverse matrix below.
	StdMeshMatrix CoordCorrectionMatrix()
	{
		StdMeshMatrix matrix;
		StdMeshMatrix helper;

		//matrix.SetIdentity();
		matrix.SetScale(-1.0f, 1.0f, 1.0f);

		//helper.SetRotate(M_PI/2.0f, 1.0f, 0.0f, 0.0f);
		helper.SetRotate(M_PI/2.0f, 1.0f, 0.0f, 0.0f);
		matrix.Mul(helper);

		helper.SetRotate(M_PI/2.0f, 0.0f, 0.0f, 1.0f);
		matrix.Mul(helper);

		return matrix;
	}

	StdMeshMatrix CoordCorrectionMatrixInverse()
	{
		StdMeshMatrix matrix;
		StdMeshMatrix helper;

		matrix.SetRotate(-M_PI/2.0f, 0.0f, 0.0f, 1.0f);

		//helper.SetRotate(-M_PI/2.0f, 1.0f, 0.0f, 0.0f);
		helper.SetRotate(-M_PI/2.0f, 1.0f, 0.0f, 0.0f);
		matrix.Mul(helper);

		//helper.SetIdentity();
		helper.SetScale(-1.0f, 1.0f, 1.0f);
		matrix.Mul(helper);

		return matrix;
	}

	StdMeshMatrix CoordCorrection = CoordCorrectionMatrix();
	StdMeshMatrix CoordCorrectionInverse = CoordCorrectionMatrixInverse();
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
		Error(FormatString("Element '%s' does not have integer attribute '%s'", element->Value(), attribute), element);
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

void StdMeshMatrix::SetIdentity()
{
	a[0][0] = 1.0f; a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = 0.0f;
	a[1][0] = 0.0f; a[1][1] = 1.0f; a[1][2] = 0.0f; a[1][3] = 0.0f;
	a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = 1.0f; a[2][3] = 0.0f;
}

void StdMeshMatrix::SetTranslate(float dx, float dy, float dz)
{
	a[0][0] = 1.0f; a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = dx;
	a[1][0] = 0.0f; a[1][1] = 1.0f; a[1][2] = 0.0f; a[1][3] = dy;
	a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = 1.0f; a[2][3] = dz;
}

void StdMeshMatrix::SetScale(float sx, float sy, float sz)
{
	a[0][0] = sx;	 a[0][1] = 0.0f; a[0][2] = 0.0f; a[0][3] = 0.0f;
	a[1][0] = 0.0f; a[1][1] = sy;	 a[1][2] = 0.0f; a[1][3] = 0.0f;
	a[2][0] = 0.0f; a[2][1] = 0.0f; a[2][2] = sz;	 a[2][3] = 0.0f;
}

void StdMeshMatrix::SetRotate(float angle, float rx, float ry, float rz)
{
	// We do normalize the rx,ry,rz vector here: This is only required for
	// precalculations anyway, thus not time-critical.
	float abs = sqrt(rx*rx+ry*ry+rz*rz);
	rx/=abs; ry/=abs; rz/=abs;
	float c = cos(angle), s = sin(angle);

	a[0][0] = rx*rx*(1-c)+c;		a[0][1] = rx*ry*(1-c)-rz*s; a[0][2] = rx*rz*(1-c)+ry*s; a[0][3] = 0.0f;
	a[1][0] = ry*rx*(1-c)+rz*s; a[1][1] = ry*ry*(1-c)+c;		a[1][2] = ry*rz*(1-c)-rx*s; a[1][3] = 0.0f;
	a[2][0] = rz*rx*(1-c)-ry*s; a[2][1] = ry*rz*(1-c)+rx*s; a[2][2] = rz*rz*(1-c)+c;		a[2][3] = 0.0f;
}

void StdMeshMatrix::Mul(const StdMeshMatrix& other)
{
	StdMeshMatrix old(*this);

	a[0][0] = old.a[0][0]*other.a[0][0] + old.a[0][1]*other.a[1][0] + old.a[0][2]*other.a[2][0];
	a[1][0] = old.a[1][0]*other.a[0][0] + old.a[1][1]*other.a[1][0] + old.a[1][2]*other.a[2][0];
	a[2][0] = old.a[2][0]*other.a[0][0] + old.a[2][1]*other.a[1][0] + old.a[2][2]*other.a[2][0];

	a[0][1] = old.a[0][0]*other.a[0][1] + old.a[0][1]*other.a[1][1] + old.a[0][2]*other.a[2][1];
	a[1][1] = old.a[1][0]*other.a[0][1] + old.a[1][1]*other.a[1][1] + old.a[1][2]*other.a[2][1];
	a[2][1] = old.a[2][0]*other.a[0][1] + old.a[2][1]*other.a[1][1] + old.a[2][2]*other.a[2][1];

	a[0][2] = old.a[0][0]*other.a[0][2] + old.a[0][1]*other.a[1][2] + old.a[0][2]*other.a[2][2];
	a[1][2] = old.a[1][0]*other.a[0][2] + old.a[1][1]*other.a[1][2] + old.a[1][2]*other.a[2][2];
	a[2][2] = old.a[2][0]*other.a[0][2] + old.a[2][1]*other.a[1][2] + old.a[2][2]*other.a[2][2];

	a[0][3] = old.a[0][0]*other.a[0][3] + old.a[0][1]*other.a[1][3] + old.a[0][2]*other.a[2][3] + old.a[0][3];
	a[1][3] = old.a[1][0]*other.a[0][3] + old.a[1][1]*other.a[1][3] + old.a[1][2]*other.a[2][3] + old.a[1][3];
	a[2][3] = old.a[2][0]*other.a[0][3] + old.a[2][1]*other.a[1][3] + old.a[2][2]*other.a[2][3] + old.a[2][3];
}

void StdMeshMatrix::Mul(float f)
{
	a[0][0] *= f;
	a[0][1] *= f;
	a[0][2] *= f;
	a[0][3] *= f;
	a[1][0] *= f;
	a[1][1] *= f;
	a[1][2] *= f;
	a[1][3] *= f;
	a[2][0] *= f;
	a[2][1] *= f;
	a[2][2] *= f;
	a[2][3] *= f;
}

void StdMeshMatrix::Add(const StdMeshMatrix& other)
{
	a[0][0] += other.a[0][0];
	a[0][1] += other.a[0][1];
	a[0][2] += other.a[0][2];
	a[0][3] += other.a[0][3];
	a[1][0] += other.a[1][0];
	a[1][1] += other.a[1][1];
	a[1][2] += other.a[1][2];
	a[1][3] += other.a[1][3];
	a[2][0] += other.a[2][0];
	a[2][1] += other.a[2][1];
	a[2][2] += other.a[2][2];
	a[2][3] += other.a[2][3];
}

void StdMeshMatrix::Transform(const StdMeshMatrix& other)
{
	StdMeshMatrix old(*this);

	a[0][0] = other.a[0][0]*old.a[0][0] + other.a[0][1]*old.a[1][0] + other.a[0][2]*old.a[2][0];
	a[1][0] = other.a[1][0]*old.a[0][0] + other.a[1][1]*old.a[1][0] + other.a[1][2]*old.a[2][0];
	a[2][0] = other.a[2][0]*old.a[0][0] + other.a[2][1]*old.a[1][0] + other.a[2][2]*old.a[2][0];

	a[0][1] = other.a[0][0]*old.a[0][1] + other.a[0][1]*old.a[1][1] + other.a[0][2]*old.a[2][1];
	a[1][1] = other.a[1][0]*old.a[0][1] + other.a[1][1]*old.a[1][1] + other.a[1][2]*old.a[2][1];
	a[2][1] = other.a[2][0]*old.a[0][1] + other.a[2][1]*old.a[1][1] + other.a[2][2]*old.a[2][1];

	a[0][2] = other.a[0][0]*old.a[0][2] + other.a[0][1]*old.a[1][2] + other.a[0][2]*old.a[2][2];
	a[1][2] = other.a[1][0]*old.a[0][2] + other.a[1][1]*old.a[1][2] + other.a[1][2]*old.a[2][2];
	a[2][2] = other.a[2][0]*old.a[0][2] + other.a[2][1]*old.a[1][2] + other.a[2][2]*old.a[2][2];

	a[0][3] = other.a[0][0]*old.a[0][3] + other.a[0][1]*old.a[1][3] + other.a[0][2]*old.a[2][3] + other.a[0][3];
	a[1][3] = other.a[1][0]*old.a[0][3] + other.a[1][1]*old.a[1][3] + other.a[1][2]*old.a[2][3] + other.a[1][3];
	a[2][3] = other.a[2][0]*old.a[0][3] + other.a[2][1]*old.a[1][3] + other.a[2][2]*old.a[2][3] + other.a[2][3];
}

void StdMeshVertex::Transform(const StdMeshMatrix& trans)
{
	StdMeshVertex old(*this);

	x = trans(0,0)*old.x + trans(0,1)*old.y + trans(0,2)*old.z + trans(0,3);
	y = trans(1,0)*old.x + trans(1,1)*old.y + trans(1,2)*old.z + trans(1,3);
	z = trans(2,0)*old.x + trans(2,1)*old.y + trans(2,2)*old.z + trans(2,3);
	nx = trans(0,0)*old.nx + trans(0,1)*old.ny + trans(0,2)*old.nz;
	ny = trans(1,0)*old.nx + trans(1,1)*old.ny + trans(0,2)*old.nz;
	nz = trans(2,0)*old.nx + trans(2,1)*old.ny + trans(2,2)*old.nz;
}

void StdMeshVertex::Mul(float f)
{
	x *= f;
	y *= f;
	z *= f;

	// We also multiplicate normals because we expect this to happen in
	// an expression such as a*v1 + (1-a)*v2 which would ensure normalization
	// of the normals again. (TODO: It doesn't. We should just always
	// keep them normalized).
	nx *= f;
	ny *= f;
	nz *= f;
}

void StdMeshVertex::Add(const StdMeshVertex& other)
{
	x += other.x;
	y += other.y;
	z += other.z;

	nx += other.nx;
	ny += other.ny;
	nz += other.nz;
}

StdMeshMatrix StdMeshTrack::GetTransformAt(float time) const
{
	std::map<float, StdMeshKeyFrame>::const_iterator iter = Frames.lower_bound(time);

	// If this points to end(), then either
	// a) time > animation length
	// b) The track does not include a frame for the very end of the animation
	// Both is considered an error
	assert(iter != Frames.end());

	if(iter == Frames.begin())
		return iter->second.Trans;

	std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
	-- prev_iter;

	float dt = iter->first - prev_iter->first;
	float weight1 = (time - prev_iter->first) / dt;
	float weight2 = (iter->first - time) / dt;

	assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
	assert(fabs(weight1 + weight2 - 1) < 1e-6);

	StdMeshMatrix trans1 = iter->second.Trans;
	StdMeshMatrix trans2 = prev_iter->second.Trans;
	trans1.Mul(weight1);
	trans2.Mul(weight2);

	trans1.Add(trans2);
	return trans1;
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

		// Convert to Clonk coordinate system
		Vertices[i].Transform(CoordCorrection);

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

			TiXmlElement* position_elem = skeleton.RequireFirstChild(bone_elem, "position");
			TiXmlElement* rotation_elem = skeleton.RequireFirstChild(bone_elem, "rotation");
			TiXmlElement* axis_elem = skeleton.RequireFirstChild(rotation_elem, "axis");

			float dx = skeleton.RequireFloatAttribute(position_elem, "x");
			float dy = skeleton.RequireFloatAttribute(position_elem, "y");
			float dz = skeleton.RequireFloatAttribute(position_elem, "z");
			float angle = skeleton.RequireFloatAttribute(rotation_elem, "angle");
			float rx = skeleton.RequireFloatAttribute(axis_elem, "x");
			float ry = skeleton.RequireFloatAttribute(axis_elem, "y");
			float rz = skeleton.RequireFloatAttribute(axis_elem, "z");

			StdMeshMatrix helper;
			helper.SetTranslate(dx, dy, dz);
			bone->Trans.SetRotate(angle, rx, ry, rz);
			bone->Trans.Transform(helper);

			// Transform to Clonk coordinate system
			bone->Trans.Mul(CoordCorrectionInverse);
			bone->Trans.Transform(CoordCorrection);

			helper.SetRotate(-angle, rx, ry, rz);
			bone->InverseTrans.SetTranslate(-dx, -dy, -dz);
			bone->InverseTrans.Transform(helper);

			// Transform to Clonk coordinate system
			bone->InverseTrans.Mul(CoordCorrectionInverse);
			bone->InverseTrans.Transform(CoordCorrection);

			bone->Parent = NULL;

			// Index of bone will be set when building Master Bone Table later
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
			child->Trans.Transform(parent->Trans);
			child->InverseTrans.Mul(parent->InverseTrans);
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
			//StdStrBuf name(skeleton.RequireStrAttribute(animation_elem, "name"));
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

					float dx = skeleton.RequireFloatAttribute(translate_elem, "x");
					float dy = skeleton.RequireFloatAttribute(translate_elem, "y");
					float dz = skeleton.RequireFloatAttribute(translate_elem, "z");
					float sx = skeleton.RequireFloatAttribute(scale_elem, "x");
					float sy = skeleton.RequireFloatAttribute(scale_elem, "y");
					float sz = skeleton.RequireFloatAttribute(scale_elem, "z");
					float angle = skeleton.RequireFloatAttribute(rotate_elem, "angle");
					float rx = skeleton.RequireFloatAttribute(axis_elem, "x");
					float ry = skeleton.RequireFloatAttribute(axis_elem, "y");
					float rz = skeleton.RequireFloatAttribute(axis_elem, "z");

					// TODO: Make sure the order is correct here - I am not sure about scale
					StdMeshMatrix helper;
					frame.Trans.SetRotate(angle, rx, ry, rz);
					helper.SetScale(sx, sy, sz);
					frame.Trans.Transform(helper);
					helper.SetTranslate(-dx, -dy, -dz);
					frame.Trans.Transform(helper);

					// Transform into Clonk coordinate system
					frame.Trans.Transform(CoordCorrection);
					frame.Trans.Mul(CoordCorrectionInverse);
				}
			}

#if 0
			// Apply bone transformation on animation frames. We need to do this
			// after the actual loading because we need to walk the bone list
			// hierarchically.
			for(unsigned int i = 0; i < Bones.size(); ++i)
			{
				if(animation.Tracks[i])
				{
					StdMeshTrack& track = *animation.Tracks[i];

					// Get next parent track
					StdMeshTrack* parent_track = NULL;
					StdMeshBone* parent_bone = Bones[i]->Parent;
					while(parent_bone && !(parent_track = animation.Tracks[parent_bone->Index]))
						parent_bone = parent_bone->Parent;
					assert(!parent_bone || parent_track);

					for(std::map<float, StdMeshKeyFrame>::iterator iter = track.Frames.begin(); iter != track.Frames.end(); ++iter)
					{
						// TODO: If this bone's track is not as smooth as the parent animation
						// (which means if there is more than one keyframe in the parent
						// animation for each keyframe pair in the this bone's animation),
						// then we need to insert additional child keyframes, so that the
						// transformation for the child does not skip parent keyframes.
						StdMeshKeyFrame& frame = iter->second;

						// Apply transformation of parent tracks (for which we computed
						// already the bone transformations, as we walk the bone list
						// hierarchically) in bone's coordinate system.
						frame.Trans.Mul(Bones[i]->InverseTrans);
						frame.Trans.Transform(Bones[i]->Trans);

						if(parent_bone)
							frame.Trans.Transform(parent_track->GetTransformAt(iter->first));
					}
				}
			}
#endif
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
	Instance(instance), Anim(NULL), Changed(false)
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
	Instance(instance), Anim(NULL), Changed(false)
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
	assert(position <= Anim->Animation->Length);
	Anim->Position = position;
	Changed = true;
}

void StdMeshInstance::AnimationRef::SetWeight(float weight)
{
	Anim->Weight = weight;
	Changed = true;
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh):
	Mesh(mesh), CurrentFaceOrdering(FO_Fixed),
	BoneTransforms(Mesh.GetNumBones()), Vertices(Mesh.GetNumVertices()),
	Faces(Mesh.GetNumFaces())
{
	for(unsigned int i = 0; i < Mesh.GetNumVertices(); ++i)
		Vertices[i] = Mesh.GetVertex(i);

	// This is FO_Fixed actually
	for(unsigned int i = 0; i < Mesh.GetNumFaces(); ++i)
		Faces[i] = &Mesh.GetFace(i);
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
	CurrentFaceOrdering = ordering;
	ReorderFaces();
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
	UpdateBoneTransforms();
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
			UpdateBoneTransforms();
			return true;
		}
	}

	return false;
}

void StdMeshInstance::UpdateBoneTransforms()
{
	// Compute transformation matrix for each bone.
	for(unsigned int i = 0; i < BoneTransforms.size(); ++i)
	{
		float accum_weight = 0.0f;
		BoneTransforms[i].SetScale(0,0,0); // zero matrix

		for(unsigned int j = 0; j < Animations.size(); ++j)
		{
			StdMeshTrack* track = Animations[j].MeshAnimation->Tracks[i];
			if(track)
			{
				accum_weight += Animations[j].Weight;
				StdMeshMatrix matr(track->GetTransformAt(Animations[j].Position));
				matr.Mul(Animations[j].Weight);
				BoneTransforms[i].Add(matr);
			}
		}
		
		if(!accum_weight)
			BoneTransforms[i].SetIdentity();
		else
			BoneTransforms[i].Mul(1.0f/accum_weight);

		const StdMeshBone* bone = Mesh.Bones[i];
		
		BoneTransforms[i].Mul(bone->InverseTrans);
		BoneTransforms[i].Transform(bone->Trans);

		const StdMeshBone* parent = bone->GetParent();
		assert(!parent || parent->Index < i);
		if(parent)
			BoneTransforms[i].Transform(BoneTransforms[parent->Index]);
	}

	// Compute transformation for each vertex. We could later think about
	// doing this on the GPU using a vertex shader. This would then probably
	// need to go to CStdGL::PerformMesh and CStdD3D::PerformMesh.
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
				StdMeshVertex vtx = vertex;
				vtx.Transform(BoneTransforms[assignment.BoneIndex]);
				vtx.Mul(assignment.Weight);
				Vertices[i].Add(vtx);
			}
		}
		else
		{
			Vertices[i] = vertex;
		}
	}

	if(CurrentFaceOrdering != FO_Fixed)
		ReorderFaces();
}

void StdMeshInstance::ReorderFaces()
{
	StdMeshInstanceFaceOrderingCmpPred pred(*this);
	std::sort(Faces.begin(), Faces.end(), pred);
}
