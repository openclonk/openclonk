/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

#include "C4Include.h"
#include "object/C4MeshDenumerator.h"

#include "object/C4Def.h"
#include "object/C4Object.h"

const StdMeshInstance::AttachedMesh::DenumeratorFactoryFunc C4MeshDenumeratorFactory = StdMeshInstance::AttachedMesh::DenumeratorFactory<C4MeshDenumerator>;

void C4MeshDenumerator::CompileFunc(StdCompiler* pComp, StdMeshInstance::AttachedMesh* attach)
{
	if (pComp->isDeserializer())
	{
		int32_t def;
		pComp->Value(mkNamingCountAdapt(def, "ChildInstance"));

		if (def)
		{
			C4DefGraphics* pGfx = nullptr;
			pComp->Value(mkNamingAdapt(C4DefGraphicsAdapt(pGfx), "ChildMesh"));
			Def = pGfx->pDef;

			if (pGfx->Type != C4DefGraphics::TYPE_Mesh)
			{
				pComp->excCorrupt("ChildMesh points to non-mesh graphics");
			}
			assert(!attach->Child);
			pComp->Value(mkParAdapt(mkNamingContextPtrAdapt(attach->Child, *pGfx->Mesh, "ChildInstance"), C4MeshDenumeratorFactory));
			assert(attach->Child != nullptr);
			attach->OwnChild = true; // Delete the newly allocated child instance when the parent instance is gone

			// TODO: Do we leak pGfx?
		}
		else
		{
			pComp->Value(mkNamingAdapt(Object, "ChildObject"));
			attach->OwnChild = false; // Keep child instance when parent instance is gone since it belongs to a different object
		}
	}
	else
	{
		int32_t def = 0;
		if (Def)
		{
			++def;
		}
		pComp->Value(mkNamingCountAdapt(def, "ChildInstance"));

		if (Def)
		{
			assert(attach->OwnChild);
			C4DefGraphics* pGfx = &Def->Graphics;
			assert(pGfx->Type == C4DefGraphics::TYPE_Mesh);
			pComp->Value(mkNamingAdapt(C4DefGraphicsAdapt(pGfx), "ChildMesh"));
			pComp->Value(mkParAdapt(mkNamingContextPtrAdapt(attach->Child, *pGfx->Mesh, "ChildInstance"), C4MeshDenumeratorFactory));
		}
		else
		{
			assert(!attach->OwnChild);
			pComp->Value(mkNamingAdapt(Object, "ChildObject"));
		}
	}
}

void C4MeshDenumerator::DenumeratePointers(StdMeshInstance::AttachedMesh* attach)
{
	Object.DenumeratePointers();

	// Set child instance of attach after denumeration
	if (Object)
	{
		assert(!attach->OwnChild);
		assert(!attach->Child || attach->Child == Object->pMeshInstance);
		if (!attach->Child)
		{
			attach->Child = Object->pMeshInstance;
		}
	}
}

bool C4MeshDenumerator::ClearPointers(C4Object* pObj)
{
	if (Object == pObj)
	{
		Object = nullptr;
		// Return false causes the attached mesh to be deleted by StdMeshInstance
		return false;
	}

	return true;
}
