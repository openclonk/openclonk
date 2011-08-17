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

#include "C4Include.h"
#include <StdMesh.h>
#include <StdMeshMaterial.h>
#include <StdMeshUpdate.h>

StdMeshMaterialUpdate::StdMeshMaterialUpdate(StdMeshMatManager& manager):
	MaterialManager(manager)
{
}

void StdMeshMaterialUpdate::Update(StdMesh* mesh) const
{
	for(std::vector<StdSubMesh>::iterator iter = mesh->SubMeshes.begin(); iter != mesh->SubMeshes.end(); ++iter)
	{
		std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator mat_iter =	Materials.find(iter->Material);
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
				MaterialManager.Materials[mat_iter->second.Name] = mat_iter->second;
				iter->Material = MaterialManager.GetMaterial(mat_iter->second.Name.getData());
			}
		}
	}
}

void StdMeshMaterialUpdate::Update(StdMeshInstance* instance) const
{
	UpdateSingle(instance);

	for(StdMeshInstance::AttachedMeshIter iter = instance->AttachedMeshesBegin(); iter != instance->AttachedMeshesEnd(); ++iter)
		Update((*iter)->Child);
}

void StdMeshMaterialUpdate::Cancel() const
{
	// Reset all materials in manager
	for(std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator iter = Materials.begin(); iter != Materials.end(); ++iter)
		MaterialManager.Materials[iter->second.Name] = iter->second;
}

void StdMeshMaterialUpdate::UpdateSingle(StdMeshInstance* instance) const
{
	for(unsigned int i = 0; i < instance->SubMeshInstances.size(); ++i) //std::vector<StdSubMeshInstance*>::iterator iter = instance->SubMeshInstances->begin(); iter != instance->SubMeshInstances->end(); ++iter)
	{
		StdSubMeshInstance* sub_instance = instance->SubMeshInstances[i];
		std::map<const StdMeshMaterial*, StdMeshMaterial>::const_iterator mat_iter =	Materials.find(sub_instance->Material);
		if(mat_iter != Materials.end())
		{
			// Material needs to be updated
			const StdMeshMaterial* new_material = MaterialManager.GetMaterial(mat_iter->second.Name.getData());
			// If new material is not available, fall back to StdMesh (definition) material
			if(!new_material) new_material = instance->Mesh.GetSubMesh(i).Material;
			sub_instance->SetMaterial(*new_material);
		}
	}
		std::vector<StdSubMeshInstance*> SubMeshInstances;
}

void StdMeshMaterialUpdate::Add(const StdMeshMaterial* material)
{
	assert(Materials.find(material) == Materials.end());
	Materials[material] = *material;
}
