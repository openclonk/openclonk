/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009 Armin Burgmeier
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

#ifndef INC_StdMesh
#define INC_StdMesh

#include <StdMeshMaterial.h>

// Loader for OGRE meshes. Currently supports XML files only.

class StdMeshError: public std::exception
{
public:
	StdMeshError(const StdStrBuf& message, const char* file, unsigned int line);
	virtual ~StdMeshError() throw() {}

	virtual const char* what() const throw() { return Buf.getData(); }

protected:
	StdCopyStrBuf Buf;
};

// Interface to load skeleton files. Given a filename occuring in the
// mesh file, this should load the skeleton file from wherever the mesh file
// was loaded from, for example from a C4Group. Return default-construted
// StdStrBuf with NULL data in case of error.
class StdMeshSkeletonLoader
{
public:
	virtual StdStrBuf LoadSkeleton(const char* filename) = 0;
};

class StdMeshMatrix
{
public:
	void SetIdentity();
	void SetTranslate(float dx, float dy, float dz);
	void SetScale(float sx, float sy, float sz);
	void SetRotate(float angle, float rx, float ry, float rz);

	float& operator()(int i, int j) { return a[i][j]; }
	float operator()(int i, int j) const { return a[i][j]; }

	// *this *= other
	void Mul(const StdMeshMatrix& other);
	void Mul(float f);
	// *this += other
	void Add(const StdMeshMatrix& other);

	// *this = other * *this
	void Transform(const StdMeshMatrix& other);
private:
	// 3x3 orthogonal + translation in last column
	float a[3][4];
};

class StdMeshBone
{
	friend class StdMesh;
public:
	StdMeshBone() {}

	unsigned int Index; // Index in master bone table
	int ID; // Bone ID
	StdStrBuf Name; // Bone name

	// Bone transformation
	StdMeshMatrix Trans;
	// Inverse transformation
	StdMeshMatrix InverseTrans;

	const StdMeshBone* GetParent() const { return Parent; }

	const StdMeshBone& GetChild(unsigned int i) const { return *Children[i]; }
	unsigned int GetNumChildren() const { return Children.size(); }

private:
	StdMeshBone* Parent; // Parent bone
	std::vector<StdMeshBone*> Children; // Children. Not owned.

	StdMeshBone(const StdMeshBone&); // non-copyable
	StdMeshBone& operator=(const StdMeshBone&); // non-assignable
};

class StdMeshVertexBoneAssignment
{
public:
	unsigned int BoneIndex;
	float Weight;
};

class StdMeshVertex
{
public:
	float x, y, z;
	float nx, ny, nz;
	float u, v;

	// *this = trans * *this
	void Transform(const StdMeshMatrix& trans);

	// *this *= f;
	void Mul(float f);
	// *this += other;
	void Add(const StdMeshVertex& other);
};

class StdMeshFace
{
public:
	unsigned int Vertices[3];
};

// Keyframe, specifies transformation for one bone in a particular frame
class StdMeshKeyFrame
{
public:
	StdMeshMatrix Trans;
};

// Animation track, specifies transformation for one bone for each keyframe
class StdMeshTrack
{
	friend class StdMesh;
public:
	StdMeshMatrix GetTransformAt(float time) const;

private:
	std::map<float, StdMeshKeyFrame> Frames;
};

// Animation, consists of one Track for each animated Bone
class StdMeshAnimation
{
	friend class StdMesh;
	friend class StdMeshInstance;
public:
	StdMeshAnimation() {}
	StdMeshAnimation(const StdMeshAnimation& other);
	~StdMeshAnimation();

	StdMeshAnimation& operator=(const StdMeshAnimation& other);

	StdCopyStrBuf Name;
	float Length;

private:
	std::vector<StdMeshTrack*> Tracks; // bone-indexed
};

struct StdMeshBox
{
	float x1, y1, z1;
	float x2, y2, z2;
};

class StdMesh
{
	friend class StdMeshInstance;
public:
	StdMesh();
	~StdMesh();

	// filename is only used to show it in error messages. The model is
	// loaded from xml_data.
	// Throws StdMeshError.
	void InitXML(const char* filename, const char* xml_data, StdMeshSkeletonLoader& skel_loader, const StdMeshMatManager& manager);

	const StdMeshVertex& GetVertex(unsigned int i) const { return Vertices[i]; }
	unsigned int GetNumVertices() const { return Vertices.size(); }

	const StdMeshFace& GetFace(unsigned int i) const { return Faces[i]; }
	unsigned int GetNumFaces() const { return Faces.size(); }

	const StdMeshBone& GetBone(unsigned int i) const { return *Bones[i]; }
	unsigned int GetNumBones() const { return Bones.size(); }

	const StdMeshAnimation* GetAnimationByName(const StdStrBuf& name) const;
	const StdMeshMaterial& GetMaterial() const { return *Material; }

	const StdMeshBox& GetBoundingBox() const { return BoundingBox; }

private:
	void AddMasterBone(StdMeshBone* bone);

	StdMesh(const StdMesh& other); // non-copyable
	StdMesh& operator=(const StdMesh& other); // non-assignable

	// Remember bone assignments for vertices
	class Vertex: public StdMeshVertex
	{
	public:
		std::vector<StdMeshVertexBoneAssignment> BoneAssignments;
	};

	std::vector<Vertex> Vertices;
	std::vector<StdMeshFace> Faces;
	std::vector<StdMeshBone*> Bones; // Master Bone Table

	std::map<StdCopyStrBuf, StdMeshAnimation> Animations;

	StdMeshBox BoundingBox;
	const StdMeshMaterial* Material;
};

class StdMeshInstance
{
protected:
	struct Animation;

public:
	StdMeshInstance(const StdMesh& mesh);

	enum FaceOrdering {
		FO_Fixed, // don't reorder, keep faces as in mesh
		FO_FarthestToNearest,
		FO_NearestToFarthest
	};

	FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
	void SetFaceOrdering(FaceOrdering ordering);

	// Public API to modify animation. Updates bone transforms on
	// destruction, so make sure to let this go out of scope before
	// relying on the values set.
	struct AnimationRef
	{
		AnimationRef(StdMeshInstance* instance, const StdStrBuf& animation_name);
		AnimationRef(StdMeshInstance* instance, const StdMeshAnimation& animation);

		~AnimationRef()
		{
			if(Changed) Instance->UpdateBoneTransforms();
		}
		
		operator void*() const { return Anim; } // for use in boolean expressions

		const StdMeshAnimation& GetAnimation() const;

		void SetPosition(float position);
		void SetWeight(float weight);
	private:
		AnimationRef(const AnimationRef&); // noncopyable
		AnimationRef& operator=(const AnimationRef&); // noncopyable

		StdMeshInstance* Instance;
		Animation* Anim;
		bool Changed;
	};

	bool PlayAnimation(const StdStrBuf& animation_name, float weight);
	bool PlayAnimation(const StdMeshAnimation& animation, float weight);
	bool StopAnimation(const StdStrBuf& animation_name);
	bool StopAnimation(const StdMeshAnimation& animation);

	// Get vertex of instance, with current animation applied. This needs to
	// go elsewhere if/when we want to calculate this on the hardware.
	const StdMeshVertex& GetVertex(unsigned int i) const { return Vertices[i]; }
	unsigned int GetNumVertices() const { return Vertices.size(); }

	// Get face of instance. The instance faces are the same as the mesh faces,
	// with the exception that they are differently ordered, depending on the
	// current FaceOrdering. See also SetFaceOrdering.
	const StdMeshFace& GetFace(unsigned int i) const { return *Faces[i]; }
	unsigned int GetNumFaces() const { return Faces.size(); }

	const StdMesh& Mesh;

protected:
	void UpdateBoneTransforms();
	void ReorderFaces();

	FaceOrdering CurrentFaceOrdering;

	struct Animation
	{
		const StdMeshAnimation* Animation;
		float Position;
		float Weight;
	};
	
	std::vector<Animation> Animations;

	std::vector<StdMeshMatrix> BoneTransforms;

	std::vector<StdMeshVertex> Vertices;
	std::vector<const StdMeshFace*> Faces;
};

#endif
