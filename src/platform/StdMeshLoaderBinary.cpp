/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010 Nicolas Hake
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

// A loader for the OGRE .mesh binary file format

#include "C4Include.h"
#include "C4Log.h"
#include "StdMesh.h"
#include "StdMeshLoader.h"
#include "StdMeshLoaderBinaryChunks.h"
#include "StdMeshLoaderDataStream.h"
#include "StdMeshMaterial.h"
#include <cassert>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace
{
	bool VertexDeclarationIsSane(const boost::ptr_vector<Ogre::Mesh::ChunkGeometryVertexDeclElement> &decl)
	{
		bool semanticSeen[Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_MAX + 1] = { false };
		BOOST_FOREACH(Ogre::Mesh::ChunkGeometryVertexDeclElement element, decl)
		{
			switch (element.semantic)
			{
			case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Texcoords:
				// Normally, you can use multiple texture coordinates, but we currently support only one.
				// So complain if we get multiple sets.
			case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Position:
			case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Normals:
			case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Diffuse:
			case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Specular:
				// Only one set of each of these elements allowed
				if (semanticSeen[element.semantic])
					return false;
			}
			semanticSeen[element.semantic] = true;
		}
		return true;
	}
	
	template<size_t N>
	void ReadNormalizedVertexData(float (&dest)[N], const char *source, Ogre::Mesh::ChunkGeometryVertexDeclElement::Type vdet)
	{
		BOOST_STATIC_ASSERT(N >= 4);
		dest[0] = dest[1] = dest[2] = 0; dest[3] = 1;
		switch (vdet)
		{
			// All VDET_Float* fall through.
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Float4:
			dest[3] = *reinterpret_cast<const float*>(source + sizeof(float) * 3);
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Float3:
			dest[2] = *reinterpret_cast<const float*>(source + sizeof(float) * 2);
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Float2:
			dest[1] = *reinterpret_cast<const float*>(source + sizeof(float) * 1);
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Float1:
			dest[0] = *reinterpret_cast<const float*>(source + sizeof(float) * 0);
			break;
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Color_ABGR:
			dest[3] = *reinterpret_cast<const float*>(source);
			for (int i = 0; i < 3; ++i)
				dest[i] = *reinterpret_cast<const float*>(source + sizeof(float) * (3 - i));
			break;
		case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDET_Color_ARGB:
			dest[3] = *reinterpret_cast<const float*>(source);
			for (int i = 0; i < 3; ++i)
				dest[i] = *reinterpret_cast<const float*>(source + sizeof(float) * (i + 1));
			break;
		}
	}
	
	std::vector<StdSubMesh::Vertex> ReadSubmeshGeometry(const Ogre::Mesh::ChunkGeometry &geo)
	{
		if (!VertexDeclarationIsSane(geo.vertexDeclaration))
			throw Ogre::Mesh::InvalidVertexDeclaration();

		// Generate array of vertex buffer cursors
		std::vector<const char *> cursors;
		BOOST_FOREACH(const Ogre::Mesh::ChunkGeometryVertexBuffer &buf, geo.vertexBuffers)
			cursors.push_back(static_cast<const char *>(buf.data->data));

		// Generate vertices
		std::vector<StdSubMesh::Vertex> vertices;
		vertices.reserve(geo.vertexCount);
		for (size_t i = 0; i < geo.vertexCount; ++i)
		{
			StdSubMesh::Vertex vertex;
			vertex.nx = vertex.ny = vertex.nz = 0;
			vertex.x = vertex.y = vertex.z = 0;
			vertex.u = vertex.v = 0;
			// Read vertex declaration
			BOOST_FOREACH(Ogre::Mesh::ChunkGeometryVertexDeclElement element, geo.vertexDeclaration)
			{
				float values[4];
				ReadNormalizedVertexData(values, cursors[element.source] + element.offset, element.type);
				switch (element.semantic)
				{
				case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Position:
					vertex.x = values[0];
					vertex.y = values[1];
					vertex.z = values[2];
					break;
				case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Normals:
					vertex.nx = values[0];
					vertex.ny = values[1];
					vertex.nz = values[2];
					break;
				case Ogre::Mesh::ChunkGeometryVertexDeclElement::VDES_Texcoords:
					vertex.u = values[0];
					vertex.v = values[1];
					break;
				}
			}
			vertices.push_back(vertex);
			// Advance vertex buffer cursors
			for (size_t cursor = 0; cursor < geo.vertexBuffers.size(); ++cursor)
				cursors[cursor] += geo.vertexBuffers[cursor].vertexSize;
		}

		return vertices;
	}
}

StdMesh *StdMeshLoader::LoadMeshBinary(const char *src, size_t length, const StdMeshMatManager &mat_mgr, StdMeshSkeletonLoader &loader, const char *filename)
{
	boost::scoped_ptr<Ogre::Mesh::Chunk> root;
	Ogre::DataStream stream(src, length);

	// First chunk must be the header
	root.reset(Ogre::Mesh::Chunk::Read(&stream));
	if (root->GetType() != Ogre::Mesh::CID_Header)
		throw Ogre::Mesh::InvalidVersion();

	// Second chunk is the mesh itself
	root.reset(Ogre::Mesh::Chunk::Read(&stream));
	if (root->GetType() != Ogre::Mesh::CID_Mesh)
		throw Ogre::Mesh::InvalidVersion();
	
	// Generate mesh from data
	Ogre::Mesh::ChunkMesh &cmesh = *static_cast<Ogre::Mesh::ChunkMesh*>(root.get());
	std::auto_ptr<StdMesh> mesh(new StdMesh);
	mesh->BoundingBox = cmesh.bounds;

	// Read skeleton (if exists)
	if (!cmesh.skeletonFile.empty())
	{
		StdStrBuf skel = loader.LoadSkeleton(cmesh.skeletonFile.c_str());
		if (skel.isNull())
			throw Ogre::InsufficientData("The specified skeleton file was not found");
		LoadSkeletonBinary(mesh.get(), skel.getData(), skel.getLength());
	}

	// Build bone handle->index quick access table
	std::map<uint16_t, size_t> bone_lookup;
	for (size_t i = 0; i < mesh->GetNumBones(); ++i)
	{
		bone_lookup[mesh->GetBone(i).ID] = i;
	}

	// Read submeshes
	mesh->SubMeshes.reserve(cmesh.submeshes.size());
	for (size_t i = 0; i < cmesh.submeshes.size(); ++i)
	{
		mesh->SubMeshes.push_back(StdSubMesh());
		StdSubMesh &sm = mesh->SubMeshes.back();
		Ogre::Mesh::ChunkSubmesh &csm = cmesh.submeshes[i];
		sm.Material = mat_mgr.GetMaterial(csm.material.c_str());
		if (!sm.Material)
			throw Ogre::Mesh::InvalidMaterial();
		if (csm.operation != Ogre::Mesh::ChunkSubmesh::SO_TriList)
			throw Ogre::Mesh::NotImplemented("Submesh operations other than TriList aren't implemented yet");
		sm.Faces.resize(csm.faceVertices.size() / 3);
		for (size_t face = 0; face < sm.Faces.size(); ++face)
		{
			sm.Faces[face].Vertices[0] = csm.faceVertices[face * 3 + 0];
			sm.Faces[face].Vertices[1] = csm.faceVertices[face * 3 + 1];
			sm.Faces[face].Vertices[2] = csm.faceVertices[face * 3 + 2];
		}
		Ogre::Mesh::ChunkGeometry &geo = *(csm.hasSharedVertices ? cmesh.geometry : csm.geometry);
		sm.Vertices = ReadSubmeshGeometry(geo);

		// Read bone assignments
		BOOST_FOREACH(const Ogre::Mesh::BoneAssignment &ba, csm.boneAssignments)
		{
			if (ba.vertex >= sm.GetNumVertices())
				throw Ogre::Mesh::VertexNotFound();
			if (bone_lookup.find(ba.bone) == bone_lookup.end())
				throw Ogre::Skeleton::BoneNotFound();
			StdMeshVertexBoneAssignment assignment;
			assignment.BoneIndex = bone_lookup[ba.bone];
			assignment.Weight = ba.weight;
			sm.Vertices[ba.vertex].BoneAssignments.push_back(assignment);
		}

		// Normalize bone assignments
		BOOST_FOREACH(StdSubMesh::Vertex &vertex, sm.Vertices)
		{
			float sum = 0;
			BOOST_FOREACH(StdMeshVertexBoneAssignment &ba, vertex.BoneAssignments)
				sum += ba.Weight;
			BOOST_FOREACH(StdMeshVertexBoneAssignment &ba, vertex.BoneAssignments)
				ba.Weight /= sum;
		}
		DebugLogF("Loaded submesh with %d faces, %d vertices, material %s", sm.GetNumFaces(), sm.GetNumVertices(), sm.GetMaterial().Name.getData());
	}
	return mesh.release();
}

void StdMeshLoader::LoadSkeletonBinary(StdMesh *mesh, const char *src, size_t size)
{
	boost::scoped_ptr<Ogre::Skeleton::Chunk> chunk;
	Ogre::DataStream stream(src, size);

	// First chunk must be the header
	chunk.reset(Ogre::Skeleton::Chunk::Read(&stream));
	if (chunk->GetType() != Ogre::Skeleton::CID_Header)
		throw Ogre::Skeleton::InvalidVersion();

	boost::ptr_map<uint16_t, StdMeshBone> bones;
	boost::ptr_vector<Ogre::Skeleton::ChunkAnimation> animations;
	for (Ogre::Skeleton::ChunkID id = Ogre::Skeleton::Chunk::Peek(&stream);
		id == Ogre::Skeleton::CID_Bone || id == Ogre::Skeleton::CID_Bone_Parent || id == Ogre::Skeleton::CID_Animation;
		id = Ogre::Skeleton::Chunk::Peek(&stream)
		)
	{
		std::auto_ptr<Ogre::Skeleton::Chunk> chunk(Ogre::Skeleton::Chunk::Read(&stream));
		switch (chunk->GetType())
		{
		case Ogre::Skeleton::CID_Bone:
			{
				Ogre::Skeleton::ChunkBone &cbone = *static_cast<Ogre::Skeleton::ChunkBone*>(chunk.get());
				// Check that the bone ID is unique
				if (bones.find(cbone.handle) != bones.end())
					throw Ogre::Skeleton::IdNotUnique();
				StdMeshBone *bone = new StdMeshBone;
				bone->Parent = NULL;
				bone->ID = cbone.handle;
				bone->Name = cbone.name.c_str();
				bone->Transformation.translate = cbone.position;
				bone->Transformation.rotate = cbone.orientation;
				bone->Transformation.scale = cbone.scale;
				bone->InverseTransformation = StdMeshTransformation::Inverse(bone->Transformation);
				bones.insert(cbone.handle, bone);
			}
			break;
		case Ogre::Skeleton::CID_Bone_Parent:
			{
				Ogre::Skeleton::ChunkBoneParent &cbparent = *static_cast<Ogre::Skeleton::ChunkBoneParent*>(chunk.get());
				if (bones.find(cbparent.parentHandle) == bones.end() || bones.find(cbparent.childHandle) == bones.end())
					throw Ogre::Skeleton::BoneNotFound();
				bones[cbparent.parentHandle].Children.push_back(&bones[cbparent.childHandle]);
				bones[cbparent.childHandle].Parent = &bones[cbparent.parentHandle];
			}
			break;
		case Ogre::Skeleton::CID_Animation:
			// Collect animations for later (need bone table index, which we don't know yet)
			animations.push_back(static_cast<Ogre::Skeleton::ChunkAnimation*>(chunk.release()));
			break;
		}
		if (stream.AtEof()) break;
	}

	// Find master bone (i.e., the one without a parent)
	StdMeshBone *master = NULL;
	for (boost::ptr_map<uint16_t, StdMeshBone>::iterator it = bones.begin(); it != bones.end(); ++it)
	{
		if (!it->second->Parent)
		{
			if (master)
				DebugLogF("More than one master bone: %s has no parent, but %s already master", it->second->Name.getData(), master->Name.getData());
			master = it->second;
			mesh->AddMasterBone(master);
		}
	}
	if (!master)
		throw Ogre::Skeleton::MissingMasterBone();

	// Transfer bone ownership to mesh (double .release() is correct)
	bones.release().release();

	// Build handle->index quick access table
	std::map<uint16_t, size_t> handle_lookup;
	for (size_t i = 0; i < mesh->GetNumBones(); ++i)
	{
		handle_lookup[mesh->GetBone(i).ID] = i;
	}

	// Fixup animations
	BOOST_FOREACH(Ogre::Skeleton::ChunkAnimation &canim, animations)
	{
		StdMeshAnimation &anim = mesh->Animations[StdCopyStrBuf(canim.name.c_str())];
		anim.Name = canim.name.c_str();
		anim.Length = canim.duration;
		anim.Tracks.resize(mesh->GetNumBones());
		BOOST_FOREACH(Ogre::Skeleton::ChunkAnimationTrack &catrack, canim.tracks)
		{
			const StdMeshBone &bone = mesh->GetBone(handle_lookup[catrack.bone]);
			StdMeshTrack *&track = anim.Tracks[bone.Index];
			if (track != NULL)
				throw Ogre::Skeleton::MultipleBoneTracks();
			track = new StdMeshTrack;
			BOOST_FOREACH(Ogre::Skeleton::ChunkAnimationTrackKF &catkf, catrack.keyframes)
			{
				StdMeshKeyFrame &kf = track->Frames[catkf.time];
				kf.Transformation.rotate = catkf.rotation;
				kf.Transformation.scale = catkf.scale;
				kf.Transformation.translate = bone.InverseTransformation.rotate * (bone.InverseTransformation.scale * catkf.translation);
			}
		}
	}

	// Fixup bone transforms
	BOOST_FOREACH(StdMeshBone *bone, mesh->Bones)
	{
		if (bone->Parent)
		{
			bone->Transformation = bone->Parent->Transformation * bone->Transformation;
			bone->InverseTransformation = StdMeshTransformation::Inverse(bone->Transformation);
		}
	}

	DebugLogF("Loaded skeleton with %d bones, %d animations", mesh->GetNumBones(), animations.size());
}
