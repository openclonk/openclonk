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

#ifndef INC_StdMeshUpdate
#define INC_StdMeshUpdate

// This is a helper class to fix pointers after an update of StdMeshMaterials.
// To update one or more materials, remove them from the MaterialManager with
// erase(), then add new materials, then run Update() on all StdMeshes.
// Afterwards, run Update on all StdMeshInstances.
// If Cancel() is called before any Update() call then the Update() calls
// will reset all materials to what they have been before they were removed
// from the material manager.
class StdMeshMaterialUpdate
{
	friend class StdMeshMatManager; // calls Add() for each removed material
public:
	StdMeshMaterialUpdate(StdMeshMatManager& manager);

	void Update(StdMesh* mesh) const;
	void Update(StdMeshInstance* instance) const; // note this is NOT recursive

	void Cancel() const;

private:
	void Add(const StdMeshMaterial* material);

	StdMeshMatManager& MaterialManager;
	std::map<const StdMeshMaterial*, StdMeshMaterial> Materials;
};

// This is a helper class to update the underlying StdMesh of certain mesh
// instances. It updated the StdMeshInstance::Mesh pointer, and tries to
// preserve attached meshes, however might not be able to do so in which case
// attached meshes are removed.
class StdMeshUpdate
{
public:
	StdMeshUpdate(const StdMesh& old_mesh);

	// Not recursive for attached meshes
	void Update(StdMeshInstance* instance, const StdMesh& new_mesh) const;

	const StdMesh& GetOldMesh() const { return *OldMesh; }

private:
	bool UpdateAnimationNode(StdMeshInstance* instance, StdMeshInstanceAnimationNode* node) const;

	const StdMesh* OldMesh;

	std::vector<StdCopyStrBuf> BoneNamesByIndex;
};

// This is a helper class to update the animation state of all mesh instances
// after a definition reload. It tries to keep the animation state by
// animation name and removes animations if that is not possible.
class StdMeshAnimationUpdate
{
public:
	StdMeshAnimationUpdate(const StdMeshSkeletonLoader& skeleton_loader);

	// Note that this is not recursive for attached meshes, and that the
	// pointer to the StdMesh object must already have been fixed with
	// a call to StdMeshUpdate::Update().
	void Update(StdMeshInstance* instance) const;

private:
	bool UpdateAnimationNode(StdMeshInstance* instance, StdMeshInstanceAnimationNode* node) const;

	std::map<const StdMeshAnimation*, StdCopyStrBuf> AnimationNames;
};

#endif
