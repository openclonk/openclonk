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
// game object lists

#include "C4Include.h"
#include "object/C4GameObjects.h"

#include "control/C4Record.h"
#include "game/C4Physics.h"
#include "lib/C4Random.h"
#include "network/C4Network2Stats.h"
#include "object/C4Def.h"
#include "object/C4Object.h"
#include "object/C4ObjectCom.h"
#include "player/C4PlayerList.h"
#include "script/C4Effect.h"

C4GameObjects::C4GameObjects()
{
	Default();
}

C4GameObjects::~C4GameObjects()
{
	Sectors.Clear();
}

void C4GameObjects::Default()
{
	Sectors.Clear();
	LastUsedMarker = 0;
	ForeObjects.Default();
}

void C4GameObjects::Init(int32_t iWidth, int32_t iHeight)
{
	// Init sectors
	Sectors.Init(iWidth, iHeight);
}

bool C4GameObjects::Add(C4Object *nObj)
{
	// Add inactive objects to the inactive list only
	if (nObj->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Add(nObj, C4ObjectList::stMain);
	}
	// If this is a foreground object, add it to the list
	if (nObj->Category & C4D_Foreground)
	{
		ForeObjects.Add(nObj, C4ObjectList::stMain);
	}
	// Manipulate main list
	if (!C4ObjectList::Add(nObj, C4ObjectList::stMain))
		return false;
	// Add to sectors
	Sectors.Add(nObj, this);
	return true;
}


bool C4GameObjects::Remove(C4Object *pObj)
{
	// If it's an inactive object, simply remove from the inactiv elist
	if (pObj->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Remove(pObj);
	}
	// Remove from sectors
	Sectors.Remove(pObj);
	// Remove from forelist
	ForeObjects.Remove(pObj);
	// Manipulate main list
	return C4ObjectList::Remove(pObj);
}

C4ObjectList &C4GameObjects::ObjectsAt(int ix, int iy)
{
	return Sectors.SectorAt(ix, iy)->ObjectShapes;
}

void C4GameObjects::CrossCheck() // Every Tick1 by ExecObjects
{
	DWORD focf, tocf;

	// Reverse area check: Checks for all obj2 at obj1

	focf = tocf = OCF_None;
	// High level: Collection, Hit
	if (!::Game.iTick3)
	{
		tocf |= OCF_Carryable;
	}
	focf |= OCF_Collection;
	focf |= OCF_Alive;
	tocf |= OCF_HitSpeed2;

	for (C4Object* obj1 : *this)
	{
		if (obj1->Status && !obj1->Contained && (obj1->OCF & focf))
		{
			uint32_t Marker = GetNextMarker();
			C4LSector *pSct;
			for (C4ObjectList *pLst = obj1->Area.FirstObjects(&pSct); pLst; pLst = obj1->Area.NextObjects(pLst, &pSct))
			{
				for (C4Object* obj2 : *pLst)
				{
					if ((obj2 != obj1) && obj2->Status && !obj2->Contained && (obj2->OCF & tocf) &&
					    Inside<int32_t>(obj2->GetX() - (obj1->GetX() + obj1->Shape.x), 0, obj1->Shape.Wdt - 1) &&
					    Inside<int32_t>(obj2->GetY() - (obj1->GetY() + obj1->Shape.y), 0, obj1->Shape.Hgt - 1) &&
					    obj1->Layer == obj2->Layer)
					{
						// Handle collision only once
						if (obj2->Marker == Marker)
						{
							continue;
						}
						obj2->Marker = Marker;
						// Only hit if target is alive and projectile is an object
						if ((obj1->OCF & OCF_Alive) && (obj2->Category & C4D_Object))
						{
							C4Real dXDir = obj2->xdir - obj1->xdir;
							C4Real dYDir = obj2->ydir - obj1->ydir;
							C4Real speed = dXDir * dXDir + dYDir * dYDir;
							// Only hit if obj2's speed and relative speeds are larger than HitSpeed2
							if ((obj2->OCF & OCF_HitSpeed2) && speed > HitSpeed2 &&
							   !obj1->Call(PSF_QueryCatchBlow, &C4AulParSet(obj2)))
							{
								int32_t iHitEnergy = fixtoi(speed * obj2->Mass / 5);
								// Hit energy reduced to 1/3rd, but do not drop to zero because of this division
								iHitEnergy = std::max<int32_t>(iHitEnergy/3, !!iHitEnergy);
								obj1->DoEnergy(-iHitEnergy / 5, false, C4FxCall_EngObjHit, obj2->Controller);
								int tmass = std::max<int32_t>(obj1->Mass, 50);
								C4PropList* pActionDef = obj1->GetAction();
								if (!::Game.iTick3 || (pActionDef && pActionDef->GetPropertyP(P_Procedure) != DFA_FLIGHT))
								{
									obj1->Fling(obj2->xdir * 50 / tmass, -Abs(obj2->ydir / 2) * 50 / tmass, false);
								}
								obj1->Call(PSF_CatchBlow, &C4AulParSet(-iHitEnergy / 5, obj2));
								// obj1 might have been tampered with
								if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
								{
									goto out1;
								}
								continue;
							}
						}
						// Collection
						if ((obj1->OCF & OCF_Collection) && (obj2->OCF & OCF_Carryable) &&
						    Inside<int32_t>(obj2->GetX() - (obj1->GetX() + obj1->Def->Collection.x), 0, obj1->Def->Collection.Wdt - 1) &&
						    Inside<int32_t>(obj2->GetY() - (obj1->GetY() + obj1->Def->Collection.y), 0, obj1->Def->Collection.Hgt - 1))
						{
							obj1->Collect(obj2);
							// obj1 might have been tampered with
							if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
							{
								goto out1;
							}
						}
					}
				}
			}
			out1: ;
		}
	}
}

C4Object* C4GameObjects::AtObject(int ctx, int cty, DWORD &ocf, C4Object *exclude)
{
	DWORD cocf;
	for (C4Object *cObj : ObjectsAt(ctx, cty))
	{
		if (!exclude || (cObj != exclude && exclude->Layer == cObj->Layer)) if (cObj->Status)
			{
				cocf = ocf | OCF_Exclusive;
				if (cObj->At(ctx, cty, cocf))
				{
					// Search match
					if (cocf & ocf)
					{
						ocf = cocf;
						return cObj;
					}
					// EXCLUSIVE block
					else
					{
						return nullptr;
					}
				}
			}
	}
	return nullptr;
}

void C4GameObjects::Synchronize()
{
	// Synchronize unsorted objects
	ResortUnsorted();
	// Synchronize solidmasks
	UpdateSolidMasks();
}

C4Object *C4GameObjects::ObjectPointer(int32_t iNumber)
{
	// Search own list
	C4PropList *pObj = C4PropListNumbered::GetByNumber(iNumber);
	if (pObj)
	{
		return pObj->GetObject();
	}
	return nullptr;
}

C4Object *C4GameObjects::SafeObjectPointer(int32_t iNumber)
{
	C4Object *pObj = ObjectPointer(iNumber);
	if (pObj)
	{
		if (!pObj->Status)
		{
			return nullptr;
		}
	}
	return pObj;
}

void C4GameObjects::UpdateSolidMasks()
{
	for (C4Object *obj : *this)
	{
		if (obj->Status)
		{
			obj->UpdateSolidMask(false);
		}
	}
}

void C4GameObjects::DeleteObjects(bool fDeleteInactive)
{
	C4ObjectList::DeleteObjects();
	Sectors.ClearObjects();
	ForeObjects.Clear();
	if (fDeleteInactive)
	{
		InactiveObjects.DeleteObjects();
	}
}

void C4GameObjects::Clear(bool fClearInactive)
{
	DeleteObjects(fClearInactive);
	if (fClearInactive)
	{
		InactiveObjects.Clear();
	}
	LastUsedMarker = 0;
}

int C4GameObjects::PostLoad(bool fKeepInactive, C4ValueNumbers * numbers)
{
	// Process objects
	int32_t iMaxObjectNumber = 0;
	for (C4Object *pObj : reverse())
	{
		// Keep track of numbers
		iMaxObjectNumber = std::max(iMaxObjectNumber, pObj->Number);
		// Add to list of foreobjects
		if (pObj->Category & C4D_Foreground)
		{
			ForeObjects.Add(pObj, C4ObjectList::stMain, this);
		}
		// Unterminate end
	}

	// Denumerate pointers:
	// On section load, inactive object numbers will be adjusted afterwards,
	// so fake inactive object list empty. Meanwhile, note this has to be done
	// to prevent an assertion fail when denumerating non-enumerated inactive objects,
	// even if object numbers did not collide.
	C4ObjectList inactiveObjectsCopy;
	if (fKeepInactive)
	{
		inactiveObjectsCopy.Copy(InactiveObjects);
		InactiveObjects.Clear();
	}
	// Denumerate pointers
	Denumerate(numbers);
	// Update object enumeration index now, because calls like OnSynchronized might create objects
	C4PropListNumbered::SetEnumerationIndex(iMaxObjectNumber);
	// end faking and adjust object numbers
	if (fKeepInactive)
	{
		InactiveObjects.Copy(inactiveObjectsCopy);
		inactiveObjectsCopy.Clear();
		C4PropListNumbered::UnshelveNumberedPropLists();
	}

	// Special checks:
	// -contained/contents-consistency
	// -StaticBack-objects zero speed
	for (C4Object *pObj : *this)
	{
		if (pObj->Status)
		{
			// Staticback must not have speed
			if (pObj->Category & C4D_StaticBack)
			{
				pObj->xdir = pObj->ydir = 0;
			}
			// contained must be in contents list
			if (pObj->Contained)
			{
				if (!pObj->Contained->Contents.GetLink(pObj))
				{
					DebugLogF("Error in Objects.txt: Container of #%d is #%d, but not found in contents list!", pObj->Number, pObj->Contained->Number);
					pObj->Contained->Contents.Add(pObj, C4ObjectList::stContents);
				}
			}
			// All contents must have contained set; otherwise, remove them!
			auto contentsIt = pObj->Contents.begin();
			while (!contentsIt.atEnd())
			{
				C4Object* pObj2 = *contentsIt;
				// Check double links
				auto it2 = pObj->Contents.begin();
				if (it2.find(pObj2) && it2 != contentsIt)
				{
					DebugLogF("Error in Objects.txt: Double containment of #%d by #%d!", pObj2->Number, pObj->Number);
					// This remove-call will only remove the previous (doubled) link, so cLnkCont should be save
					pObj->Contents.Remove(pObj2);
					// Contents checked already
					continue;
				}
				// Check contents/contained-relation
				if (pObj2->Status && pObj2->Contained != pObj)
				{
					DebugLogF("Error in Objects.txt: Object #%d not in container #%d as referenced!", pObj2->Number, pObj->Number);
					pObj2->Contained = pObj;
				}
				contentsIt++;
			}
		}
	}
	// Sort out inactive objects
	for (C4Object *obj : *this)
	{
		if (obj->Status == C4OS_INACTIVE)
		{
			Remove(obj);
			InactiveObjects.Add(obj, C4ObjectList::stNone);
		}
	}

	{
		C4DebugRecOff DBGRECOFF; // - script callbacks that would kill DebugRec-sync for runtime start
		// Update graphics
		UpdateGraphics(false);
		// Update faces
		UpdateFaces(false);
		// Update ocf
		SetOCF();
	}

	// Make sure list is sorted by category - after sorting out inactives, because inactives aren't sorted into the main list
	FixObjectOrder();

	// Misc updates
	for (C4Object *pObj : *this)
	{
		if (pObj->Status)
		{
			// Add to plrview
			pObj->UpdateLight();
			// Update flipdir (for old objects.txt with no flipdir defined),
			// assigns Action.DrawDir as well
			pObj->UpdateFlipDir();
			// Initial OCF update
			pObj->SetOCF();
		}
	}
	// Done
	return ObjectCount();
}

void C4GameObjects::Denumerate(C4ValueNumbers * numbers)
{
	C4ObjectList::Denumerate(numbers);
	InactiveObjects.Denumerate(numbers);
}

void C4GameObjects::UpdateScriptPointers()
{
	// Call in sublists
	C4ObjectList::UpdateScriptPointers();
	InactiveObjects.UpdateScriptPointers();
}

C4Value C4GameObjects::GRBroadcast(const char *szFunction, C4AulParSet *pPars, bool fPassError, bool fRejectTest)
{
	// Call objects first - scenario script might overwrite hostility, etc...
	for (C4Object *pObj : *this)
	{
		if (pObj && (pObj->Category & (C4D_Goal | C4D_Rule | C4D_Environment)) && pObj->Status)
		{
			C4Value vResult = pObj->Call(szFunction, pPars, fPassError);
			// Rejection tests abort on first nonzero result
			if (fRejectTest && !!vResult)
			{
				return vResult;
			}
		}
	}
	return C4Value();
}

void C4GameObjects::UpdatePos(C4Object *pObj)
{
	// Position might have changed. Update sector lists
	Sectors.Update(pObj, this);
}

void C4GameObjects::UpdatePosResort(C4Object *pObj)
{
	// Object order for this object was changed. Readd object to sectors
	Sectors.Remove(pObj);
	Sectors.Add(pObj, this);
}

void C4GameObjects::FixObjectOrder()
{
	// Fixes the object order so it matches the global object order sorting constraints
	C4ObjectLink *pLnk0 = First;
	C4ObjectLink *pLnkL = Last;
	while (pLnk0 != pLnkL)
	{
		C4ObjectLink *pLnk1stUnsorted = nullptr;
		C4ObjectLink *pLnkLastUnsorted = nullptr;
		C4ObjectLink *pLnkPrev = nullptr;
		C4ObjectLink *pLnk;
		C4Object *pLastWarnObj = nullptr;
		// Forward fix
		int lastPlane = 2147483647; //INT32_MAX;
		for (pLnk = pLnk0; pLnk != pLnkL->Next; pLnk = pLnk->Next)
		{
			C4Object *pObj = pLnk->Obj;
			if (pObj->Unsorted || !pObj->Status)
			{
				continue;
			}
			int currentPlane = pObj->GetPlane();
			// Must have nonzero Plane
			if (!currentPlane)
			{
				DebugLogF("Objects.txt: Object #%d has zero Plane!", (int) pObj->Number);
				pObj->SetPlane(lastPlane); currentPlane = lastPlane;
			}
			// Fix order
			if (currentPlane > lastPlane)
			{
				// SORT ERROR! (note that pLnkPrev can't be 0)
				if (pLnkPrev->Obj != pLastWarnObj)
				{
					DebugLogF("Objects.txt: Wrong object order of #%d-#%d! (down)", (int) pObj->Number, (int) pLnkPrev->Obj->Number);
					pLastWarnObj = pLnkPrev->Obj;
				}
				pLnk->Obj = pLnkPrev->Obj;
				pLnkPrev->Obj = pObj;
				pLnkLastUnsorted = pLnkPrev;
			}
			else
			{
				lastPlane = currentPlane;
			}
			pLnkPrev = pLnk;
		}
		if (!pLnkLastUnsorted)
		{
			break; // Done
		}
		pLnkL = pLnkLastUnsorted;
		// Backwards fix
		lastPlane = -2147483647-1; //INT32_MIN;
		for (pLnk = pLnkL; pLnk != pLnk0->Prev; pLnk = pLnk->Prev)
		{
			C4Object *pObj = pLnk->Obj;
			if (pObj->Unsorted || !pObj->Status)
			{
				continue;
			}
			int currentPlane = pObj->GetPlane();
			if (currentPlane < lastPlane)
			{
				// SORT ERROR! (note that pLnkPrev can't be 0)
				if (pLnkPrev->Obj != pLastWarnObj)
				{
					DebugLogF("Objects.txt: Wrong object order of #%d-#%d! (up)", (int) pObj->Number, (int) pLnkPrev->Obj->Number);
					pLastWarnObj = pLnkPrev->Obj;
				}
				pLnk->Obj = pLnkPrev->Obj;
				pLnkPrev->Obj = pObj;
				pLnk1stUnsorted = pLnkPrev;
			}
			else
			{
				lastPlane = currentPlane;
			}
			pLnkPrev = pLnk;
		}
		if (!pLnk1stUnsorted)
		{
			break; // Done
		}
		pLnk0 = pLnk1stUnsorted;
	}
	// Objects fixed!
}

void C4GameObjects::ResortUnsorted()
{
	for (C4Object *cObj : *this)
	{
		if (cObj->Unsorted)
		{
			// Readd to main object list
			Remove(cObj);
			// Reset flag so that Add correctly sorts this object
			cObj->Unsorted = false;
			if (!Add(cObj))
			{
				// readd failed: Better kill object to prevent leaking...
				Game.ClearPointers(cObj);
				delete cObj;
			}
		}
	}
}

bool C4GameObjects::ValidateOwners()
{
	// Validate in sublists
	bool fSucc = true;
	if (!C4ObjectList::ValidateOwners())
	{
		fSucc = false;
	}
	if (!InactiveObjects.ValidateOwners())
	{
		fSucc = false;
	}
	return fSucc;
}

bool C4GameObjects::AssignInfo()
{
	// Assign in sublists
	bool fSucc = true;
	if (!C4ObjectList::AssignInfo())
	{
		fSucc = false;
	}
	if (!InactiveObjects.AssignInfo())
	{
		fSucc = false;
	}
	return fSucc;
}

void C4GameObjects::AssignLightRange()
{
	for (C4Object *obj : reverse())
	{
		if (obj->Status)
		{
			obj->AssignLightRange();
		}
	}
}

void C4GameObjects::SyncClearance()
{
	for (C4Object *obj : *this)
	{
		if (obj)
		{
			obj->SyncClearance();
		}
	}
}

void C4GameObjects::OnSynchronized()
{
	for (C4Object *obj : *this)
	{
		if (obj)
		{
			obj->Call(PSF_OnSynchronized);
		}
	}
}

void C4GameObjects::ResetAudibility()
{
	for (C4Object *obj : *this)
	{
		if (obj)
		{
			obj->Audible = obj->AudiblePan = 0;
			obj->AudiblePlayer = NO_OWNER;
		}
	}
}

void C4GameObjects::SetOCF()
{
	for (C4Object *obj : *this)
	{
		if (obj->Status)
		{
			obj->SetOCF();
		}
	}
}

uint32_t C4GameObjects::GetNextMarker()
{
	// Get a new marker.
	uint32_t marker = ++LastUsedMarker;
	// If all markers are exceeded, restart marker at 1 and reset all object markers to zero.
	if (!marker)
	{
		for (C4Object *cobj : *this)
		{
			if (cobj)
			{
				cobj->Marker = 0;
			}
		}
		marker = ++LastUsedMarker;
	}
	return marker;
}
