/*
 * OpenClonk, http://www.openclonk.org
 *
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

// A loader for the OGRE .mesh binary file format
#include "lib/StdMesh.h"

#ifndef INC_StdMeshLoader
#define INC_StdMeshLoader
#include <stdexcept>

namespace OgreToClonk
{
	StdMeshVector TransformVector(const StdMeshVector& vector);
	StdMeshVector TransformPseudoVector(const StdMeshVector& vector);
	StdMeshVector TransformNormalVector(const StdMeshVector& vector);
	StdMeshVector TransformScaleVector(const StdMeshVector& vector);
	StdMeshQuaternion TransformQuaternion(const StdMeshQuaternion& quaternion);
	StdSubMesh::Vertex TransformVertex(const StdSubMesh::Vertex& vertex);
	StdMeshTransformation TransformTransformation(const StdMeshTransformation& transformation);
}

class StdMeshLoader
{
public:
	class StdMeshXML;
	class LoaderException : public std::runtime_error { public: LoaderException(const char *msg) : std::runtime_error(msg) {} };

	// filename is only used to show it in error messages. The model is
	// loaded from src. Throws LoaderException.
	static StdMesh *LoadMeshBinary(const char *sourcefile, size_t size, const StdMeshMatManager &mat_mgr, StdMeshSkeletonLoader &loader, const char *filename = 0);
	static StdMesh *LoadMeshXml(const char *sourcefile, size_t size, const StdMeshMatManager &mat_mgr, StdMeshSkeletonLoader &loader, const char *filename = 0);
};

// Interface to load skeleton files. Given a filename occuring in the
// mesh file, this should load the skeleton file from wherever the mesh file
// was loaded from, for example from a C4Group. Return default-construted
// StdStrBuf with nullptr data in case of error.
class StdMeshSkeletonLoader
{
public:
	virtual ~StdMeshSkeletonLoader() {}

	void StoreSkeleton(const char* groupname, const char* filename, std::shared_ptr<StdMeshSkeleton> skeleton);
	void RemoveSkeleton(const StdCopyStrBuf& filepath);
	void RemoveSkeleton(const char* groupname, const char* filename);
	void RemoveSkeletonsInGroup(const char* groupname);

	virtual StdMeshSkeleton* GetSkeletonByDefinition(const char* definition) const = 0;
	std::shared_ptr<StdMeshSkeleton> GetSkeletonByName(const StdStrBuf& name) const;
	void LoadSkeletonXml(const char* groupname, const char* filename, const char *sourcefile, size_t size);
	void LoadSkeletonBinary(const char* groupname, const char* filename, const char *sourcefile, size_t size);

	void ResolveIncompleteSkeletons();

	static void MakeFullSkeletonPath(StdCopyStrBuf &out, const char* groupname, const char* filename)
	{
		out = StdCopyStrBuf(groupname);
		out.AppendBackslash();
		out.Append(filename);
		out.ToLowerCase();
	}

	void Clear()
	{
		Skeletons.clear();
		AppendtoSkeletons.clear();
		IncludeSkeletons.clear();
	}

	// Allows to iterate over loaded skeletons:
	typedef std::map<StdCopyStrBuf, std::shared_ptr<StdMeshSkeleton>> SkeletonMap;
	typedef SkeletonMap::const_iterator skeleton_iterator;

	skeleton_iterator skeletons_begin() const { return Skeletons.begin(); }
	skeleton_iterator skeletons_end() const { return Skeletons.end(); }

private:
	void AddSkeleton(const StdCopyStrBuf& filepath, std::shared_ptr<StdMeshSkeleton> skeleton);
	void DoResetSkeletons();
	void DoAppendSkeletons();
	void DoIncludeSkeletons();

	SkeletonMap Skeletons;

	// was a map <pointer to skeleton, id> first, but that lead to problems with reloading:
	// the old skeleton was still in the map, because the new skeleton was a different object of course
	// so instead of finding the old skeleton it seemed simpler to just save a key that can be overloaded

	std::map<StdCopyStrBuf, StdCopyStrBuf> AppendtoSkeletons; // key is the filepath, value is the id to append to
	std::map<StdCopyStrBuf, StdCopyStrBuf> IncludeSkeletons;  // key is the filepath, value is the definition to include
};

#define DEFINE_EXCEPTION(_cls, _text) class _cls : public StdMeshLoader::LoaderException { public: _cls(const char *msg = _text) : LoaderException(msg) {} }

namespace Ogre
{
	DEFINE_EXCEPTION(InsufficientData, "Premature end of data stream");
	DEFINE_EXCEPTION(MultipleSingletonChunks, "A singleton chunk was found multiple times");
	namespace Mesh
	{
		DEFINE_EXCEPTION(InvalidVersion, "Mesh header does not contain the expected version");
		DEFINE_EXCEPTION(SharedVertexGeometryForbidden, "A CID_Geometry chunk was found in a submesh with shared vertices");
		DEFINE_EXCEPTION(InvalidSubmeshOp, "The render operation of a CID_Submesh_Op chunk was invalid");
		DEFINE_EXCEPTION(InvalidVertexType, "The vertex type of a CID_Geometry_Vertex_Decl_Element chunk was invalid");
		DEFINE_EXCEPTION(InvalidVertexSemantic, "The vertex semantic of a CID_Geometry_Vertex_Decl_Element chunk was invalid");
		DEFINE_EXCEPTION(InvalidVertexDeclaration, "The vertex declaration of a CID_Geometry chunk was invalid");
		DEFINE_EXCEPTION(InvalidMaterial, "The material referenced by a mesh or submesh is not defined");
		DEFINE_EXCEPTION(VertexNotFound, "A specified vertex was not found");
		DEFINE_EXCEPTION(EmptyBoundingBox, "Bounding box is empty");
		DEFINE_EXCEPTION(NotImplemented, "The requested operation is not implemented");
	}
	namespace Skeleton
	{
		DEFINE_EXCEPTION(InvalidVersion, "Skeleton header does not contain the expected version");
		DEFINE_EXCEPTION(IdNotUnique, "An element with an unique ID appeared multiple times");
		DEFINE_EXCEPTION(BoneNotFound, "A specified bone was not found");
		DEFINE_EXCEPTION(MissingMasterBone, "The skeleton does not have a master bone");
		DEFINE_EXCEPTION(MultipleBoneTracks, "An animation has multiple tracks for one bone");
	}
}

#undef DEFINE_EXCEPTION

#endif
