/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2010 Armin Burgmeier
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

struct StdMeshVector
{
	float x, y, z;

	static StdMeshVector Zero();
	static StdMeshVector UnitScale();
	static StdMeshVector Translate(float dx, float dy, float dz);
	static StdMeshVector Cross(const StdMeshVector& lhs, const StdMeshVector& rhs);
};

struct StdMeshVertex
{
	// Match GL_T2F_N3F_V3F
	float u, v;
	float nx, ny, nz;
	float x, y, z;

	//void Normalize();
};

struct StdMeshQuaternion
{
	float w;
	// union
	// {
	//   struct { float x, y, z; };
	//   StdMeshVector v;
	// };
	float x, y, z;

	static StdMeshQuaternion Zero();
	static StdMeshQuaternion AngleAxis(float theta, const StdMeshVector& axis);

	float LenSqr() const { return w*w+x*x+y*y+z*z; }
	void Normalize();

	static StdMeshQuaternion Nlerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w);
	//static StdMeshQuaternion Slerp(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs, float w);
};

struct StdMeshTransformation
{
	StdMeshVector scale;
	StdMeshQuaternion rotate;
	StdMeshVector translate;

	static StdMeshTransformation Zero();
	static StdMeshTransformation Identity();
	static StdMeshTransformation Inverse(const StdMeshTransformation& transform);
	static StdMeshTransformation Translate(float dx, float dy, float dz);
	static StdMeshTransformation Scale(float sx, float sy, float sz);
	static StdMeshTransformation Rotate(float angle, float rx, float ry, float rz);

	// TODO: Might add path parameter if necessary
	static StdMeshTransformation Nlerp(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs, float w);
	//static  StdMeshQuaternion Slerp(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs, float w);
};

class StdMeshMatrix
{
public:
	static StdMeshMatrix Zero();
	static StdMeshMatrix Identity();
	static StdMeshMatrix Inverse(const StdMeshMatrix& mat);
	static StdMeshMatrix Translate(float dx, float dy, float dz);
	static StdMeshMatrix Scale(float sx, float sy, float sz);
	static StdMeshMatrix Rotate(float angle, float rx, float ry, float rz);
	static StdMeshMatrix Transform(const StdMeshTransformation& transform);
	static StdMeshMatrix TransformInverse(const StdMeshTransformation& transform);

	float& operator()(int i, int j) { return a[i][j]; }
	float operator()(int i, int j) const { return a[i][j]; }

private:
	// 3x3 orthogonal + translation in last column
	float a[3][4];
};

StdMeshMatrix operator*(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(float lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(const StdMeshMatrix& lhs, float rhs);
StdMeshMatrix operator+(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshQuaternion operator-(const StdMeshQuaternion& rhs);
StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion& operator*=(StdMeshQuaternion& lhs, float rhs);
StdMeshQuaternion operator*(const StdMeshQuaternion& lhs, float rhs);
StdMeshQuaternion operator*(float lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion& operator+=(StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion operator+(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshQuaternion operator-(const StdMeshQuaternion& lhs, const StdMeshQuaternion& rhs);
StdMeshTransformation operator*(const StdMeshTransformation& lhs, const StdMeshTransformation& rhs);

StdMeshVector operator-(const StdMeshVector& rhs);
StdMeshVector& operator+=(StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator+(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator*(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector& operator*=(StdMeshVector& lhs, float rhs);
StdMeshVector operator*(const StdMeshVector& lhs, float rhs);
StdMeshVector operator*(float lhs, const StdMeshVector& rhs);
StdMeshVector operator/(const StdMeshVector& lhs, const StdMeshVector& rhs);
StdMeshVector operator/(float lhs, const StdMeshVector& rhs);
StdMeshVector operator/(const StdMeshVector& lhs, float rhs);

StdMeshVector operator*(const StdMeshMatrix& lhs, const StdMeshVector& rhs); // does not apply translation part
StdMeshVector operator*(const StdMeshQuaternion& lhs, const StdMeshVector& rhs);

StdMeshVertex& operator+=(StdMeshVertex& lhs, const StdMeshVertex& rhs);
StdMeshVertex operator+(const StdMeshVertex& lhs, const StdMeshVertex& rhs);
StdMeshVertex operator*(float lhs, const StdMeshVertex& rhs);
StdMeshVertex operator*(const StdMeshVertex& lhs, float rhs);
StdMeshVertex operator*(const StdMeshMatrix& lhs, const StdMeshVertex& rhs);

class StdMeshBone
{
	friend class StdMesh;
public:
	StdMeshBone() {}

	unsigned int Index; // Index in master bone table
	int ID; // Bone ID
	StdCopyStrBuf Name; // Bone name

	// Bone transformation
	StdMeshTransformation Transformation;
	// Inverse transformation
	StdMeshTransformation InverseTransformation;

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

class StdMeshFace
{
public:
	unsigned int Vertices[3];
};

// Keyframe, specifies transformation for one bone in a particular frame
class StdMeshKeyFrame
{
public:
	StdMeshTransformation Transformation;
};

// Animation track, specifies transformation for one bone for each keyframe
class StdMeshTrack
{
	friend class StdMesh;
public:
	StdMeshTransformation GetTransformAt(float time) const;

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

class StdSubMesh
{
	friend class StdMesh;
public:
	// Remember bone assignments for vertices
	class Vertex: public StdMeshVertex
	{
	public:
		std::vector<StdMeshVertexBoneAssignment> BoneAssignments;
	};

	const Vertex& GetVertex(unsigned int i) const { return Vertices[i]; }
	unsigned int GetNumVertices() const { return Vertices.size(); }

	const StdMeshFace& GetFace(unsigned int i) const { return Faces[i]; }
	unsigned int GetNumFaces() const { return Faces.size(); }

	const StdMeshMaterial& GetMaterial() const { return *Material; }

private:
	StdSubMesh();

	std::vector<Vertex> Vertices;
	std::vector<StdMeshFace> Faces;

	const StdMeshMaterial* Material;
};

class StdMesh
{
	//friend class StdMeshInstance;
public:
	StdMesh();
	~StdMesh();

	// filename is only used to show it in error messages. The model is
	// loaded from xml_data.
	// Throws StdMeshError.
	void InitXML(const char* filename, const char* xml_data, StdMeshSkeletonLoader& skel_loader, const StdMeshMatManager& manager);

	const StdSubMesh& GetSubMesh(unsigned int i) const { return SubMeshes[i]; }
	unsigned int GetNumSubMeshes() const { return SubMeshes.size(); }

	const StdMeshBone& GetBone(unsigned int i) const { return *Bones[i]; }
	unsigned int GetNumBones() const { return Bones.size(); }

	const StdMeshAnimation* GetAnimationByName(const StdStrBuf& name) const;

	const StdMeshBox& GetBoundingBox() const { return BoundingBox; }

private:
	void AddMasterBone(StdMeshBone* bone);

	StdMesh(const StdMesh& other); // non-copyable
	StdMesh& operator=(const StdMesh& other); // non-assignable

	std::vector<StdSubMesh> SubMeshes;
	std::vector<StdMeshBone*> Bones; // Master Bone Table

	std::map<StdCopyStrBuf, StdMeshAnimation> Animations;

	StdMeshBox BoundingBox;
};

class StdMeshInstance
{
public:
	StdMeshInstance(const StdMesh& mesh);
	~StdMeshInstance();

	enum FaceOrdering {
		FO_Fixed, // don't reorder, keep faces as in mesh
		FO_FarthestToNearest,
		FO_NearestToFarthest
	};

	FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
	void SetFaceOrdering(FaceOrdering ordering);

	// Provider for animation position or weight.
	class ValueProvider
	{
	public:
		ValueProvider(): Value(0.0f) {}
		virtual ~ValueProvider() {}

		// Return false if the corresponding node is to be removed or true
		// otherwise.
		virtual bool Execute() = 0;

		float Value; // Current provider value
	};

	// A node in the animation tree
	// Can be either a leaf node, or interpolation between two other nodes
	class AnimationNode
	{
		friend class StdMeshInstance;
	public:
		enum NodeType { LeafNode, LinearInterpolationNode };

		AnimationNode(const StdMeshAnimation* animation, ValueProvider* position);
		AnimationNode(AnimationNode* child_left, AnimationNode* child_right, ValueProvider* weight);
		~AnimationNode();

		bool GetBoneTransform(unsigned int bone, StdMeshTransformation& transformation);

		int GetSlot() const { return Slot; }
		unsigned int GetNumber() const { return Number; }
		NodeType GetType() const { return Type; }
		AnimationNode* GetParent() { return Parent; }

		const StdMeshAnimation* GetAnimation() const { assert(Type == LeafNode); return Leaf.Animation; }
		ValueProvider* GetPositionProvider() { assert(Type == LeafNode); return Leaf.Position; }
		float GetPosition() const { assert(Type == LeafNode); return Leaf.Position->Value; }

		AnimationNode* GetLeftChild() { assert(Type == LinearInterpolationNode); return LinearInterpolation.ChildLeft; }
		AnimationNode* GetRightChild() { assert(Type == LinearInterpolationNode); return LinearInterpolation.ChildRight; }
		ValueProvider* GetWeightProvider() { assert(Type == LinearInterpolationNode); return LinearInterpolation.Weight; }
		float GetWeight() const { assert(Type == LinearInterpolationNode); return LinearInterpolation.Weight->Value; }

	protected:
		int Slot;
		unsigned int Number;
		NodeType Type;
		AnimationNode* Parent;

		union
		{
			struct
			{
				const StdMeshAnimation* Animation;
				ValueProvider* Position;
			} Leaf;

			struct
			{
				AnimationNode* ChildLeft;
				AnimationNode* ChildRight;
				ValueProvider* Weight;
			} LinearInterpolation;
		};
	};

	AnimationNode* PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight);
	AnimationNode* PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight);
	void StopAnimation(AnimationNode* node);

	AnimationNode* GetAnimationNodeByNumber(unsigned int number);
	AnimationNode* GetRootAnimationForSlot(int slot);

	// Set new value providers for a node's position or weight - cannot be in
	// class AnimationNode since we need to mark BoneTransforms dirty.
	void SetAnimationPosition(AnimationNode* node, ValueProvider* position);
	void SetAnimationWeight(AnimationNode* node, ValueProvider* weight);

	// Update animations' value providers; call once a frame
	void ExecuteAnimation();

	struct AttachedMesh
	{
		unsigned int Number;
		StdMeshInstance* Parent;
		StdMeshInstance* Child;
		StdMeshMatrix AttachTrans;
		unsigned int ParentBone;
		unsigned int ChildBone;
		// Cache final transformation, updated in UpdateBoneTransforms()
		StdMeshMatrix FinalTrans;
	};

	typedef std::list<AttachedMesh> AttachedMeshList;
	typedef AttachedMeshList::const_iterator AttachedMeshIter;

	// Returns number of added attachment, or 0 on failure
	const AttachedMesh* AttachMesh(const StdMesh& mesh, const StdStrBuf& own_bone, const StdStrBuf& other_bone, const StdMeshMatrix& transformation = StdMeshMatrix::Identity());
	// Removes attachment with given number
	bool DetachMesh(unsigned int number);
	// Returns attached mesh with given number
	const AttachedMesh* GetAttachedMeshByNumber(unsigned int number) const;

	// To iterate through attachments
	AttachedMeshIter AttachedMeshesBegin() const { return AttachChildren.begin(); }
	AttachedMeshIter AttachedMeshesEnd() const { return AttachChildren.end(); }
	const AttachedMesh* GetAttachParent() const { return AttachParent; }

	// Get vertex of instance, with current animation applied. This needs to
	// go elsewhere if/when we want to calculate this on the hardware.
	//const StdMeshVertex& GetVertex(unsigned int i) const { return Vertices[i]; }
	const StdMeshVertex* GetVertices(unsigned int submesh) const { return &Vertices[submesh][0]; }
	unsigned int GetNumVertices(unsigned int submesh) const { return Vertices[submesh].size(); }

	// Get face of instance. The instance faces are the same as the mesh faces,
	// with the exception that they are differently ordered, depending on the
	// current FaceOrdering. See also SetFaceOrdering.
	//const StdMeshFace& GetFace(unsigned int i) const { return Faces[i]; }
	const StdMeshFace* GetFaces(unsigned int submesh) const { return &Faces[submesh][0]; }
	unsigned int GetNumFaces(unsigned int submesh) const { return Faces[submesh].size(); }

	const StdMeshMatrix& GetBoneTransform(unsigned int i) const { return BoneTransforms[i]; }

	// Update bone transformation matrices, and vertex positions. Call this
	// before rendering. This is called recursively for attached children. Do not
	// call this on attached children only, since it will not update the
	// attachment transform otherwise.
	void UpdateBoneTransforms();

	const StdMesh& Mesh;

protected:
	typedef std::vector<AnimationNode*> AnimationNodeList;

	AnimationNodeList::iterator GetStackIterForSlot(int slot, bool create);
	bool ExecuteAnimationNode(AnimationNode* node);
	void ReorderFaces();

	FaceOrdering CurrentFaceOrdering;

	AnimationNodeList AnimationNodes; // for simple lookup of animation nodes by their unique number
	AnimationNodeList AnimationStack; // contains top level nodes only, ordered by slot number
	std::vector<StdMeshMatrix> BoneTransforms;

	// Vertices transformed according to current animation, for each submesh
	// Faces sorted according to current face ordering, for each submesh
	// TODO: We can skip these if we decide to either
	// a) recompute Vertex positions each frame or
	// b) compute them on the GPU
	std::vector<std::vector<StdMeshVertex> > Vertices;
	std::vector<std::vector<StdMeshFace> > Faces;

	// Not asymptotically efficient, but we do not expect many attached meshes anyway.
	// In theory map would fit better, but it's probably not worth the extra overhead.
	// Don't use vector though so that pointers to AttachedMesh (AttachParent) stay valid.
	std::list<AttachedMesh> AttachChildren;
	AttachedMesh* AttachParent;

	bool BoneTransformsDirty;
private:
	StdMeshInstance(const StdMeshInstance& other); // noncopyable
	StdMeshInstance& operator=(const StdMeshInstance& other); // noncopyable
};

#endif
