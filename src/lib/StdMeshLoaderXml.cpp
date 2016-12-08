/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

// A loader for the OGRE .mesh XML file format

#include "C4Include.h"
#include "lib/StdMesh.h"
#include "lib/StdMeshLoader.h"
#include <tinyxml.h>

// Helper class to load things from an XML file with error checking
class StdMeshLoader::StdMeshXML
{
public:
	StdMeshXML(const char* filename, const char* xml_data);

	const char* RequireStrAttribute(TiXmlElement* element, const char* attribute) const;
	int RequireIntAttribute(TiXmlElement* element, const char* attribute) const;
	float RequireFloatAttribute(TiXmlElement* element, const char* attribute) const;

	TiXmlElement* RequireFirstChild(TiXmlElement* element, const char* child);

	void LoadGeometry(StdMesh& mesh, std::vector<StdSubMesh::Vertex>& vertices, TiXmlElement* boneassignments_elem);
	void LoadBoneAssignments(StdMesh& mesh, std::vector<StdSubMesh::Vertex>& vertices, TiXmlElement* boneassignments_elem);

	void Error(const StdStrBuf& message, TiXmlElement* element) const;
	void Error(const StdStrBuf& message, int row) const;

private:
	TiXmlDocument Document;
	StdStrBuf FileName;
};

StdMeshLoader::StdMeshXML::StdMeshXML(const char* filename, const char* xml_data):
		FileName(filename)
{
	Document.Parse(xml_data);
	if (Document.Error())
		Error(StdStrBuf(Document.ErrorDesc()), Document.ErrorRow());
}

const char* StdMeshLoader::StdMeshXML::RequireStrAttribute(TiXmlElement* element, const char* attribute) const
{
	const char* value = element->Attribute(attribute);
	if (!value) Error(FormatString("Element '%s' does not have attribute '%s'", element->Value(), attribute), element);
	return value;
}

int StdMeshLoader::StdMeshXML::RequireIntAttribute(TiXmlElement* element, const char* attribute) const
{
	int retval;
	if (element->QueryIntAttribute(attribute, &retval) != TIXML_SUCCESS)
		Error(FormatString("Element '%s' does not have integer attribute '%s'", element->Value(), attribute), element);
	return retval;
}

float StdMeshLoader::StdMeshXML::RequireFloatAttribute(TiXmlElement* element, const char* attribute) const
{
	float retval = 0;
	if (element->QueryFloatAttribute(attribute, &retval) != TIXML_SUCCESS)
		Error(FormatString("Element '%s' does not have floating point attribute '%s'", element->Value(), attribute), element);
	return retval;
}

TiXmlElement* StdMeshLoader::StdMeshXML::RequireFirstChild(TiXmlElement* element, const char* child)
{
	TiXmlElement* retval;

	if (element)
	{
		retval = element->FirstChildElement(child);
		if (!retval)
			Error(FormatString("Element '%s' does not contain '%s' child", element->Value(), child), element);
	}
	else
	{
		retval = Document.RootElement();
		if (strcmp(retval->Value(), child) != 0)
			Error(FormatString("Root element is not '%s'", child), retval);
	}

	return retval;
}

void StdMeshLoader::StdMeshXML::Error(const StdStrBuf& message, TiXmlElement* element) const
{
	Error(message, element->Row());
}

void StdMeshLoader::StdMeshXML::Error(const StdStrBuf& message, int row) const
{
	throw StdMeshLoader::LoaderException(FormatString("%s:%u: %s", FileName.getData(), row, message.getData()).getData());
}

void StdMeshLoader::StdMeshXML::LoadGeometry(StdMesh& mesh, std::vector<StdSubMesh::Vertex>& vertices, TiXmlElement* geometry_elem)
{
	// Check whether mesh has any vertices so far -- we need this later for
	// initialization of bounding box and bounding sphere.
	bool hasVertices = false;
	if(!mesh.SharedVertices.empty()) hasVertices = true;
	for(unsigned int i = 0; i < mesh.SubMeshes.size(); ++i)
		if(!mesh.SubMeshes[i].Vertices.empty())
			hasVertices = true;

	int VertexCount = RequireIntAttribute(geometry_elem, "vertexcount");
	vertices.resize(VertexCount);

	static const unsigned int POSITIONS = 1;
	static const unsigned int NORMALS = 2;
	static const unsigned int TEXCOORDS = 4;

	// Individual vertex attributes can be split up in multiple vertex buffers
	unsigned int loaded_attributes = 0;
	for(TiXmlElement* buffer_elem = geometry_elem->FirstChildElement("vertexbuffer"); buffer_elem != nullptr; buffer_elem = buffer_elem->NextSiblingElement("vertexbuffer"))
	{
		unsigned int attributes = 0;
		if(buffer_elem->Attribute("positions")) attributes |= POSITIONS;
		if(buffer_elem->Attribute("normals")) attributes |= NORMALS;
		if(buffer_elem->Attribute("texture_coords")) attributes |= TEXCOORDS;

		unsigned int i;
		TiXmlElement* vertex_elem;
		for (vertex_elem = buffer_elem->FirstChildElement("vertex"), i = 0; vertex_elem != nullptr && i < vertices.size(); vertex_elem = vertex_elem->NextSiblingElement("vertex"), ++i)
		{
			if(attributes & POSITIONS)
			{
				TiXmlElement* position_elem = RequireFirstChild(vertex_elem, "position");

				vertices[i].x = RequireFloatAttribute(position_elem, "x");
				vertices[i].y = RequireFloatAttribute(position_elem, "y");
				vertices[i].z = RequireFloatAttribute(position_elem, "z");
			}

			if(attributes & NORMALS)
			{
				TiXmlElement* normal_elem = RequireFirstChild(vertex_elem, "normal");

				vertices[i].nx = RequireFloatAttribute(normal_elem, "x");
				vertices[i].ny = RequireFloatAttribute(normal_elem, "y");
				vertices[i].nz = RequireFloatAttribute(normal_elem, "z");
			}

			if(attributes & TEXCOORDS)
			{
				// FIXME: The Ogre format supports denoting multiple texture coordinates, but the rendering code only supports one
				// currently only the first set is read, any additional ones are ignored
				TiXmlElement* texcoord_elem = RequireFirstChild(vertex_elem, "texcoord");
				vertices[i].u = RequireFloatAttribute(texcoord_elem, "u");
				vertices[i].v = RequireFloatAttribute(texcoord_elem, "v");
			}

			vertices[i] = OgreToClonk::TransformVertex(vertices[i]);

			if (attributes & POSITIONS)
			{
				const float d = std::sqrt(vertices[i].x*vertices[i].x
				                        + vertices[i].y*vertices[i].y
				                        + vertices[i].z*vertices[i].z);

				// Construct BoundingBox
				StdMeshBox& BoundingBox = mesh.BoundingBox;
				if (i == 0 && !hasVertices)
				{
					// First vertex
					BoundingBox.x1 = BoundingBox.x2 = vertices[i].x;
					BoundingBox.y1 = BoundingBox.y2 = vertices[i].y;
					BoundingBox.z1 = BoundingBox.z2 = vertices[i].z;
					mesh.BoundingRadius = d;
				}
				else
				{
					BoundingBox.x1 = std::min(vertices[i].x, BoundingBox.x1);
					BoundingBox.x2 = std::max(vertices[i].x, BoundingBox.x2);
					BoundingBox.y1 = std::min(vertices[i].y, BoundingBox.y1);
					BoundingBox.y2 = std::max(vertices[i].y, BoundingBox.y2);
					BoundingBox.z1 = std::min(vertices[i].z, BoundingBox.z1);
					BoundingBox.z2 = std::max(vertices[i].z, BoundingBox.z2);
					mesh.BoundingRadius = std::max(mesh.BoundingRadius, d);
				}
			}
		}

		if(vertex_elem != nullptr)
			Error(FormatString("Too many vertices in vertexbuffer"), buffer_elem);
		if(i < vertices.size())
			Error(FormatString("Not enough vertices in vertexbuffer"), buffer_elem);

		loaded_attributes |= attributes;
	}

	static const unsigned int REQUIRED_ATTRIBUTES = POSITIONS | NORMALS | TEXCOORDS;
	if((loaded_attributes & REQUIRED_ATTRIBUTES) != REQUIRED_ATTRIBUTES)
		Error(FormatString("Not all required vertex attributes (positions, normals, texcoords) present in mesh geometry"), geometry_elem);
}

void StdMeshLoader::StdMeshXML::LoadBoneAssignments(StdMesh& mesh, std::vector<StdSubMesh::Vertex>& vertices, TiXmlElement* boneassignments_elem)
{
	for (TiXmlElement* vertexboneassignment_elem = boneassignments_elem->FirstChildElement("vertexboneassignment"); vertexboneassignment_elem != nullptr; vertexboneassignment_elem = vertexboneassignment_elem->NextSiblingElement("vertexboneassignment"))
	{
		int BoneID = RequireIntAttribute(vertexboneassignment_elem, "boneindex");
		int VertexIndex = RequireIntAttribute(vertexboneassignment_elem, "vertexindex");
		float weight = RequireFloatAttribute(vertexboneassignment_elem, "weight");

		if (VertexIndex < 0 || static_cast<unsigned int>(VertexIndex) >= vertices.size())
			Error(FormatString("Vertex index in bone assignment (%d) is out of range", VertexIndex), vertexboneassignment_elem);

		// maybe not needed, see comment below
		const StdMeshBone* bone = nullptr;
		for (unsigned int i = 0; !bone && i < mesh.GetSkeleton().GetNumBones(); ++i)
			if (mesh.GetSkeleton().GetBone(i).ID == BoneID)
				bone = &mesh.GetSkeleton().GetBone(i);

		if (!bone) Error(FormatString("There is no such bone with ID %d", BoneID), vertexboneassignment_elem);

		// Find first bone assignment with a zero weight (i.e. is unused)
		StdSubMesh::Vertex& vertex = vertices[VertexIndex];
		// Check quickly if all weight slots are used
		if (vertex.bone_weight[StdMeshVertex::MaxBoneWeightCount - 1] != 0)
		{
			Error(FormatString("Vertex %d is influenced by more than %d bones", VertexIndex, static_cast<int>(StdMeshVertex::MaxBoneWeightCount)), vertexboneassignment_elem);
		}
		for (size_t weight_index = 0; weight_index < StdMeshVertex::MaxBoneWeightCount; ++weight_index)
		{
			if (vertex.bone_weight[weight_index] == 0)
			{
				vertex.bone_weight[weight_index] = weight;
				vertex.bone_index[weight_index] = bone->Index;
				break;
			}
		}
	}

	// Normalize vertex bone assignment weights (this is not guaranteed in the
	// Ogre file format).
	for (unsigned int i = 0; i < vertices.size(); ++i)
	{
		StdSubMesh::Vertex& vertex = vertices[i];
		float sum = 0.0;
		for (float weight : vertex.bone_weight)
			sum += weight;
		if (sum != 0)
			for (float &weight : vertex.bone_weight)
				weight /= sum;
		else
			vertex.bone_weight[0] = 1.0f;
	}
}

StdMesh *StdMeshLoader::LoadMeshXml(const char* xml_data, size_t size, const StdMeshMatManager& manager, StdMeshSkeletonLoader& skel_loader, const char* filename)
{
	StdMeshXML xml(filename ? filename : "<unknown>", xml_data);

	std::unique_ptr<StdMesh> mesh(new StdMesh);

	TiXmlElement* mesh_elem = xml.RequireFirstChild(nullptr, "mesh");

	// Load shared geometry, if any
	TiXmlElement* sharedgeometry_elem = mesh_elem->FirstChildElement("sharedgeometry");
	if(sharedgeometry_elem != nullptr)
		xml.LoadGeometry(*mesh, mesh->SharedVertices, sharedgeometry_elem);

	TiXmlElement* submeshes_elem = xml.RequireFirstChild(mesh_elem, "submeshes");

	TiXmlElement* submesh_elem_base = xml.RequireFirstChild(submeshes_elem, "submesh");
	for (TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != nullptr; submesh_elem = submesh_elem->NextSiblingElement("submesh"))
	{
		mesh->SubMeshes.push_back(StdSubMesh());
		StdSubMesh& submesh = mesh->SubMeshes.back();

		const char* material = xml.RequireStrAttribute(submesh_elem, "material");
		submesh.Material = manager.GetMaterial(material);
		if (!submesh.Material)
			xml.Error(FormatString("There is no such material named '%s'", material), submesh_elem);

		const char* usesharedvertices = submesh_elem->Attribute("usesharedvertices");
		const std::vector<StdMesh::Vertex>* vertices;
		if(!usesharedvertices || strcmp(usesharedvertices, "true") != 0)
		{
			TiXmlElement* geometry_elem = xml.RequireFirstChild(submesh_elem, "geometry");
			xml.LoadGeometry(*mesh, submesh.Vertices, geometry_elem);
			vertices = &submesh.Vertices;
		}
		else
		{
			if(mesh->SharedVertices.empty())
				xml.Error(StdCopyStrBuf("Submesh specifies to use shared vertices but there is no shared geometry"), submesh_elem);
			vertices = &mesh->SharedVertices;
		}

		TiXmlElement* faces_elem = xml.RequireFirstChild(submesh_elem, "faces");
		int FaceCount = xml.RequireIntAttribute(faces_elem, "count");
		submesh.Faces.resize(FaceCount);

		unsigned int i = 0;
		for (TiXmlElement* face_elem = faces_elem->FirstChildElement("face"); face_elem != nullptr && i < submesh.Faces.size(); face_elem = face_elem->NextSiblingElement("face"), ++i)
		{
			int v[3];

			v[0] = xml.RequireIntAttribute(face_elem, "v1");
			v[1] = xml.RequireIntAttribute(face_elem, "v2");
			v[2] = xml.RequireIntAttribute(face_elem, "v3");

			for (unsigned int j = 0; j < 3; ++j)
			{
				if (v[j] < 0 || static_cast<unsigned int>(v[j]) >= vertices->size())
					xml.Error(FormatString("Vertex index v%u (%d) is out of range", j+1, v[j]), face_elem);
				submesh.Faces[i].Vertices[j] = v[j];
			}
		}
	}

	// We allow bounding box to be empty if it's only due to Z direction since
	// this is what goes inside the screen in Clonk.
	if(mesh->BoundingBox.x1 == mesh->BoundingBox.x2 || mesh->BoundingBox.y1 == mesh->BoundingBox.y2)
		xml.Error(StdCopyStrBuf("Bounding box is empty"), mesh_elem);

	// Read skeleton, if any
	TiXmlElement* skeletonlink_elem = mesh_elem->FirstChildElement("skeletonlink");
	if (skeletonlink_elem)
	{
		const char* name = xml.RequireStrAttribute(skeletonlink_elem, "name");
		StdCopyStrBuf xml_filename(name); xml_filename.Append(".xml");

		StdCopyStrBuf skeleton_filename;
		StdMeshSkeletonLoader::MakeFullSkeletonPath(skeleton_filename, filename, xml_filename.getData());

		mesh->Skeleton = skel_loader.GetSkeletonByName(skeleton_filename);
		if (!mesh->Skeleton) xml.Error(FormatString("Failed to load '%s'", skeleton_filename.getData()), skeletonlink_elem);

		// Vertex<->Bone assignments for shared geometry
		if (sharedgeometry_elem)
		{
			TiXmlElement* boneassignments_elem = xml.RequireFirstChild(mesh_elem, "boneassignments");
			xml.LoadBoneAssignments(*mesh, mesh->SharedVertices, boneassignments_elem);
		}

		// Vertex<->Bone assignments for all vertices (need to go through SubMeshes again...)
		unsigned int submesh_index = 0;
		for (TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != nullptr; submesh_elem = submesh_elem->NextSiblingElement("submesh"), ++submesh_index)
		{
			StdSubMesh& submesh = mesh->SubMeshes[submesh_index];
			if (!submesh.Vertices.empty())
			{
				TiXmlElement* boneassignments_elem = xml.RequireFirstChild(submesh_elem, "boneassignments");
				xml.LoadBoneAssignments(*mesh, submesh.Vertices, boneassignments_elem);
			}
		}
	}
	else
	{
		// Mesh has no skeleton
		// Bone assignements do not make sense then, as the
		// actual bones are defined in the skeleton file.
		for (TiXmlElement* submesh_elem = submesh_elem_base; submesh_elem != nullptr; submesh_elem = submesh_elem->NextSiblingElement("submesh"))
		{
			TiXmlElement* boneassignments_elem = submesh_elem->FirstChildElement("boneassignments");
			if (boneassignments_elem)
				xml.Error(StdStrBuf("Mesh has bone assignments, but no skeleton"), boneassignments_elem);
		}

		TiXmlElement* boneassignments_elem = mesh_elem->FirstChildElement("boneassignments");
		if (boneassignments_elem)
			xml.Error(StdStrBuf("Mesh has bone assignments, but no skeleton"), boneassignments_elem);
	}

	return mesh.release();
}

void StdMeshSkeletonLoader::LoadSkeletonXml(const char* groupname, const char* filename, const char *sourcefile, size_t size)
{
	if (sourcefile == nullptr)
	{
		throw Ogre::InsufficientData(FormatString("Failed to load '%s/%s'", groupname, filename).getData());
	}

	std::shared_ptr<StdMeshLoader::StdMeshXML> skeleton(new StdMeshLoader::StdMeshXML(filename, sourcefile));

	TiXmlElement* skeleton_elem = skeleton->RequireFirstChild(nullptr, "skeleton");
	TiXmlElement* bones_elem = skeleton->RequireFirstChild(skeleton_elem, "bones");

	// Read bones. Don't insert into Master bone table yet, as the master bone
	// table is sorted hierarchically, and we will read the hierarchy only
	// afterwards.
	std::vector<StdMeshBone*> bones;
	for (TiXmlElement* bone_elem = bones_elem->FirstChildElement("bone"); bone_elem != nullptr; bone_elem = bone_elem->NextSiblingElement("bone"))
	{
		StdMeshBone* bone = new StdMeshBone;
		bones.push_back(bone);

		bone->ID = skeleton->RequireIntAttribute(bone_elem, "id");
		bone->Name = skeleton->RequireStrAttribute(bone_elem, "name");
		// TODO: Make sure ID and name are unique

		bone->Parent = nullptr;
		// Index of bone will be set when building Master Bone Table later

		TiXmlElement* position_elem = skeleton->RequireFirstChild(bone_elem, "position");
		TiXmlElement* rotation_elem = skeleton->RequireFirstChild(bone_elem, "rotation");
		TiXmlElement* axis_elem = skeleton->RequireFirstChild(rotation_elem, "axis");

		StdMeshVector d, r;
		d.x = skeleton->RequireFloatAttribute(position_elem, "x");
		d.y = skeleton->RequireFloatAttribute(position_elem, "y");
		d.z = skeleton->RequireFloatAttribute(position_elem, "z");
		float angle = skeleton->RequireFloatAttribute(rotation_elem, "angle");
		r.x = skeleton->RequireFloatAttribute(axis_elem, "x");
		r.y = skeleton->RequireFloatAttribute(axis_elem, "y");
		r.z = skeleton->RequireFloatAttribute(axis_elem, "z");

		bone->Transformation.scale = StdMeshVector::UnitScale();
		bone->Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, r);
		bone->Transformation.translate = d;

		// We need this later for applying animations, and attaching meshes, therefore cache it here
		bone->InverseTransformation = StdMeshTransformation::Inverse(bone->Transformation);
	}

	// Bone hierarchy
	TiXmlElement* bonehierarchy_elem = skeleton->RequireFirstChild(skeleton_elem, "bonehierarchy");
	for (TiXmlElement* boneparent_elem = bonehierarchy_elem->FirstChildElement("boneparent"); boneparent_elem != nullptr; boneparent_elem = boneparent_elem->NextSiblingElement("boneparent"))
	{
		const char* child_name = skeleton->RequireStrAttribute(boneparent_elem, "bone");
		const char* parent_name = skeleton->RequireStrAttribute(boneparent_elem, "parent");

		// Lookup the two bones
		StdMeshBone* child = nullptr;
		StdMeshBone* parent = nullptr;
		for (unsigned int i = 0; i < bones.size() && (!child || !parent); ++i)
		{
			if (!child && bones[i]->Name == child_name)
				child = bones[i];
			if (!parent && bones[i]->Name == parent_name)
				parent = bones[i];
		}

		if (!child) skeleton->Error(FormatString("There is no such bone with name '%s'", child_name), boneparent_elem);
		if (!parent) skeleton->Error(FormatString("There is no such bone with name '%s'", parent_name), boneparent_elem);

		child->Parent = parent;
		parent->Children.push_back(child);
	}

	std::shared_ptr<StdMeshSkeleton> Skeleton(new StdMeshSkeleton);

	// Fill master bone table in hierarchical order:
	for (unsigned int i = 0; i < bones.size(); ++i)
		if (bones[i]->Parent == nullptr)
			Skeleton->AddMasterBone(bones[i]);

	// Load Animations
	TiXmlElement* animations_elem = skeleton_elem->FirstChildElement("animations");
	if (animations_elem)
	{
		for (TiXmlElement* animation_elem = animations_elem->FirstChildElement("animation"); animation_elem != nullptr; animation_elem = animation_elem->NextSiblingElement("animation"))
		{
			StdCopyStrBuf name(skeleton->RequireStrAttribute(animation_elem, "name"));
			if (Skeleton->Animations.find(name) != Skeleton->Animations.end())
				skeleton->Error(FormatString("There is already an animation with name '%s'", name.getData()), animation_elem);

			StdMeshAnimation& animation = Skeleton->Animations.insert(std::make_pair(name, StdMeshAnimation())).first->second;
			animation.Name = name;
			animation.Length = skeleton->RequireFloatAttribute(animation_elem, "length");
			animation.Tracks.resize(Skeleton->GetNumBones());
			animation.OriginSkeleton = &(*Skeleton);

			TiXmlElement* tracks_elem = skeleton->RequireFirstChild(animation_elem, "tracks");
			for (TiXmlElement* track_elem = tracks_elem->FirstChildElement("track"); track_elem != nullptr; track_elem = track_elem->NextSiblingElement("track"))
			{
				const char* bone_name = skeleton->RequireStrAttribute(track_elem, "bone");
				StdMeshBone* bone = nullptr;
				for (unsigned int i = 0; !bone && i < Skeleton->GetNumBones(); ++i)
					if (Skeleton->Bones[i]->Name == bone_name)
						bone = Skeleton->Bones[i];
				if (!bone) skeleton->Error(FormatString("There is no such bone with name '%s'", bone_name), track_elem);

				if (animation.Tracks[bone->Index] != nullptr) skeleton->Error(FormatString("There is already a track for bone '%s' in animation '%s'", bone_name, animation.Name.getData()), track_elem);

				StdMeshTrack* track = new StdMeshTrack;
				animation.Tracks[bone->Index] = track;

				TiXmlElement* keyframes_elem = skeleton->RequireFirstChild(track_elem, "keyframes");
				for (TiXmlElement* keyframe_elem = keyframes_elem->FirstChildElement("keyframe"); keyframe_elem != nullptr; keyframe_elem = keyframe_elem->NextSiblingElement("keyframe"))
				{
					float time = skeleton->RequireFloatAttribute(keyframe_elem, "time");
					StdMeshKeyFrame& frame = track->Frames[time];

					TiXmlElement* translate_elem = keyframe_elem->FirstChildElement("translate");
					TiXmlElement* rotate_elem = keyframe_elem->FirstChildElement("rotate");
					TiXmlElement* scale_elem = keyframe_elem->FirstChildElement("scale");

					StdMeshVector d, s, r;
					d.x = d.y = d.z = 0.0f;
					s = StdMeshVector::UnitScale();
					r.x = r.y = 0.0f; r.z = 1.0f;
					float angle = 0.0f;

					if (translate_elem)
					{
						d.x = skeleton->RequireFloatAttribute(translate_elem, "x");
						d.y = skeleton->RequireFloatAttribute(translate_elem, "y");
						d.z = skeleton->RequireFloatAttribute(translate_elem, "z");
					}

					if (rotate_elem)
					{
						TiXmlElement* axis_elem = skeleton->RequireFirstChild(rotate_elem, "axis");
						angle = skeleton->RequireFloatAttribute(rotate_elem, "angle");
						r.x = skeleton->RequireFloatAttribute(axis_elem, "x");
						r.y = skeleton->RequireFloatAttribute(axis_elem, "y");
						r.z = skeleton->RequireFloatAttribute(axis_elem, "z");
					}

					if (scale_elem)
					{
						s.x = skeleton->RequireFloatAttribute(scale_elem, "x");
						s.y = skeleton->RequireFloatAttribute(scale_elem, "y");
						s.z = skeleton->RequireFloatAttribute(scale_elem, "z");
					}

					frame.Transformation.scale = s;
					frame.Transformation.rotate = StdMeshQuaternion::AngleAxis(angle, r);
					frame.Transformation.translate = bone->InverseTransformation.rotate * (bone->InverseTransformation.scale * d);
					frame.Transformation = OgreToClonk::TransformTransformation(frame.Transformation);
				}
			}
		}
	}

	// is there even any xml file that we load from?
	// it looks like this could never work: if the mesh has no skeleton, then the code below will fail because of a null pointer...

	// Apply parent transformation to each bone transformation. We need to
	// do this late since animation keyframe computation needs the bone
	// transformations, not bone+parent.
	for (unsigned int i = 0; i < Skeleton->GetNumBones(); ++i)
	{
		// Apply parent transformation
		if (Skeleton->Bones[i]->Parent)
			Skeleton->Bones[i]->Transformation = Skeleton->Bones[i]->Parent->Transformation * OgreToClonk::TransformTransformation(Skeleton->Bones[i]->Transformation);
		else
			Skeleton->Bones[i]->Transformation = OgreToClonk::TransformTransformation(Skeleton->Bones[i]->Transformation);

		// Update inverse
		Skeleton->Bones[i]->InverseTransformation = StdMeshTransformation::Inverse(Skeleton->Bones[i]->Transformation);
	}

	StoreSkeleton(groupname, filename, Skeleton);
}
