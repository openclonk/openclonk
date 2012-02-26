/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006, 2010  Sven Eberhardt
 * Copyright (c) 2009-2011  Armin Burgmeier
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Nicolas Hake
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

#include <StdMeshMath.h>
#include <StdMeshMaterial.h>

class StdCompiler;

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
	friend class StdMesh;
	friend class StdMeshLoader;
public:
	StdMeshTransformation GetTransformAt(float time) const;

private:
	std::map<float, StdMeshKeyFrame> Frames;
};

// Animation, consists of one Track for each animated Bone
class StdMeshAnimation
{
	friend class StdMesh;
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
	friend class StdMeshMaterialUpdate;
public:
	// Remember bone assignments for vertices
	class Vertex: public StdMeshVertex
	{
	public:
		std::vector<StdMeshVertexBoneAssignment> BoneAssignments;
	};

	const std::vector<Vertex>& GetVertices() const { return Vertices; }
	const Vertex& GetVertex(size_t i) const { return Vertices[i]; }
	size_t GetNumVertices() const { return Vertices.size(); }

	const StdMeshFace& GetFace(size_t i) const { return Faces[i]; }
	size_t GetNumFaces() const { return Faces.size(); }

	const StdMeshMaterial& GetMaterial() const { return *Material; }

private:
	StdSubMesh();

	std::vector<Vertex> Vertices; // Empty if we use shared vertices
	std::vector<StdMeshFace> Faces;

	const StdMeshMaterial* Material;
};

class StdMesh
{
	friend class StdMeshLoader;
	friend class StdMeshMaterialUpdate;
	friend class StdMeshUpdate;

	StdMesh();
public:
	~StdMesh();

	typedef StdSubMesh::Vertex Vertex;

	const StdSubMesh& GetSubMesh(size_t i) const { return SubMeshes[i]; }
	size_t GetNumSubMeshes() const { return SubMeshes.size(); }

	const std::vector<Vertex>& GetSharedVertices() const { return SharedVertices; }

	const StdMeshBone& GetBone(size_t i) const { return *Bones[i]; }
	size_t GetNumBones() const { return Bones.size(); }
	const StdMeshBone* GetBoneByName(const StdStrBuf& name) const;

	const StdMeshAnimation* GetAnimationByName(const StdStrBuf& name) const;

	const StdMeshBox& GetBoundingBox() const { return BoundingBox; }
	float GetBoundingRadius() const { return BoundingRadius; }

	// TODO: This code should maybe better be placed in StdMeshLoader...
	void MirrorAnimation(const StdStrBuf& name, const StdMeshAnimation& animation);
	void PostInit();

private:
	void AddMasterBone(StdMeshBone* bone);

	StdMesh(const StdMesh& other); // non-copyable
	StdMesh& operator=(const StdMesh& other); // non-assignable

	std::vector<Vertex> SharedVertices;

	std::vector<StdSubMesh> SubMeshes;
	std::vector<StdMeshBone*> Bones; // Master Bone Table

	std::map<StdCopyStrBuf, StdMeshAnimation> Animations;

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

	// Get vertex of instance, with current animation applied. This needs to
	// go elsewhere if/when we want to calculate this on the hardware.
	const std::vector<StdMeshVertex>& GetVertices() const { return Vertices; }
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

	FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
protected:
	void SetFaceOrdering(const StdSubMesh& submesh, FaceOrdering ordering);
	void SetFaceOrderingForClrModulation(const StdSubMesh& submesh, uint32_t clrmod);

	// Vertices transformed according to current animation
	// Faces sorted according to current face ordering
	// TODO: We can skip these if we decide to either
	// a) recompute Vertex positions each frame or
	// b) compute them on the GPU
	std::vector<StdMeshVertex> Vertices;
	std::vector<StdMeshFace> Faces; // TODO: Indices could also be stored on GPU in a vbo (element index array). Should be done in a next step if at all.

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
	// TODO: GLuint vbo; // NoSave, replacing vertices list -- can be mapped into memory for writing. Should be moved to StdMesh once we apply skeletal transformation on the GPU.

private:
	StdSubMeshInstance(const StdSubMeshInstance& other); // noncopyable
	StdSubMeshInstance& operator=(const StdSubMeshInstance& other); // noncopyable
};

class StdMeshInstance
{
	friend class StdMeshMaterialUpdate;
	friend class StdMeshUpdate;
public:
	StdMeshInstance(const StdMesh& mesh, float completion = 1.0f);
	~StdMeshInstance();

	typedef StdSubMeshInstance::FaceOrdering FaceOrdering;

	enum AttachMeshFlags {
		AM_None        = 0,
		AM_DrawBefore  = 1 << 0
	};

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

			virtual ~IDBase()
			{
				assert(IDs);
				IDs->erase(std::find(IDs->begin(), IDs->end(), this));
				if (!IDs->size()) { delete IDs; IDs = NULL; }
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
		virtual void DenumeratePointers() {}
	};

	// A node in the animation tree
	// Can be either a leaf node, or interpolation between two other nodes
	class AnimationNode
	{
		friend class StdMeshInstance;
		friend class StdMeshUpdate;
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

	//FaceOrdering GetFaceOrdering() const { return CurrentFaceOrdering; }
	void SetFaceOrdering(FaceOrdering ordering);
	void SetFaceOrderingForClrModulation(uint32_t clrmod);

	const std::vector<StdMeshVertex>& GetSharedVertices() const { return SharedVertices; }
	size_t GetNumSharedVertices() const { return SharedVertices.size(); }

	// Set completion of the mesh. For incompleted meshes not all faces will be available.
	void SetCompletion(float completion);
	float GetCompletion() const { return Completion; }

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
	void DenumeratePointers();

	const StdMesh& GetMesh() const { assert(Mesh != NULL); return *Mesh; }

protected:
	typedef std::vector<AnimationNode*> AnimationNodeList;

	AnimationNodeList::iterator GetStackIterForSlot(int slot, bool create);
	bool ExecuteAnimationNode(AnimationNode* node);
	void ApplyBoneTransformToVertices(const std::vector<StdSubMesh::Vertex>& mesh_vertices, std::vector<StdMeshVertex>& instance_vertices);

	const StdMesh* Mesh;

	float Completion; // NoSave

	std::vector<StdMeshVertex> SharedVertices;

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
