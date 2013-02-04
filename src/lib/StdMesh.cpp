/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2011  Armin Burgmeier
 * Copyright (c) 2009  Mark Haßelbusch
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

#include "C4Include.h"
#include <StdMesh.h>
#include <algorithm>

static int StdMeshFaceCmp(const StdMeshFace& face1, const StdMeshFace& face2);

#define SORT_NAME StdMesh
#define SORT_TYPE StdMeshFace
#define SORT_CMP StdMeshFaceCmp
#include "timsort/sort.h"

std::vector<StdMeshInstance::SerializableValueProvider::IDBase*>* StdMeshInstance::SerializableValueProvider::IDs = NULL;

namespace
{
	// Helper to sort submeshes so that opaque ones appear before non-opaque ones
	struct StdMeshSubMeshVisibilityCmpPred
	{
		bool operator()(const StdSubMesh& first, const StdSubMesh& second)
		{
			return first.GetMaterial().IsOpaque() > second.GetMaterial().IsOpaque();
		}
	};

	// Helper to sort faces for FaceOrdering
	struct StdMeshInstanceFaceOrderingCmpPred
	{
		const StdMeshVertex* m_vertices;
		StdSubMeshInstance::FaceOrdering m_face_ordering;
		const StdMeshMatrix& m_global_trans;

		StdMeshInstanceFaceOrderingCmpPred(const StdMeshInstance& mesh_inst, const StdSubMeshInstance& sub_inst,
		                                   StdSubMeshInstance::FaceOrdering face_ordering, const StdMeshMatrix& global_trans):
				m_face_ordering(face_ordering), m_global_trans(global_trans)
		{
			if(sub_inst.GetNumVertices() > 0)
				m_vertices = &sub_inst.GetVertices()[0];
			else
				m_vertices = &mesh_inst.GetSharedVertices()[0];
		}

		inline float get_z(const StdMeshVertex& vtx) const
		{
			// We need to evaluate the Z coordinate of the transformed vertex
			// (for all three vertices of the two faces), something like
			// float z11 = (m_global_trans*m_vertices[face1.Vertices[0]]).z;
			// However we don't do the full matrix multiplication as we are
			// only interested in the Z coordinate of the result, also we are
			// not interested in the resulting normals.
			return m_global_trans(2,0)*vtx.x + m_global_trans(2,1)*vtx.y + m_global_trans(2,2)*vtx.z + m_global_trans(2,3);
		}

		bool operator()(const StdMeshFace& face1, const StdMeshFace& face2) const
		{
			return compare(face1, face2) < 0;
		}

		int compare(const StdMeshFace& face1, const StdMeshFace& face2) const
		{
			// TODO: Need to apply attach matrix in case of attached meshes
			switch (m_face_ordering)
			{
			case StdSubMeshInstance::FO_Fixed:
				assert(false);
				return 0;
			case StdSubMeshInstance::FO_FarthestToNearest:
			case StdSubMeshInstance::FO_NearestToFarthest:
			{
				float z11 = get_z(m_vertices[face1.Vertices[0]]);
				float z12 = get_z(m_vertices[face1.Vertices[1]]);
				float z13 = get_z(m_vertices[face1.Vertices[2]]);
				float z21 = get_z(m_vertices[face2.Vertices[0]]);
				float z22 = get_z(m_vertices[face2.Vertices[1]]);
				float z23 = get_z(m_vertices[face2.Vertices[2]]);

				float z1 = std::max(std::max(z11, z12), z13);
				float z2 = std::max(std::max(z21, z22), z23);

				if (m_face_ordering == StdSubMeshInstance::FO_FarthestToNearest)
					return (z1 < z2 ? -1 : (z1 > z2 ? +1 : 0));
				else
					return (z2 < z1 ? -1 : (z2 > z1 ? +1 : 0));
			}
			default:
				assert(false);
				return 0;
			}
		}
	};

	// Serialize a ValueProvider with StdCompiler
	struct ValueProviderAdapt
	{
		ValueProviderAdapt(StdMeshInstance::ValueProvider** Provider):
				ValueProvider(Provider) {}

		StdMeshInstance::ValueProvider** ValueProvider;

		void CompileFunc(StdCompiler* pComp)
		{
			const StdMeshInstance::SerializableValueProvider::IDBase* id;
			StdMeshInstance::SerializableValueProvider* svp = NULL;

			if(pComp->isCompiler())
			{
				StdCopyStrBuf id_str;
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));

				id = StdMeshInstance::SerializableValueProvider::Lookup(id_str.getData());
				if(!id) pComp->excCorrupt("No value provider for ID \"%s\"", id_str.getData());
			}
			else
			{
				svp = dynamic_cast<StdMeshInstance::SerializableValueProvider*>(*ValueProvider);
				if(!svp) pComp->excCorrupt("Value provider cannot be compiled");
				id = StdMeshInstance::SerializableValueProvider::Lookup(typeid(*svp));
				if(!id) pComp->excCorrupt("No ID for value provider registered");

				StdCopyStrBuf id_str(id->name);
				pComp->Value(mkParAdapt(id_str, StdCompiler::RCT_Idtf));
			}

			pComp->Separator(StdCompiler::SEP_START);
			pComp->Value(mkContextPtrAdapt(svp, *id, false));
			pComp->Separator(StdCompiler::SEP_END);

			if(pComp->isCompiler())
				*ValueProvider = svp;
		}

		ALLOW_TEMP_TO_REF(ValueProviderAdapt)
	};

	ValueProviderAdapt mkValueProviderAdapt(StdMeshInstance::ValueProvider** ValueProvider) { return ValueProviderAdapt(ValueProvider); }

	// Serialize a bone index by name with StdCompiler
	struct TransformAdapt
	{
		StdMeshMatrix& Matrix;
		TransformAdapt(StdMeshMatrix& matrix): Matrix(matrix) {}

		void CompileFunc(StdCompiler* pComp)
		{
			pComp->Separator(StdCompiler::SEP_START);
			for(unsigned int i = 0; i < 3; ++i)
			{
				for(unsigned int j = 0; j < 4; ++j)
				{
					if(i != 0 || j != 0) pComp->Separator();
					// TODO: Teach StdCompiler how to handle float
//					pComp->Value(Matrix(i, j));
					
					if(pComp->isCompiler())
					{
						C4Real f;
						pComp->Value(f);
						Matrix(i,j) = fixtof(f);
					}
					else
					{
						C4Real f = ftofix(Matrix(i,j));
						pComp->Value(f);
					}
				}
			}

			pComp->Separator(StdCompiler::SEP_END);
		}

		ALLOW_TEMP_TO_REF(TransformAdapt)
	};
	
	TransformAdapt mkTransformAdapt(StdMeshMatrix& Matrix) { return TransformAdapt(Matrix); }

	// Reset all animation list entries corresponding to node or its children
	void ClearAnimationListRecursively(std::vector<StdMeshInstance::AnimationNode*>& list, StdMeshInstance::AnimationNode* node)
	{
		list[node->GetNumber()] = NULL;

		if (node->GetType() == StdMeshInstance::AnimationNode::LinearInterpolationNode)
		{
			ClearAnimationListRecursively(list, node->GetLeftChild());
			ClearAnimationListRecursively(list, node->GetRightChild());
		}
	}

	// Mirror is wrt X axis
	void MirrorKeyFrame(StdMeshKeyFrame& frame, const StdMeshTransformation& old_bone_transformation, const StdMeshTransformation& new_inverse_bone_transformation)
	{
		// frame was a keyframe of a track for old_bone and was now transplanted to new_bone.
		frame.Transformation.rotate.y = -frame.Transformation.rotate.y;
		frame.Transformation.rotate.z = -frame.Transformation.rotate.z;

		StdMeshVector d = old_bone_transformation.scale * (old_bone_transformation.rotate * frame.Transformation.translate);
		d.x = -d.x;
		frame.Transformation.translate = new_inverse_bone_transformation.rotate * (new_inverse_bone_transformation.scale * d);

		// TODO: scale
	}

	bool MirrorName(StdStrBuf& buf)
	{
		unsigned int len = buf.getLength();

		if(buf.Compare_(".R", len-2) == 0)
			buf.getMData()[len-1] = 'L';
		else if(buf.Compare_(".L", len-2) == 0)
			buf.getMData()[len-1] = 'R';
		else
			return false;

		return true;
	}

	StdMeshInstanceFaceOrderingCmpPred* g_pred = NULL;
}

static int StdMeshFaceCmp(const StdMeshFace& face1, const StdMeshFace& face2)
{
	return g_pred->compare(face1, face2);
}

StdMeshTransformation StdMeshTrack::GetTransformAt(float time) const
{
	std::map<float, StdMeshKeyFrame>::const_iterator iter = Frames.lower_bound(time);

	// If this points to end(), then either
	// a) time > animation length
	// b) The track does not include a frame for the very end of the animation
	// Both is considered an error
	assert(iter != Frames.end());

	if (iter == Frames.begin())
		return iter->second.Transformation;

	std::map<float, StdMeshKeyFrame>::const_iterator prev_iter = iter;
	-- prev_iter;

	float dt = iter->first - prev_iter->first;
	float weight1 = (time - prev_iter->first) / dt;
	float weight2 = (iter->first - time) / dt;

	assert(weight1 >= 0 && weight2 >= 0 && weight1 <= 1 && weight2 <= 1);
	assert(fabs(weight1 + weight2 - 1) < 1e-6);

	/*StdMeshTransformation transformation;
	transformation.scale = weight1 * iter->second.Transformation.scale + weight2 * prev_iter->second.Transformation.scale;
	transformation.rotate = weight1 * iter->second.Transformation.rotate + weight2 * prev_iter->second.Transformation.rotate; // TODO: slerp or renormalize
	transformation.translate = weight1 * iter->second.Transformation.translate + weight2 * prev_iter->second.Transformation.translate;
	return transformation;*/
	return StdMeshTransformation::Nlerp(prev_iter->second.Transformation, iter->second.Transformation, weight1);
}

StdMeshAnimation::StdMeshAnimation(const StdMeshAnimation& other):
		Name(other.Name), Length(other.Length), Tracks(other.Tracks.size())
{
	// Note that all Tracks are already default-initialized to zero
	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);
}

StdMeshAnimation::~StdMeshAnimation()
{
	for (unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];
}

StdMeshAnimation& StdMeshAnimation::operator=(const StdMeshAnimation& other)
{
	if (this == &other) return *this;

	Name = other.Name;
	Length = other.Length;

	for (unsigned int i = 0; i < Tracks.size(); ++i)
		delete Tracks[i];

	Tracks.resize(other.Tracks.size());

	for (unsigned int i = 0; i < Tracks.size(); ++i)
		if (other.Tracks[i])
			Tracks[i] = new StdMeshTrack(*other.Tracks[i]);

	return *this;
}

StdSubMesh::StdSubMesh():
		Material(NULL)
{
}

StdMesh::StdMesh()
{
	BoundingBox.x1 = BoundingBox.y1 = BoundingBox.z1 = 0.0f;
	BoundingBox.x2 = BoundingBox.y2 = BoundingBox.z2 = 0.0f;
	BoundingRadius = 0.0f;
}

StdMesh::~StdMesh()
{
	for (unsigned int i = 0; i < Bones.size(); ++i)
		delete Bones[i];
}

void StdMesh::AddMasterBone(StdMeshBone* bone)
{
	bone->Index = Bones.size(); // Remember index in master bone table
	Bones.push_back(bone);
	for (unsigned int i = 0; i < bone->Children.size(); ++i)
		AddMasterBone(bone->Children[i]);
}

const StdMeshBone* StdMesh::GetBoneByName(const StdStrBuf& name) const
{
	// Lookup parent bone
	for (unsigned int i = 0; i < Bones.size(); ++i)
		if (Bones[i]->Name == name)
			return Bones[i];

	return NULL;
}

const StdMeshAnimation* StdMesh::GetAnimationByName(const StdStrBuf& name) const
{
	StdCopyStrBuf name2(name);
	std::map<StdCopyStrBuf, StdMeshAnimation>::const_iterator iter = Animations.find(name2);
	if (iter == Animations.end()) return NULL;
	return &iter->second;
}

void StdMesh::MirrorAnimation(const StdStrBuf& name, const StdMeshAnimation& animation)
{
	StdCopyStrBuf name2(name);
	assert(Animations.find(name2) == Animations.end());

	StdMeshAnimation& new_anim = Animations.insert(std::make_pair(name2, animation)).first->second;
	new_anim.Name = name2;

	// Go through all bones
	for(unsigned int i = 0; i < GetNumBones(); ++i)
	{
		const StdMeshBone& bone = GetBone(i);
		StdCopyStrBuf other_bone_name(bone.Name);
		if(MirrorName(other_bone_name))
		{
			const StdMeshBone* other_bone = GetBoneByName(other_bone_name);
			if(!other_bone)
				throw std::runtime_error(std::string("No counterpart for bone ") + bone.Name.getData() + " found");

			// Make sure to not swap tracks twice
			if( (animation.Tracks[i] != NULL || animation.Tracks[other_bone->Index] != NULL) &&
			   other_bone->Index > bone.Index)
			{
				std::swap(new_anim.Tracks[i], new_anim.Tracks[other_bone->Index]);

				StdMeshTransformation own_trans = bone.GetParent()->InverseTransformation * bone.Transformation;
				StdMeshTransformation other_own_trans = other_bone->GetParent()->InverseTransformation * other_bone->Transformation;

				// Mirror all the keyframes of both tracks
				if(new_anim.Tracks[i] != NULL)
					for(std::map<float, StdMeshKeyFrame>::iterator iter = new_anim.Tracks[i]->Frames.begin(); iter != new_anim.Tracks[i]->Frames.end(); ++iter)
						MirrorKeyFrame(iter->second, own_trans, StdMeshTransformation::Inverse(other_own_trans));

				if(new_anim.Tracks[other_bone->Index] != NULL)
					for(std::map<float, StdMeshKeyFrame>::iterator iter = new_anim.Tracks[other_bone->Index]->Frames.begin(); iter != new_anim.Tracks[other_bone->Index]->Frames.end(); ++iter)
						MirrorKeyFrame(iter->second, other_own_trans, StdMeshTransformation::Inverse(own_trans));
			}
		}
		else if(bone.Name.Compare_(".N", bone.Name.getLength()-2) != 0)
		{
			if(new_anim.Tracks[i] != NULL)
			{
				StdMeshTransformation own_trans = bone.Transformation;
				if(bone.GetParent()) own_trans = bone.GetParent()->InverseTransformation * bone.Transformation;

				for(std::map<float, StdMeshKeyFrame>::iterator iter = new_anim.Tracks[i]->Frames.begin(); iter != new_anim.Tracks[i]->Frames.end(); ++iter)
					MirrorKeyFrame(iter->second, own_trans, StdMeshTransformation::Inverse(own_trans));
			}
		}
	}
}

void StdMesh::PostInit()
{
	// Mirror .R and .L animations without counterpart
	for(std::map<StdCopyStrBuf, StdMeshAnimation>::iterator iter = Animations.begin(); iter != Animations.end(); ++iter)
	{
		// For debugging purposes:
//		if(iter->second.Name == "Jump")
//			MirrorAnimation(StdCopyStrBuf("Jump.Mirror"), iter->second);

		StdCopyStrBuf buf = iter->second.Name;
		if(MirrorName(buf))
		{
			if(Animations.find(buf) == Animations.end())
				MirrorAnimation(buf, iter->second);
		}
	}

	// Order submeshes so that opaque submeshes come before non-opaque ones
	std::sort(SubMeshes.begin(), SubMeshes.end(), StdMeshSubMeshVisibilityCmpPred());
}

StdSubMeshInstance::StdSubMeshInstance(StdMeshInstance& instance, const StdSubMesh& submesh, float completion):
		Vertices(submesh.GetNumVertices()),
		Material(NULL), CurrentFaceOrdering(FO_Fixed)
{
	// Copy initial Vertices/Faces
	for (unsigned int i = 0; i < submesh.GetNumVertices(); ++i)
		Vertices[i] = submesh.GetVertex(i);
	LoadFacesForCompletion(instance, submesh, completion);

	SetMaterial(submesh.GetMaterial());
}

void StdSubMeshInstance::LoadFacesForCompletion(StdMeshInstance& instance, const StdSubMesh& submesh, float completion)
{
	// First: Copy all faces
	Faces.resize(submesh.GetNumFaces());
	for (unsigned int i = 0; i < submesh.GetNumFaces(); ++i)
		Faces[i] = submesh.GetFace(i);

	if(completion < 1.0f)
	{
		// Second: Order by Y position. StdMeshInstanceFaceOrderingCmpPred orders by Z position,
		// however we can simply give an appropriate transformation matrix to the face ordering.
		// At this point, all vertices are in the OGRE coordinate frame, and Z in OGRE equals
		// Y in Clonk, so we are fine without additional transformation.
		StdMeshInstanceFaceOrderingCmpPred pred(instance, *this, FO_FarthestToNearest, StdMeshMatrix::Identity());
		g_pred = &pred;
		StdMesh_tim_sort(&Faces[0], Faces.size());
		g_pred = NULL;

		// Third: Only use the first few ones
		Faces.resize(static_cast<unsigned int>(completion * submesh.GetNumFaces() + 0.5));
	}
}

void StdSubMeshInstance::SetMaterial(const StdMeshMaterial& material)
{
	Material = &material;

	// Setup initial texture animation data
	assert(Material->BestTechniqueIndex >= 0);
	const StdMeshMaterialTechnique& technique = Material->Techniques[Material->BestTechniqueIndex];
	PassData.resize(technique.Passes.size());
	for (unsigned int i = 0; i < PassData.size(); ++i)
	{
		const StdMeshMaterialPass& pass = technique.Passes[i];
		// Clear from previous material
		PassData[i].TexUnits.clear();

		for (unsigned int j = 0; j < pass.TextureUnits.size(); ++j)
		{
			TexUnit unit;
			unit.Phase = 0;
			unit.PhaseDelay = 0.0f;
			unit.Position = 0.0;
			PassData[i].TexUnits.push_back(unit);
		}
	}

	// TODO: Reorder this submesh so that opaque submeshes are drawn
	// before non-opaque ones.
	// TODO: Reset face ordering
}

void StdSubMeshInstance::SetFaceOrdering(const StdSubMesh& submesh, FaceOrdering ordering)
{
	if (CurrentFaceOrdering != ordering)
	{
		CurrentFaceOrdering = ordering;
		if (ordering == FO_Fixed)
		{
			for (unsigned int i = 0; i < submesh.GetNumFaces(); ++i)
				Faces[i] = submesh.GetFace(i);
		}
	}
}

void StdSubMeshInstance::SetFaceOrderingForClrModulation(const StdSubMesh& submesh, uint32_t clrmod)
{
	bool opaque = Material->IsOpaque();

	if(!opaque)
		SetFaceOrdering(submesh, FO_FarthestToNearest);
	else if( ((clrmod >> 24) & 0xff) != 0xff)
		SetFaceOrdering(submesh, FO_NearestToFarthest);
	else
		SetFaceOrdering(submesh, FO_Fixed);
}

void StdMeshInstance::SerializableValueProvider::CompileFunc(StdCompiler* pComp)
{
	pComp->Value(Value);
}

StdMeshInstance::AnimationNode::AnimationNode():
		Type(LeafNode), Parent(NULL)
{
	Leaf.Animation = NULL;
	Leaf.Position = NULL;
}

StdMeshInstance::AnimationNode::AnimationNode(const StdMeshAnimation* animation, ValueProvider* position):
		Type(LeafNode), Parent(NULL)
{
	Leaf.Animation = animation;
	Leaf.Position = position;
}

StdMeshInstance::AnimationNode::AnimationNode(AnimationNode* child_left, AnimationNode* child_right, ValueProvider* weight):
		Type(LinearInterpolationNode), Parent(NULL)
{
	LinearInterpolation.ChildLeft = child_left;
	LinearInterpolation.ChildRight = child_right;
	LinearInterpolation.Weight = weight;
}

StdMeshInstance::AnimationNode::~AnimationNode()
{
	switch (Type)
	{
	case LeafNode:
		delete Leaf.Position;
		break;
	case LinearInterpolationNode:
		delete LinearInterpolation.ChildLeft;
		delete LinearInterpolation.ChildRight;
		delete LinearInterpolation.Weight;
		break;
	}
}

bool StdMeshInstance::AnimationNode::GetBoneTransform(unsigned int bone, StdMeshTransformation& transformation)
{
	StdMeshTransformation combine_with;
	StdMeshTrack* track;

	switch (Type)
	{
	case LeafNode:
		//if(!Leaf.Animation) return false;
		track = Leaf.Animation->Tracks[bone];
		if (!track) return false;
		transformation = track->GetTransformAt(fixtof(Leaf.Position->Value));
		return true;
	case LinearInterpolationNode:
		if (!LinearInterpolation.ChildLeft->GetBoneTransform(bone, transformation))
			return LinearInterpolation.ChildRight->GetBoneTransform(bone, transformation);
		if (!LinearInterpolation.ChildRight->GetBoneTransform(bone, combine_with))
			return true; // First Child affects bone

		transformation = StdMeshTransformation::Nlerp(transformation, combine_with, fixtof(LinearInterpolation.Weight->Value));
		return true;
	default:
		assert(false);
		return false;
	}
}

void StdMeshInstance::AnimationNode::CompileFunc(StdCompiler* pComp, const StdMesh* Mesh)
{
	static const StdEnumEntry<NodeType> NodeTypes[] =
	{
		{ "Leaf",                  LeafNode                      },
		{ "LinearInterpolation",   LinearInterpolationNode       },

		{ NULL,     static_cast<NodeType>(0)  }
	};

	pComp->Value(mkNamingAdapt(Slot, "Slot"));
	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(Type, NodeTypes), "Type"));

	switch(Type)
	{
	case LeafNode:
		if(pComp->isCompiler())
		{
			StdCopyStrBuf anim_name;
			pComp->Value(mkNamingAdapt(toC4CStrBuf(anim_name), "Animation"));
			Leaf.Animation = Mesh->GetAnimationByName(anim_name);
			if(!Leaf.Animation) pComp->excCorrupt("No such animation: \"%s\"", anim_name.getData());
		}
		else
		{
			pComp->Value(mkNamingAdapt(mkParAdapt(mkDecompileAdapt(Leaf.Animation->Name), StdCompiler::RCT_All), "Animation"));
		}

		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&Leaf.Position), "Position"));
		break;
	case LinearInterpolationNode:
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildLeft, "ChildLeft"), Mesh));
		pComp->Value(mkParAdapt(mkNamingPtrAdapt(LinearInterpolation.ChildRight, "ChildRight"), Mesh));
		pComp->Value(mkNamingAdapt(mkValueProviderAdapt(&LinearInterpolation.Weight), "Weight"));
		if(pComp->isCompiler())
		{
			if(LinearInterpolation.ChildLeft->Slot != Slot)
				pComp->excCorrupt("Slot of left child does not match parent slot");
			if(LinearInterpolation.ChildRight->Slot != Slot)
				pComp->excCorrupt("Slof of right child does not match parent slot");
			LinearInterpolation.ChildLeft->Parent = this;
			LinearInterpolation.ChildRight->Parent = this;
		}
		break;
	default:
		pComp->excCorrupt("Invalid animation node type");
		break;
	}
}

void StdMeshInstance::AnimationNode::DenumeratePointers()
{
	SerializableValueProvider* value_provider = NULL;
	switch(Type)
	{
	case LeafNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(Leaf.Position);
		break;
	case LinearInterpolationNode:
		value_provider = dynamic_cast<SerializableValueProvider*>(LinearInterpolation.Weight);
		break;
	}

	if(value_provider) value_provider->DenumeratePointers();
}

StdMeshInstance::AttachedMesh::AttachedMesh():
	Number(0), Parent(NULL), Child(NULL), OwnChild(true), ChildDenumerator(NULL), ParentBone(0), ChildBone(0), FinalTransformDirty(false)
{
}

StdMeshInstance::AttachedMesh::AttachedMesh(unsigned int number, StdMeshInstance* parent, StdMeshInstance* child, bool own_child, Denumerator* denumerator,
		unsigned int parent_bone, unsigned int child_bone, const StdMeshMatrix& transform, uint32_t flags):
		Number(number), Parent(parent), Child(child), OwnChild(own_child), ChildDenumerator(denumerator),
		ParentBone(parent_bone), ChildBone(child_bone), AttachTrans(transform), Flags(flags),
		FinalTransformDirty(true)
{
}

StdMeshInstance::AttachedMesh::~AttachedMesh()
{
	if (OwnChild)
		delete Child;
	delete ChildDenumerator;
}

bool StdMeshInstance::AttachedMesh::SetParentBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Parent->GetMesh().GetBoneByName(bone);
	if (!bone_obj) return false;
	ParentBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

bool StdMeshInstance::AttachedMesh::SetChildBone(const StdStrBuf& bone)
{
	const StdMeshBone* bone_obj = Child->GetMesh().GetBoneByName(bone);
	if (!bone_obj) return false;
	ChildBone = bone_obj->Index;

	FinalTransformDirty = true;
	return true;
}

void StdMeshInstance::AttachedMesh::SetAttachTransformation(const StdMeshMatrix& transformation)
{
	AttachTrans = transformation;
	FinalTransformDirty = true;
}

void StdMeshInstance::AttachedMesh::CompileFunc(StdCompiler* pComp, DenumeratorFactoryFunc Factory)
{
	if(pComp->isCompiler())
	{
		FinalTransformDirty = true;
		ChildDenumerator = Factory();
	}

	const StdBitfieldEntry<uint8_t> AM_Entries[] =
	{
		{ "DrawBefore",    AM_DrawBefore },

		{ NULL,            0 }
	};

	pComp->Value(mkNamingAdapt(Number, "Number"));
	pComp->Value(mkNamingAdapt(ParentBone, "ParentBone")); // TODO: Save as string
	pComp->Value(mkNamingAdapt(ChildBone, "ChildBone")); // TODO: Save as string (note we can only resolve this in DenumeratePointers then!)
	pComp->Value(mkNamingAdapt(mkTransformAdapt(AttachTrans), "AttachTransformation"));
	
	uint8_t dwSyncFlags = static_cast<uint8_t>(Flags);
	pComp->Value(mkNamingAdapt(mkBitfieldAdapt(dwSyncFlags, AM_Entries), "Flags", 0u));
	if(pComp->isCompiler()) Flags = dwSyncFlags;

	pComp->Value(mkParAdapt(*ChildDenumerator, this));
}

void StdMeshInstance::AttachedMesh::DenumeratePointers()
{
	ChildDenumerator->DenumeratePointers(this);
}

StdMeshInstance::StdMeshInstance(const StdMesh& mesh, float completion):
		Mesh(&mesh), SharedVertices(mesh.GetSharedVertices().size()), Completion(completion),
		BoneTransforms(Mesh->GetNumBones(), StdMeshMatrix::Identity()),
		SubMeshInstances(Mesh->GetNumSubMeshes()), AttachParent(NULL),
		BoneTransformsDirty(false)
{
	// Copy initial shared vertices
	for (unsigned int i = 0; i < SharedVertices.size(); ++i)
		SharedVertices[i] = mesh.GetSharedVertices()[i];

	// Create submesh instances
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
	{
		const StdSubMesh& submesh = Mesh->GetSubMesh(i);
		SubMeshInstances[i] = new StdSubMeshInstance(*this, submesh, completion);
	}
}

StdMeshInstance::~StdMeshInstance()
{
	// If we are attached then detach from parent
	if (AttachParent)
		AttachParent->Parent->DetachMesh(AttachParent->Number);

	// Remove all attach children
	while (!AttachChildren.empty())
		DetachMesh(AttachChildren.back()->Number);

	while (!AnimationStack.empty())
		StopAnimation(AnimationStack.front());
	assert(AnimationNodes.empty());

	// Delete submeshes
	for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		delete SubMeshInstances[i];
}

void StdMeshInstance::SetFaceOrdering(FaceOrdering ordering)
{
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->SetFaceOrdering(Mesh->GetSubMesh(i), ordering);

	// Update attachments (only own meshes for now... others might be displayed both attached and non-attached...)
	// still not optimal.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->OwnChild)
			(*iter)->Child->SetFaceOrdering(ordering);
}

void StdMeshInstance::SetFaceOrderingForClrModulation(uint32_t clrmod)
{
	for (unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->SetFaceOrderingForClrModulation(Mesh->GetSubMesh(i), clrmod);

	// Update attachments (only own meshes for now... others might be displayed both attached and non-attached...)
	// still not optimal.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->OwnChild)
			(*iter)->Child->SetFaceOrderingForClrModulation(clrmod);
}

void StdMeshInstance::SetCompletion(float completion)
{
	Completion = completion;

	// TODO: Load all submesh faces and then determine the ones to use from the
	// full pool.
	for(unsigned int i = 0; i < Mesh->GetNumSubMeshes(); ++i)
		SubMeshInstances[i]->LoadFacesForCompletion(*this, Mesh->GetSubMesh(i), completion);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdStrBuf& animation_name, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	const StdMeshAnimation* animation = Mesh->GetAnimationByName(animation_name);
	if (!animation) { delete position; delete weight; return NULL; }

	return PlayAnimation(*animation, slot, sibling, position, weight);
}

StdMeshInstance::AnimationNode* StdMeshInstance::PlayAnimation(const StdMeshAnimation& animation, int slot, AnimationNode* sibling, ValueProvider* position, ValueProvider* weight)
{
	// Default
	if (!sibling) sibling = GetRootAnimationForSlot(slot);
	assert(!sibling || sibling->Slot == slot);

	// Find two subsequent numbers in case we need to create two nodes, so
	// script can deduce the second node.
	unsigned int Number1, Number2;
	for (Number1 = 0; Number1 < AnimationNodes.size(); ++Number1)
		if (AnimationNodes[Number1] == NULL && (!sibling || Number1+1 == AnimationNodes.size() || AnimationNodes[Number1+1] == NULL))
			break;
	/*  for(Number2 = Number1+1; Number2 < AnimationNodes.size(); ++Number2)
	    if(AnimationNodes[Number2] == NULL)
	      break;*/
	Number2 = Number1 + 1;

	position->Value = BoundBy(position->Value, Fix0, ftofix(animation.Length));
	weight->Value = BoundBy(weight->Value, Fix0, itofix(1));

	if (Number1 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) NULL);
	if (sibling && Number2 == AnimationNodes.size()) AnimationNodes.push_back( (StdMeshInstance::AnimationNode*) NULL);

	AnimationNode* child = new AnimationNode(&animation, position);
	AnimationNodes[Number1] = child;
	child->Number = Number1;
	child->Slot = slot;

	if (sibling)
	{
		AnimationNode* parent = new AnimationNode(child, sibling, weight);
		AnimationNodes[Number2] = parent;
		parent->Number = Number2;
		parent->Slot = slot;

		child->Parent = parent;
		parent->Parent = sibling->Parent;
		parent->LinearInterpolation.ChildLeft = sibling;
		parent->LinearInterpolation.ChildRight = child;
		if (sibling->Parent)
		{
			if (sibling->Parent->LinearInterpolation.ChildLeft == sibling)
				sibling->Parent->LinearInterpolation.ChildLeft = parent;
			else
				sibling->Parent->LinearInterpolation.ChildRight = parent;
		}
		else
		{
			// set new parent
			AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
			// slot must not be empty, since sibling uses same slot
			assert(iter != AnimationStack.end() && *iter != NULL);
			*iter = parent;
		}

		sibling->Parent = parent;
	}
	else
	{
		delete weight;
		AnimationNodeList::iterator iter = GetStackIterForSlot(slot, true);
		assert(!*iter); // we have a sibling if slot is not empty
		*iter = child;
	}

	BoneTransformsDirty = true;
	return child;
}

void StdMeshInstance::StopAnimation(AnimationNode* node)
{
	ClearAnimationListRecursively(AnimationNodes, node);

	AnimationNode* parent = node->Parent;
	if (parent == NULL)
	{
		AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
		assert(iter != AnimationStack.end() && *iter == node);
		AnimationStack.erase(iter);
		delete node;
	}
	else
	{
		assert(parent->Type == AnimationNode::LinearInterpolationNode);

		// Remove parent interpolation node and re-link
		AnimationNode* other_child;
		if (parent->LinearInterpolation.ChildLeft == node)
		{
			other_child = parent->LinearInterpolation.ChildRight;
			parent->LinearInterpolation.ChildRight = NULL;
		}
		else
		{
			other_child = parent->LinearInterpolation.ChildLeft;
			parent->LinearInterpolation.ChildLeft = NULL;
		}

		if (parent->Parent)
		{
			assert(parent->Parent->Type == AnimationNode::LinearInterpolationNode);
			if (parent->Parent->LinearInterpolation.ChildLeft == parent)
				parent->Parent->LinearInterpolation.ChildLeft = other_child;
			else
				parent->Parent->LinearInterpolation.ChildRight = other_child;
			other_child->Parent = parent->Parent;
		}
		else
		{
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, false);
			assert(iter != AnimationStack.end() && *iter == parent);
			*iter = other_child;

			other_child->Parent = NULL;
		}

		AnimationNodes[parent->Number] = NULL;
		// Recursively deletes parent and its descendants
		delete parent;
	}

	while (!AnimationNodes.empty() && AnimationNodes.back() == NULL)
		AnimationNodes.erase(AnimationNodes.end()-1);
	BoneTransformsDirty = true;
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetAnimationNodeByNumber(unsigned int number)
{
	if (number >= AnimationNodes.size()) return NULL;
	return AnimationNodes[number];
}

StdMeshInstance::AnimationNode* StdMeshInstance::GetRootAnimationForSlot(int slot)
{
	AnimationNodeList::iterator iter = GetStackIterForSlot(slot, false);
	if (iter == AnimationStack.end()) return NULL;
	return *iter;
}

void StdMeshInstance::SetAnimationPosition(AnimationNode* node, ValueProvider* position)
{
	assert(node->GetType() == AnimationNode::LeafNode);
	delete node->Leaf.Position;
	node->Leaf.Position = position;

	position->Value = BoundBy(position->Value, Fix0, ftofix(node->Leaf.Animation->Length));

	BoneTransformsDirty = true;
}

void StdMeshInstance::SetAnimationWeight(AnimationNode* node, ValueProvider* weight)
{
	assert(node->GetType() == AnimationNode::LinearInterpolationNode);
	delete node->LinearInterpolation.Weight; node->LinearInterpolation.Weight = weight;

	weight->Value = BoundBy(weight->Value, Fix0, itofix(1));

	BoneTransformsDirty = true;
}

void StdMeshInstance::ExecuteAnimation(float dt)
{
	// Iterate from the back since slots might be removed
	for (unsigned int i = AnimationStack.size(); i > 0; --i)
		if(!ExecuteAnimationNode(AnimationStack[i-1]))
			StopAnimation(AnimationStack[i-1]);

	// Update animated textures
	for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
	{
		StdSubMeshInstance& submesh = *SubMeshInstances[i];
		const StdMeshMaterial& material = submesh.GetMaterial();
		const StdMeshMaterialTechnique& technique = material.Techniques[material.BestTechniqueIndex];
		for (unsigned int j = 0; j < submesh.PassData.size(); ++j)
		{
			StdSubMeshInstance::Pass& pass = submesh.PassData[j];
			for (unsigned int k = 0; k < pass.TexUnits.size(); ++k)
			{
				const StdMeshMaterialTextureUnit& texunit = technique.Passes[j].TextureUnits[k];
				StdSubMeshInstance::TexUnit& texunit_instance = submesh.PassData[j].TexUnits[k];
				if (texunit.HasFrameAnimation())
				{
					const unsigned int NumPhases = texunit.GetNumTextures();
					const float PhaseDuration = texunit.Duration / NumPhases;

					const float Position = texunit_instance.PhaseDelay + dt;
					const unsigned int AddPhases = static_cast<unsigned int>(Position / PhaseDuration);

					texunit_instance.Phase = (texunit_instance.Phase + AddPhases) % NumPhases;
					texunit_instance.PhaseDelay = Position - AddPhases * PhaseDuration;
				}

				if (texunit.HasTexCoordAnimation())
					texunit_instance.Position += dt;
			}
		}
	}

	// Update animation for attached meshes
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		(*iter)->Child->ExecuteAnimation(dt);
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(const StdMesh& mesh, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags)
{
	StdMeshInstance* instance = new StdMeshInstance(mesh, 1.0f);
	AttachedMesh* attach = AttachMesh(*instance, denumerator, parent_bone, child_bone, transformation, flags, true);
	if (!attach) { delete instance; return NULL; }
	return attach;
}

StdMeshInstance::AttachedMesh* StdMeshInstance::AttachMesh(StdMeshInstance& instance, AttachedMesh::Denumerator* denumerator, const StdStrBuf& parent_bone, const StdStrBuf& child_bone, const StdMeshMatrix& transformation, uint32_t flags, bool own_child)
{
	std::auto_ptr<AttachedMesh::Denumerator> auto_denumerator(denumerator);

	// We don't allow an instance to be attached to multiple parent instances for now
	if (instance.AttachParent) return NULL;

	// Make sure there are no cyclic attachments
	for (StdMeshInstance* Parent = this; Parent->AttachParent != NULL; Parent = Parent->AttachParent->Parent)
		if (Parent == &instance)
			return NULL;

	AttachedMesh* attach = NULL;
	unsigned int number = 1;

	// Find free index.
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->Number >= number)
			number = (*iter)->Number + 1;

	const StdMeshBone* parent_bone_obj = Mesh->GetBoneByName(parent_bone);
	const StdMeshBone* child_bone_obj = instance.Mesh->GetBoneByName(child_bone);
	if (!parent_bone_obj || !child_bone_obj) return NULL;

	attach = new AttachedMesh(number, this, &instance, own_child, auto_denumerator.release(), parent_bone_obj->Index, child_bone_obj->Index, transformation, flags);
	instance.AttachParent = attach;

	// If DrawInFront is set then sort before others so that drawing order is easy
	if(flags & AM_DrawBefore)
		AttachChildren.insert(AttachChildren.begin(), attach);
	else
		AttachChildren.insert(AttachChildren.end(), attach);

	return attach;
}

bool StdMeshInstance::DetachMesh(unsigned int number)
{
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		if ((*iter)->Number == number)
		{
			// Reset attach parent of child so it does not try
			// to detach itself on destruction.
			(*iter)->Child->AttachParent = NULL;

			delete *iter;
			AttachChildren.erase(iter);
			return true;
		}
	}

	return false;
}

StdMeshInstance::AttachedMesh* StdMeshInstance::GetAttachedMeshByNumber(unsigned int number) const
{
	for (AttachedMeshIter iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
		if ((*iter)->Number == number)
			return *iter;
	return NULL;
}

bool StdMeshInstance::UpdateBoneTransforms()
{
	bool was_dirty = BoneTransformsDirty;

	// Nothing changed since last time
	if (BoneTransformsDirty)
	{
		// Compute transformation matrix for each bone.
		for (unsigned int i = 0; i < BoneTransforms.size(); ++i)
		{
			StdMeshTransformation Transformation;

			const StdMeshBone& bone = Mesh->GetBone(i);
			const StdMeshBone* parent = bone.GetParent();
			assert(!parent || parent->Index < i);

			bool have_transform = false;
			for (unsigned int j = 0; j < AnimationStack.size(); ++j)
			{
				if (have_transform)
				{
					StdMeshTransformation other;
					if (AnimationStack[j]->GetBoneTransform(i, other))
						Transformation = StdMeshTransformation::Nlerp(Transformation, other, 1.0f); // TODO: Allow custom weighing for slot combination
				}
				else
				{
					have_transform = AnimationStack[j]->GetBoneTransform(i, Transformation);
				}
			}

			if (!have_transform)
			{
				if (parent)
					BoneTransforms[i] = BoneTransforms[parent->Index];
				else
					BoneTransforms[i] = StdMeshMatrix::Identity();
			}
			else
			{
				BoneTransforms[i] = StdMeshMatrix::Transform(bone.Transformation * Transformation * bone.InverseTransformation);
				if (parent) BoneTransforms[i] = BoneTransforms[parent->Index] * BoneTransforms[i];
			}
		}

		// Compute transformation for each vertex. We could later think about
		// doing this on the GPU using a vertex shader. This would then probably
		// need to go to CStdGL::PerformMesh and CStdD3D::PerformMesh.
		// But first, we need to move vertex data to the GPU.
		if(!Mesh->GetSharedVertices().empty())
			ApplyBoneTransformToVertices(Mesh->GetSharedVertices(), SharedVertices);
		for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
		{
			const StdSubMesh& submesh = Mesh->GetSubMesh(i);
			if(!submesh.GetVertices().empty())
				ApplyBoneTransformToVertices(submesh.GetVertices(), SubMeshInstances[i]->Vertices);
		}
	}

	// Update attachment's attach transformations. Note this is done recursively.
	for (AttachedMeshList::iterator iter = AttachChildren.begin(); iter != AttachChildren.end(); ++iter)
	{
		AttachedMesh* attach = *iter;
		const bool ChildBoneTransformsDirty = attach->Child->BoneTransformsDirty;
		attach->Child->UpdateBoneTransforms();

		if (BoneTransformsDirty || ChildBoneTransformsDirty || attach->FinalTransformDirty)
		{
			was_dirty = true;

			// Compute matrix to change the coordinate system to the one of the attached bone:
			// The idea is that a vertex at the child bone's position transforms to the parent bone's position.
			// Therefore (read from right to left) we first apply the inverse of the child bone transformation,
			// then an optional scaling matrix, and finally the parent bone transformation

			// TODO: we can cache the three matrices in the middle since they don't change over time,
			// reducing this to two matrix multiplications instead of four each frame.
			// Might even be worth to compute the complete transformation directly when rendering then
			// (saves per-instance memory, but requires recomputation if the animation does not change).
			// TODO: We might also be able to cache child inverse, and only recomupte it if
			// child bone transforms are dirty (saves matrix inversion for unanimated attach children).
			attach->FinalTrans = BoneTransforms[attach->ParentBone]
			                     * StdMeshMatrix::Transform(Mesh->GetBone(attach->ParentBone).Transformation)
			                     * attach->AttachTrans
			                     * StdMeshMatrix::Transform(attach->Child->Mesh->GetBone(attach->ChildBone).InverseTransformation)
			                     * StdMeshMatrix::Inverse(attach->Child->BoneTransforms[attach->ChildBone]);

			attach->FinalTransformDirty = false;
		}
	}

	BoneTransformsDirty = false;
	return was_dirty;
}

void StdMeshInstance::ReorderFaces(StdMeshMatrix* global_trans)
{
	for (unsigned int i = 0; i < SubMeshInstances.size(); ++i)
	{
		StdSubMeshInstance& inst = *SubMeshInstances[i];
		if(inst.CurrentFaceOrdering != StdSubMeshInstance::FO_Fixed)
		{
			StdMeshInstanceFaceOrderingCmpPred pred(*this, inst, inst.CurrentFaceOrdering, global_trans ? *global_trans : StdMeshMatrix::Identity());

			// The usage of timsort instead of std::sort at this point is twofold.
			// First, it's faster in our case where the array is already sorted in
			// many cases (remember this is called at least once a frame).
			// And it's not just a bit faster either but a lot. I have measured
			// a factor of 7 on my system.
			// Second, in our Windows autobuilds there is a crash within std::sort
			// which is very hard to debug because it's hardly reproducible with
			// anything other than the autobuilds (I tried hard). If the crash goes
			// away with timsort then great, if not then maybe it's easier to debug
			// since the code is in our tree.

			//std::sort(inst.Faces.begin(), inst.Faces.end(), pred);

			g_pred = &pred;
			StdMesh_tim_sort(&inst.Faces[0], inst.Faces.size());
			g_pred = NULL;
		}
	}

	// TODO: Also reorder submeshes, attached meshes and include AttachTransformation for attached meshes...
}

void StdMeshInstance::CompileFunc(StdCompiler* pComp, AttachedMesh::DenumeratorFactoryFunc Factory)
{
	if(pComp->isCompiler())
	{
		// Only initially created instances can be compiled
		assert(!AttachParent);
		assert(AttachChildren.empty());
		assert(AnimationStack.empty());
		BoneTransformsDirty = true;

		bool valid;
		pComp->Value(mkNamingAdapt(valid, "Valid"));
		if(!valid) pComp->excCorrupt("Mesh instance is invalid");

		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(int32_t i = 0; i < iAnimCnt; ++i)
		{
			AnimationNode* node = NULL;
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(node, "AnimationNode"), Mesh));
			AnimationNodeList::iterator iter = GetStackIterForSlot(node->Slot, true);
			if(*iter != NULL) { delete node; pComp->excCorrupt("Duplicate animation slot index"); }
			*iter = node;

			// Add nodes into lookup table
			std::vector<AnimationNode*> nodes(1, node);
			while(!nodes.empty())
			{
				node = nodes.back();
				nodes.erase(nodes.end()-1);

				if (AnimationNodes.size() <= node->Number)
					AnimationNodes.resize(node->Number+1);
				if(AnimationNodes[node->Number] != NULL) pComp->excCorrupt("Duplicate animation node number");
				AnimationNodes[node->Number] = node;

				if(node->Type == AnimationNode::LinearInterpolationNode)
				{
					nodes.push_back(node->LinearInterpolation.ChildLeft);
					nodes.push_back(node->LinearInterpolation.ChildRight);
				}
			}
		}

		int32_t iAttachedCnt;
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));

		for(int32_t i = 0; i < iAttachedCnt; ++i)
		{
			AttachChildren.push_back(new AttachedMesh);
			AttachedMesh* attach = AttachChildren.back();

			attach->Parent = this;
			pComp->Value(mkNamingAdapt(mkParAdapt(*attach, Factory), "Attached"));
		}
	}
	else
	{
		// Write something to make sure that the parent
		// named section ([Mesh] or [ChildInstance]) is written.
		// StdCompilerIni does not make a difference between
		// non-existing and empty named sections.
		bool valid = true;
		pComp->Value(mkNamingAdapt(valid, "Valid"));

		int32_t iAnimCnt = AnimationStack.size();
		pComp->Value(mkNamingCountAdapt(iAnimCnt, "AnimationNode"));

		for(AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
			pComp->Value(mkParAdapt(mkNamingPtrAdapt(*iter, "AnimationNode"), Mesh));

		int32_t iAttachedCnt = AttachChildren.size();
		pComp->Value(mkNamingCountAdapt(iAttachedCnt, "Attached"));
		
		for(unsigned int i = 0; i < AttachChildren.size(); ++i)
			pComp->Value(mkNamingAdapt(mkParAdapt(*AttachChildren[i], Factory), "Attached"));
	}
}

void StdMeshInstance::DenumeratePointers()
{
	for(unsigned int i = 0; i < AnimationNodes.size(); ++i)
		if(AnimationNodes[i])
			AnimationNodes[i]->DenumeratePointers();

	for(unsigned int i = 0; i < AttachChildren.size(); ++i)
		AttachChildren[i]->DenumeratePointers();
}

StdMeshInstance::AnimationNodeList::iterator StdMeshInstance::GetStackIterForSlot(int slot, bool create)
{
	// TODO: bsearch
	for (AnimationNodeList::iterator iter = AnimationStack.begin(); iter != AnimationStack.end(); ++iter)
	{
		if ((*iter)->Slot == slot)
		{
			return iter;
		}
		else if ((*iter)->Slot > slot)
		{
			if (!create)
				return AnimationStack.end();
			else
				return AnimationStack.insert(iter, NULL);
		}
	}

	if (!create)
		return AnimationStack.end();
	else
		return AnimationStack.insert(AnimationStack.end(), NULL);
}

bool StdMeshInstance::ExecuteAnimationNode(AnimationNode* node)
{
	ValueProvider* provider = NULL;
	C4Real min;
	C4Real max;

	switch (node->GetType())
	{
	case AnimationNode::LeafNode:
		provider = node->GetPositionProvider();
		min = Fix0;
		max = ftofix(node->GetAnimation()->Length);
		break;
	case AnimationNode::LinearInterpolationNode:
		provider = node->GetWeightProvider();
		min = Fix0;
		max = itofix(1);
		break;
	default:
		assert(false);
		break;
	}
	const C4Real old_value = provider->Value;

	if (!provider->Execute())
	{
		if (node->GetType() == AnimationNode::LeafNode) return false;

		// Remove the child with less weight (normally weight reaches 0.0 or 1.0)
		if (node->GetWeight() > itofix(1, 2))
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetRightChild())) return false;
			// Remove left child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildLeft);
		}
		else
		{
			// Remove both children (by parent) if other wants to be deleted as well
			if (!ExecuteAnimationNode(node->GetLeftChild())) return false;
			// Remove right child as it has less weight
			StopAnimation(node->LinearInterpolation.ChildRight);
		}
	}
	else
	{
		if (provider->Value != old_value)
		{
			provider->Value = BoundBy(provider->Value, min, max);
			BoneTransformsDirty = true;
		}

		if (node->GetType() == AnimationNode::LinearInterpolationNode)
		{
			const bool left_result = ExecuteAnimationNode(node->GetLeftChild());
			const bool right_result = ExecuteAnimationNode(node->GetRightChild());

			// Remove this node completely
			if (!left_result && !right_result)
				return false;

			// Note that either of this also removes node
			if (!left_result)
				StopAnimation(node->GetLeftChild());
			if (!right_result)
				StopAnimation(node->GetRightChild());
		}
	}

	return true;
}

void StdMeshInstance::ApplyBoneTransformToVertices(const std::vector<StdSubMesh::Vertex>& mesh_vertices, std::vector<StdMeshVertex>& instance_vertices)
{
	assert(mesh_vertices.size() == instance_vertices.size());
	for (unsigned int j = 0; j < instance_vertices.size(); ++j)
	{
		const StdSubMesh::Vertex& vertex = mesh_vertices[j];
		StdMeshVertex& instance_vertex = instance_vertices[j];
		if (!vertex.BoneAssignments.empty())
		{
			instance_vertex.x = instance_vertex.y = instance_vertex.z = 0.0f;
			instance_vertex.nx = instance_vertex.ny = instance_vertex.nz = 0.0f;
			instance_vertex.u = vertex.u; instance_vertex.v = vertex.v;

			for (unsigned int k = 0; k < vertex.BoneAssignments.size(); ++k)
			{
				const StdMeshVertexBoneAssignment& assignment = vertex.BoneAssignments[k];
				instance_vertex += assignment.Weight * (BoneTransforms[assignment.BoneIndex] * vertex);
			}
		}
		else
		{
			instance_vertex = vertex;
		}
	}
}
