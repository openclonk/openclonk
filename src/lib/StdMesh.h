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

#ifndef INC_StdMesh
#define INC_StdMesh

#include "C4ForbidLibraryCompilation.h"
#include "lib/StdMeshMath.h"
#include "lib/StdMeshMaterial.h"

#include <string>

class StdMeshBone
{
	friend class StdMeshSkeleton;
	friend class StdMeshSkeletonLoader;
	friend class StdMeshXML;
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

private:
	StdMeshBone* Parent; // Parent bone
	std::vector<StdMeshBone*> Children; // Children. Not owned.

	StdMeshBone(const StdMeshBone&) = delete;
	StdMeshBone& operator=(const StdMeshBone&) = delete;
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
	friend class StdMeshSkeleton;
	friend class StdMeshSkeletonLoader;
public:
	StdMeshTransformation GetTransformAt(float time, float length) const;

private:
	std::map<float, StdMeshKeyFrame> Frames;
};

// Animation, consists of one Track for each animated Bone
class StdMeshAnimation
{
	friend class StdMeshSkeleton;
	friend class StdMeshSkeletonLoader;
	friend class StdMeshInstance;
	friend class StdMeshInstanceAnimationNode;
public:
	StdMeshAnimation() {}
	StdMeshAnimation(const StdMeshAnimation& other);
	~StdMeshAnimation();

	StdMeshAnimation& operator=(const StdMeshAnimation& other);

	StdCopyStrBuf Name;
	float Length;

private:
	std::vector<StdMeshTrack*> Tracks; // bone-indexed
	const class StdMeshSkeleton* OriginSkeleton; // saves, where the animation came from
};

class StdMeshSkeleton
{
	friend class StdMeshSkeletonLoader;
	friend class StdMeshXML;
	friend class StdMesh;
	friend class StdMeshAnimationUpdate;

	StdMeshSkeleton();
public:
	~StdMeshSkeleton();

	const StdMeshBone& GetBone(size_t i) const { return *Bones[i]; }
	size_t GetNumBones() const { return Bones.size(); }
	const StdMeshBone* GetBoneByName(const StdStrBuf& name) const;

	const StdMeshAnimation* GetAnimationByName(const StdStrBuf& name) const;
	bool IsAnimated() const { return !Animations.empty(); }

	// TODO: This code should maybe better be placed in StdMeshLoader...
	void MirrorAnimation(const StdMeshAnimation& animation);
	void InsertAnimation(const StdMeshAnimation& animation);
	void InsertAnimation(const StdMeshSkeleton& source, const StdMeshAnimation& animation);
	void PostInit();

	std::vector<int> GetMatchingBones(const StdMeshSkeleton& skeleton) const;

	std::vector<const StdMeshAnimation*> GetAnimations() const;

private:
	void AddMasterBone(StdMeshBone* bone);

	StdMeshSkeleton(const StdMeshSkeleton& other) = delete;
	StdMeshSkeleton& operator=(const StdMeshSkeleton& other) = delete;

	std::vector<StdMeshBone*> Bones; // Master Bone Table

	std::map<StdCopyStrBuf, StdMeshAnimation> Animations;
};

struct StdMeshBox
{
	float x1, y1, z1;
	float x2, y2, z2;

	StdMeshVector GetCenter() const
	{
		return StdMeshVector{ (x2 + x1) / 2.0f, (y2 + y1) / 2.0f, (z2 + z1) / 2.0f };
	}
};

class StdSubMesh
{
	friend class StdMesh;
	friend class StdMeshLoader;
	friend class StdMeshMaterialUpdate;
public:
	typedef StdMeshVertex Vertex;

	const std::vector<Vertex>& GetVertices() const { return Vertices; }
	const Vertex& GetVertex(size_t i) const { return Vertices[i]; }
	size_t GetNumVertices() const { return Vertices.size(); }

	const StdMeshFace& GetFace(size_t i) const { return Faces[i]; }
	size_t GetNumFaces() const { return Faces.size(); }

	const StdMeshMaterial& GetMaterial() const { return *Material; }

	// Return the offset into the backing vertex buffer where this SubMesh's data starts
	size_t GetOffsetInVBO() const { return vertex_buffer_offset; }
	size_t GetOffsetInIBO() const { return index_buffer_offset; }

private:
	StdSubMesh();

	std::vector<Vertex> Vertices; // Empty if we use shared vertices
	std::vector<StdMeshFace> Faces;
	size_t vertex_buffer_offset;
	size_t index_buffer_offset;

	const StdMeshMaterial* Material;
};

class StdMesh
{
	friend class StdMeshLoader;
	friend class StdMeshMaterialUpdate;

	StdMesh();
public:
	~StdMesh();

	typedef StdSubMesh::Vertex Vertex;

	const StdSubMesh& GetSubMesh(size_t i) const { return SubMeshes[i]; }
	size_t GetNumSubMeshes() const { return SubMeshes.size(); }

	const std::vector<Vertex>& GetSharedVertices() const { return SharedVertices; }

	const StdMeshSkeleton& GetSkeleton() const { return *Skeleton; }
	StdMeshSkeleton& GetSkeleton() { return *Skeleton; }

	const StdMeshBox& GetBoundingBox() const { return BoundingBox; }
	float GetBoundingRadius() const { return BoundingRadius; }

	void PostInit();

#ifndef USE_CONSOLE
	GLuint GetVBO() const { return vbo; }
	GLuint GetIBO() const { return ibo; }
	unsigned int GetVAOID() const { return vaoid; }
#endif

	void SetLabel(const std::string &label) { Label = label; }

private:
#ifndef USE_CONSOLE
	GLuint vbo;
	GLuint ibo;
	unsigned int vaoid;
	void UpdateVBO();
	void UpdateIBO();
#endif

	StdMesh(const StdMesh& other) = delete;
	StdMesh& operator=(const StdMesh& other) = delete;

	std::vector<Vertex> SharedVertices;

	std::vector<StdSubMesh> SubMeshes;
	std::shared_ptr<StdMeshSkeleton> Skeleton; // Skeleton

	std::string Label;

	StdMeshBox BoundingBox;
	float BoundingRadius;
};

class StdSubMeshInstance
{
	friend class StdMeshInstance;
	friend class StdMeshMaterialUpdate;
public:

	enum FaceOrdering
	{
		FO_Fixed, // don't reorder, keep faces as in mesh
		FO_FarthestToNearest,
		FO_NearestToFarthest
	};

	StdSubMeshInstance(class StdMeshInstance& instance, const StdSubMesh& submesh, float completion);
	void LoadFacesForCompletion(class StdMeshInstance& instance, const StdSubMesh& submesh, float completion);

	void CompileFunc(StdCompiler* pComp);

	// Get face of instance. The instance faces are the same as the mesh faces,
	// with the exception that they are differently ordered, depending on the
	// current FaceOrdering. See FaceOrdering in StdMeshInstance.
	const StdMeshFace* GetFaces() const { return Faces.size() > 0 ? &Faces[0] : 0; }
	size_t GetNumFaces() const { return Faces.size(); }
	const StdSubMesh &GetSubMesh() const { return *base; }

	unsigned int GetTexturePhase(size_t pass, size_t texunit) const { return PassData[pass].TexUnits[texunit].Phase; }
	double GetTexturePosition(size_t pass, size_t texunit) const { return PassData[pass].TexUnits[texunit].Position; }

	const StdMeshMaterial& GetMaterial() const { return *Material; }

	FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
protected:
	void SetMaterial(const StdMeshMaterial& material);
	void SetFaceOrdering(class StdMeshInstance& instance, const StdSubMesh& submesh, FaceOrdering ordering);
	void SetFaceOrderingForClrModulation(class StdMeshInstance& instance, const StdSubMesh& submesh, uint32_t clrmod);

	const StdSubMesh *base;
	// Faces sorted according to current face ordering
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
	FaceOrdering CurrentFaceOrdering; // NoSave

	// TODO: GLuint texenv_list; // NoSave, texture environment setup could be stored in a display list (w/ and w/o ClrMod). What about PlayerColor?

private:
	StdSubMeshInstance(const StdSubMeshInstance& other) = delete;
	StdSubMeshInstance& operator=(const StdSubMeshInstance& other) = delete;
};


// Provider for animation position or weight.
class StdMeshInstanceValueProvider
{
public:
	StdMeshInstanceValueProvider(): Value(Fix0) {}
	virtual ~StdMeshInstanceValueProvider() {}

	// Return false if the corresponding node is to be removed or true
	// otherwise.
	virtual bool Execute() = 0;

	C4Real Value; // Current provider value
};

// A node in the animation tree
// Can be either a leaf node, or interpolation between two other nodes
class StdMeshInstanceAnimationNode
{
	friend class StdMeshInstance;
	friend class StdMeshUpdate;
	friend class StdMeshAnimationUpdate;
public:
	typedef StdMeshInstanceAnimationNode AnimationNode;
	typedef StdMeshInstanceValueProvider ValueProvider;
	enum NodeType { LeafNode, CustomNode, LinearInterpolationNode };

	StdMeshInstanceAnimationNode();
	StdMeshInstanceAnimationNode(const StdMeshAnimation* animation, ValueProvider* position);
	StdMeshInstanceAnimationNode(const StdMeshBone* bone, const StdMeshTransformation& trans);
	StdMeshInstanceAnimationNode(AnimationNode* child_left, AnimationNode* child_right, ValueProvider* weight);
	~StdMeshInstanceAnimationNode();

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

	void CompileFunc(StdCompiler* pComp, const StdMesh *Mesh);
	void DenumeratePointers();
	void ClearPointers(class C4Object* pObj);

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
			unsigned int BoneIndex;
			StdMeshTransformation* Transformation;
		} Custom;

		struct
		{
			AnimationNode* ChildLeft;
			AnimationNode* ChildRight;
			ValueProvider* Weight;
		} LinearInterpolation;
	};
};


class StdMeshInstance
{
	friend class StdMeshMaterialUpdate;
	friend class StdMeshAnimationUpdate;
	friend class StdMeshUpdate;
public:
	typedef StdMeshInstanceAnimationNode AnimationNode;
	StdMeshInstance(const StdMesh& mesh, float completion = 1.0f);
	~StdMeshInstance();

	typedef StdSubMeshInstance::FaceOrdering FaceOrdering;

	enum AttachMeshFlags {
		AM_None          = 0,
		AM_DrawBefore    = 1 << 0,
		AM_MatchSkeleton = 1 << 1
	};

	typedef StdMeshInstanceValueProvider ValueProvider;

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

			virtual ~IDBase()
			{
				assert(IDs);
				IDs->erase(std::find(IDs->begin(), IDs->end(), this));
				if (!IDs->size()) { delete IDs; IDs = nullptr; }
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
			if(!IDs) return nullptr;
			for(unsigned int i = 0; i < IDs->size(); ++i)
				if(strcmp((*IDs)[i]->name, name) == 0)
					return (*IDs)[i];
			return nullptr;
		}

		static const IDBase* Lookup(const std::type_info& type)
		{
			if(!IDs) return nullptr;
			for(unsigned int i = 0; i < IDs->size(); ++i)
				if((*IDs)[i]->type == type)
					return (*IDs)[i];
			return nullptr;
		}

		virtual void CompileFunc(StdCompiler* pComp);
		virtual void DenumeratePointers() {}
		virtual void ClearPointers(class C4Object* pObj) {}
	};

	class AttachedMesh
	{
		friend class StdMeshInstance;
		friend class StdMeshUpdate;
	public:
		// The job of this class is to help serialize the Child and OwnChild members of AttachedMesh
		class Denumerator
		{
		public:
			virtual ~Denumerator() {}

			virtual void CompileFunc(StdCompiler* pComp, AttachedMesh* attach) = 0;
			virtual void DenumeratePointers(AttachedMesh* attach) {}
			virtual bool ClearPointers(class C4Object* pObj) { return true; }
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
		void DenumeratePointers();
		bool ClearPointers(class C4Object* pObj);

		unsigned int GetParentBone() const { return ParentBone; }
		unsigned int GetChildBone() const { return ChildBone; }

	private:
		unsigned int ParentBone;
		unsigned int ChildBone;
		StdMeshMatrix AttachTrans;
		uint32_t Flags;

		// Cache final attach transformation, updated in UpdateBoneTransform
		StdMeshMatrix FinalTrans; // NoSave
		bool FinalTransformDirty; // NoSave; Whether FinalTrans is up to date or not

		std::vector<int> MatchedBoneInParentSkeleton; // Only filled if AM_MatchSkeleton is set

		void MapBonesOfChildToParent(const StdMeshSkeleton& parent_skeleton, const StdMeshSkeleton& child_skeleton);
	};

	typedef std::vector<AttachedMesh*> AttachedMeshList;
	typedef AttachedMeshList::const_iterator AttachedMeshIter;

	void SetFaceOrdering(FaceOrdering ordering);
	void SetFaceOrderingForClrModulation(uint32_t clrmod);

	const std::vector<StdMeshVertex>& GetSharedVertices() const { return Mesh->GetSharedVertices(); }
	size_t GetNumSharedVertices() const { return GetSharedVertices().size(); }

	// Set completion of the mesh. For incompleted meshes not all faces will be available.
	void SetCompletion(float completion);
	float GetCompletion() const { return Completion; }

	AnimationNode* PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight, bool stop_previous_animation);
	AnimationNode* PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight, bool stop_previous_animation);
	AnimationNode* PlayAnimation(const StdMeshBone* bone, const StdMeshTransformation& trans, int slot, AnimationNode* sibling, ValueProvider* weight, bool stop_previous_animation);
	void StopAnimation(AnimationNode* node);

	AnimationNode* GetAnimationNodeByNumber(unsigned int number);
	AnimationNode* GetRootAnimationForSlot(int slot);
	// child bone transforms are dirty (saves matrix inversion for unanimated attach children).
	// Set new value providers for a node's position or weight - cannot be in
	// class AnimationNode since we need to mark BoneTransforms dirty.
	void SetAnimationPosition(AnimationNode* node, ValueProvider* position);
	void SetAnimationBoneTransform(AnimationNode* node, const StdMeshTransformation& trans);
	void SetAnimationWeight(AnimationNode* node, ValueProvider* weight);

	// Update animations; call once a frame
	// dt is used for texture animation, skeleton animation is updated via value providers
	void ExecuteAnimation(float dt);

	// Create a new instance and attach it to this mesh. Takes ownership of denumerator
	AttachedMesh* AttachMesh(const StdMesh& mesh, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation = StdMeshMatrix::Identity(), uint32_t flags = AM_None, unsigned int attach_number = 0);
	// Attach an instance to this instance. Takes ownership of denumerator. If own_child is true deletes instance on detach.
	AttachedMesh* AttachMesh(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation = StdMeshMatrix::Identity(), uint32_t flags = AM_None, bool own_child = false, unsigned int attach_number = 0);
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
	const StdSubMeshInstance& GetSubMeshOrdered(size_t i) const { return *SubMeshInstancesOrdered[i]; }

	// Set material of submesh i.
	void SetMaterial(size_t i, const StdMeshMaterial& material);

	const StdMeshMatrix& GetBoneTransform(size_t i) const;
	size_t GetBoneCount() const;

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
	void DenumeratePointers();
	void ClearPointers(class C4Object* pObj);

	const StdMesh& GetMesh() const { return *Mesh; }

#ifndef USE_CONSOLE
	GLuint GetIBO() const { return ibo ? ibo : Mesh->GetIBO(); }
	unsigned int GetVAOID() const { return vaoid ? vaoid : Mesh->GetVAOID(); }
#endif

protected:
#ifndef USE_CONSOLE
	void UpdateIBO();
#endif

	AttachedMesh* AttachMeshImpl(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags, bool own_child, unsigned int new_attach_number);

	template<typename IteratorType, typename FuncObj>
	static bool ScanAttachTree(IteratorType begin, IteratorType end, const FuncObj& obj);

	typedef std::vector<AnimationNode*> AnimationNodeList;

	AnimationNodeList::iterator GetStackIterForSlot(int slot, bool create);
	void InsertAnimationNode(AnimationNode* node, int slot, AnimationNode* sibling, ValueProvider* weight, bool stop_previous_animation);
	bool ExecuteAnimationNode(AnimationNode* node);
	void ApplyBoneTransformToVertices(const std::vector<StdSubMesh::Vertex>& mesh_vertices, std::vector<StdMeshVertex>& instance_vertices);
	void SetBoneTransformsDirty(bool value);

	const StdMesh *Mesh;

	float Completion; // NoSave

	AnimationNodeList AnimationNodes; // for simple lookup of animation nodes by their unique number
	AnimationNodeList AnimationStack; // contains top level nodes only, ordered by slot number
	std::vector<StdMeshMatrix> BoneTransforms;

	std::vector<StdSubMeshInstance*> SubMeshInstances;
	std::vector<StdSubMeshInstance*> SubMeshInstancesOrdered; // ordered by opacity, in case materials were changed

	// Not asymptotically efficient, but we do not expect many attached meshes anyway.
	// In theory map would fit better, but it's probably not worth the extra overhead.
	std::vector<AttachedMesh*> AttachChildren;
	AttachedMesh* AttachParent;

	bool BoneTransformsDirty;

#ifndef USE_CONSOLE
	// private instance index buffer, and a VAO that is bound to it
	// instead of the mesh's. We use a private IBO when we use custom
	// face ordering. Otherwise, when we use the default face ordering,
	// these members are 0 and we use the mesh's IBO and VAO instead.
	GLuint ibo;
	unsigned int vaoid;
#endif
private:
	StdMeshInstance(const StdMeshInstance& other) = delete;
	StdMeshInstance& operator=(const StdMeshInstance& other) = delete;
};

inline void CompileNewFuncCtx(StdMeshInstance::SerializableValueProvider *&pStruct, StdCompiler *pComp, const StdMeshInstance::SerializableValueProvider::IDBase& rID)
{
	std::unique_ptr<StdMeshInstance::SerializableValueProvider> temp(rID.newfunc());
	pComp->Value(*temp);
	pStruct = temp.release();
}

#endif
