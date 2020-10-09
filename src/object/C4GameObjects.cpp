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

bool C4GameObjects::Add(C4Object *object)
{
	// Add inactive objects to the inactive list only
	if (object->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Add(object, C4ObjectList::stMain);
	}
	// If this is a foreground object, add it to the list
	if (object->Category & C4D_Foreground)
	{
		ForeObjects.Add(object, C4ObjectList::stMain);
	}
	// Manipulate main list
	if (!C4ObjectList::Add(object, C4ObjectList::stMain))
		return false;
	// Add to sectors
	Sectors.Add(object, this);
	return true;
}


bool C4GameObjects::Remove(C4Object *object)
{
	// If it's an inactive object, simply remove it from the inactive list
	if (object->Status == C4OS_INACTIVE)
	{
		return InactiveObjects.Remove(object);
	}
	// Remove from sectors
	Sectors.Remove(object);
	// Remove from forelist
	ForeObjects.Remove(object);
	// Manipulate main list
	return C4ObjectList::Remove(object);
}

void C4GameObjects::CrossCheck() // Every Tick1 by ExecObjects
{
	// Reverse area check: Checks for all <ball> at <goal>
	// Using a sports metaphor here, because that is easier to visualize when reading the code

	// Only those objects that have one of the required OCFs will be checked for collision/collection.
	// This lets us filter the objects early, so that the loop does not get too large.
	DWORD goal_required_ocf = OCF_None | OCF_Collection | OCF_Alive;

	// Only those objects that have one of the required OCFs can cause a collision/collection
	DWORD ball_required_ocf = OCF_None | OCF_HitSpeed2;
	// Fewer checks for collection
	if (!::Game.iTick3)
	{
		ball_required_ocf |= OCF_Carryable;
	}

	for (C4Object* goal : *this)
	{
		if (goal->Status && !goal->Contained && (goal->OCF & goal_required_ocf))
		{
			uint32_t Marker = GetNextMarker();
			C4LSector *sector;
			for (C4ObjectList *in_goal_area = goal->Area.FirstObjects(&sector); in_goal_area; in_goal_area = goal->Area.NextObjects(in_goal_area, &sector))
			{
				for (C4Object* ball : *in_goal_area)
				{
					if ((ball != goal)                 // Ball should not hit itself,
					&&  ball->Status                   // it cannot hit if it was deleted,
					&& !ball->Contained                // it cannot hit if it is contained,
					&& (ball->OCF & ball_required_ocf) // it must have either of the required OFCs,
					&& (goal->Layer == ball->Layer)    // and must be in the correct layer
					// TODO: Instead of a custom check, use C4Rect::Contains with the correct coordinates
					&&  Inside<int32_t>(ball->GetX() - (goal->GetX() + goal->Shape.x), 0, goal->Shape.Wdt - 1)
					&&  Inside<int32_t>(ball->GetY() - (goal->GetY() + goal->Shape.y), 0, goal->Shape.Hgt - 1))
					{
						// Handle cross check only once
						if (ball->Marker == Marker)
						{
							continue;
						}
						ball->Marker = Marker;

						// Collision check

						// Note: the layer check was already done further above.

						if ((goal->OCF & OCF_Alive)        // <goal> must be alive,
						&&  (ball->OCF & OCF_HitSpeed2)    // <ball> is fast enough (otherwise a fast <goal> will collide when passing a non-moving ball)
						&&  (ball->Category & C4D_Object)) // <ball> is an object
						{
							C4Real relative_xdir = ball->xdir - goal->xdir;
							C4Real relative_ydir = ball->ydir - goal->ydir;
							C4Real hit_speed = relative_xdir * relative_xdir + relative_ydir * relative_ydir;
							// Only hit if the relative speed is larger than HitSpeed2, and the <goal> does not prevent getting hit
							if ((hit_speed > HitSpeed2) &&  !goal->Call(PSF_QueryCatchBlow, &C4AulParSet(ball)))
							{
								int32_t hit_energy = fixtoi(hit_speed * ball->Mass / 5);
								// Hit energy reduced to 1/3rd, but do not drop to zero because of this division.
								// However, if the hit energy is low because of either speed or <ball> mass, then
								// having it stay 0 is OK.
								if (hit_energy != 0)
								{
									hit_energy = std::max(hit_energy / 3, 1);
								}
								// Apply damage to the goal - not sure why this is divided by 5 yet again,
								// and this time we allow it being reduced to 0...
								int32_t damage = -hit_energy / 5;
								goal->DoEnergy(damage, false, C4FxCall_EngObjHit, ball->Controller);
								// Fling it around:
								// light objects will be flung with full speed,
								// heavier objects will be affected less
								int min_mass = 50;
								int goal_mass = std::max<int32_t>(goal->Mass, min_mass);
								C4PropList* pActionDef = goal->GetAction();
								if (!::Game.iTick3 || (pActionDef && pActionDef->GetPropertyP(P_Procedure) != DFA_FLIGHT))
								{
									goal->Fling(ball->xdir * min_mass / goal_mass, -Abs(ball->ydir / 2) * min_mass / goal_mass, false);
								}
								// Callback with the damage value
								goal->Call(PSF_CatchBlow, &C4AulParSet(damage, ball));
								// <goal> might have been tampered with
								if (!goal->Status || goal->Contained || !(goal->OCF & goal_required_ocf))
								{
									goto check_next_goal;
								}
								// Skip collection check
								continue;
							}
						}

						// Collection check

						// Note: the layer check was already done further above.
						// This is confusing, because this requires
						// the ball to be both inside the goal shape AND the goal collection area, so
						// collection areas that go further than the goal shape are useless, as well
						// as collection areas that are entirely outside of the goal shape.

						if ((goal->OCF & OCF_Collection) // <goal> has a collection area?
						&&  (ball->OCF & OCF_Carryable)  // <ball> can be collected?
						// TODO: Instead of a custom check, use C4Rect::Contains with the correct coordinates
						&&  Inside<int32_t>(ball->GetX() - (goal->GetX() + goal->Def->Collection.x), 0, goal->Def->Collection.Wdt - 1)
						&&  Inside<int32_t>(ball->GetY() - (goal->GetY() + goal->Def->Collection.y), 0, goal->Def->Collection.Hgt - 1))
						{
							goal->Collect(ball);
							// <goal> might have been tampered with
							if (!goal->Status || goal->Contained || !(goal->OCF & goal_required_ocf))
							{
								goto check_next_goal;
							}
						}
					}
				}
			}
			// Goto-marker for more obvious loop-control
			check_next_goal: ;
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
	C4PropList *object = C4PropListNumbered::GetByNumber(object_number);
	if (object)
	{
		return object->GetObject();
	}
	return nullptr;
}

C4Object *C4GameObjects::SafeObjectPointer(int32_t object_number)
{
	C4Object *object = ObjectPointer(object_number);
	if (object && !object->Status)
	{
		return nullptr;
	}
	return object;
}

void C4GameObjects::UpdateSolidMasks()
{
	for (C4Object *object : *this)
	{
		if (object->Status)
		{
			object->UpdateSolidMask(false);
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
	for (C4Object *object : reverse())
	{
		// Keep track of numbers
		max_object_number = std::max(max_object_number, object->Number);
		// Add to list of foreobjects
		if (object->Category & C4D_Foreground)
		{
			ForeObjects.Add(object, C4ObjectList::stMain, this);
		}
		// Unterminate end
	}

	// Denumerate pointers:
	// On section load, inactive object numbers will be adjusted afterwards,
	// so fake inactive object list empty meanwhile.
	// Note: this has to be done to prevent an assertion fail when denumerating
	// non-enumerated inactive objects, even if object numbers did not collide.
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
	for (C4Object *object : *this)
	{
		if (object->Status)
		{
			// Staticback must not have speed
			if (object->Category & C4D_StaticBack)
			{
				object->xdir = object->ydir = 0;
			}
			// contained must be in contents list
			if (object->Contained)
			{
				if (!object->Contained->Contents.GetLink(object))
				{
					DebugLogF("Error in Objects.txt: Container of #%d is #%d, but not found in contents list!", object->Number, object->Contained->Number);
					object->Contained->Contents.Add(object, C4ObjectList::stContents);
				}
			}
			// All contents must have contained set; otherwise, remove them!
			auto contents_iterator = object->Contents.begin();
			while (!contents_iterator.atEnd())
			{
				C4Object* contained_object = *contents_iterator;
				// Check double links
				auto contents_iterator2 = object->Contents.begin();
				if (contents_iterator2.find(contained_object) && contents_iterator2 != contents_iterator)
				{
					DebugLogF("Error in Objects.txt: Double containment of #%d by #%d!", contained_object->Number, object->Number);
					// This remove-call will only remove the previous (doubled) link, so cLnkCont should be save
					object->Contents.Remove(contained_object);
					// Contents checked already
					continue;
				}
				// Check contents/contained-relation
				if (contained_object->Status && contained_object->Contained != object)
				{
					DebugLogF("Error in Objects.txt: Object #%d not in container #%d as referenced!", contained_object->Number, object->Number);
					contained_object->Contained = object;
				}
				contents_iterator++;
			}
		}
	}
	// Sort out inactive objects
	for (C4Object *object : *this)
	{
		if (object->Status == C4OS_INACTIVE)
		{
			Remove(object);
			InactiveObjects.Add(object, C4ObjectList::stNone);
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
	for (C4Object *object : *this)
	{
		if (object->Status)
		{
			// Add to plrview
			object->UpdateLight();
			// Update flipdir (for old objects.txt with no flipdir defined),
			// assigns Action.DrawDir as well
			object->UpdateFlipDir();
			// Initial OCF update
			object->SetOCF();
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
	for (C4Object *object : *this)
	{
		if (object && (object->Category & (C4D_Goal | C4D_Rule | C4D_Environment)) && object->Status)
		{
			C4Value vResult = object->Call(function_name, parameters, pass_error);
			// Rejection tests abort on first nonzero result
			if (reject_test && !!vResult)
			{
				return vResult;
			}
		}
	}
	return C4Value();
}

void C4GameObjects::UpdatePos(C4Object *object)
{
	// Position might have changed. Update sector lists
	Sectors.Update(object, this);
}

void C4GameObjects::UpdatePosResort(C4Object *object)
{
	// Object order for this object was changed. Readd object to sectors
	Sectors.Remove(object);
	Sectors.Add(object, this);
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
	for (C4Object *object : *this)
	{
		if (object->Unsorted)
		{
			// Readd to main object list
			Remove(object);
			// Reset flag so that Add correctly sorts this object
			object->Unsorted = false;
			if (!Add(object))
			{
				// readd failed: Better kill object to prevent leaking...
				Game.ClearPointers(object);
				delete object;
			}
		}
	}
}

bool C4GameObjects::ValidateOwners()
{
	// Validate in sublists
	// Note: Both functions need to be called,
	// before the evaluation. Do not call foo() && bar()
	// because then only foo() will be evaluated.
	bool object_list_valid = C4ObjectList::ValidateOwners();
	bool inactive_objects_valid = InactiveObjects.ValidateOwners();
	return object_list_valid && inactive_objects_valid;
}

bool C4GameObjects::AssignInfo()
{
	// Assign in sublists
	// Note: Both functions need to be called,
	// before the evaluation. Do not call foo() && bar()
	// because then only foo() will be evaluated.
	bool object_list_assigned = C4ObjectList::AssignInfo();
	bool inactive_objects_assigned = InactiveObjects.AssignInfo();
	return object_list_assigned && inactive_objects_assigned;
}

void C4GameObjects::AssignLightRange()
{
	for (C4Object *object : reverse())
	{
		if (object->Status)
		{
			object->AssignLightRange();
		}
	}
}

void C4GameObjects::SyncClearance()
{
	for (C4Object *object : *this)
	{
		if (object)
		{
			object->SyncClearance();
		}
	}
}

void C4GameObjects::OnSynchronized()
{
	for (C4Object *object : *this)
	{
		if (object)
		{
			object->Call(PSF_OnSynchronized);
		}
	}
}

void C4GameObjects::ResetAudibility()
{
	for (C4Object *object : *this)
	{
		if (object)
		{
			object->Audible = object->AudiblePan = 0;
			object->AudiblePlayer = NO_OWNER;
		}
	}
}

void C4GameObjects::SetOCF()
{
	for (C4Object *object : *this)
	{
		if (object->Status)
		{
			object->SetOCF();
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
		for (C4Object *object : *this)
		{
			if (object)
			{
				object->Marker = 0;
			}
		}
		marker = ++LastUsedMarker;
	}
	return marker;
}
