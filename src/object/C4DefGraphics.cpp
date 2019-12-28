/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// graphics used by object definitions (object and portraits)

#include "C4Include.h"
#include "object/C4DefGraphics.h"

#include "c4group/C4Components.h"
#include "game/C4Application.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Surface.h"
#include "gui/C4Menu.h"
#include "landscape/C4Material.h"
#include "lib/StdMeshLoader.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4MeshAnimation.h"
#include "object/C4Object.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4RankSystem.h"

//-------------------------------- C4DefGraphics -----------------------------------------------

C4DefGraphics::C4DefGraphics(C4Def *pOwnDef)
{
	// store def
	pDef = pOwnDef;
	// zero fields
	Type = TYPE_None;
	Bmp.Bitmap = Bmp.BitmapClr = Bmp.BitmapNormal = nullptr;
	pNext = nullptr;
	fColorBitmapAutoCreated = false;
}

C4DefGraphics *C4DefGraphics::GetLast()
{
	C4DefGraphics *pLast = this;
	while (pLast->pNext) pLast = pLast->pNext;
	return pLast;
}

void C4DefGraphics::Clear()
{
	// zero own fields
	switch (Type)
	{
	case TYPE_None:
		break;
	case TYPE_Bitmap:
		if (Bmp.BitmapNormal) { delete Bmp.BitmapNormal; Bmp.BitmapNormal=nullptr; }
		if (Bmp.BitmapClr) { delete Bmp.BitmapClr; Bmp.BitmapClr=nullptr; }
		if (Bmp.Bitmap) { delete Bmp.Bitmap; Bmp.Bitmap=nullptr; }
		break;
	case TYPE_Mesh:
		if (Mesh) { delete Mesh; Mesh = nullptr; }
		break;
	}
	Type = TYPE_None;

	// delete additonal graphics
	C4AdditionalDefGraphics *pGrp2N = pNext, *pGrp2;
	while ((pGrp2=pGrp2N)) { pGrp2N = pGrp2->pNext; pGrp2->pNext = nullptr; delete pGrp2; }
	pNext = nullptr; fColorBitmapAutoCreated = false;
}

bool C4DefGraphics::LoadBitmap(C4Group &hGroup, const char *szFilename, const char *szOverlay, const char *szNormal, bool fColorByOwner)
{
	if (!szFilename) return false;
	Type = TYPE_Bitmap; // will be reset to TYPE_None in Clear() if loading fails
	Bmp.Bitmap = new C4Surface();
	if (!Bmp.Bitmap->Load(hGroup, szFilename, false, true, C4SF_MipMap))
	{
		Clear();
		return false;
	}

	// Create owner color bitmaps
	if (fColorByOwner)
	{
		// Create additionmal bitmap
		Bmp.BitmapClr=new C4Surface();
		// if overlay-surface is present, load from that
		if (szOverlay && Bmp.BitmapClr->Load(hGroup, szOverlay, false, false, C4SF_MipMap))
		{
			// set as Clr-surface, also checking size
			if (!Bmp.BitmapClr->SetAsClrByOwnerOf(Bmp.Bitmap))
			{
				DebugLogF("    Gfx loading error in %s: %s (%d x %d) doesn't match overlay %s (%d x %d) - invalid file or size mismatch",
				          hGroup.GetFullName().getData(), szFilename, Bmp.Bitmap ? Bmp.Bitmap->Wdt : -1, Bmp.Bitmap ? Bmp.Bitmap->Hgt : -1,
				          szOverlay, Bmp.BitmapClr->Wdt, Bmp.BitmapClr->Hgt);
				Clear();
				return false;
			}
		}
		else
		{
			// otherwise, create by all blue shades
			if (!Bmp.BitmapClr->CreateColorByOwner(Bmp.Bitmap))
			{
				Clear();
				return false;
			}
		}
		fColorBitmapAutoCreated = true;
	}

	if (szNormal)
	{
		Bmp.BitmapNormal = new C4Surface();
		if (Bmp.BitmapNormal->Load(hGroup, szNormal, false, true, C4SF_MipMap))
		{
			// Normal map loaded. Sanity check and link.
			if(Bmp.BitmapNormal->Wdt != Bmp.Bitmap->Wdt ||
			   Bmp.BitmapNormal->Hgt != Bmp.Bitmap->Hgt)
			{
				DebugLogF("    Gfx loading error in %s: %s (%d x %d) doesn't match normal %s (%d x %d) - invalid file or size mismatch",
				          hGroup.GetFullName().getData(), szFilename, Bmp.Bitmap ? Bmp.Bitmap->Wdt : -1, Bmp.Bitmap ? Bmp.Bitmap->Hgt : -1,
				          szNormal, Bmp.BitmapNormal->Wdt, Bmp.BitmapNormal->Hgt);
				Clear();
				return false;
			}

			Bmp.Bitmap->pNormalSfc = Bmp.BitmapNormal;
			if(Bmp.BitmapClr) Bmp.BitmapClr->pNormalSfc = Bmp.BitmapNormal;
		}
		else
		{
			// No normal map
			delete Bmp.BitmapNormal;
			Bmp.BitmapNormal = nullptr;
		}
	}

	Type = TYPE_Bitmap;
	// success
	return true;
}

bool C4DefGraphics::LoadMesh(C4Group &hGroup, const char* szFileName, StdMeshSkeletonLoader& loader)
{
	char* buf = nullptr;
	size_t size;

	try
	{
		if(!hGroup.LoadEntry(szFileName, &buf, &size, 1)) return false;

		if (SEqualNoCase(GetExtension(szFileName), "xml"))
		{
			Mesh = StdMeshLoader::LoadMeshXml(buf, size, ::MeshMaterialManager, loader, hGroup.GetName());
		}
		else
		{
			Mesh = StdMeshLoader::LoadMeshBinary(buf, size, ::MeshMaterialManager, loader, hGroup.GetName());
		}
		delete[] buf;

		Mesh->SetLabel(pDef->id.ToString());

		// order submeshes
		Mesh->PostInit();
	}
	catch (const std::runtime_error& ex)
	{
		DebugLogF("Failed to load mesh in definition %s: %s", hGroup.GetName(), ex.what());
		delete[] buf;
		return false;
	}

	Type = TYPE_Mesh;
	return true;
}

bool C4DefGraphics::LoadSkeleton(C4Group &hGroup, const char* szFileName, StdMeshSkeletonLoader& loader)
{
	char* buf = nullptr;
	size_t size;

	try
	{
		if (!hGroup.LoadEntry(szFileName, &buf, &size, 1)) return false;

		// delete skeleton from the map for reloading, or else if you delete or rename
		// a skeleton file in the folder the old skeleton will still exist in the map
		loader.RemoveSkeleton(hGroup.GetName(), szFileName);

		if (SEqualNoCase(GetExtension(szFileName), "xml"))
		{
			loader.LoadSkeletonXml(hGroup.GetName(), szFileName, buf, size);
		}
		else
		{
			loader.LoadSkeletonBinary(hGroup.GetName(), szFileName, buf, size);
		}

		delete[] buf;
	}
	catch (const std::runtime_error& ex)
	{
		DebugLogF("Failed to load skeleton in definition %s: %s", hGroup.GetName(), ex.what());
		delete[] buf;
		return false;
	}

	return true;
}

bool C4DefGraphics::Load(C4Group &hGroup, StdMeshSkeletonLoader &loader, bool fColorByOwner)
{
	char Filename[_MAX_PATH_LEN]; *Filename=0;

	// load skeletons
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry("*", Filename, nullptr, !!*Filename))
	{
		if (!WildcardMatch(C4CFN_DefSkeleton, Filename) && !WildcardMatch(C4CFN_DefSkeletonXml, Filename)) continue;
		LoadSkeleton(hGroup, Filename, loader);
	}

	// Try from Mesh first
	if (!LoadMesh(hGroup, C4CFN_DefMesh, loader))
		if(!LoadMesh(hGroup, C4CFN_DefMeshXml, loader))
			LoadBitmap(hGroup, C4CFN_DefGraphics, C4CFN_ClrByOwner, C4CFN_NormalMap, fColorByOwner);

	// load additional graphics
	C4DefGraphics *pLastGraphics = this;
	const int32_t iOverlayWildcardPos = SCharPos('*', C4CFN_ClrByOwnerEx);
	hGroup.ResetSearch(); *Filename=0;
	const char* const AdditionalGraphics[] = { C4CFN_DefGraphicsEx, C4CFN_DefGraphicsExMesh, C4CFN_DefGraphicsExMeshXml, nullptr };
	while (hGroup.FindNextEntry("*", Filename, nullptr, !!*Filename))
	{
		for(const char* const* szWildcard = AdditionalGraphics; *szWildcard != nullptr; ++szWildcard)
		{
			if(!WildcardMatch(*szWildcard, Filename)) continue;
			// skip def graphics
			if (SEqualNoCase(Filename, C4CFN_DefGraphics) || SEqualNoCase(Filename, C4CFN_DefMesh) || SEqualNoCase(Filename, C4CFN_DefMeshXml)) continue;
			// skip scaled def graphics
			if (WildcardMatch(C4CFN_DefGraphicsScaled, Filename)) continue;
			// get name
			char GrpName[_MAX_PATH_LEN];
			const int32_t iWildcardPos = SCharPos('*', *szWildcard);
			SCopy(Filename + iWildcardPos, GrpName, _MAX_PATH);
			RemoveExtension(GrpName);
			// remove trailing number for scaled graphics
			int32_t extpos; int scale;
			if ((extpos = SCharLastPos('.', GrpName)) > -1)
				if (sscanf(GrpName+extpos+1, "%d", &scale) == 1)
					GrpName[extpos] = '\0';
			// clip to max length
			GrpName[C4MaxName]=0;
			// create new graphics
			pLastGraphics->pNext = new C4AdditionalDefGraphics(pDef, GrpName);
			pLastGraphics = pLastGraphics->pNext;
			if(*szWildcard == AdditionalGraphics[0])
			{
				// create overlay-filename
				char OverlayFn[_MAX_PATH_LEN];
				if(fColorByOwner)
				{
					// GraphicsX.png -> OverlayX.png
					SCopy(C4CFN_ClrByOwnerEx, OverlayFn, _MAX_PATH);
					OverlayFn[iOverlayWildcardPos]=0;
					SAppend(Filename + iWildcardPos, OverlayFn);
					EnforceExtension(OverlayFn, GetExtension(C4CFN_ClrByOwnerEx));
				}

				// create normal filename
				char NormalFn[_MAX_PATH_LEN];
				SCopy(C4CFN_NormalMapEx, NormalFn, _MAX_PATH);
				NormalFn[iOverlayWildcardPos]=0;
				SAppend(Filename + iWildcardPos, NormalFn);
				EnforceExtension(NormalFn, GetExtension(C4CFN_NormalMapEx));

				// load them
				if (!pLastGraphics->LoadBitmap(hGroup, Filename, fColorByOwner ? OverlayFn : nullptr, NormalFn, fColorByOwner))
					return false;
			}
			else
			{
				if (!pLastGraphics->LoadMesh(hGroup, Filename, loader))
					return false;
			}
		}
	}
	// done, success
	return true;
}

C4DefGraphics *C4DefGraphics::Get(const char *szGrpName)
{
	// no group or empty string: base graphics
	if (!szGrpName || !szGrpName[0]) return this;
	// search additional graphics
	for (C4AdditionalDefGraphics *pGrp = pNext; pGrp; pGrp=pGrp->pNext)
		if (SEqualNoCase(pGrp->GetName(), szGrpName)) return pGrp;
	// nothing found
	return nullptr;
}

void C4DefGraphics::Draw(C4Facet &cgo, DWORD iColor, C4Object *pObj, int32_t iPhaseX, int32_t iPhaseY, C4DrawTransform* trans)
{
	// default: def picture rect
	C4Rect fctPicRect = pDef->PictureRect;
	C4Facet fctPicture;

	// if assigned: use object specific rect and graphics
	if (pObj) if (pObj->PictureRect.Wdt) fctPicRect = pObj->PictureRect;

	// specific object color?
	if (pObj) pObj->PrepareDrawing();

	switch(Type)
	{
	case C4DefGraphics::TYPE_None:
		// Def has no graphics
		break;
	case C4DefGraphics::TYPE_Bitmap:
		fctPicture.Set(GetBitmap(iColor),fctPicRect.x,fctPicRect.y,fctPicRect.Wdt,fctPicRect.Hgt);
		fctPicture.DrawTUnscaled(cgo,true,iPhaseX,iPhaseY,trans);
		break;
	case C4DefGraphics::TYPE_Mesh:
		// TODO: Allow rendering of a mesh directly, without instance (to render pose; no animation)
		std::unique_ptr<StdMeshInstance> dummy;
		StdMeshInstance* instance;

		C4Value value;
		if (pObj)
		{
			instance = pObj->pMeshInstance;
			pObj->GetProperty(P_PictureTransformation, &value);
		}
		else
		{
			dummy = std::make_unique<StdMeshInstance>(*Mesh, 1.0f);
			instance = dummy.get();
			pDef->GetProperty(P_PictureTransformation, &value);
		}

		StdMeshMatrix matrix;
		if (C4ValueToMatrix(value, &matrix))
			pDraw->SetMeshTransform(&matrix);

		pDraw->SetPerspective(true);
		pDraw->RenderMesh(*instance, cgo.Surface, cgo.X,cgo.Y, cgo.Wdt, cgo.Hgt, pObj ? pObj->Color : iColor, trans);
		pDraw->SetPerspective(false);
		pDraw->SetMeshTransform(nullptr);

		break;
	}

	if (pObj) pObj->FinishedDrawing();

	// draw overlays
	if (pObj && pObj->pGfxOverlay)
		for (C4GraphicsOverlay *pGfxOvrl = pObj->pGfxOverlay; pGfxOvrl; pGfxOvrl = pGfxOvrl->GetNext())
			if (pGfxOvrl->IsPicture())
				pGfxOvrl->DrawPicture(cgo, pObj, trans);
}

void C4DefGraphics::DrawClr(C4Facet &cgo, bool fAspect, DWORD dwClr)
{
	if (Type != TYPE_Bitmap) return; // TODO
	// create facet and draw it
	C4Surface *pSfc = Bmp.BitmapClr ? Bmp.BitmapClr : Bmp.Bitmap; if (!pSfc) return;
	C4Facet fct(pSfc, 0,0,pSfc->Wdt, pSfc->Hgt);
	fct.DrawClr(cgo, fAspect, dwClr);
}

void C4DefGraphicsAdapt::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	// nothing?
	if (!deserializing && !pDefGraphics) return;
	// definition
	C4ID id; if (!deserializing) id = pDefGraphics->pDef->id;
	pComp->Value(id);
	// go over two separators ("::"). Expect them if an id was found.
	if (!pComp->Separator(StdCompiler::SEP_PART2) || !pComp->Separator(StdCompiler::SEP_PART2))
		pComp->excCorrupt(R"(DefGraphics: expected "::")");
	// compile name
	StdStrBuf Name; if (!deserializing) Name = pDefGraphics->GetName();
	pComp->Value(mkDefaultAdapt(mkParAdapt(Name, StdCompiler::RCT_Idtf), ""));
	// reading: search def-graphics
	if (deserializing)
	{
		// search definition, throw expection if not found
		C4Def *pDef = ::Definitions.ID2Def(id);
		// search def-graphics
		if (!pDef || !( pDefGraphics = pDef->Graphics.Get(Name.getData()) ))
			pComp->excCorrupt(R"(DefGraphics: could not find graphics "%s" in %s(%s)!)", Name.getData(), id.ToString(), pDef ? pDef->GetName() : "def not found");
	}
}

C4AdditionalDefGraphics::C4AdditionalDefGraphics(C4Def *pOwnDef, const char *szName) : C4DefGraphics(pOwnDef)
{
	// store name
	SCopy(szName, Name, C4MaxName);
}

// ---------------------------------------------------------------------------
// C4DefGraphicsPtrBackup: Functionality to reload def graphics at runtime

C4DefGraphicsPtrBackupEntry::C4DefGraphicsPtrBackupEntry(C4DefGraphics *pSourceGraphics):
	pMeshUpdate(nullptr)
{
	// assign graphics + def
	pGraphicsPtr = pSourceGraphics;
	pDef = pSourceGraphics->pDef;
	// assign name
	const char *szName = pGraphicsPtr->GetName();
	if (szName) SCopy(szName, Name, C4MaxName); else *Name=0;

	// assign mesh update
	if(pSourceGraphics->Type == C4DefGraphics::TYPE_Mesh)
		pMeshUpdate = new StdMeshUpdate(*pSourceGraphics->Mesh);
}

C4DefGraphicsPtrBackupEntry::~C4DefGraphicsPtrBackupEntry()
{
	// graphics ptr still assigned? then remove dead graphics pointers from objects
	if (pGraphicsPtr) AssignRemoval();
	delete pMeshUpdate;
}

void C4DefGraphicsPtrBackupEntry::AssignUpdate()
{
	// Update all attached meshes that were using this mesh
	UpdateAttachedMeshes();

	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		for (C4Object *pObj : Objects)
		{
			if (pObj && pObj->Status)
				{
					if (pObj->pGraphics == pGraphicsPtr)
					{
						// same graphics found. Update mesh graphics if any.
						if(pMeshUpdate)
						{
							assert(pObj->pMeshInstance != nullptr); // object had mesh graphics, so mesh instance should be present
							assert(&pObj->pMeshInstance->GetMesh() == &pMeshUpdate->GetOldMesh()); // mesh instance of correct type even

							// Get new mesh from reloaded graphics
							C4DefGraphics *pGrp = pDef->Graphics.Get(Name);
							if(pGrp && pGrp->Type == C4DefGraphics::TYPE_Mesh)
								pMeshUpdate->Update(pObj->pMeshInstance, *pGrp->Mesh);
						}

						// try to set new graphics
						if (!pObj->SetGraphics(Name, pDef))
							if (!pObj->SetGraphics(Name, pObj->Def))
							{
								// shouldn't happen
								pObj->AssignRemoval(); pObj->pGraphics=nullptr;
							}
					}

					// remove any overlay graphics
					for (;;)
					{
						C4GraphicsOverlay *pGfxOverlay;
						for (pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
							if (pGfxOverlay->GetGfx() == pGraphicsPtr)
							{
								// then remove this overlay and redo the loop, because iterator has become invalid
								pObj->RemoveGraphicsOverlay(pGfxOverlay->GetID());
								break;
							}
						// looped through w/o removal?
						if (!pGfxOverlay) break;
					}
					// update menu frame decorations - may do multiple updates to the same deco if multiple menus share it...
					C4GUI::FrameDecoration *pDeco;
					if (pDef && pObj->Menu && (pDeco = pObj->Menu->GetFrameDecoration()))
						if (pDeco->idSourceDef == pDef->id)
							if (!pDeco->UpdateGfx())
								pObj->Menu->SetFrameDeco(nullptr);
				}
		}
		// done; reset field to indicate finished update
		pGraphicsPtr = nullptr;
	}
}

void C4DefGraphicsPtrBackupEntry::AssignRemoval()
{
	// Remove all attached meshes that were using this mesh
	UpdateAttachedMeshes();

	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		for (C4Object *pObj : Objects)
			if (pObj && pObj->Status)
				{
					if (pObj->pGraphics == pGraphicsPtr)
					{
						// same graphics found. If these are mesh graphics then remove
						// the object because the StdMesh has already been unloaded.
						if(pObj->pMeshInstance)
						{
							assert(&pObj->pMeshInstance->GetMesh() == &pMeshUpdate->GetOldMesh());

							pObj->AssignRemoval();
							delete pObj->pMeshInstance;
							pObj->pMeshInstance = nullptr;
							pObj->pGraphics = nullptr;
						}
						// sprite graphics; reset them
						else if (!pObj->SetGraphics()) { pObj->AssignRemoval(); pObj->pGraphics=nullptr; }
					}
					// remove any overlay graphics
					for (;;)
					{
						C4GraphicsOverlay *pGfxOverlay;
						for (pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
							if (pGfxOverlay->GetGfx() == pGraphicsPtr)
							{
								// then remove this overlay and redo the loop, because iterator has become invalid
								pObj->RemoveGraphicsOverlay(pGfxOverlay->GetID());
								break;
							}
						// looped through w/o removal?
						if (!pGfxOverlay) break;
					}
					// remove menu frame decorations
					C4GUI::FrameDecoration *pDeco;
					if (pDef && pObj->Menu && (pDeco = pObj->Menu->GetFrameDecoration()))
						if (pDeco->idSourceDef == pDef->id)
							pObj->Menu->SetFrameDeco(nullptr);
				}
		// done; reset field to indicate finished update
		pGraphicsPtr = nullptr;
	}
}

void C4DefGraphicsPtrBackupEntry::UpdateAttachedMeshes()
{
	for (C4Object *pObj : Objects)
	{
		if (pObj && pObj->Status)
		{
			if(pObj->pMeshInstance)
				UpdateAttachedMesh(pObj->pMeshInstance);
			for (C4GraphicsOverlay* pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
				if(pGfxOverlay->pMeshInstance)
					UpdateAttachedMesh(pGfxOverlay->pMeshInstance);
		}
	}
}

void C4DefGraphicsPtrBackupEntry::UpdateAttachedMesh(StdMeshInstance* instance)
{
	// Update if instance is an owned attached mesh
	if(pMeshUpdate &&
	   &instance->GetMesh() == &pMeshUpdate->GetOldMesh() &&
	   instance->GetAttachParent() &&
	   instance->GetAttachParent()->OwnChild)
	{
		C4DefGraphics *pGrp = pDef->Graphics.Get(Name); // null for failed def. reload
		if(pGrp && pGrp->Type == C4DefGraphics::TYPE_Mesh)
		{
			pMeshUpdate->Update(instance, *pGrp->Mesh); // might detach from parent
		}
		else
		{
			instance->GetAttachParent()->Parent->DetachMesh(instance->GetAttachParent()->Number);
		}
	}
	// Non-attached meshes and unowned attached meshes are updated in
	// AssignUpdate or AssignRemoval, respectively, since they are
	// contained in the object list.

	// Copy the attached mesh list before recursion because the recursive
	// call might detach meshes, altering the list and invalidating
	// iterators.
	std::vector<StdMeshInstance::AttachedMesh*> attached_meshes;
	for(StdMeshInstance::AttachedMeshIter iter = instance->AttachedMeshesBegin(); iter != instance->AttachedMeshesEnd(); ++iter)
		attached_meshes.push_back(*iter);

	for(auto & attached_mesh : attached_meshes)
		// TODO: Check that this mesh is still attached?
		UpdateAttachedMesh(attached_mesh->Child);
}

C4DefGraphicsPtrBackup::C4DefGraphicsPtrBackup():
	MeshMaterialUpdate(::MeshMaterialManager),
	MeshAnimationUpdate(::Definitions.GetSkeletonLoader())
{
}

C4DefGraphicsPtrBackup::~C4DefGraphicsPtrBackup()
{
	if(!fApplied) AssignRemoval();

	for(auto & Entry : Entries)
		delete Entry;
}

void C4DefGraphicsPtrBackup::Add(C4DefGraphics* pGfx)
{
	for(C4DefGraphics* pCur = pGfx; pCur != nullptr; pCur = pCur->pNext)
		Entries.push_back(new C4DefGraphicsPtrBackupEntry(pCur));

	// Remove all mesh materials that were loaded from this definition
	C4Def* pDef = pGfx->pDef;
	for(StdMeshMatManager::Iterator iter = ::MeshMaterialManager.Begin(); iter != MeshMaterialManager.End(); )
	{
		StdStrBuf Filename;
		Filename.Copy(pDef->Filename);
		Filename.Append("/"); Filename.Append(GetFilename(iter->FileName.getData()));

		if(Filename == iter->FileName)
			iter = ::MeshMaterialManager.Remove(iter, &MeshMaterialUpdate);
		else
			++iter;
	}
}

void C4DefGraphicsPtrBackup::AssignRemoval()
{
	MeshMaterialUpdate.Cancel();

	// Remove gfx
	for(auto & Entry : Entries)
		Entry->AssignRemoval();

	fApplied = true;
}

void C4DefGraphicsPtrBackup::AssignUpdate()
{
	// Update mesh materials for all meshes
	for(auto & iter : Definitions.table)
		if(iter.second->Graphics.Type == C4DefGraphics::TYPE_Mesh)
			MeshMaterialUpdate.Update(iter.second->Graphics.Mesh);

	// Then, update mesh references in instances, attach bones by name, and update sprite gfx
	for(auto & Entry : Entries)
		Entry->AssignUpdate();

	// Update mesh materials and animations for all mesh instances.
	for (C4Object *pObj : Objects)
	{
		if (pObj && pObj->Status)
		{
			if(pObj->pMeshInstance)
				UpdateMesh(pObj->pMeshInstance);
			for (C4GraphicsOverlay* pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
				if(pGfxOverlay->pMeshInstance)
					UpdateMesh(pGfxOverlay->pMeshInstance);
		}
	}

	fApplied = true;
}

void C4DefGraphicsPtrBackup::UpdateMesh(StdMeshInstance* instance)
{
	MeshMaterialUpdate.Update(instance);
	MeshAnimationUpdate.Update(instance);

	// Recursive for attached meshes not in object list
	for (StdMeshInstance::AttachedMeshIter iter = instance->AttachedMeshesBegin(); iter != instance->AttachedMeshesEnd(); ++iter)
		if ((*iter)->OwnChild)
			UpdateMesh((*iter)->Child);
}

// ---------------------------------------------------------------------------
// C4GraphicsOverlay: graphics overlay used to attach additional graphics to objects

C4GraphicsOverlay::~C4GraphicsOverlay()
{
	// Free mesh instance
	delete pMeshInstance; pMeshInstance = nullptr;
	// free any additional overlays
	C4GraphicsOverlay *pNextOther = pNext, *pOther;
	while ((pOther = pNextOther))
	{
		pNextOther = pOther->pNext;
		pOther->pNext = nullptr;
		delete pOther;
	}
}

void C4GraphicsOverlay::UpdateFacet()
{
	// special: Nothing to update for object and pSourceGfx may be nullptr
	// If there will ever be something to init here, UpdateFacet() will also need to be called when objects have been loaded
	if (eMode == MODE_Object) return;
	// otherwise, source graphics must be specified
	if (!pSourceGfx) return;
	C4Def *pDef = pSourceGfx->pDef;
	assert(pDef);
	fZoomToShape = false;
	// Clear old mesh instance, if any
	delete pMeshInstance; pMeshInstance = nullptr;
	// update by mode
	switch (eMode)
	{
	case MODE_None:
		break;

	case MODE_Base: // def base graphics
		if (pSourceGfx->Type == C4DefGraphics::TYPE_Bitmap)
			fctBlit.Set(pSourceGfx->GetBitmap(), 0, 0, pDef->Shape.Wdt, pDef->Shape.Hgt, pDef->Shape.x+pDef->Shape.Wdt/2, pDef->Shape.y+pDef->Shape.Hgt/2);
		else if (pSourceGfx->Type == C4DefGraphics::TYPE_Mesh)
			pMeshInstance = new StdMeshInstance(*pSourceGfx->Mesh, 1.0f);
		break;

	case MODE_Action: // graphics of specified action
	{
		// Clear old facet
		fctBlit.Default();

		// Ensure there is actually an action set
		if (!Action[0])
			return;

		C4Value v;
		pDef->GetProperty(P_ActMap, &v);
		C4PropList *actmap = v.getPropList();
		if (!actmap)
			return;

		actmap->GetPropertyByS(::Strings.RegString(Action), &v);
		C4PropList *action = v.getPropList();
		if (!action)
			return;

		if (pSourceGfx->Type == C4DefGraphics::TYPE_Bitmap)
		{
			fctBlit.Set(pSourceGfx->GetBitmap(),
			            action->GetPropertyInt(P_X), action->GetPropertyInt(P_Y),
			            action->GetPropertyInt(P_Wdt), action->GetPropertyInt(P_Hgt));
			// FIXME: fctBlit.TargetX has to be set here
		}
		else if (pSourceGfx->Type == C4DefGraphics::TYPE_Mesh)
		{
			C4String* AnimationName = action->GetPropertyStr(P_Animation);
			if (!AnimationName) return;

			pMeshInstance = new StdMeshInstance(*pSourceGfx->Mesh, 1.0f);
			const StdMeshAnimation* Animation = pSourceGfx->Mesh->GetSkeleton().GetAnimationByName(AnimationName->GetData());
			if (!Animation) return;

			pMeshInstance->PlayAnimation(*Animation, 0, nullptr, new C4ValueProviderRef<int32_t>(iPhase, ftofix(Animation->Length / action->GetPropertyInt(P_Length))), new C4ValueProviderConst(itofix(1)), true);
		}

		break;
	}
	case MODE_ObjectPicture: // ingame picture of object
		// calculated at runtime
		break;

	case MODE_IngamePicture:
	case MODE_Picture: // def picture
		fZoomToShape = true;
		// drawn at runtime
		break;

	case MODE_ExtraGraphics: // like ColorByOwner-sfc
		// calculated at runtime
		break;

	case MODE_Rank:
		// drawn at runtime
		break;

	case MODE_Object:
		// TODO
		break;
	}
}

void C4GraphicsOverlay::Set(Mode aMode, C4DefGraphics *pGfx, const char *szAction, DWORD dwBMode, C4Object *pOvrlObj)
{
	// set values
	eMode = aMode;
	pSourceGfx = pGfx;
	if (szAction) SCopy(szAction, Action, C4MaxName); else *Action=0;
	dwBlitMode = dwBMode;
	OverlayObj = pOvrlObj;
	// (keep transform)
	// reset phase
	iPhase = 0;
	// update used facet
	UpdateFacet();
}

bool C4GraphicsOverlay::IsValid(const C4Object *pForObj) const
{
	assert(pForObj);
	if (eMode == MODE_Object || eMode == MODE_Rank || eMode == MODE_ObjectPicture)
	{
		if (!OverlayObj || !OverlayObj->Status) return false;
		return (eMode == MODE_Rank) || !OverlayObj->HasGraphicsOverlayRecursion(pForObj);
	}
	else if (eMode == MODE_ExtraGraphics)
	{
		return !!pSourceGfx;
	}
	else if (pSourceGfx)
	{
		if(eMode == MODE_Picture || eMode == MODE_IngamePicture)
			return true;
		else if (pSourceGfx->Type == C4DefGraphics::TYPE_Bitmap)
			return !!fctBlit.Surface;
		else if (pSourceGfx->Type == C4DefGraphics::TYPE_Mesh)
			return !!pMeshInstance;
		return false;
	}
	else
	{
		return false;
	}
}

void C4GraphicsOverlay::CompileFunc(StdCompiler *pComp)
{
	// read ID
	pComp->Value(iID); pComp->Separator();
	// read def-graphics
	pComp->Value(mkDefaultAdapt(C4DefGraphicsAdapt(pSourceGfx), (C4DefGraphics *)nullptr));
	pComp->Separator();
	// read mode
	pComp->Value(mkIntAdapt(eMode)); pComp->Separator();
	// read action (identifier)
	pComp->Value(mkStringAdaptMIE(Action)); pComp->Separator();
	// read blit mode
	pComp->Value(dwBlitMode); pComp->Separator();
	// read phase
	pComp->Value(iPhase); pComp->Separator();
	// read transform
	pComp->Separator(StdCompiler::SEP_START);
	pComp->Value(Transform);
	pComp->Separator(StdCompiler::SEP_END);
	// read color-modulation
	if (pComp->Separator())
		pComp->Value(mkIntAdapt(dwClrModulation));
	else
		// default
		if (pComp->isDeserializer()) dwClrModulation = 0xffffff;
	// read overlay target object
	if (pComp->Separator())
		pComp->Value(OverlayObj);
	else
		// default
		if (pComp->isDeserializer()) OverlayObj = nullptr;
	// update used facet according to read data
	if (pComp->isDeserializer()) UpdateFacet();
}

void C4GraphicsOverlay::DenumeratePointers()
{
	OverlayObj.DenumeratePointers();
}

void C4GraphicsOverlay::Draw(C4TargetFacet &cgo, C4Object *pForObj, int32_t iByPlayer)
{
	assert(!IsPicture());
	// note: Also called with pForObj==nullptr for editor placement preview
	// get target pos
	float offX, offY;
	float newzoom;
	if (pForObj)
	{
		pForObj->GetDrawPosition(cgo, offX, offY, newzoom);
	}
	else
	{
		// offset in editor mode preview
		offX = cgo.X;
		offY = cgo.Y;
		newzoom = cgo.Zoom;
	}
	ZoomDataStackItem zdsi(newzoom);

	// special blit mode
	if (dwBlitMode == C4GFXBLIT_PARENT)
	{
		assert(pForObj);
		(OverlayObj ? static_cast<C4Object*>(OverlayObj) : pForObj)->PrepareDrawing();
	}
	else
	{
		pDraw->SetBlitMode(dwBlitMode);
		if (dwClrModulation != 0xffffff) pDraw->ActivateBlitModulation(dwClrModulation);

		if (pMeshInstance)
			pMeshInstance->SetFaceOrderingForClrModulation(dwClrModulation);
	}
	if (eMode == MODE_Rank)
	{
		assert(pForObj);
		C4TargetFacet ccgo;
		ccgo.Set(cgo.Surface, offX+pForObj->Shape.x,offY+pForObj->Shape.y,pForObj->Shape.Wdt,pForObj->Shape.Hgt, cgo.TargetX, cgo.TargetY);
		DrawRankSymbol(ccgo, OverlayObj);
	}
	// drawing specific object?
	else if (OverlayObj)
	{
		assert(pForObj);
		// TODO: Shouldn't have called PrepareDrawing/set ClrModulation here, since
		// OverlayObj drawing will do it on its own.
		if (eMode == MODE_ObjectPicture)
		{
			C4Facet fctTarget;
			fctTarget.Set(cgo.Surface, offX+pForObj->Shape.x, offY+pForObj->Shape.y, pForObj->Shape.Wdt, pForObj->Shape.Hgt);

			OverlayObj->DrawPicture(fctTarget, false, &C4DrawTransform(Transform, fctTarget.X+float(fctTarget.Wdt)/2, fctTarget.Y+float(fctTarget.Hgt)/2));
		}
		else
		{
			// Draw specified object at target pos of this object; offset by transform.
			OverlayObj->Draw(cgo, iByPlayer, C4Object::ODM_Overlay, offX + Transform.GetXOffset(), offY + Transform.GetYOffset());
			OverlayObj->DrawTopFace(cgo, iByPlayer, C4Object::ODM_Overlay, offX + Transform.GetXOffset(), offY + Transform.GetYOffset());
		}
	}
	else if (eMode == MODE_ExtraGraphics)
	{
		assert(pForObj);
		// draw self with specified gfx
		if (pSourceGfx)
		{
			C4DefGraphics *pPrevGfx = pForObj->GetGraphics();
			C4DrawTransform *pPrevTrf = pForObj->pDrawTransform;
			C4DrawTransform trf;
			if (pPrevTrf)
			{
				trf = *pPrevTrf;
				trf *= Transform;
			}
			else
			{
				trf = Transform;
			}
			pForObj->SetGraphics(pSourceGfx, true);
			pForObj->pDrawTransform = &trf;
			pForObj->Draw(cgo, iByPlayer, C4Object::ODM_BaseOnly);
			pForObj->DrawTopFace(cgo, iByPlayer, C4Object::ODM_BaseOnly);
			pForObj->SetGraphics(pPrevGfx, true);
			pForObj->pDrawTransform = pPrevTrf;
		}
	}
	else if(eMode == MODE_Picture || eMode == MODE_IngamePicture)
	{
		assert(pForObj);
		float twdt, thgt;
		if (fZoomToShape)
		{
			twdt = pForObj->Shape.Wdt;
			thgt = pForObj->Shape.Hgt;
		}
		else
		{
			twdt = pSourceGfx->pDef->Shape.Wdt;
			thgt = pSourceGfx->pDef->Shape.Hgt;
		}

		C4TargetFacet ccgo;
		ccgo.Set(cgo.Surface, offX-twdt/2, offY-thgt/2, twdt, thgt, cgo.TargetX, cgo.TargetY);
		C4DrawTransform trf(Transform, offX, offY);

		// Don't set pForObj because we don't draw the picture of pForObj, but the picture of another definition on top of pForObj:
		pSourceGfx->Draw(ccgo, pForObj->Color, nullptr, iPhase, 0, &trf);
	}
	else
	{
		// no object specified: Draw from fctBlit
		// update by object color
		if (fctBlit.Surface && pForObj) fctBlit.Surface->SetClr(pForObj->Color);

		if (!pMeshInstance)
		{
			// draw there
			C4DrawTransform trf(Transform, offX, offY);
			if (fZoomToShape)
			{
				assert(pForObj);
				float fZoom = std::min(pForObj->Shape.Wdt / std::max(fctBlit.Wdt, 1.0f), pForObj->Shape.Hgt / std::max(fctBlit.Hgt, 1.0f));
				trf.ScaleAt(fZoom, fZoom, offX, offY);
			}

			fctBlit.DrawT(cgo.Surface, offX - fctBlit.Wdt/2 + fctBlit.TargetX, offY - fctBlit.Hgt/2 + fctBlit.TargetY, iPhase, 0, &trf);
		}
		else
		{
			C4Def *pDef = pSourceGfx->pDef;

			// draw there
			C4DrawTransform trf(Transform, offX, offY);
			if (fZoomToShape)
			{
				assert(pForObj);
				float fZoom = std::min((float)pForObj->Shape.Wdt / std::max(pDef->Shape.Wdt, 1), (float)pForObj->Shape.Hgt / std::max(pDef->Shape.Hgt, 1));
				trf.ScaleAt(fZoom, fZoom,  offX, offY);
			}

			C4Value value;
			pDef->GetProperty(P_MeshTransformation, &value);
			StdMeshMatrix matrix;
			if (C4ValueToMatrix(value, &matrix))
				pDraw->SetMeshTransform(&matrix);

			pDraw->RenderMesh(*pMeshInstance, cgo.Surface, offX - pDef->Shape.Wdt/2.0, offY - pDef->Shape.Hgt/2.0, pDef->Shape.Wdt, pDef->Shape.Hgt, pForObj ? pForObj->Color : 0xff, &trf);
			pDraw->SetMeshTransform(nullptr);
		}
	}

	// cleanup
	if (dwBlitMode == C4GFXBLIT_PARENT)
		(OverlayObj ? static_cast<C4Object*>(OverlayObj) : pForObj)->FinishedDrawing();
	else
	{
		pDraw->ResetBlitMode();
		pDraw->DeactivateBlitModulation();
	}
}

void C4GraphicsOverlay::DrawRankSymbol(C4Facet &cgo, C4Object *rank_obj)
{
	// Determine source gfx for rank
	if (!rank_obj || !rank_obj->Info) return;
	C4RankSystem *pRankSys = &::DefaultRanks;
	C4Facet *pRankRes=&::GraphicsResource.fctRank;
	int iRankCnt=::GraphicsResource.iNumRanks;
	C4Def *rank_def=rank_obj->Def;
	if (rank_def->pRankSymbols)
	{
		pRankRes=rank_def->pRankSymbols;
		iRankCnt=rank_def->iNumRankSymbols;
	}
	if (rank_def->pRankNames)
	{
		pRankSys = rank_def->pRankNames;
		iRankCnt = rank_def->pRankNames->GetBaseRankNum();
	}
	pRankSys->DrawRankSymbol(nullptr, rank_obj->Info->Rank, pRankRes, iRankCnt, false, 0, &cgo);
}

void C4GraphicsOverlay::DrawPicture(C4Facet &cgo, C4Object *pForObj, C4DrawTransform* trans)
{
	assert(IsPicture());

	// special blit mode
	if (dwBlitMode == C4GFXBLIT_PARENT)
	{
		if (pForObj) pForObj->PrepareDrawing();
	}
	else
	{
		pDraw->SetBlitMode(dwBlitMode);
		if (dwClrModulation != 0xffffff) pDraw->ActivateBlitModulation(dwClrModulation);

		if (pMeshInstance)
			pMeshInstance->SetFaceOrderingForClrModulation(dwClrModulation);
	}

	// the picture we are rendering is the one with trans applied, and the overlay transformation
	// is applied to the picture we are rendering, so apply it afterwards. Note that
	// C4BltTransform::operator*= does this = other * this.
	C4DrawTransform trf(Transform, cgo.X + cgo.Wdt/2.0f, cgo.Y + cgo.Hgt/2.0f);
	if(trans) trf *= *trans;

	// Don't set pForObj because we don't draw the picture of pForObj, but the picture of another definition on top of pForObj:
	pSourceGfx->Draw(cgo, pForObj->Color, nullptr, iPhase, 0, &trf);

	// cleanup
	if (dwBlitMode == C4GFXBLIT_PARENT)
	{
		if (pForObj) pForObj->FinishedDrawing();
	}
	else
	{
		pDraw->ResetBlitMode();
		pDraw->DeactivateBlitModulation();
	}
}

bool C4GraphicsOverlay::operator == (const C4GraphicsOverlay &rCmp) const
{
	// compare relevant fields
	// ignoring phase, because different animation state may be concatenated in graphics display
	return (eMode == rCmp.eMode)
	       && (pSourceGfx == rCmp.pSourceGfx)
	       && SEqual(Action, rCmp.Action)
	       && (dwBlitMode == rCmp.dwBlitMode)
	       && (dwClrModulation == rCmp.dwClrModulation)
	       && (Transform == rCmp.Transform)
	       && (OverlayObj == rCmp.OverlayObj);
}

void C4GraphicsOverlayListAdapt::CompileFunc(StdCompiler *pComp)
{
	bool fNaming = pComp->hasNaming();
	if (pComp->isDeserializer())
	{
		// clear list
		delete [] pOverlay; pOverlay = nullptr;
		// read the whole list
		C4GraphicsOverlay *pLast = nullptr;
		bool fContinue;
		do
		{
			C4GraphicsOverlay *pNext = new C4GraphicsOverlay();
			try
			{
				// read an overlay
				pComp->Value(*pNext);
			}
			catch (StdCompiler::NotFoundException *e)
			{
				delete e;
				// delete unused overlay
				delete pNext; pNext = nullptr;
				// clear up
				if (!pLast) pOverlay = nullptr;
				// done
				return;
			}
			// link it
			if (pLast)
				pLast->SetNext(pNext);
			else
				pOverlay = pNext;
			// step
			pLast = pNext;
			// continue?
			if (fNaming)
				fContinue = pComp->Separator(StdCompiler::SEP_SEP2) || pComp->Separator(StdCompiler::SEP_SEP);
			else
				pComp->Value(fContinue);
		}
		while (fContinue);
	}
	else
	{
		// write everything
		bool fContinue = true;
		for (C4GraphicsOverlay *pPos = pOverlay; pPos; pPos = pPos->GetNext())
		{
			// separate
			if (pPos != pOverlay)
			{
				if (fNaming)
					pComp->Separator(StdCompiler::SEP_SEP2);
				else
					pComp->Value(fContinue);
			}
			// write
			pComp->Value(*pPos);
		}
		// terminate
		if (!fNaming)
		{
			fContinue = false;
			pComp->Value(fContinue);
		}
	}
}

C4Surface *C4DefGraphics::GetBitmap(DWORD dwClr)
{
	if (Type != TYPE_Bitmap)
		return nullptr;
	if (Bmp.BitmapClr)
	{
		Bmp.BitmapClr->SetClr(dwClr);
		return Bmp.BitmapClr;
	}
	else
		return Bmp.Bitmap;
}
