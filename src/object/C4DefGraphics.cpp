/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// graphics used by object definitions (object and portraits)

#include <C4Include.h>
#include <C4DefGraphics.h>

#include <C4DefList.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4Config.h>
#include <C4Components.h>
#include <C4Application.h>
#include <C4Game.h>
#include <C4Menu.h>
#include <C4ObjectMenu.h>
#include <C4Player.h>
#include <C4Log.h>
#include <C4Material.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4RankSystem.h>
#include <C4GraphicsResource.h>
#include <C4MeshAnimation.h>
#include "StdMeshLoader.h"

// Helper class to load additional resources required for meshes from
// a C4Group.
class AdditionalResourcesLoader:
		public StdMeshMaterialTextureLoader, public StdMeshSkeletonLoader
{
public:
	AdditionalResourcesLoader(C4Group& hGroup): Group(hGroup) {}

	virtual C4Surface* LoadTexture(const char* filename)
	{
		if (!Group.AccessEntry(filename)) return NULL;
		C4Surface* surface = new C4Surface;
		// Suppress error message here, StdMeshMaterial loader
		// will show one.
		if (!surface->Read(Group, GetExtension(filename)))
			{ delete surface; surface = NULL; }
		return surface;
	}

	virtual StdStrBuf LoadSkeleton(const char* filename)
	{
		StdStrBuf ret;
		if (!Group.LoadEntryString(filename, &ret)) return StdStrBuf();
		return ret;
	}

private:
	C4Group& Group;
};

//-------------------------------- C4DefGraphics -----------------------------------------------

C4DefGraphics::C4DefGraphics(C4Def *pOwnDef)
{
	// store def
	pDef = pOwnDef;
	// zero fields
	Type = TYPE_Bitmap;
	Bmp.Bitmap = Bmp.BitmapClr = NULL;
	pNext = NULL;
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
	case TYPE_Bitmap:
		if (Bmp.BitmapClr) { delete Bmp.BitmapClr; Bmp.BitmapClr=NULL; }
		if (Bmp.Bitmap) { delete Bmp.Bitmap; Bmp.Bitmap=NULL; }
		break;
	case TYPE_Mesh:
		if (Mesh) { delete Mesh; Mesh = NULL; }
		break;
	}

	// delete additonal graphics
	C4AdditionalDefGraphics *pGrp2N = pNext, *pGrp2;
	while ((pGrp2=pGrp2N)) { pGrp2N = pGrp2->pNext; pGrp2->pNext = NULL; delete pGrp2; }
	pNext = NULL; fColorBitmapAutoCreated = false;
}

bool C4DefGraphics::LoadBitmap(C4Group &hGroup, const char *szFilename, const char *szOverlay, bool fColorByOwner)
{
	if (!szFilename) return false;
	Bmp.Bitmap = new C4Surface();
	if (!Bmp.Bitmap->Load(hGroup, szFilename)) return false;

	// Create owner color bitmaps
	if (fColorByOwner)
	{
		// Create additionmal bitmap
		Bmp.BitmapClr=new C4Surface();
		// if overlay-surface is present, load from that
		if (szOverlay && Bmp.BitmapClr->Load(hGroup, szOverlay))
		{
			// set as Clr-surface, also checking size
			if (!Bmp.BitmapClr->SetAsClrByOwnerOf(Bmp.Bitmap))
			{
				DebugLogF("    Gfx loading error in %s: %s (%d x %d) doesn't match overlay %s (%d x %d) - invalid file or size mismatch",
				          hGroup.GetFullName().getData(), szFilename, Bmp.Bitmap ? Bmp.Bitmap->Wdt : -1, Bmp.Bitmap ? Bmp.Bitmap->Hgt : -1,
				          szOverlay, Bmp.BitmapClr->Wdt, Bmp.BitmapClr->Hgt);
				delete Bmp.BitmapClr; Bmp.BitmapClr = NULL;
				return false;
			}
		}
		else
		{
			// otherwise, create by all blue shades
			if (!Bmp.BitmapClr->CreateColorByOwner(Bmp.Bitmap)) return false;
		}
		fColorBitmapAutoCreated = true;
	}
	Type = TYPE_Bitmap;
	// success
	return true;
}

bool C4DefGraphics::LoadMesh(C4Group &hGroup, const char* szFileName, StdMeshSkeletonLoader& loader)
{
	char* buf = NULL;
	size_t size;

	try
	{
		if(!hGroup.LoadEntry(szFileName, &buf, &size, 1)) return false;

		if(SEqualNoCase(GetExtension(szFileName), "xml"))
			Mesh = StdMeshLoader::LoadMeshXml(buf, size, ::MeshMaterialManager, loader, hGroup.GetName());
		else
			Mesh = StdMeshLoader::LoadMeshBinary(buf, size, ::MeshMaterialManager, loader, hGroup.GetName());
		delete[] buf;

		// Create mirrored animations (#401), order submeshes
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

bool C4DefGraphics::Load(C4Group &hGroup, bool fColorByOwner)
{
	char Filename[_MAX_PATH+1]; *Filename=0;
	AdditionalResourcesLoader loader(hGroup);

	// Load all materials for this definition:
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefMaterials, Filename, NULL, !!*Filename))
	{
		StdStrBuf material;
		if (hGroup.LoadEntryString(Filename, &material))
		{
			try
			{
				StdStrBuf buf;
				buf.Copy(hGroup.GetName());
				buf.Append("/"); buf.Append(Filename);
				::MeshMaterialManager.Parse(material.getData(), buf.getData(), loader);
			}
			catch (const StdMeshMaterialError& ex)
			{
				DebugLogF("Failed to read material script: %s", ex.what());
			}
		}
	}

	// Try from Mesh first
	if (!LoadMesh(hGroup, C4CFN_DefMesh, loader) && !LoadMesh(hGroup, C4CFN_DefMeshXml, loader) && !LoadBitmap(hGroup, C4CFN_DefGraphics, C4CFN_ClrByOwner, fColorByOwner)) return false;

	// load additional graphics
	C4DefGraphics *pLastGraphics = this;
	const int32_t iOverlayWildcardPos = SCharPos('*', C4CFN_ClrByOwnerEx);
	hGroup.ResetSearch(); *Filename=0;
	const char* const AdditionalGraphics[] = { C4CFN_DefGraphicsEx, C4CFN_DefGraphicsExMesh, C4CFN_DefGraphicsExMeshXml, NULL };
	while (hGroup.FindNextEntry("*", Filename, NULL, !!*Filename))
	{
		for(const char* const* szWildcard = AdditionalGraphics; *szWildcard != NULL; ++szWildcard)
		{
			if(!WildcardMatch(*szWildcard, Filename)) continue;
			// skip def graphics
			if (SEqualNoCase(Filename, C4CFN_DefGraphics) || SEqualNoCase(Filename, C4CFN_DefMesh) || SEqualNoCase(Filename, C4CFN_DefMeshXml)) continue;
			// skip scaled def graphics
			if (WildcardMatch(C4CFN_DefGraphicsScaled, Filename)) continue;
			// get name
			char GrpName[_MAX_PATH+1];
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
				char OverlayFn[_MAX_PATH+1];
				if(fColorByOwner)
				{
					// GraphicsX.png -> OverlayX.png
					SCopy(C4CFN_ClrByOwnerEx, OverlayFn, _MAX_PATH);
					OverlayFn[iOverlayWildcardPos]=0;
					SAppend(Filename + iWildcardPos, OverlayFn);
					EnforceExtension(OverlayFn, GetExtension(C4CFN_ClrByOwnerEx));
				}

				// load them
				if (!pLastGraphics->LoadBitmap(hGroup, Filename, fColorByOwner ? OverlayFn : NULL, fColorByOwner))
					return false;
			}
			else
			{
				if(!pLastGraphics->LoadMesh(hGroup, Filename, loader))
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
	return NULL;
}

bool C4DefGraphics::CopyGraphicsFrom(C4DefGraphics &rSource)
{
	if (Type != TYPE_Bitmap) return false; // TODO!
	// clear previous
	if (Bmp.BitmapClr) { delete Bmp.BitmapClr; Bmp.BitmapClr=NULL; }
	if (Bmp.Bitmap) { delete Bmp.Bitmap; Bmp.Bitmap=NULL; }
	// copy from source
	if (rSource.Bmp.Bitmap)
	{
		Bmp.Bitmap = new C4Surface();
		if (!Bmp.Bitmap->Copy(*rSource.Bmp.Bitmap))
			{ delete Bmp.Bitmap; Bmp.Bitmap=NULL; return false; }
	}
	if (rSource.Bmp.BitmapClr)
	{
		Bmp.BitmapClr = new C4Surface();
		if (!Bmp.BitmapClr->Copy(*rSource.Bmp.BitmapClr))
		{
			if (Bmp.Bitmap) { delete Bmp.Bitmap; Bmp.Bitmap=NULL; }
			delete Bmp.BitmapClr; Bmp.BitmapClr=NULL; return false;
		}
		if (Bmp.Bitmap) Bmp.BitmapClr->SetAsClrByOwnerOf(Bmp.Bitmap);
	}
	// done, success
	return true;
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
			dummy.reset(new StdMeshInstance(*Mesh, 1.0f));
			instance = dummy.get();
			pDef->GetProperty(P_PictureTransformation, &value);
		}

		StdMeshMatrix matrix;
		if (C4ValueToMatrix(value, &matrix))
			pDraw->SetMeshTransform(&matrix);

		pDraw->SetPerspective(true);
		pDraw->RenderMesh(*instance, cgo.Surface, cgo.X,cgo.Y, cgo.Wdt, cgo.Hgt, pObj ? pObj->Color : iColor, trans);
		pDraw->SetPerspective(false);
		pDraw->SetMeshTransform(NULL);

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
	bool fCompiler = pComp->isCompiler();
	// nothing?
	if (!fCompiler && !pDefGraphics) return;
	// definition
	C4ID id; if (!fCompiler) id = pDefGraphics->pDef->id;
	pComp->Value(id);
	// go over two separators ("::"). Expect them if an id was found.
	if (!pComp->Separator(StdCompiler::SEP_PART2) || !pComp->Separator(StdCompiler::SEP_PART2))
		pComp->excCorrupt("DefGraphics: expected \"::\"");
	// compile name
	StdStrBuf Name; if (!fCompiler) Name = pDefGraphics->GetName();
	pComp->Value(mkDefaultAdapt(mkParAdapt(Name, StdCompiler::RCT_Idtf), ""));
	// reading: search def-graphics
	if (fCompiler)
	{
		// search definition, throw expection if not found
		C4Def *pDef = ::Definitions.ID2Def(id);
		// search def-graphics
		if (!pDef || !( pDefGraphics = pDef->Graphics.Get(Name.getData()) ))
			pComp->excCorrupt("DefGraphics: could not find graphics \"%s\" in %s(%s)!", Name.getData(), id.ToString(), pDef ? pDef->GetName() : "def not found");
	}
}

C4AdditionalDefGraphics::C4AdditionalDefGraphics(C4Def *pOwnDef, const char *szName) : C4DefGraphics(pOwnDef)
{
	// store name
	SCopy(szName, Name, C4MaxName);
}

C4DefGraphicsPtrBackup::C4DefGraphicsPtrBackup(C4DefGraphics *pSourceGraphics):
	MeshMaterialUpdate(::MeshMaterialManager), pMeshUpdate(NULL)
{
	// assign graphics + def
	pGraphicsPtr = pSourceGraphics;
	pDef = pSourceGraphics->pDef;
	// assign name
	const char *szName = pGraphicsPtr->GetName();
	if (szName) SCopy(szName, Name, C4MaxName); else *Name=0;

	// Remove all mesh materials that were loaded from this definition
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

	// assign mesh update
	if(pSourceGraphics->Type == C4DefGraphics::TYPE_Mesh)
		pMeshUpdate = new StdMeshUpdate(*pSourceGraphics->Mesh);

	// create next graphics recursively
	C4DefGraphics *pNextGfx = pGraphicsPtr->pNext;
	if (pNextGfx)
		pNext = new C4DefGraphicsPtrBackup(pNextGfx);
	else
		pNext = NULL;
}

C4DefGraphicsPtrBackup::~C4DefGraphicsPtrBackup()
{
	// graphics ptr still assigned? then remove dead graphics pointers from objects
	if (pGraphicsPtr) AssignRemoval();
	delete pMeshUpdate;
	// delete following graphics recursively
	if (pNext) delete pNext;
}

void C4DefGraphicsPtrBackup::AssignUpdate(C4DefGraphics *pNewGraphics)
{
	UpdateMeshes();

	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		C4Object *pObj;
		for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk=pLnk->Next)
			if ((pObj=pLnk->Obj)) if (pObj->Status)
				{
					if (pObj->pGraphics == pGraphicsPtr)
					{
						// same graphics found. Update mesh graphics if any.
						if(pMeshUpdate)
						{
							assert(pObj->pMeshInstance != NULL); // object had mesh graphics, so mesh instance should be present
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
								pObj->AssignRemoval(); pObj->pGraphics=NULL;
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
								pObj->Menu->SetFrameDeco(NULL);
				}
		// done; reset field to indicate finished update
		pGraphicsPtr = NULL;
	}
	// check next graphics
	if (pNext) pNext->AssignUpdate(pNewGraphics);
}

void C4DefGraphicsPtrBackup::AssignRemoval()
{
	// Reset all mesh materials to what they were before the update
	MeshMaterialUpdate.Cancel();
	UpdateMeshes();

	// only if graphics are assigned
	if (pGraphicsPtr)
	{
		// check all objects
		C4Object *pObj;
		for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk=pLnk->Next)
			if ((pObj=pLnk->Obj)) if (pObj->Status)
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
							pObj->pMeshInstance = NULL;
							pObj->pGraphics = NULL;
						}
						// sprite graphics; reset them
						else if (!pObj->SetGraphics()) { pObj->AssignRemoval(); pObj->pGraphics=NULL; }
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
							pObj->Menu->SetFrameDeco(NULL);
				}
		// done; reset field to indicate finished update
		pGraphicsPtr = NULL;
	}
	// check next graphics
	if (pNext) pNext->AssignRemoval();
}

void C4DefGraphicsPtrBackup::UpdateMeshes()
{
	// Update mesh materials for all meshes
	for(C4DefList::Table::iterator iter = Definitions.table.begin(); iter != Definitions.table.end(); ++iter)
		if(iter->second->Graphics.Type == C4DefGraphics::TYPE_Mesh)
			MeshMaterialUpdate.Update(iter->second->Graphics.Mesh);

	// Update mesh materials for all mesh instances.
	C4Object *pObj;
	for (C4ObjectLink *pLnk = ::Objects.First; pLnk; pLnk=pLnk->Next)
		if ((pObj=pLnk->Obj)) if (pObj->Status)
		{
			if(pObj->pMeshInstance)
				UpdateMesh(pObj->pMeshInstance);
			for (C4GraphicsOverlay* pGfxOverlay = pObj->pGfxOverlay; pGfxOverlay; pGfxOverlay = pGfxOverlay->GetNext())
				if(pGfxOverlay->pMeshInstance)
					UpdateMesh(pGfxOverlay->pMeshInstance);
		}
}

void C4DefGraphicsPtrBackup::UpdateMesh(StdMeshInstance* instance)
{
	if(pMeshUpdate)
	{
		// Update materials for meshes that need not to be updated
		if(&instance->GetMesh() != &pMeshUpdate->GetOldMesh()) // TODO: Won't work for multiple graphics
		{
			MeshMaterialUpdate.Update(instance);
		}
		// Updated if instance is an owned attached mesh
		else if(instance->GetAttachParent() && instance->GetAttachParent()->OwnChild)
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
		// AssignUpdate or AssignRemoval, respectively, since they are contained
		// in the object list.
	}

	// Copy the attached mesh list before recursion because the recursive call
	// might detach meshes, altering the list and invalidating iterators.
	std::vector<StdMeshInstance::AttachedMesh*> attached_meshes;
	for(StdMeshInstance::AttachedMeshIter iter = instance->AttachedMeshesBegin(); iter != instance->AttachedMeshesEnd(); ++iter)
		attached_meshes.push_back(*iter);

	for(std::vector<StdMeshInstance::AttachedMesh*>::iterator iter = attached_meshes.begin(); iter != attached_meshes.end(); ++iter)
		// TODO: Check that this mesh is still attached?
		UpdateMesh((*iter)->Child);
}

// ---------------------------------------------------------------------------
// C4GraphicsOverlay: graphics overlay used to attach additional graphics to objects

C4GraphicsOverlay::~C4GraphicsOverlay()
{
	// Free mesh instance
	delete pMeshInstance; pMeshInstance = NULL;
	// free any additional overlays
	C4GraphicsOverlay *pNextOther = pNext, *pOther;
	while ((pOther = pNextOther))
	{
		pNextOther = pOther->pNext;
		pOther->pNext = NULL;
		delete pOther;
	}
}

void C4GraphicsOverlay::UpdateFacet()
{
	// special: Nothing to update for object and pSourceGfx may be NULL
	// If there will ever be something to init here, UpdateFacet() will also need to be called when objects have been loaded
	if (eMode == MODE_Object) return;
	// otherwise, source graphics must be specified
	if (!pSourceGfx) return;
	C4Def *pDef = pSourceGfx->pDef;
	assert(pDef);
	fZoomToShape = false;
	// Clear old mesh instance, if any
	delete pMeshInstance; pMeshInstance = NULL;
	// update by mode
	switch (eMode)
	{
	case MODE_None:
		break;

	case MODE_Base: // def base graphics
		if (pSourceGfx->Type == C4DefGraphics::TYPE_Bitmap)
			fctBlit.Set(pSourceGfx->GetBitmap(), 0, 0, pDef->Shape.Wdt, pDef->Shape.Hgt, pDef->Shape.x+pDef->Shape.Wdt/2, pDef->Shape.y+pDef->Shape.Hgt/2);
		else
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
		else
		{
			C4String* AnimationName = action->GetPropertyStr(P_Animation);
			if (!AnimationName) return;

			pMeshInstance = new StdMeshInstance(*pSourceGfx->Mesh, 1.0f);
			const StdMeshAnimation* Animation = pSourceGfx->Mesh->GetAnimationByName(AnimationName->GetData());
			if (!Animation) return;

			pMeshInstance->PlayAnimation(*Animation, 0, NULL, new C4ValueProviderRef<int32_t>(iPhase, ftofix(Animation->Length / action->GetPropertyInt(P_Length))), new C4ValueProviderConst(itofix(1)));
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
		else
			return !!pMeshInstance;
	}
	else
	{
		return false;
	}
}

void C4GraphicsOverlay::Read(const char **ppInput)
{
	// deprecated
	assert(false && "C4GraphicsOverlay::Read: deprecated");
#if 0
	const char *szReadFrom = *ppInput;
	// defaults
	eMode = MODE_None; pSourceGfx = NULL; *Action=0; dwBlitMode = 0; iPhase = 0; iID=0;
	// read ID
	SCopyUntil(szReadFrom, OSTR, ',', C4MaxName);
	szReadFrom += strlen(OSTR); if (*szReadFrom) ++szReadFrom;
	sscanf(OSTR, "%i", &iID);
	// read C4ID::Gfxname
	int32_t iLineLength = SLen(szReadFrom);
	// not C4ID::Name?
	if (iLineLength < 6 || szReadFrom[4]!=':' || szReadFrom[5]!=':')
	{
		DebugLog("C4Compiler error: Malformed graphics overlay definition!");
		return;
	}
	// get ID
	char id[5]; SCopy(szReadFrom, id, 4); szReadFrom += 6;
	C4Def *pSrcDef = ::Definitions.ID2Def(C4Id(id)); // defaults to NULL for unloaded def
	if (pSrcDef)
	{
		char GfxName[C4MaxName+1];
		SCopyUntil(szReadFrom, GfxName, ',', C4MaxName);
		szReadFrom += strlen(GfxName); if (*szReadFrom) ++szReadFrom;
		// get graphics - "C4ID::" leads to *szLine == NULL, and thus the default graphic of pSrcDef!
		pSourceGfx = pSrcDef->Graphics.Get(GfxName);
	}
	// read mode
	DWORD dwRead;
	SCopyUntil(szReadFrom, OSTR, ',', C4MaxName);
	szReadFrom += strlen(OSTR); if (*szReadFrom) ++szReadFrom;
	sscanf(OSTR, "%i", &dwRead); eMode = (Mode) dwRead;
	// read action
	SCopyUntil(szReadFrom, Action, ',', C4MaxName);
	szReadFrom += strlen(Action); if (*szReadFrom) ++szReadFrom;
	// read blit mode
	SCopyUntil(szReadFrom, OSTR, ',', C4MaxName);
	szReadFrom += strlen(OSTR); if (*szReadFrom) ++szReadFrom;
	sscanf(OSTR, "%i", &dwBlitMode);
	// read phase
	SCopyUntil(szReadFrom, OSTR, ',', C4MaxName);
	szReadFrom += strlen(OSTR); if (*szReadFrom) ++szReadFrom;
	sscanf(OSTR, "%i", &iPhase);
	// read transform
	if (*szReadFrom) ++szReadFrom; // '('
	int32_t iScanCnt = sscanf(szReadFrom, "%f,%f,%f,%f,%f,%f,%d",
	                          &Transform.mat[0], &Transform.mat[1], &Transform.mat[2],
	                          &Transform.mat[3], &Transform.mat[4], &Transform.mat[5], &Transform.FlipDir);
	if (iScanCnt != 7) { DebugLog("C4Compiler: malformed C4CV_Transform"); }
	iScanCnt = SCharPos(')', szReadFrom); if (iScanCnt>=0) szReadFrom += iScanCnt+1;
	// assign ptr immediately after read overlay
	*ppInput = szReadFrom;
	// update used facet according to read data
	UpdateFacet();
#endif
}

void C4GraphicsOverlay::Write(char *szOutput)
{
	// deprecated
	assert(false && "C4GraphicsOverlay::Write: deprecated");
#if 0
	// safety: Don't save invalid
	if (!pSourceGfx) return;
	C4Def *pDef = pSourceGfx->pDef;
	assert(pDef);
	// get to end of buffer
	szOutput += strlen(szOutput);
	// store ID
	sprintf(OSTR, "%i", iID); SCopy(OSTR, szOutput); szOutput += strlen(szOutput);
	*szOutput = ','; ++szOutput;
	// append C4ID::Graphicsname (or C4ID:: for def graphics)
	SCopy(pDef->id.ToString(), szOutput); szOutput += strlen(szOutput);
	SCopy("::", szOutput); szOutput += strlen(szOutput);
	const char *szGrpName = pSourceGfx->GetName();
	if (szGrpName) { SCopy(szGrpName, szOutput); szOutput += strlen(szOutput); }
	*szOutput = ','; ++szOutput;
	// store mode
	DWORD dwMode = eMode;
	sprintf(OSTR, "%i", dwMode); SCopy(OSTR, szOutput); szOutput += strlen(OSTR);
	// store action
	*szOutput = ','; ++szOutput;
	SCopy(Action, szOutput); szOutput += strlen(szOutput);
	// store blit mode
	*szOutput = ','; ++szOutput;
	sprintf(OSTR, "%i", dwBlitMode); SCopy(OSTR, szOutput); szOutput += strlen(szOutput);
	// store phase
	*szOutput = ','; ++szOutput;
	sprintf(OSTR, "%i", iPhase); SCopy(OSTR, szOutput); szOutput += strlen(szOutput);
	// store transform
	*szOutput = ','; ++szOutput;
	sprintf(OSTR, "(%f,%f,%f,%f,%f,%f,%d)",
	        Transform.mat[0], Transform.mat[1], Transform.mat[2],
	        Transform.mat[3], Transform.mat[4], Transform.mat[5], Transform.FlipDir);
	SCopy(OSTR, szOutput); szOutput += strlen(szOutput);
	// terminate string
	*szOutput=0;
#endif
}

void C4GraphicsOverlay::CompileFunc(StdCompiler *pComp)
{
	// read ID
	pComp->Value(iID); pComp->Separator();
	// read def-graphics
	pComp->Value(mkDefaultAdapt(C4DefGraphicsAdapt(pSourceGfx), (C4DefGraphics *)NULL));
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
		if (pComp->isCompiler()) dwClrModulation = 0xffffff;
	// read overlay target object
	if (pComp->Separator())
		pComp->Value(OverlayObj);
	else
		// default
		if (pComp->isCompiler()) OverlayObj = NULL;
	// update used facet according to read data
	if (pComp->isCompiler()) UpdateFacet();
}

void C4GraphicsOverlay::DenumeratePointers()
{
	OverlayObj.DenumeratePointers();
}

void C4GraphicsOverlay::Draw(C4TargetFacet &cgo, C4Object *pForObj, int32_t iByPlayer)
{
	assert(!IsPicture());
	assert(pForObj);
	// get target pos
	float offX, offY;
	float newzoom;
	pForObj->GetDrawPosition(cgo, offX, offY, newzoom);
	ZoomDataStackItem zdsi(newzoom);

	// special blit mode
	if (dwBlitMode == C4GFXBLIT_PARENT)
		(OverlayObj ? static_cast<C4Object*>(OverlayObj) : pForObj)->PrepareDrawing();
	else
	{
		pDraw->SetBlitMode(dwBlitMode);
		if (dwClrModulation != 0xffffff) pDraw->ActivateBlitModulation(dwClrModulation);

		if (pMeshInstance)
			pMeshInstance->SetFaceOrderingForClrModulation(dwClrModulation);
	}
	if (eMode == MODE_Rank)
	{
		C4TargetFacet ccgo;
		ccgo.Set(cgo.Surface, offX+pForObj->Shape.x,offY+pForObj->Shape.y,pForObj->Shape.Wdt,pForObj->Shape.Hgt, cgo.TargetX, cgo.TargetY);
		DrawRankSymbol(ccgo, OverlayObj);
	}
	// drawing specific object?
	else if (OverlayObj)
	{
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
		pSourceGfx->Draw(ccgo, pForObj->Color, NULL, iPhase, 0, &trf);
	}
	else
	{
		// no object specified: Draw from fctBlit
		// update by object color
		if (fctBlit.Surface) fctBlit.Surface->SetClr(pForObj->Color);

		if (!pMeshInstance)
		{
			// draw there
			C4DrawTransform trf(Transform, offX, offY);
			if (fZoomToShape)
			{
				float fZoom = Min<float>((float) pForObj->Shape.Wdt / Max<int>(fctBlit.Wdt,1), (float) pForObj->Shape.Hgt / Max<int>(fctBlit.Hgt,1));
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
				float fZoom = Min<float>((float) pForObj->Shape.Wdt / Max<int>(pDef->Shape.Wdt,1), (float) pForObj->Shape.Hgt / Max<int>(pDef->Shape.Hgt,1));
				trf.ScaleAt(fZoom, fZoom,  offX, offY);
			}

			C4Value value;
			pDef->GetProperty(P_MeshTransformation, &value);
			StdMeshMatrix matrix;
			if (C4ValueToMatrix(value, &matrix))
				pDraw->SetMeshTransform(&matrix);

			pDraw->RenderMesh(*pMeshInstance, cgo.Surface, offX - pDef->Shape.Wdt/2.0, offY - pDef->Shape.Hgt/2.0, pDef->Shape.Wdt, pDef->Shape.Hgt, pForObj->Color, &trf);
			pDraw->SetMeshTransform(NULL);
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
	pRankSys->DrawRankSymbol(NULL, rank_obj->Info->Rank, pRankRes, iRankCnt, false, 0, &cgo);
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
	pSourceGfx->Draw(cgo, pForObj->Color, NULL, iPhase, 0, &trf);

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
	if (pComp->isCompiler())
	{
		// clear list
		delete [] pOverlay; pOverlay = NULL;
		// read the whole list
		C4GraphicsOverlay *pLast = NULL;
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
				delete pNext; pNext = NULL;
				// clear up
				if (!pLast) pOverlay = NULL;
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
