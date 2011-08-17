/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2009-2011  Armin Burgmeier
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

#ifndef INC_StdMeshUpdate
#define INC_StdMeshUpdate

#include <StdMeshMaterial.h>

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
	void Update(StdMeshInstance* instance) const;

	void Cancel() const;

private:
	void UpdateSingle(StdMeshInstance* instance) const;

	void Add(const StdMeshMaterial* material);

	StdMeshMatManager& MaterialManager;
	std::map<const StdMeshMaterial*, StdMeshMaterial> Materials;
};

#endif
