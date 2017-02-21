/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "lib/StdMeshLoaderBinaryChunks.h"
#include "lib/StdMeshLoaderDataStream.h"
#include <cassert>
#include <string>
#include <utility>

// deleter-agnostic unique_ptr static caster
template<typename To, typename From>
std::unique_ptr<To> static_unique_cast(From&& p) {
	return std::unique_ptr<To>(static_cast<To*>(p.release()));
}

using std::move;

namespace Ogre
{
	namespace Mesh
	{
		const uint32_t ChunkFileHeader::CurrentVersion = 1080; // Major * 1000 + Minor
		const std::map<std::string, uint32_t> ChunkFileHeader::VersionTable = {
		    // 1.8: Current version
		    std::make_pair("[MeshSerializer_v1.8]",  CurrentVersion),
		    // 1.41: Changes to morph keyframes and poses. We don't use either, so no special handling needed
		    std::make_pair("[MeshSerializer_v1.41]", 1041),
		    // 1.40: Changes to CID_Mesh_LOD chunks, we ignore those, so no special handling needed
		    std::make_pair("[MeshSerializer_v1.40]", 1040)
		};

		// Chunk factory
		std::unique_ptr<Chunk> Chunk::Read(DataStream *stream)
		{
			assert(stream->GetRemainingBytes() >= ChunkHeaderLength);

			// Read metadata
			ChunkID id = CID_Invalid;
			id = static_cast<ChunkID>(stream->Read<uint16_t>());
			size_t size = 0;
			// Special case: CID_Header doesn't have any size info.
			if (id != CID_Header)
			{
				// All others are proper chunks.
				size = stream->Read<uint32_t>();
				size -= ChunkHeaderLength;
			}

			// Create chunk
			std::unique_ptr<Chunk> chunk;
			switch (id)
			{
			case CID_Header: chunk.reset(new ChunkFileHeader()); break;
			case CID_Mesh: chunk.reset(new ChunkMesh()); break;
			case CID_Mesh_Bone_Assignment:
			case CID_Submesh_Bone_Assignment:
				chunk.reset(new ChunkMeshBoneAssignments()); break;
			case CID_Mesh_Skeleton_Link: chunk.reset(new ChunkMeshSkeletonLink()); break;
			case CID_Mesh_Bounds: chunk.reset(new ChunkMeshBounds()); break;
			case CID_Submesh: chunk.reset(new ChunkSubmesh()); break;
			case CID_Submesh_Op: chunk.reset(new ChunkSubmeshOp()); break;
			case CID_Geometry: chunk.reset(new ChunkGeometry()); break;
			case CID_Geometry_Vertex_Buffer: chunk.reset(new ChunkGeometryVertexBuffer()); break;
			case CID_Geometry_Vertex_Data: chunk.reset(new ChunkGeometryVertexData()); break;
			case CID_Geometry_Vertex_Decl: chunk.reset(new ChunkGeometryVertexDecl()); break;
			case CID_Geometry_Vertex_Decl_Element: chunk.reset(new ChunkGeometryVertexDeclElement()); break;
			default:
				LogF("StdMeshLoader: I don't know what to do with a chunk of type 0x%xu", id);
				// Fall through
			case CID_Edge_List: case CID_Submesh_Name_Table:
				// We don't care about these
				chunk.reset(new ChunkUnknown()); break;
			};
			chunk->type = id;
			chunk->size = size;
			chunk->ReadImpl(stream);
			return chunk;
		}

		void ChunkUnknown::ReadImpl(DataStream *stream) { stream->Seek(GetSize()); }

		void ChunkFileHeader::ReadImpl(DataStream *stream)
		{
			// Simple version check
			VersionTable_t::const_iterator it = VersionTable.find(stream->Read<std::string>());
			if (it == VersionTable.end())
				throw InvalidVersion();
		}

		void ChunkMesh::ReadImpl(DataStream *stream)
		{
			hasAnimatedSkeleton = stream->Read<bool>();
			for (ChunkID id = Chunk::Peek(stream);
			     id == CID_Geometry || id == CID_Submesh || id == CID_Mesh_Skeleton_Link || id == CID_Mesh_Bone_Assignment || id == CID_Mesh_LOD || id == CID_Submesh_Name_Table || id == CID_Mesh_Bounds || id == CID_Edge_List || id == CID_Pose_List || id == CID_Animation_List;
			     id = Chunk::Peek(stream)
			    )
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				switch (chunk->GetType())
				{
				case CID_Geometry:
					if (geometry)
						throw MultipleSingletonChunks("There's only one CID_Geometry chunk allowed within a CID_Mesh chunk");
					geometry = static_unique_cast<ChunkGeometry>(move(chunk));
					break;
				case CID_Submesh:
					submeshes.push_back(static_unique_cast<ChunkSubmesh>(move(chunk)));
					break;
				case CID_Mesh_Skeleton_Link:
					if (!skeletonFile.empty())
						throw MultipleSingletonChunks("There's only one CID_Mesh_Skeleton_Link chunk allowed within a CID_Mesh chunk");
					skeletonFile = static_cast<ChunkMeshSkeletonLink*>(chunk.get())->skeleton;
					break;
				case CID_Mesh_Bounds:
					bounds = static_cast<ChunkMeshBounds*>(chunk.get())->bounds;
					radius = static_cast<ChunkMeshBounds*>(chunk.get())->radius;
					break;
				case CID_Mesh_Bone_Assignment:
					// Collect bone assignments
					{
					ChunkMeshBoneAssignments *assignments = static_cast<ChunkMeshBoneAssignments*>(chunk.get());
					boneAssignments.insert(boneAssignments.end(), assignments->assignments.begin(), assignments->assignments.end());
					break;
					}
				default:
					LogF("StdMeshLoader: I don't know what to do with a chunk of type 0x%xu inside a CID_Mesh chunk", chunk->GetType());
					// Fall through
				case CID_Submesh_Name_Table:
				case CID_Edge_List:
					// Ignore those
					break;
				}
				if (stream->AtEof()) break;
			}
		}

		void ChunkMeshSkeletonLink::ReadImpl(DataStream *stream)
		{
			skeleton = stream->Read<std::string>();
		}

		void ChunkSubmesh::ReadImpl(DataStream *stream)
		{
			operation = SO_TriList; // default if no CID_Submesh_Op chunk exists
			material = stream->Read<std::string>();
			hasSharedVertices = stream->Read<bool>();
			size_t index_count = stream->Read<uint32_t>();
			bool indexes_are_32bit = stream->Read<bool>();
			faceVertices.reserve(index_count);
			while (index_count--)
			{
				size_t index;
				if (indexes_are_32bit)
					index = stream->Read<uint32_t>();
				else
					index = stream->Read<uint16_t>();
				faceVertices.push_back(index);
			}
			for (ChunkID id = Chunk::Peek(stream);
			     id == CID_Geometry || id == CID_Submesh_Op || id == CID_Submesh_Bone_Assignment;
			     id = Chunk::Peek(stream)
			    )
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);

				switch (chunk->GetType())
				{
				case CID_Geometry:
					if (hasSharedVertices)
						// Can't have own vertices and at the same time use those of the parent
						throw SharedVertexGeometryForbidden();
					if (geometry)
						throw MultipleSingletonChunks("There's only one CID_Geometry chunk allowed within a CID_Submesh chunk");
					geometry = static_unique_cast<ChunkGeometry>(move(chunk));
					break;
				case CID_Submesh_Op:
					operation = static_cast<ChunkSubmeshOp*>(chunk.get())->operation;
					break;
				case CID_Submesh_Bone_Assignment:
				{
					// Collect bone assignments
					ChunkMeshBoneAssignments *assignments = static_cast<ChunkMeshBoneAssignments*>(chunk.get());
					boneAssignments.insert(boneAssignments.end(), assignments->assignments.begin(), assignments->assignments.end());
				}
				break;
				default:
					LogF("StdMeshLoader: I don't know what to do with a chunk of type 0x%xu inside a CID_Submesh chunk", chunk->GetType());
					break;
				}
				if (stream->AtEof()) break;
			}
		}

		void ChunkSubmeshOp::ReadImpl(DataStream *stream)
		{
			uint32_t op = stream->Read<uint16_t>();
			if (op < ChunkSubmesh::SO_MIN || op > ChunkSubmesh::SO_MAX)
				throw InvalidSubmeshOp();
			operation = static_cast<ChunkSubmesh::SubmeshOperation>(op);
		}

		void ChunkMeshBoneAssignments::ReadImpl(DataStream *stream)
		{
			size_t bone_count = GetSize() / (sizeof(uint32_t)+sizeof(uint16_t)+sizeof(float));
			BoneAssignment assignment;
			while (bone_count-- > 0)
			{
				assignment.vertex = stream->Read<uint32_t>();
				assignment.bone = stream->Read<uint16_t>();
				assignment.weight = stream->Read<float>();
				assignments.push_back(assignment);
			}
		}

		void ChunkMeshBounds::ReadImpl(DataStream *stream)
		{
			bounds.x1 = stream->Read<float>();
			bounds.y1 = stream->Read<float>();
			bounds.z1 = stream->Read<float>();
			bounds.x2 = stream->Read<float>();
			bounds.y2 = stream->Read<float>();
			bounds.z2 = stream->Read<float>();
			radius = stream->Read<float>();
		}

		void ChunkGeometry::ReadImpl(DataStream *stream)
		{
			vertexCount = stream->Read<uint32_t>();
			for (ChunkID id = Chunk::Peek(stream);
			     id == CID_Geometry_Vertex_Decl || id == CID_Geometry_Vertex_Buffer;
			     id = Chunk::Peek(stream)
			    )
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);

				switch (chunk->GetType())
				{
				case CID_Geometry_Vertex_Decl:
					if (!vertexDeclaration.empty())
						throw MultipleSingletonChunks("There's only one CID_Geometry_Vertex_Decl chunk allowed within a CID_Geometry chunk");
					vertexDeclaration.swap(static_cast<ChunkGeometryVertexDecl*>(chunk.get())->declaration);
					break;
				case CID_Geometry_Vertex_Buffer:
					vertexBuffers.push_back(static_unique_cast<ChunkGeometryVertexBuffer>(move(chunk)));
					break;
				default:
					LogF("StdMeshLoader: I don't know what to do with a chunk of type 0x%xu inside a CID_Geometry chunk", chunk->GetType());
					break;
				}
				if (stream->AtEof()) break;
			}
		}

		void ChunkGeometryVertexDecl::ReadImpl(DataStream *stream)
		{
			while (Chunk::Peek(stream) == CID_Geometry_Vertex_Decl_Element)
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				assert(chunk->GetType() == CID_Geometry_Vertex_Decl_Element);
				declaration.push_back(static_unique_cast<ChunkGeometryVertexDeclElement>(chunk));
				if (stream->AtEof()) break;
			}
		}

		void ChunkGeometryVertexDeclElement::ReadImpl(DataStream *stream)
		{
			source = stream->Read<uint16_t>();
			int32_t t = stream->Read<uint16_t>();
			if (t < VDET_MIN || t > VDET_MAX)
				throw InvalidVertexType();
			type = static_cast<Type>(t);
			t = stream->Read<uint16_t>();
			if (t < VDES_MIN || t > VDES_MAX)
				throw InvalidVertexSemantic();
			semantic = static_cast<Semantic>(t);
			offset = stream->Read<uint16_t>();
			index = stream->Read<uint16_t>();
		}

		void ChunkGeometryVertexBuffer::ReadImpl(DataStream *stream)
		{
			index = stream->Read<uint16_t>();
			vertexSize = stream->Read<uint16_t>();

			while (Chunk::Peek(stream) == CID_Geometry_Vertex_Data)
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				assert(chunk->GetType() == CID_Geometry_Vertex_Data);
				if (data)
					throw MultipleSingletonChunks("There's only one CID_Geometry_Vertex_Data chunk allowed within a CID_Geometry_Vertex_Buffer chunk");
				data = static_unique_cast<ChunkGeometryVertexData>(move(chunk));
				if (stream->AtEof()) break;
			}
		}

		void ChunkGeometryVertexData::ReadImpl(DataStream *stream)
		{
			data = new char[GetSize()];
			stream->Read(data, GetSize());
		}
	}

	namespace Skeleton
	{
		const uint32_t ChunkFileHeader::CurrentVersion = 1080; // Major * 1000 + Minor
		const std::map<std::string, uint32_t> ChunkFileHeader::VersionTable = {
		    // 1.80: Current version
		    std::make_pair("[Serializer_v1.80]",  CurrentVersion),
		    // 1.10: adds SKELETON_BLENDMODE and SKELETON_ANIMATION_BASEINFO chunks. The chunks have been added to the loader, but we ignore them for now.
		    std::make_pair("[Serializer_v1.10]", 1010)
		};

		std::unique_ptr<Chunk> Chunk::Read(DataStream *stream)
		{
			assert(stream->GetRemainingBytes() >= ChunkHeaderLength);

			// Read metadata
			ChunkID id = CID_Invalid;
			id = static_cast<ChunkID>(stream->Read<uint16_t>());
			size_t size = 0;
			// Special case: CID_Header doesn't have any size info.
			if (id != CID_Header)
			{
				// All others are proper chunks.
				size = stream->Read<uint32_t>();
				size -= ChunkHeaderLength;
			}

			// Create chunk
			std::unique_ptr<Chunk> chunk;
			switch (id)
			{
			case CID_Header: chunk.reset(new ChunkFileHeader()); break;
			case CID_BlendMode: chunk.reset(new ChunkBlendMode()); break;
			case CID_Bone: chunk.reset(new ChunkBone()); break;
			case CID_Bone_Parent: chunk.reset(new ChunkBoneParent()); break;
			case CID_Animation: chunk.reset(new ChunkAnimation()); break;
			case CID_Animation_BaseInfo: chunk.reset(new ChunkAnimationBaseInfo()); break;
			case CID_Animation_Track: chunk.reset(new ChunkAnimationTrack()); break;
			case CID_Animation_Track_KF: chunk.reset(new ChunkAnimationTrackKF()); break;
			case CID_Animation_Link: chunk.reset(new ChunkAnimationLink()); break;
			default:
				LogF("StdMeshLoader: I don't know what to do with a chunk of type 0x%xu", id);
				chunk.reset(new ChunkUnknown()); break;
			};
			chunk->type = id;
			chunk->size = size;
			chunk->ReadImpl(stream);
			return chunk;
		}

		void ChunkUnknown::ReadImpl(DataStream *stream) { stream->Seek(GetSize()); }

		void ChunkFileHeader::ReadImpl(DataStream *stream)
		{
			// Simple version check
			VersionTable_t::const_iterator it = VersionTable.find(stream->Read<std::string>());
			if (it == VersionTable.end())
				throw InvalidVersion();
		}

		void ChunkBlendMode::ReadImpl(DataStream* stream)
		{
			blend_mode = stream->Read<uint16_t>();
		}

		void ChunkBone::ReadImpl(DataStream *stream)
		{
			name = stream->Read<std::string>();
			handle = stream->Read<uint16_t>();
			position.x = stream->Read<float>();
			position.y = stream->Read<float>();
			position.z = stream->Read<float>();
			orientation.x = stream->Read<float>();
			orientation.y = stream->Read<float>();
			orientation.z = stream->Read<float>();
			orientation.w = stream->Read<float>();
			// Guess whether we have a scale element
			if (GetSize() > name.size() + 1 + sizeof(handle) + sizeof(float) * 7)
			{
				scale.x = stream->Read<float>();
				scale.y = stream->Read<float>();
				scale.z = stream->Read<float>();
			}
			else
			{
				scale = StdMeshVector::UnitScale();
			}
		}

		void ChunkBoneParent::ReadImpl(DataStream *stream)
		{
			childHandle = stream->Read<uint16_t>();
			parentHandle = stream->Read<uint16_t>();
		}

		void ChunkAnimation::ReadImpl(DataStream *stream)
		{
			name = stream->Read<std::string>();
			duration = stream->Read<float>();

			if(!stream->AtEof() && Chunk::Peek(stream) == CID_Animation_BaseInfo)
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				assert(chunk->GetType() == CID_Animation_BaseInfo);
				// TODO: Handle it
				LogF("StdMeshLoader: CID_Animation_BaseInfo not implemented. Skeleton might not be imported properly.");
			}

			while (!stream->AtEof() && Chunk::Peek(stream) == CID_Animation_Track)
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				assert(chunk->GetType() == CID_Animation_Track);
				tracks.push_back(static_unique_cast<ChunkAnimationTrack>(move(chunk)));
			}
		}

		void ChunkAnimationBaseInfo::ReadImpl(DataStream* stream)
		{
			base_animation_name = stream->Read<std::string>();
			base_key_frame_time = stream->Read<float>();
		}

		void ChunkAnimationTrack::ReadImpl(DataStream *stream)
		{
			bone = stream->Read<uint16_t>();
			while (Chunk::Peek(stream) == CID_Animation_Track_KF)
			{
				std::unique_ptr<Chunk> chunk = Chunk::Read(stream);
				assert(chunk->GetType() == CID_Animation_Track_KF);
				keyframes.push_back(static_unique_cast<ChunkAnimationTrackKF>(move(chunk)));
				if (stream->AtEof()) break;
			}
		}

		void ChunkAnimationTrackKF::ReadImpl(DataStream *stream)
		{
			time = stream->Read<float>();
			rotation.x = stream->Read<float>();
			rotation.y = stream->Read<float>();
			rotation.z = stream->Read<float>();
			rotation.w = stream->Read<float>();
			translation.x = stream->Read<float>();
			translation.y = stream->Read<float>();
			translation.z = stream->Read<float>();
			// Guess whether we have a scale element
			if (GetSize() > sizeof(float) * 8)
			{
				scale.x = stream->Read<float>();
				scale.y = stream->Read<float>();
				scale.z = stream->Read<float>();
			}
			else
			{
				scale = StdMeshVector::UnitScale();
			}
		}

		void ChunkAnimationLink::ReadImpl(DataStream *stream)
		{
			file = stream->Read<std::string>();
			scale.x = stream->Read<float>();
			scale.y = stream->Read<float>();
			scale.z = stream->Read<float>();
		}
	}
}
