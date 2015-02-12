/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
#include <StdMesh.h>
#include <StdMeshMaterial.h>
#include <StdMeshUpdate.h>
#include <StdMeshLoader.h>

StdMeshMaterialUpdate::StdMeshMaterialUpdate(StdMeshMatManager& manager):
	MaterialManager(manager)
{
}

void StdMeshMaterialUpdate::Update(StdMesh* mesh) const
{
	for(std::vector<StdSubMesh>::iterator iter = mesh->SubMeshes.begin(); iter != mesh->SubMeshes.end(); ++iter)
	{
		std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator mat_iter = Materials.find(iter->Material);
		if(mat_iter != Materials.end())
		{
			const StdMeshMaterial* new_material = MaterialManager.GetMaterial(mat_iter->second.Name.getData());

			if(new_material)
			{
				iter->Material = new_material;
			}
			else
			{
				// If no replacement material is available, then re-insert the previous
				// material into the material map. This is mainly just to keep things
				// going - next time the scenario will be started the mesh will fail
				// to load because the material cannot be found.
				MaterialManager.Materials[mat_iter->second.Name] = mat_iter->second; // TODO: could be moved
				iter->Material = MaterialManager.GetMaterial(mat_iter->second.Name.getData());
			}
		}
	}
}

void StdMeshMaterialUpdate::Update(StdMeshInstance* instance) const
{
	for(unsigned int i = 0; i < instance->SubMeshInstances.size(); ++i) //std::vector<StdSubMeshInstance*>::iterator iter = instance->SubMeshInstances->begin(); iter != instance->SubMeshInstances->end(); ++iter)
	{
		StdSubMeshInstance* sub_instance = instance->SubMeshInstances[i];
		std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator mat_iter = Materials.find(sub_instance->Material);
		if(mat_iter != Materials.end())
		{
			// Material needs to be updated
			const StdMeshMaterial* new_material = MaterialManager.GetMaterial(mat_iter->second.Name.getData());
			// If new material is not available, fall back to StdMesh (definition) material
			if(!new_material) new_material = instance->GetMesh().GetSubMesh(i).Material;
			sub_instance->SetMaterial(*new_material);
		}
	}
}

void StdMeshMaterialUpdate::Cancel() const
{
	// Reset all materials in manager
	for(std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator iter = Materials.begin(); iter != Materials.end(); ++iter)
		MaterialManager.Materials[iter->second.Name] = iter->second; // TODO: could be moved
}

void StdMeshMaterialUpdate::Add(const StdMeshMaterial* material)
{
	assert(Materials.find(material) == Materials.end());
	Materials[material] = *material; // TODO: could be moved
}

StdMeshUpdate::StdMeshUpdate(const StdMesh& old_mesh):
	OldMesh(&old_mesh), BoneNamesByIndex(OldMesh->GetSkeleton().GetNumBones())
{
	for (unsigned int i = 0; i < OldMesh->GetSkeleton().GetNumBones(); ++i)
		BoneNamesByIndex[i] = OldMesh->GetSkeleton().GetBone(i).Name;
}

void StdMeshUpdate::Update(StdMeshInstance* instance, const StdMesh& new_mesh) const
{
	assert(&instance->GetMesh() == OldMesh);

	// Update instance to represent new mesh
	instance->Mesh = &new_mesh;
	instance->BoneTransforms = std::vector<StdMeshMatrix>(new_mesh.GetSkeleton().GetNumBones(), StdMeshMatrix::Identity());
	instance->BoneTransformsDirty = true;

	for (unsigned int i = 0; i < instance->SubMeshInstances.size(); ++i)
		delete instance->SubMeshInstances[i];
	instance->SubMeshInstances.resize(new_mesh.GetNumSubMeshes());
	for (unsigned int i = 0; i < instance->SubMeshInstances.size(); ++i)
	{
		instance->SubMeshInstances[i] = new StdSubMeshInstance(*instance, new_mesh.GetSubMesh(i), instance->GetCompletion());

		// Submeshes are reset to their default material,
		// so the submesh order is unaltered
		instance->SubMeshInstancesOrdered = instance->SubMeshInstances;

		// TODO: We might try to restore the previous mesh material here, using global material manager, maybe iff the number of submesh instances is unchanged.
	}

	// Update child bone of attach parent. If the bone does not exist anymore
	// in the updated mesh, then detach the mesh from its parent
	if(instance->AttachParent)
	{
		if(!instance->AttachParent->SetChildBone(BoneNamesByIndex[instance->AttachParent->ChildBone]))
		{
			bool OwnChild = instance->AttachParent->OwnChild;
			instance->AttachParent->Parent->DetachMesh(instance->AttachParent->Number);
			
			// If the attachment owned the child instance then detach procedure
			// deleted the child instance. In that case we do not want to proceed
			// with the mesh update procedure.
			if(OwnChild) return;
		}
	}

	// Update parent bones of attach children. If a bone does not exist in the
	// updated mesh then detach the mesh from its parent.
	std::vector<unsigned int> Removal;
	for(StdMeshInstance::AttachedMeshIter iter = instance->AttachedMeshesBegin(); iter != instance->AttachedMeshesEnd(); ++iter)
	{
		if(!(*iter)->SetParentBone(BoneNamesByIndex[(*iter)->ParentBone]))
		{
			// Do not detach the mesh right here so we can finish iterating over
			// all attached meshes first.
			Removal.push_back((*iter)->Number);
		}
	}

	for(unsigned int i = 0; i < Removal.size(); ++i)
		instance->DetachMesh(Removal[i]);

	// Update custom nodes in the animation tree. Leaf nodes which refer to an animation that
	// does not exist anymore are removed.
	for (unsigned int i = instance->AnimationStack.size(); i > 0; --i)
		if(!UpdateAnimationNode(instance, instance->AnimationStack[i-1]))
			instance->StopAnimation(instance->AnimationStack[i-1]);
}

bool StdMeshUpdate::UpdateAnimationNode(StdMeshInstance* instance, StdMeshInstance::AnimationNode* node) const
{
	const StdMesh& new_mesh = *instance->Mesh;
	switch (node->GetType())
	{
	case StdMeshInstance::AnimationNode::LeafNode:
		// Animation pointer is updated by StdMeshAnimationUpdate
		return true;
	case StdMeshInstance::AnimationNode::CustomNode:
		{
			// Update bone index by bone name
			StdCopyStrBuf bone_name = BoneNamesByIndex[node->Custom.BoneIndex];
			const StdMeshBone* bone = new_mesh.GetSkeleton().GetBoneByName(bone_name);
			if(!bone) return false;
			node->Custom.BoneIndex = bone->Index;
			return true;
		}
	case StdMeshInstance::AnimationNode::LinearInterpolationNode:
		{
			const bool left_result = UpdateAnimationNode(instance, node->GetLeftChild());
			const bool right_result = UpdateAnimationNode(instance, node->GetRightChild());

			// Remove this node completely
			if (!left_result && !right_result)
				return false;

			// Note that either of this also removes this node (and replaces by
			// the other child in the tree).
			if (!left_result)
				instance->StopAnimation(node->GetLeftChild());
			if (!right_result)
				instance->StopAnimation(node->GetRightChild());

			return true;
		}
	default:
		assert(false);
		return false;
	}
}

StdMeshAnimationUpdate::StdMeshAnimationUpdate(const StdMeshSkeletonLoader& skeleton_loader)
{
	// Store all animation names of all skeletons by pointer, we need those when
	// updating the animation state of mesh instances after the update.
	for(StdMeshSkeletonLoader::skeleton_iterator iter = skeleton_loader.skeletons_begin(); iter != skeleton_loader.skeletons_end(); ++iter)
	{
		const StdMeshSkeleton& skeleton = *iter->second;
		for (std::map<StdCopyStrBuf, StdMeshAnimation>::const_iterator iter = skeleton.Animations.begin(); iter != skeleton.Animations.end(); ++iter)
		{
			AnimationNames[&iter->second] = iter->first;
		}
	}
}

void StdMeshAnimationUpdate::Update(StdMeshInstance* instance) const
{
	// Update the animation tree. Leaf nodes which refer to an animation that
	// does not exist anymore are removed.
	for (unsigned int i = instance->AnimationStack.size(); i > 0; --i)
		if(!UpdateAnimationNode(instance, instance->AnimationStack[i-1]))
			instance->StopAnimation(instance->AnimationStack[i-1]);
}

bool StdMeshAnimationUpdate::UpdateAnimationNode(StdMeshInstance* instance, StdMeshInstance::AnimationNode* node) const
{
	const StdMesh& new_mesh = *instance->Mesh;
	switch (node->GetType())
	{
	case StdMeshInstance::AnimationNode::LeafNode:
		{
			// Find dead animation
			std::map<const StdMeshAnimation*, StdCopyStrBuf>::const_iterator iter = AnimationNames.find(node->Leaf.Animation);

			// If this assertion fires, it typically means that UpdateAnimations() was called twice for the same mesh instance
			assert(iter != AnimationNames.end());

			// Update to new animation
			node->Leaf.Animation = new_mesh.GetSkeleton().GetAnimationByName(iter->second);
			if(!node->Leaf.Animation) return false;

			// Clamp provider value
			StdMeshInstance::ValueProvider* provider = node->GetPositionProvider();
			C4Real min = Fix0;
			C4Real max = ftofix(node->GetAnimation()->Length);
			provider->Value = Clamp(provider->Value, min, max);
			return true;
		}
	case StdMeshInstance::AnimationNode::CustomNode:
		// Nothing to do here; bone index is updated in StdMeshUpdate
		return true;
	case StdMeshInstance::AnimationNode::LinearInterpolationNode:
		{
			const bool left_result = UpdateAnimationNode(instance, node->GetLeftChild());
			const bool right_result = UpdateAnimationNode(instance, node->GetRightChild());

			// Remove this node completely
			if (!left_result && !right_result)
				return false;

			// Note that either of this also removes this node (and replaces by
			// the other child in the tree).
			if (!left_result)
				instance->StopAnimation(node->GetLeftChild());
			if (!right_result)
				instance->StopAnimation(node->GetRightChild());

			return true;
		}
	default:
		assert(false);
		return false;
	}
}
