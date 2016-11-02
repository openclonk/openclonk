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

#ifndef INC_StdMeshLoaderChunks
#define INC_StdMeshLoaderChunks

#include "lib/StdMesh.h"
#include "lib/StdMeshLoaderDataStream.h"

// ==== Ogre file format ====
// The Ogre file format is a chunked format similar to PNG.
// Each chunk except for the file header (type 0x1000) has the following format:
//  uint32_t chunk_type, values dependent on file type (mesh or skeleton)
//  uint32_t chunk_length, in bytes; includes the size of the header
//  uint8_t data[]
// The file header omits the length field.
// Ogre files can be stored in either big-endian or little-endian byte order. The
// byte order is determined by the first two bytes in the file, which must always
// be the chunk type of the file header (0x1000). If the bytes are 0x00 0x10, the
// file is assumed to be little-endian, if they are 0x10 0x00, the file is big-
// endian. If any other bytes are encountered, the file is invalid.
// This implementation only reads files stored in the platform's native byte order.
//
// ==== Data types ====
// Most of the numeric data is stored directly in binary as a memory representation.
// Strings are stored without a leading length field, and terminated by a 0x0A byte.
// Boolean fields are stored as a single byte, which is 0x00 for false or anything
// else for true.
//
// ==== Mesh chunks ====
// Unless specified, this section will only describe the data part of each chunk.
// Any values that are in parentheses are not available in this implementation.
//
// Type 0x1000: File Header
//  string version
//    Depending on the version, different features are enabled or disabled while
//    loading the file.
//    This implementation does not support fallback to different versions of the
//    file format.
//  This chunk does not contain a length field; the type id is directly followed
//  by the version string.
//  The file header must be immediately followed by a 0x3000 Mesh Data chunk.
// ----------
// Type 0x3000: Mesh Data
//  bool skeletal_animation
//    This field is only used by Ogre to optimize rendering. Set if the mesh has
//    animation data associated with it.
//  Allowed subchunks:
//    0x4000 Submesh Data
//    0x5000 Geometry Data (may occur at most once)
//    0x6000 Skeleton Link (may occur at most once)
//    0x7000 Mesh Bone Assignments
//    (0x8000 Precomputed Level of Detail Data)
//    0x9000 Mesh Boundaries
//    (0xA000 Submesh Name Table)
//    (0xB000 Edge List)
//    (0xC000 Animation Poses)
//    (0xD000 Animation List)
//    (0xE000 Table of Extremes)
// ----------
// Type 0x4000: Submesh data
//  string material
//    The name of a material to be used for this submesh. Must be loaded externally.
//  bool vertices_shared
//    If set, the submesh uses the geometry data of its parent mesh. In this case, the
//    submesh may not contain its own 0x5000 Geometry Data chunk.
//  uint32_t index_count
//    The number of vertex indices used by this submesh.
//  bool indices_are_32_bit
//    If set, each vertex index is 32 bits wide. If unset, each vertex index is 16 bits
//    wide.
//  uint16_t/uint32_t indices[index_count]
//    A list of vertex indices used by this submesh.
//  Allowed subchunks:
//    0x5000 Geometry Data (only if vertices_shared == false)
//    0x4010 Submesh Operation
//    0x4100 Submesh Bone Assignments
//    (0x4200 Submesh Texture Replacement)
//  If no 0x4010 Submesh Operation chunk is present, the submesh uses SO_TriList.
// ----------
// Type 0x4010: Submesh Operation
//  uint16_t operation
//    This value specifies how the indices of a submesh are to be interpreted.
//    Acceptable values:
//      (0x01 SO_PointList)
//        Each index defines a single point.
//      (0x02 SO_LineList)
//        Each pair (2k, 2k+1) of indices defines a line.
//      (0x03 SO_LineStrip)
//        Each pair (k, k+1) of indices defines a line.
//      0x04 SO_TriList
//        Each triplet (3k, 3k+1, 3k+2) of indices defines a triangle.
//      (0x05 SO_TriStrip)
//        For odd k, (k, k+1, k+2) define a triangle. For even k, (k+1, k, k+2) define a triangle.
//      (0x06 SO_TriFan)
//        Indices (1, k+1, k+2) define a triangle.
// ----------
// Type 0x4100: Submesh Bone Assignments
// Type 0x7000: Mesh Bone Assignments
//  uint32_t vertex_index
//  uint32_t bone_index
//  float weight
//    This defines the strength of the influence a bone has on a vertex. The sum of
//    the weights on each vertex is not guaranteed to be 1.0.
//  These values repeat until the size of the chunk is exhausted.
// ----------
// Type 0x5000: Geometry Data
//  uint32_t vertex_count
//    The number of vertices stored in each contained 0x5020 Vertex Buffer chunk.
//  Allowed subchunks:
//    0x5100 Vertex Declaration (required; must occur exactly once)
//    0x5200 Vertex Buffer (required; must occur at least once)
// ----------
// Type 0x5100 Vertex Declaration
//  This chunk does not store any values itself. It only acts as a container for
//  0x5110 Vertex Declaration Element chunks.
//  Allowed subchunks:
//    0x5110 Vertex Declaration Element (required; repeats until the size of the chunk is exhausted)
// ----------
// Type 0x5110 Vertex Declaration Element
//  uint16_t source
//    The index of the stream where this element is found.
//  uint16_t type
//    The type of data this element contains.
//    Acceptable values:
//      0x00 VDET_Float1
//        A single 32-bit float, expanded to (a, 0, 0, 1)
//      0x01 VDET_Float2
//        Two 32-bit floats, expanded to (a, b, 0, 1)
//      0x02 VDET_Float3
//        Three 32-bit floats, expanded to (a, b, c, 1)
//      0x03 VDET_Float4
//        Four 32-bit floats
//      (0x04 VDET_Reserved)
//      (0x05 VDET_Short1)
//        A single 16-bit integer
//      (0x06 VDET_Short2)
//        Two 16-bit integers
//      (0x07 VDET_Short3)
//        Three 16-bit integers
//      (0x08 VDET_Short4)
//        Four 16-bit integers
//      (0x09 VDET_UByte4)
//        Four 8-bit integers
//      0x0A VDET_Color_ARGB
//        Four 8-bit integers, describing a color in the order Alpha, Red, Green, Blue.
//        Acceptable values range from 0 to 255.
//      0x0B VDET_Color_ABGR
//        Four 8-bit integers, describing a color in the order Alpha, Blue, Green, Red.
//        Acceptable values range from 0 to 255.
//  uint16_t semantic
//    The semantic of the data this element contains.
//    Acceptable values:
//      0x01 VDES_Position
//        The element contains the vertex's position in world space.
//      (0x02 VDES_Blend_Weights)
//      (0x03 VDES_Blend_Indices)
//      0x04 VDES_Normals
//        The element contains vertex normals.
//      (0x05 VDES_Diffuse)
//        The element contains the diffuse color of the vertex.
//      (0x06 VDES_Specular)
//        The element contains the specular color of the vertex.
//      0x07 VDES_Texcoords
//        The element contains the texture coordinates of the vertex.
//  uint16_t offset
//    The element's offset in bytes from the beginning of the stream.
//  uint16_t index
//    Index of the element semantic. Used with colors and texture coordinates.
// ----------
// 0x5200 Vertex Buffer
//  uint16_t index
//    The index of the stream this buffer will get bound to.
//  uint16_t stride
//    The distance in bytes between two elements inside the buffer.
//  Allowed subchunks:
//    0x5210 Vertex Buffer Data (required; must occur exactly once)
// ----------
// 0x5210 Vertex Buffer Data
//  uint8_t data[]
//    The buffered data. This field spans the whole extent of the chunk.
// ----------
// 0x6000 Skeleton Link
//  string file
//    The name of the file that contains the skeleton of this mesh.
// ----------
// 0x9000 Mesh Boundaries
//  float min[3]
//    The minimum extents of the axis-aligned bounding box of the mesh.
//  float max[3]
//    The maximum extents of the axis-aligned bounding box of the mesh.
//  float radius
//    The radius of the minimal enclosing sphere of the mesh.
//
// ==== Skeleton files ====
// Each skeleton file must begin with a 0x1000 File Header chunk. Afterwards,
// any combination of the following chunks may appear:
//  0x2000 Bone Data
//  0x3000 Bone Hierarchy
//  0x4000 Animation
//  (0x5000 Animation Link)
//
// Unless specified, this section will only describe the data part of each chunk.
// Any values that are in parentheses are not available in this implementation.
//
// Type 0x1000: File Header
//  string version
//    Depending on the version, different features are enabled or disabled while
//    loading the file.
//    This implementation does not support fallback to different versions of the
//    file format.
//  This chunk does not contain a length field; the type id is directly followed
//  by the version string.
// ----------
// Type 0x2000: Bone Data
//  string name
//    The name of the bone. This is only used to produce human-readable output.
//  uint16_t handle
//    The internal handle of the bone. All other chunks that refer to a bone reference
//    this.
//  float position[3]
//    The position of the bone, relative to its parent.
//  float orientation[4]
//    The orientation of the bone, as a quaternion (x,y,z,w). Relative to the parent.
//  float scale[3]
//    The scale of the bone, relative to its parent. This element only appears if the
//    chunk size is large enough; if it is omitted, it defaults to (1,1,1).
// ----------
// Type 0x3000 Bone Hierarchy
//  uint16_t child
//    The handle of the parent bone.
//  uint16_t parent
//    The handle of the parent bone.
// ----------
// Type 0x4000 Animation
//  string name
//    The name of this animation.
//  float duration
//    The length of this animation, in seconds.
//  Allowed subchunks:
//    0x4100 Animation Track
// ----------
// Type 0x4100 Animation Track
//  uint16_t bone
//    The handle of the bone this track belongs to.
//  Allowed subchunks:
//    0x4110 Animation Track Keyframe
// ----------
// Type 0x4110 Animation Track Keyframe
//  float time
//    The time this keyframe matches.
//  float rotation[4]
//    The rotation of the bone at the corresponding time, as a quaternion (x,y,z,w).
//  float translation[3]
//    The translation of the bone at the corresponding time.
//  float scale[3]
//    The scale of the bone at the time of the keyframe. This element only appears if the
//    chunk size is large enough; if it is omitted, it defaults to (1,1,1).

// Most of the chunk classes below faithfully match the abovementioned file format.
namespace Ogre
{
	// used to have boost::ptr_vector. Behaves reasonably similar
	template<typename T>
	using unique_ptr_vector = std::vector<std::unique_ptr<T>>;

	class DataStream;
	template<class _Type>
	class ChunkBase
	{
	protected:
		ChunkBase() : type(static_cast<Type>(0)) {}
		virtual void ReadImpl(DataStream *stream) = 0;
		typedef _Type Type;
		Type type;
		size_t size;
	public:
		virtual ~ChunkBase() {}
		Type GetType() const { return type; }
		size_t GetSize() const { return size; }

		static const size_t ChunkHeaderLength = sizeof(uint16_t) /* chunk type */ + sizeof(uint32_t) /* chunk length */;
		static Type Peek(const DataStream *stream)
		{
			return static_cast<Type>(stream->Peek<uint16_t>());
		}
	};

	namespace Mesh
	{
		enum ChunkID
		{
			CID_Invalid = 0,
			CID_Header = 0x1000,
			CID_Mesh = 0x3000,
			CID_Submesh = 0x4000,
			CID_Submesh_Op = 0x4010,
			CID_Submesh_Bone_Assignment = 0x4100,
			CID_Submesh_Texture_Alias = 0x4200,
			CID_Geometry = 0x5000,
			CID_Geometry_Vertex_Decl = 0x5100,
			CID_Geometry_Vertex_Decl_Element = 0x5110,
			CID_Geometry_Vertex_Buffer = 0x5200,
			CID_Geometry_Vertex_Data = 0x5210,
			CID_Mesh_Skeleton_Link = 0x6000,
			CID_Mesh_Bone_Assignment = 0x7000,
			CID_Mesh_LOD = 0x8000,
			CID_Mesh_LOD_Usage = 0x8100,
			CID_Mesh_LOD_Manual = 0x8110,
			CID_Mesh_LOD_Generated = 0x8120,
			CID_Mesh_Bounds = 0x9000,
			CID_Submesh_Name_Table = 0xA000,
			CID_Submesh_Name_Table_Entry = 0xA100,
			CID_Edge_List = 0xB000,
			CID_Edge_List_LOD = 0xB100,
			CID_Edge_Group = 0xB110,
			CID_Pose_List = 0xC000,
			CID_Pose = 0xC100,
			CID_Pose_Vertex = 0xC111,
			CID_Animation_List = 0xD000,
			CID_Animation = 0xD100,
			CID_Animation_Track = 0xD110,
			CID_Animation_Morph_Keyframe = 0xD111,
			CID_Animation_Pose_Keyframe = 0xD112,
			CID_Animation_Pose_Ref = 0xD113,
			CID_Table_Extremes = 0xE000
		};

		struct BoneAssignment
		{
			uint32_t vertex;
			uint32_t bone;
			float weight;
		};

		class Chunk : public ChunkBase<ChunkID>
		{
		public:
			static std::unique_ptr<Chunk> Read(DataStream *stream);
		};

		class ChunkUnknown; class 
		ChunkFileHeader;
		class ChunkMesh; class ChunkMeshSkeletonLink; class ChunkMeshBoneAssignments; class ChunkMeshBounds;
		class ChunkSubmesh; class ChunkSubmeshOp;
		class ChunkGeometry; class ChunkGeometryVertexDecl; class ChunkGeometryVertexDeclElement; class ChunkGeometryVertexBuffer; class ChunkGeometryVertexData;
		class ChunkPoseList; class ChunkPose; class ChunkPoseVertex;
		class ChunkAnimationList; class ChunkAnimation; class ChunkAnimationTrack;
		class ChunkAnimationMorphKF; class ChunkAnimationPoseKF; class ChunkAnimationPoseRef;

		class ChunkUnknown : public Chunk
		{
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkFileHeader : public Chunk
		{
			typedef std::map<std::string, uint32_t> VersionTable_t;
			static const VersionTable_t VersionTable;
			static const uint32_t CurrentVersion;
		public:
			std::string version;

		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkMesh : public Chunk
		{
		public:
			ChunkMesh() : hasAnimatedSkeleton(false), radius(0.0f) {}
			bool hasAnimatedSkeleton;
			std::string skeletonFile;
			std::unique_ptr<ChunkGeometry> geometry;
			unique_ptr_vector<ChunkSubmesh> submeshes;
			std::vector<BoneAssignment> boneAssignments;
			StdMeshBox bounds;
			float radius;

		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkMeshSkeletonLink : public Chunk
		{
		public:
			std::string skeleton;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkSubmesh : public Chunk
		{
		public:
			ChunkSubmesh() : operation(SO_TriList) {}
			std::string material;
			bool hasSharedVertices;
			std::vector<size_t> faceVertices;
			std::unique_ptr<ChunkGeometry> geometry;
			enum SubmeshOperation
			{
				SO_PointList = 1,
				SO_LineList = 2,
				SO_LineStrip = 3,
				SO_TriList = 4,
				SO_TriStrip = 5,
				SO_TriFan = 6,
				SO_MIN = SO_PointList,
				SO_MAX = SO_TriFan
			} operation;
			std::vector<BoneAssignment> boneAssignments;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkSubmeshOp : public Chunk
		{
		public:
			ChunkSubmesh::SubmeshOperation operation;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkMeshBoneAssignments : public Chunk
		{
		public:
			std::vector<BoneAssignment> assignments;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkMeshBounds : public Chunk
		{
		public:
			StdMeshBox bounds;
			float radius;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkGeometry : public Chunk
		{
		public:
			size_t vertexCount;
			unique_ptr_vector<ChunkGeometryVertexDeclElement> vertexDeclaration;
			unique_ptr_vector<ChunkGeometryVertexBuffer> vertexBuffers;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkGeometryVertexDecl : public Chunk
		{
		public:
			unique_ptr_vector<ChunkGeometryVertexDeclElement> declaration;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkGeometryVertexDeclElement : public Chunk
		{
		public:
			uint16_t source;
			uint16_t offset;
			enum Type
			{
				VDET_Float1 = 0,
				VDET_Float2 = 1,
				VDET_Float3 = 2,
				VDET_Float4 = 3,
				/* 4 omitted intentionally */
				VDET_Short1 = 5,
				VDET_Short2 = 6,
				VDET_Short3 = 7,
				VDET_Short4 = 8,
				VDET_UByte4 = 9,
				VDET_Color_ARGB = 10,
				VDET_Color_ABGR = 11,
				VDET_MIN = VDET_Float1,
				VDET_MAX = VDET_Color_ABGR
			} type;
			enum Semantic
			{
				VDES_Position = 1,
				VDES_Blend_Weights = 2,
				VDES_Blend_Indices = 3,
				VDES_Normals = 4,
				VDES_Diffuse = 5,
				VDES_Specular = 6,
				VDES_Texcoords = 7,
				VDES_Binormal = 8,
				VDES_Tangent = 9,
				VDES_MIN = VDES_Position,
				VDES_MAX = VDES_Tangent
			} semantic;
			uint16_t index;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkGeometryVertexBuffer : public Chunk
		{
		public:
			uint16_t index;
			uint16_t vertexSize;
			std::unique_ptr<ChunkGeometryVertexData> data;
		protected:
			void ReadImpl(DataStream *stream);
		};

		class ChunkGeometryVertexData : public Chunk
		{
		public:
			ChunkGeometryVertexData() : data(nullptr) {}
			~ChunkGeometryVertexData() { delete[] static_cast<char*>(data); }
			void *data;
		protected:
			void ReadImpl(DataStream *stream);
		};
	}

	namespace Skeleton
	{
		enum ChunkID
		{
			CID_Invalid = 0,
			CID_Header = 0x1000,
			CID_BlendMode=  0x1010,
			CID_Bone = 0x2000,
			CID_Bone_Parent = 0x3000,
			CID_Animation = 0x4000,
			CID_Animation_BaseInfo = 0x4010,
			CID_Animation_Track = 0x4100,
			CID_Animation_Track_KF = 0x4110,
			CID_Animation_Link = 0x5000
		};

		class Chunk : public ChunkBase<ChunkID>
		{
		public:
			static std::unique_ptr<Chunk> Read(DataStream *stream);
		};

		class ChunkUnknown; class ChunkFileHeader;
		class ChunkBone; class ChunkBoneParent;
		class ChunkAnimation; class ChunkAnimationTrack; class ChunkAnimationTrackKF; class ChunkAnimationLink;

		class ChunkUnknown : public Chunk
		{
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkFileHeader : public Chunk
		{
			typedef std::map<std::string, uint32_t> VersionTable_t;
			static const VersionTable_t VersionTable;
			static const uint32_t CurrentVersion;
		public:
			std::string version;

		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkBlendMode : public Chunk
		{
		public:
			uint16_t blend_mode;
		protected:
			virtual void ReadImpl(DataStream* stream);
		};

		class ChunkBone : public Chunk
		{
		public:
			std::string name;
			uint16_t handle;

			StdMeshVector position;
			StdMeshQuaternion orientation;
			StdMeshVector scale;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkBoneParent : public Chunk
		{
		public:
			uint16_t childHandle;
			uint16_t parentHandle;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkAnimation : public Chunk
		{
		public:
			std::string name;
			float duration;
			unique_ptr_vector<ChunkAnimationTrack> tracks;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkAnimationBaseInfo : public Chunk
		{
		public:
			std::string base_animation_name;
			float base_key_frame_time;
		protected:
			virtual void ReadImpl(DataStream* stream);
		};

		class ChunkAnimationTrack : public Chunk
		{
		public:
			uint16_t bone;
			unique_ptr_vector<ChunkAnimationTrackKF> keyframes;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkAnimationTrackKF : public Chunk
		{
		public:
			float time;
			StdMeshQuaternion rotation;
			StdMeshVector translation;
			StdMeshVector scale;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};

		class ChunkAnimationLink : public Chunk
		{
		public:
			std::string file;
			StdMeshVector scale;
		protected:
			virtual void ReadImpl(DataStream *stream);
		};
	}
}

#endif
