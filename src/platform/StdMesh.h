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

class StdCompiler;

// OGRE mesh

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

	float Determinant() const;

private:
	// 3x3 orthogonal + translation in last column
	float a[3][4];
};

StdMeshMatrix operator*(const StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(float lhs, const StdMeshMatrix& rhs);
StdMeshMatrix operator*(const StdMeshMatrix& lhs, float rhs);
StdMeshMatrix& operator*=(StdMeshMatrix& lhs, const StdMeshMatrix& rhs);
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
	friend class StdMeshLoader;
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

	const StdMeshBone& GetChild(size_t i) const { return *Children[i]; }
	size_t GetNumChildren() const { return Children.size(); }

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
	friend class StdMeshLoader;
public:
	StdMeshTransformation GetTransformAt(float time) const;

private:
	std::map<float, StdMeshKeyFrame> Frames;
};

// Animation, consists of one Track for each animated Bone
class StdMeshAnimation
{
	friend class StdMeshLoader;
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
	friend class StdMeshLoader;
public:
	// Remember bone assignments for vertices
	class Vertex: public StdMeshVertex
	{
	public:
		std::vector<StdMeshVertexBoneAssignment> BoneAssignments;
	};

	const Vertex& GetVertex(size_t i) const { return Vertices[i]; }
	size_t GetNumVertices() const { return Vertices.size(); }

	const StdMeshFace& GetFace(size_t i) const { return Faces[i]; }
	size_t GetNumFaces() const { return Faces.size(); }

	const StdMeshMaterial& GetMaterial() const { return *Material; }

private:
	StdSubMesh();

	std::vector<Vertex> Vertices;
	std::vector<StdMeshFace> Faces;

	const StdMeshMaterial* Material;
};

class StdMesh
{
	friend class StdMeshLoader;
	StdMesh();
public:
	~StdMesh();

	const StdSubMesh& GetSubMesh(size_t i) const { return SubMeshes[i]; }
	size_t GetNumSubMeshes() const { return SubMeshes.size(); }

	const StdMeshBone& GetBone(size_t i) const { return *Bones[i]; }
	size_t GetNumBones() const { return Bones.size(); }
	const StdMeshBone* GetBoneByName(const StdStrBuf& name) const;

	const StdMeshAnimation* GetAnimationByName(const StdStrBuf& name) const;

	const StdMeshBox& GetBoundingBox() const { return BoundingBox; }
	float GetBoundingRadius() const { return BoundingRadius; }

private:
	void AddMasterBone(StdMeshBone* bone);

	StdMesh(const StdMesh& other); // non-copyable
	StdMesh& operator=(const StdMesh& other); // non-assignable

	std::vector<StdSubMesh> SubMeshes;
	std::vector<StdMeshBone*> Bones; // Master Bone Table

	std::map<StdCopyStrBuf, StdMeshAnimation> Animations;

	StdMeshBox BoundingBox;
	float BoundingRadius;
};

class StdSubMeshInstance
{
	friend class StdMeshInstance;
public:
	StdSubMeshInstance(const StdSubMesh& submesh);

	// Get vertex of instance, with current animation applied. This needs to
	// go elsewhere if/when we want to calculate this on the hardware.
	const StdMeshVertex* GetVertices() const { return &Vertices[0]; }
	size_t GetNumVertices() const { return Vertices.size(); }

	// Get face of instance. The instance faces are the same as the mesh faces,
	// with the exception that they are differently ordered, depending on the
	// current FaceOrdering. See FaceOrdering in StdMeshInstance.
	const StdMeshFace* GetFaces() const { return &Faces[0]; }
	size_t GetNumFaces() const { return Faces.size(); }

	unsigned int GetTexturePhase(size_t pass, size_t texunit) const { return PassData[pass].TexUnits[texunit].Phase; }
	double GetTexturePosition(size_t pass, size_t texunit) const { return PassData[pass].TexUnits[texunit].Position; }

	void SetMaterial(const StdMeshMaterial& material);
	const StdMeshMaterial& GetMaterial() const { return *Material; }
protected:
	// Vertices transformed according to current animation
	// Faces sorted according to current face ordering
	// TODO: We can skip these if we decide to either
	// a) recompute Vertex positions each frame or
	// b) compute them on the GPU
	std::vector<StdMeshVertex> Vertices;
	std::vector<StdMeshFace> Faces;

	const StdMeshMaterial* Material;

	struct TexUnit // Runtime texunit data
	{
		// Frame animation
		float PhaseDelay;
		unsigned int Phase;

		// Coordinate transformation animation
		// This is never reset so use double to make sure we have enough precision
		double Position;
	};

	struct Pass // Runtime pass data
	{
		std::vector<TexUnit> TexUnits;
	};

	std::vector<Pass> PassData;
private:
	StdSubMeshInstance(const StdSubMeshInstance& other); // noncopyable
	StdSubMeshInstance& operator=(const StdSubMeshInstance& other); // noncopyable
};

class StdMeshInstance
{
public:
	StdMeshInstance(const StdMesh& mesh);
	~StdMeshInstance();

	enum FaceOrdering
	{
		FO_Fixed, // don't reorder, keep faces as in mesh
		FO_FarthestToNearest,
		FO_NearestToFarthest
	};

	FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
	void SetFaceOrdering(FaceOrdering ordering);
	void SetFaceOrderingForClrModulation(uint32_t clrmod);

	// Provider for animation position or weight.
	class ValueProvider
	{
	public:
		ValueProvider(): Value(Fix0) {}
		virtual ~ValueProvider() {}

		// Return false if the corresponding node is to be removed or true
		// otherwise.
		virtual bool Execute() = 0;

		C4Real Value; // Current provider value
	};

	// Serializable value providers need to be registered with SerializeableValueProvider::Register.
	// They also need to implement a default constructor and a compile func
	class SerializableValueProvider: public ValueProvider
	{
	public:
		struct IDBase;

	private:
		// Pointer for deterministic initialization
		static std::vector<IDBase*>* IDs;

	public:
		struct IDBase
		{
			typedef SerializableValueProvider*(*NewFunc)();
		protected:
			IDBase(const char* name, const std::type_info& type, NewFunc newfunc):
				name(name), type(type), newfunc(newfunc)
			{
				if(!IDs) IDs = new std::vector<IDBase*>;
				IDs->push_back(this);
			}

		public:
			const char* name;
			const std::type_info& type;
			NewFunc newfunc;
		};

		

		template<typename T>
		struct ID: IDBase
		{
		private:
			static SerializableValueProvider* CreateFunc() { return new T; }

		public:
			ID(const char* name):
				IDBase(name, typeid(T), CreateFunc) {}
		};

		static const IDBase* Lookup(const char* name)
		{
			if(!IDs) return NULL;
			for(unsigned int i = 0; i < IDs->size(); ++i)
				if(strcmp((*IDs)[i]->name, name) == 0)
					return (*IDs)[i];
			return NULL;
		}

		static const IDBase* Lookup(const std::type_info& type)
		{
			if(!IDs) return NULL;
			for(unsigned int i = 0; i < IDs->size(); ++i)
				if((*IDs)[i]->type == type)
					return (*IDs)[i];
			return NULL;
		}

		virtual void CompileFunc(StdCompiler* pComp);
		virtual void EnumeratePointers() {}
		virtual void DenumeratePointers() {}
	};

	// A node in the animation tree
	// Can be either a leaf node, or interpolation between two other nodes
	class AnimationNode
	{
		friend class StdMeshInstance;
	public:
		enum NodeType { LeafNode, LinearInterpolationNode };

		AnimationNode();
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
		C4Real GetPosition() const { assert(Type == LeafNode); return Leaf.Position->Value; }

		AnimationNode* GetLeftChild() { assert(Type == LinearInterpolationNode); return LinearInterpolation.ChildLeft; }
		AnimationNode* GetRightChild() { assert(Type == LinearInterpolationNode); return LinearInterpolation.ChildRight; }
		ValueProvider* GetWeightProvider() { assert(Type == LinearInterpolationNode); return LinearInterpolation.Weight; }
		C4Real GetWeight() const { assert(Type == LinearInterpolationNode); return LinearInterpolation.Weight->Value; }

		void CompileFunc(StdCompiler* pComp, const StdMesh* Mesh);
		void EnumeratePointers();
		void DenumeratePointers();

	protected:
		int Slot;
		unsigned int Number;
		NodeType Type;
		AnimationNode* Parent; // NoSave

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
	// child bone transforms are dirty (saves matrix inversion for unanimated attach children).
	// Set new value providers for a node's position or weight - cannot be in
	// class AnimationNode since we need to mark BoneTransforms dirty.
	void SetAnimationPosition(AnimationNode* node, ValueProvider* position);
	void SetAnimationWeight(AnimationNode* node, ValueProvider* weight);

	// Update animations; call once a frame
	// dt is used for texture animation, skeleton animation is updated via value providers
	void ExecuteAnimation(float dt);

	enum AttachMeshFlags {
		AM_None        = 0,
		AM_DrawBefore  = 1 << 0
	};

	class AttachedMesh
	{
		friend class StdMeshInstance;
	public:
		// The job of this class is to help serialize the Child and OwnChild members of AttachedMesh
		class Denumerator
		{
		public:
			virtual ~Denumerator() {}

			virtual void CompileFunc(StdCompiler* pComp, AttachedMesh* attach) = 0;
			virtual void EnumeratePointers(AttachedMesh* attach) {}
			virtual void DenumeratePointers(AttachedMesh* attach) {}
		};

		typedef Denumerator*(*DenumeratorFactoryFunc)();

		template<typename T>
		static Denumerator* DenumeratorFactory() { return new T; }

		AttachedMesh();
		AttachedMesh(unsigned int number, StdMeshInstance* parent, StdMeshInstance* child, bool own_child, Denumerator* denumerator,
		             unsigned int parent_bone, unsigned int child_bone, const StdMeshMatrix& transform, uint32_t flags);
		~AttachedMesh();

		uint32_t Number;
		StdMeshInstance* Parent; // NoSave (set by parent)
		StdMeshInstance* Child;
		bool OwnChild; // NoSave
		Denumerator* ChildDenumerator;

		bool SetParentBone(const StdStrBuf& bone);
		bool SetChildBone(const StdStrBuf& bone);
		void SetAttachTransformation(const StdMeshMatrix& transformation);
		const StdMeshMatrix& GetFinalTransformation() const { return FinalTrans; }
		uint32_t GetFlags() const { return Flags; }

		void CompileFunc(StdCompiler* pComp, DenumeratorFactoryFunc Factory);
		void EnumeratePointers();
		void DenumeratePointers();

	private:
		unsigned int ParentBone;
		unsigned int ChildBone;
		StdMeshMatrix AttachTrans;
		uint32_t Flags;

		// Cache final attach transformation, updated in UpdateBoneTransform
		StdMeshMatrix FinalTrans; // NoSave
		bool FinalTransformDirty; // NoSave; Whether FinalTrans is up to date or not
	};

	typedef std::vector<AttachedMesh*> AttachedMeshList;
	typedef AttachedMeshList::const_iterator AttachedMeshIter;

	// Create a new instance and attach it to this mesh. Takes ownership of denumerator
	AttachedMesh* AttachMesh(const StdMesh& mesh, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation = StdMeshMatrix::Identity(), uint32_t flags = AM_None);
	// Attach an instance to this instance. Takes ownership of denumerator. If own_child is true deletes instance on detach.
	AttachedMesh* AttachMesh(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation = StdMeshMatrix::Identity(), uint32_t flags = AM_None, bool own_child = false);
	// Removes attachment with given number
	bool DetachMesh(unsigned int number);
	// Returns attached mesh with given number
	AttachedMesh* GetAttachedMeshByNumber(unsigned int number) const;

	// To iterate through attachments
	AttachedMeshIter AttachedMeshesBegin() const { return AttachChildren.begin(); }
	AttachedMeshIter AttachedMeshesEnd() const { return AttachChildren.end(); }
	AttachedMesh* GetAttachParent() const { return AttachParent; }

	size_t GetNumSubMeshes() const { return SubMeshInstances.size(); }
	StdSubMeshInstance& GetSubMesh(size_t i) { return *SubMeshInstances[i]; }
	const StdSubMeshInstance& GetSubMesh(size_t i) const { return *SubMeshInstances[i]; }

	const StdMeshMatrix& GetBoneTransform(size_t i) const { return BoneTransforms[i]; }

	// Update bone transformation matrices, vertex positions and final attach transformations of attached children.
	// This is called recursively for attached children, so there is no need to call it on attached children only
	// which would also not update its attach transformation. Call this once before rendering. Returns true if the
	// mesh was deformed since the last execution, or false otherwise.
	bool UpdateBoneTransforms();

	// Orders faces according to current face ordering. Clal this once before rendering if one of the following is true:
	//
	// a) the call to UpdateBoneTransforms returns true
	// b) a submesh's material was changed
	// c) the global transformation changed since previous call to ReorderFaces()
	// d) some other obscure state change occurred (?)
	//
	// global_trans is a global transformation that is applied when rendering the mesh, and this is used
	// to correctly do face ordering.
	//
	// TODO: Should maybe introduce a FaceOrderingDirty flag
	void ReorderFaces(StdMeshMatrix* global_trans);

	void CompileFunc(StdCompiler* pComp, AttachedMesh::DenumeratorFactoryFunc Factory);
	void EnumeratePointers();
	void DenumeratePointers();

	const StdMesh& Mesh;

protected:
	typedef std::vector<AnimationNode*> AnimationNodeList;

	AnimationNodeList::iterator GetStackIterForSlot(int slot, bool create);
	bool ExecuteAnimationNode(AnimationNode* node);

	FaceOrdering CurrentFaceOrdering;

	AnimationNodeList AnimationNodes; // for simple lookup of animation nodes by their unique number
	AnimationNodeList AnimationStack; // contains top level nodes only, ordered by slot number
	std::vector<StdMeshMatrix> BoneTransforms;

	std::vector<StdSubMeshInstance*> SubMeshInstances;

	// Not asymptotically efficient, but we do not expect many attached meshes anyway.
	// In theory map would fit better, but it's probably not worth the extra overhead.
	std::vector<AttachedMesh*> AttachChildren;
	AttachedMesh* AttachParent;

	bool BoneTransformsDirty;
private:
	StdMeshInstance(const StdMeshInstance& other); // noncopyable
	StdMeshInstance& operator=(const StdMeshInstance& other); // noncopyable
};

inline void CompileNewFuncCtx(StdMeshInstance::SerializableValueProvider *&pStruct, StdCompiler *pComp, const StdMeshInstance::SerializableValueProvider::IDBase& rID)
{
	std::auto_ptr<StdMeshInstance::SerializableValueProvider> temp(rID.newfunc());
	pComp->Value(*temp);
	pStruct = temp.release();
}

#endif
