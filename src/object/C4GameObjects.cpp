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

void C4GameObjects::Init(int32_t width, int32_t height)
{
	// Init sectors
	Sectors.Init(width, height);
}

bool C4GameObjects::Add(C4Object *game_object)
{
	// Add inactive objects to the inactive list only
	if (game_object->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Add(game_object, C4ObjectList::stMain);
	}
	// If this is a foreground object, add it to the list
	if (game_object->Category & C4D_Foreground)
	{
		ForeObjects.Add(game_object, C4ObjectList::stMain);
	}
	// Manipulate main list
	if (!C4ObjectList::Add(game_object, C4ObjectList::stMain))
		return false;
	// Add to sectors
	Sectors.Add(game_object, this);
	return true;
}


bool C4GameObjects::Remove(C4Object *game_object)
{
	// If it's an inactive object, simply remove from the inactiv elist
	if (game_object->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Remove(game_object);
	}
	// Remove from sectors
	Sectors.Remove(game_object);
	// Remove from forelist
	ForeObjects.Remove(game_object);
	// Manipulate main list
	return C4ObjectList::Remove(game_object);
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

void C4GameObjects::Synchronize()
{
	// Synchronize unsorted objects
	ResortUnsorted();
	// Synchronize solidmasks
	UpdateSolidMasks();
}

C4Object *C4GameObjects::ObjectPointer(int32_t object_number)
{
	// Search own list
	C4PropList *game_object = C4PropListNumbered::GetByNumber(object_number);
	if (game_object)
	{
		return game_object->GetObject();
	}
	return nullptr;
}

C4Object *C4GameObjects::SafeObjectPointer(int32_t object_number)
{
	C4Object *game_object = ObjectPointer(object_number);
	if (game_object)
	{
		if (!game_object->Status)
		{
			return nullptr;
		}
	}
	return game_object;
}

void C4GameObjects::UpdateSolidMasks()
{
	for (C4Object *game_object : *this)
	{
		if (game_object->Status)
		{
			game_object->UpdateSolidMask(false);
		}
	}
}

void C4GameObjects::DeleteObjects(bool delete_inactive_objects)
{
	C4ObjectList::DeleteObjects();
	Sectors.ClearObjects();
	ForeObjects.Clear();
	if (delete_inactive_objects)
	{
		InactiveObjects.DeleteObjects();
	}
}

void C4GameObjects::Clear(bool clear_inactive_objects)
{
	DeleteObjects(clear_inactive_objects);
	if (clear_inactive_objects)
	{
		InactiveObjects.Clear();
	}
	LastUsedMarker = 0;
}

int C4GameObjects::PostLoad(bool keep_inactive_objects, C4ValueNumbers *numbers)
{
	// Process objects
	int32_t max_object_number = 0;
	for (C4Object *game_object : reverse())
	{
		// Keep track of numbers
		max_object_number = std::max(max_object_number, game_object->Number);
		// Add to list of foreobjects
		if (game_object->Category & C4D_Foreground)
		{
			ForeObjects.Add(game_object, C4ObjectList::stMain, this);
		}
		// Unterminate end
	}

	// Denumerate pointers:
	// On section load, inactive object numbers will be adjusted afterwards,
	// so fake inactive object list empty. Meanwhile, note this has to be done
	// to prevent an assertion fail when denumerating non-enumerated inactive objects,
	// even if object numbers did not collide.
	C4ObjectList inactiveObjectsCopy;
	if (keep_inactive_objects)
	{
		inactiveObjectsCopy.Copy(InactiveObjects);
		InactiveObjects.Clear();
	}
	// Denumerate pointers
	Denumerate(numbers);
	// Update object enumeration index now, because calls like OnSynchronized might create objects
	C4PropListNumbered::SetEnumerationIndex(max_object_number);
	// end faking and adjust object numbers
	if (keep_inactive_objects)
	{
		InactiveObjects.Copy(inactiveObjectsCopy);
		inactiveObjectsCopy.Clear();
		C4PropListNumbered::UnshelveNumberedPropLists();
	}

	// Special checks:
	// -contained/contents-consistency
	// -StaticBack-objects zero speed
	for (C4Object *game_object : *this)
	{
		if (game_object->Status)
		{
			// Staticback must not have speed
			if (game_object->Category & C4D_StaticBack)
			{
				game_object->xdir = game_object->ydir = 0;
			}
			// contained must be in contents list
			if (game_object->Contained)
			{
				if (!game_object->Contained->Contents.GetLink(game_object))
				{
					DebugLogF("Error in Objects.txt: Container of #%d is #%d, but not found in contents list!", game_object->Number, game_object->Contained->Number);
					game_object->Contained->Contents.Add(game_object, C4ObjectList::stContents);
				}
			}
			// All contents must have contained set; otherwise, remove them!
			auto contents_iterator = game_object->Contents.begin();
			while (!contents_iterator.atEnd())
			{
				C4Object* contained_object = *contents_iterator;
				// Check double links
				auto contents_iterator2 = game_object->Contents.begin();
				if (contents_iterator2.find(contained_object) && contents_iterator2 != contents_iterator)
				{
					DebugLogF("Error in Objects.txt: Double containment of #%d by #%d!", contained_object->Number, game_object->Number);
					// This remove-call will only remove the previous (doubled) link, so cLnkCont should be save
					game_object->Contents.Remove(contained_object);
					// Contents checked already
					continue;
				}
				// Check contents/contained-relation
				if (contained_object->Status && contained_object->Contained != game_object)
				{
					DebugLogF("Error in Objects.txt: Object #%d not in container #%d as referenced!", contained_object->Number, game_object->Number);
					contained_object->Contained = game_object;
				}
				contents_iterator++;
			}
		}
	}
	// Sort out inactive objects
	for (C4Object *game_object : *this)
	{
		if (game_object->Status == C4OS_INACTIVE)
		{
			Remove(game_object);
			InactiveObjects.Add(game_object, C4ObjectList::stNone);
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
	for (C4Object *game_object : *this)
	{
		if (game_object->Status)
		{
			// Add to plrview
			game_object->UpdateLight();
			// Update flipdir (for old objects.txt with no flipdir defined),
			// assigns Action.DrawDir as well
			game_object->UpdateFlipDir();
			// Initial OCF update
			game_object->SetOCF();
		}
	}
	// Done
	return ObjectCount();
}

void C4GameObjects::Denumerate(C4ValueNumbers *numbers)
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

C4Value C4GameObjects::GRBroadcast(const char *function_name, C4AulParSet *parameters, bool pass_error, bool reject_test)
{
	// Call objects first - scenario script might overwrite hostility, etc...
	for (C4Object *game_object : *this)
	{
		if (game_object && (game_object->Category & (C4D_Goal | C4D_Rule | C4D_Environment)) && game_object->Status)
		{
			C4Value vResult = game_object->Call(function_name, parameters, pass_error);
			// Rejection tests abort on first nonzero result
			if (reject_test && !!vResult)
			{
				return vResult;
			}
		}
	}
	return C4Value();
}

void C4GameObjects::UpdatePos(C4Object *game_object)
{
	// Position might have changed. Update sector lists
	Sectors.Update(game_object, this);
}

void C4GameObjects::UpdatePosResort(C4Object *game_object)
{
	// Object order for this object was changed. Readd object to sectors
	Sectors.Remove(game_object);
	Sectors.Add(game_object, this);
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
	for (C4Object *game_object : *this)
	{
		if (game_object->Unsorted)
		{
			// Readd to main object list
			Remove(game_object);
			// Reset flag so that Add correctly sorts this object
			game_object->Unsorted = false;
			if (!Add(game_object))
			{
				// readd failed: Better kill object to prevent leaking...
				Game.ClearPointers(game_object);
				delete game_object;
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
	for (C4Object *game_object : reverse())
	{
		if (game_object->Status)
		{
			game_object->AssignLightRange();
		}
	}
}

void C4GameObjects::SyncClearance()
{
	for (C4Object *game_object : *this)
	{
		if (game_object)
		{
			game_object->SyncClearance();
		}
	}
}

void C4GameObjects::OnSynchronized()
{
	for (C4Object *game_object : *this)
	{
		if (game_object)
		{
			game_object->Call(PSF_OnSynchronized);
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
