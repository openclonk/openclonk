/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009  Armin Burgmeier
 * Copyright (c) 2010  Nicolas Hake
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

#ifndef INC_StdMeshLoader
#define INC_StdMeshLoader
#include <stdexcept>

class StdMesh;
class StdMeshMatManager;

// Interface to load skeleton files. Given a filename occuring in the
// mesh file, this should load the skeleton file from wherever the mesh file
// was loaded from, for example from a C4Group. Return default-construted
// StdStrBuf with NULL data in case of error.
class StdMeshSkeletonLoader
{
public:
	virtual StdStrBuf LoadSkeleton(const char* filename) = 0;
	virtual ~StdMeshSkeletonLoader() {}
};

class StdMeshLoader
{
public:
class LoaderException : public std::runtime_error { public: LoaderException(const char *msg) : std::runtime_error(msg) {} };

	// filename is only used to show it in error messages. The model is
	// loaded from src. Throws LoaderException.
	static StdMesh *LoadMeshBinary(const char *src, size_t size, const StdMeshMatManager &mat_mgr, StdMeshSkeletonLoader &loader, const char *filename = 0);
	static StdMesh *LoadMeshXml(const char *src, size_t size, const StdMeshMatManager &mat_mgr, StdMeshSkeletonLoader &loader, const char *filename = 0);

private:
	static void LoadSkeletonBinary(StdMesh *mesh, const char *src, size_t size);
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
