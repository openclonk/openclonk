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

/* Dynamic object list */

#include "C4Include.h"
#include "object/C4ObjectList.h"

#include "game/C4Application.h"
#include "graphics/C4GraphicsResource.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"

static const C4ObjectLink NULL_LINK = { nullptr, nullptr, nullptr };

C4ObjectList::C4ObjectList()
{
	Default();
}

C4ObjectList::C4ObjectList(const C4ObjectList &List): FirstIter(nullptr)
{
	Default();
	Copy(List);
}

C4ObjectList::~C4ObjectList()
{
	Clear();
}

void C4ObjectList::Clear()
{
	C4ObjectLink *link;
	C4ObjectLink *next_link;
	for (link = First; link; link = next_link)
	{
		next_link = link->Next;
		delete link;
	}
	First = Last = nullptr;
	if (pEnumerated)
	{
		delete pEnumerated;
		pEnumerated = nullptr;
	}

	for (iterator* it = FirstIter; it; it = it->Next)
	{
		it->link = NULL_LINK;
	}
}

const int MaxTempListID = 500;
C4ID TempListID[MaxTempListID];

C4ID C4ObjectList::GetListID(int32_t dwCategory, int index) const
{
	C4ObjectLink *link;

	// Create a temporary list of all id's and counts
	for (int i = 0; i < MaxTempListID; i++)
	{
		TempListID[i] = C4ID::None;
	}
	for (link = First; link && link->Obj; link = link->Next)
	{
		if (link->Obj->Status)
		{
			C4Def *def;
			if ((dwCategory==C4D_All) || ( (def = C4Id2Def(link->Obj->Def->id)) && (def->Category & dwCategory) ))
			{
				for (int i = 0; i < MaxTempListID; i++)
				{
					// Already there
					if (TempListID[i] == link->Obj->Def->id)
					{
						break;
					}
					// End of list, add id
					if (TempListID[i] == C4ID::None)
					{
						TempListID[i] = link->Obj->Def->id;
						break;
					}
				}
			}
		}
	}

	// Returns indexed id
	if (Inside(index, 0, MaxTempListID - 1))
	{
		return TempListID[index];
	}

	return C4ID::None;
}

int C4ObjectList::ListIDCount(int32_t dwCategory) const
{
	C4ObjectLink *link;

	// Create a temporary list of all id's and counts
	for (int clid = 0; clid < MaxTempListID; clid++)
	{
		TempListID[clid] = C4ID::None;
	}
	for (link = First; link && link->Obj; link = link->Next)
	{
		if (link->Obj->Status)
		{
			C4Def *def;
			if ((dwCategory == C4D_All) || ( (def = C4Id2Def(link->Obj->Def->id)) && (def->Category & dwCategory) ))
			{
				for (int clid = 0; clid < MaxTempListID; clid++)
				{
					// Already there
					if (TempListID[clid] == link->Obj->Def->id)
					{
						break;
					}
					// End of list, add id
					if (TempListID[clid] == C4ID::None)
					{
						TempListID[clid] = link->Obj->Def->id;
						break;
					}
				}
			}
		}
	}

	// Count different id's
	for (int i = 0; i < MaxTempListID; i++)
	{
		if (TempListID[i] == C4ID::None)
		{
			return i;
		}
	}

	return MaxTempListID;
}



bool C4ObjectList::Add(C4Object *new_obj, SortType sort_type, C4ObjectList *sorted_list)
{
	if (!new_obj || !new_obj->Def || !new_obj->Status)
	{
		return false;
	}

#ifdef _DEBUG
	if (sort_type == stMain)
	{
		CheckCategorySort();
		if (sorted_list)
		{
			assert(CheckSort(sorted_list));
		}
	}
#endif

	// Debug: don't do double links
	assert(!GetLink(new_obj));

	// No self-sort
	assert(sorted_list != this);

	// Allocate new link
	C4ObjectLink *new_link = new C4ObjectLink;
	if (!new_link)
	{
		return false;
	}
	// Set link
	new_link->Obj = new_obj;

	// Search insert position (default: end of list)
	C4ObjectLink *current = nullptr;
	C4ObjectLink *previous = Last;

	// Should sort?
	if (sort_type == stReverse)
	{
		// Reverse sort: Add to beginning of list
		current = First;
		previous = nullptr;
	}
	else if (sort_type)
	{
		// Sort override? Leave default as is.
		bool is_sorted = !(new_obj->Unsorted);
		if (is_sorted)
		{
			// Sort by master list?
			if (sorted_list)
			{
				previous = nullptr;
				current = First;
				while (current && (!current->Obj->Status || current->Obj->Unsorted))
				{
					current = current->Next;
				}

#ifndef _DEBUG
				if (current)
#endif
				{
					C4ObjectLink* link2;
					for (link2 = sorted_list->First; link2; link2 = link2->Next)
					{
						if (link2->Obj->Status && !link2->Obj->Unsorted)
						{
							if (link2->Obj == new_obj)
							{
								assert(!current || current->Obj != new_obj);
								break;
							}

							if (current && link2->Obj == current->Obj)
							{
								previous = current;
								current = current->Next;
								while (current && (!current->Obj->Status || current->Obj->Unsorted))
								{
									current = current->Next;
								}
								
#ifndef _DEBUG
								if (!current)
								{
									break;
								}
#endif
							}
						}
					}

					assert(link2 != nullptr);
				}
			}
			else
			{
				// No master list: Find successor by matching Plane / id
				// Sort by matching Plane/id is necessary for inventory shifting.
				// It is not done for static back to allow multiobject outside structure.
				// Unsorted objects are ignored in comparison.
				if (!(new_obj->Category & C4D_StaticBack))
				{
					for (previous = nullptr, current = First; current; current = current->Next)
					{
						if (current->Obj->Status && !current->Obj->Unsorted)
						{
							if ((current->Obj->GetPlane() == new_obj->GetPlane())
							&&  (current->Obj->id == new_obj->id))
							{
								break;
							}
							previous = current;
						}
					}
				}

				// Find successor by relative category
				if (!current)
				{
					for (previous = nullptr, current = First; current; current = current->Next)
					{
						if (current->Obj->Status && !current->Obj->Unsorted)
						{
							if (current->Obj->GetPlane() <= new_obj->GetPlane())
							{
								break;
							}
							previous = current;
						}
					}
				}
			}

			current = previous ? previous->Next : First;
		}
	}

	assert(!previous || previous->Next == current);
	assert(!current || current->Prev == previous);

	// Insert new link after predecessor
	InsertLink(new_link, previous);

#ifdef _DEBUG
	// Debug: Check sort
	if (sort_type == stMain)
	{
		CheckCategorySort();
		if (sorted_list)
		{
			assert(CheckSort(sorted_list));
		}
	}
#endif

	// Add mass
	Mass += new_obj->Mass;

	return true;
}

bool C4ObjectList::Remove(C4Object *obj)
{
	C4ObjectLink *link;

	// Find link
	for (link = First; link; link = link->Next)
	{
		if (link->Obj == obj)
		{
			break;
		}
	}
	if (!link)
	{
		return false;
	}

	// Fix iterators
	for (iterator * it = FirstIter; it; it = it->Next)
	{
		// Adjust pointers of internal link field
		if (it->link.Prev == link)
		{
			it->link.Prev = link->Prev;
		}
		else if (it->link.Next == link)
		{
			it->link.Next = link->Next;
		}
		else if (it->link.Obj == link->Obj)
		{
			it->link.Obj = nullptr;
		}
	}

	// Remove link from list
	RemoveLink(link);

	// Deallocate link
	delete link;

	// Remove mass
	Mass -= obj->Mass;
	if (Mass < 0)
	{
		Mass = 0;
	}

#if defined(_DEBUG)
	assert(!GetLink(obj));
#endif

	return true;
}

C4Object* C4ObjectList::Find(C4Def * def, int owner, DWORD dwOCF)
{
	C4ObjectLink *link;
	// Find link and object
	for (link = First; link; link = link->Next)
	{
		if ((link->Obj->Status)
		&&  (link->Obj->Def == def)
		&&  ((owner == ANY_OWNER) || (link->Obj->Owner == owner))
		&&  (dwOCF & link->Obj->OCF))
		{
			return link->Obj;
		}
	}
	return nullptr;
}

C4Object* C4ObjectList::FindOther(C4ID id, int owner)
{
	C4ObjectLink *link;
	// Find link and object
	for (link = First; link; link = link->Next)
	{
		if ((link->Obj->Status)
		&&  (link->Obj->Def->id!=id)
		&&  ((owner==ANY_OWNER) || (link->Obj->Owner == owner)))
		{
			return link->Obj;
		}
	}
	return nullptr;
}

C4Object* C4ObjectList::GetObject(int index) const
{
	int cIdx;
	C4ObjectLink *link;
	// Find link and object
	for (link = First, cIdx = 0; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			if (cIdx == index)
			{
				return link->Obj;
			}
			cIdx++;
		}
	}
	return nullptr;
}

const C4ObjectLink* C4ObjectList::GetLink(const C4Object *obj) const
{
	if (!obj)
	{
		return nullptr;
	}
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj == obj)
		{
			return link;
		}
	}
	return nullptr;
}

int C4ObjectList::ObjectCount(C4ID id) const
{
	C4ObjectLink *link;
	int count = 0;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status && (id == C4ID::None || link->Obj->Def->id == id))
		{
			count++;
		}
	}
	return count;
}

int C4ObjectList::MassCount()
{
	C4ObjectLink *link;
	int mass = 0;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			mass += link->Obj->Mass;
		}
	}
	Mass = mass;
	return mass;
}

int C4ObjectList::ClearPointers(C4Object *obj)
{
	int removed_amount = 0;
	// Clear all primary list pointers
	while (Remove(obj))
	{
		removed_amount++;
	}
	// Clear all sub pointers
	C4Object *current_obj;
	C4ObjectLink *link;
	for (link = First; link && (current_obj = link->Obj); link = link->Next)
	{
		current_obj->ClearPointers(obj);
	}
	return removed_amount;
}

void C4ObjectList::Draw(C4TargetFacet &cgo, int player, int MinPlane, int MaxPlane)
{
	C4ObjectLink *link;
	C4ObjectLink *first;
	for (first = Last; first; first = first->Prev)
	{
		if (first->Obj->GetPlane() >= MinPlane)
		{
			break;
		}
	}
	// Draw objects (base)
	for (link = first; link; link = link->Prev)
	{
		if (link->Obj->GetPlane() > MaxPlane)
		{
			break;
		}
		if (link->Obj->Category & C4D_Foreground)
		{
			continue;
		}
		link->Obj->Draw(cgo, player);
	}
	// Draw objects (top face)
	for (link = first; link; link = link->Prev)
	{
		if (link->Obj->GetPlane() > MaxPlane)
		{
			break;
		}
		if (link->Obj->Category & C4D_Foreground)
		{
			continue;
		}
		link->Obj->DrawTopFace(cgo, player);
	}
}

void C4ObjectList::DrawIfCategory(C4TargetFacet &cgo, int player, uint32_t dwCategory, bool invert)
{
	C4ObjectLink *link;
	// Draw objects (base)
	for (link = Last; link; link = link->Prev)
	{
		if (!(link->Obj->Category & dwCategory) == invert)
		{
			link->Obj->Draw(cgo, player);
		}
	}
	// Draw objects (top face)
	for (link = Last; link; link = link->Prev)
	{
		if (!(link->Obj->Category & dwCategory) == invert)
		{
			link->Obj->DrawTopFace(cgo, player);
		}
	}
}

bool C4ObjectList::IsContained(const C4Object *obj) const
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj == obj)
		{
			return true;
		}
	}
	return false;
}

bool C4ObjectList::IsClear() const
{
	return ObjectCount() == 0;
}

bool C4ObjectList::DenumeratePointers()
{
	if (!pEnumerated)
	{
		return false;
	}
	// Denumerate all object pointers
	for (std::list<int32_t>::const_iterator pNum = pEnumerated->begin(); pNum != pEnumerated->end(); ++pNum)
	{
		Add(::Objects.ObjectPointer(*pNum), stNone); // Add to tail, unsorted
	}
	// Delete old list
	delete pEnumerated;
	pEnumerated = nullptr;
	return true;
}

bool C4ObjectList::Write(char *szTarget)
{
	char ostr[25];
	szTarget[0] = 0;
	C4ObjectLink *link;
	for (link = First; link && link->Obj; link = link->Next)
	{
		if (link->Obj->Status)
		{
			sprintf(ostr, "%d;", link->Obj->Number);
			SAppend(ostr, szTarget);
		}
	}
	return true;
}

void C4ObjectList::Denumerate(C4ValueNumbers * numbers)
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			link->Obj->Denumerate(numbers);
		}
	}
}

void C4ObjectList::CompileFunc(StdCompiler *pComp, bool skip_player_objects, C4ValueNumbers * numbers)
{
	// "Object" section count
	int32_t object_count = ObjectCount();
	pComp->Value(mkNamingCountAdapt(object_count, "Object"));
	if (pComp->isSerializer())
	{
		// skipping player objects would screw object counting in non-naming compilers
		assert(!skip_player_objects || pComp->hasNaming());
		// Decompile all objects in reverse order
		for (C4ObjectLink *link = Last; link; link = link->Prev)
		{
			if (link->Obj->Status && (!skip_player_objects || !link->Obj->IsUserPlayerObject()))
			{
				pComp->Value(mkNamingAdapt(mkParAdapt(*link->Obj, numbers), "Object"));
			}
		}
	}
	else
	{
		// FIXME: Check that no PlayerObjects are loaded when skip_player_objects is true
		// i.e. that loading and saving was done with the same flag.
		// Remove previous data
		Clear();
		// Load objects, add them to the list.
		for (int i = 0; i < object_count; i++)
		{
			C4Object *obj = nullptr;
			try
			{
				pComp->Value(mkNamingAdapt(mkParAdapt(mkPtrAdaptNoNull(obj), numbers), "Object"));
				Add(obj, stReverse);
			}
			catch (StdCompiler::Exception *exception)
			{
				// Failsafe object loading: If an error occurs during object loading, just skip that object and load the next one
				if (!exception->Pos.getLength())
				{
					LogF("ERROR: Object loading: %s", exception->Msg.getData());
				}
				else
				{
					LogF("ERROR: Object loading(%s): %s", exception->Pos.getData(), exception->Msg.getData());
				}
				delete exception;
			}
		}
	}
}

void C4ObjectList::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	// (Re)create list
	delete pEnumerated;
	pEnumerated = new std::list<int32_t>();
	// Decompiling: Build list
	if (!pComp->isDeserializer())
	{
		for (C4ObjectLink *link = First; link; link = link->Next)
		{
			if (link->Obj->Status)
			{
				pEnumerated->push_back(link->Obj->Number);
			}
		}
	}
	// Compile list
	pComp->Value(mkSTLContainerAdapt(*pEnumerated, StdCompiler::SEP_SEP2));
	// Decompiling: Delete list
	if (!pComp->isDeserializer())
	{
		delete pEnumerated;
		pEnumerated = nullptr;
	}
	// Compiling: Nothing to do - list will be denumerated later
}

StdStrBuf C4ObjectList::GetNameList(C4DefList &defs) const
{
	C4ID id;
	StdStrBuf Buf;
	for (int i = 0; (id = GetListID(C4D_All, i)); i++)
	{
		C4Def *current_def = defs.ID2Def(id);
		if (current_def)
		{
			int idcount = ObjectCount(id);
			if (i > 0)
			{
				Buf.Append(", ");
			}
			Buf.AppendFormat("%dx %s", idcount, current_def->GetName());
		}
	}
	return Buf;
}

bool C4ObjectList::ValidateOwners()
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			link->Obj->ValidateOwner();
		}
	}
	return true;
}

bool C4ObjectList::AssignInfo()
{
	// the list seems to be traced backwards here, to ensure crew objects are added in correct order
	// (or semi-correct, because this will work only if the crew order matches the main object list order)
	// this is obsolete now, because the crew list is stored in the savegame
	C4ObjectLink *link;
	for (link = Last; link; link = link->Prev)
	{
		if (link->Obj->Status)
		{
			link->Obj->AssignInfo();
		}
	}
	return true;
}

void C4ObjectList::ClearInfo(C4ObjectInfo *pInfo)
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			link->Obj->ClearInfo(pInfo);
		}
	}
}

void C4ObjectList::Sort()
{
	C4ObjectLink *link;
	bool is_sorted;
	// Sort by id
	do
	{
		is_sorted = true;
		for (link = First; link && link->Next; link = link->Next)
		{
			if (link->Obj->id > link->Next->Obj->id)
			{
				RemoveLink(link);
				InsertLink(link, link->Next);
				is_sorted = false;
				break;
			}
		}
	}
	while (!is_sorted);
}

void C4ObjectList::RemoveLink(C4ObjectLink *link)
{
	// Table format is OK here
	if (link->Prev) { link->Prev->Next = link->Next; }  else  { First = link->Next; }
	if (link->Next) { link->Next->Prev = link->Prev; }  else  {  Last = link->Prev; }
}

void C4ObjectList::InsertLink(C4ObjectLink *link, C4ObjectLink *after_link)
{
	// Insert after
	if (after_link)
	{
		link->Prev = after_link;
		link->Next = after_link->Next;
		if (after_link->Next)
		{
			after_link->Next->Prev = link;
		}
		else
		{
			Last = link;
		}
		after_link->Next = link;
	}
	// Insert at head
	else
	{
		link->Prev = nullptr;
		link->Next = First;
		if (First)
		{
			First->Prev = link;
		}
		else
		{
			Last = link;
		}
		First = link;
	}

	// Adjust iterators
	if (after_link)
	{
		for (iterator* it = FirstIter; it; it = it->Next)
		{
			if (it->link.Obj == after_link->Obj)
			{
				it->link.Next = link;
			}
		}
	}
}

void C4ObjectList::InsertLinkBefore(C4ObjectLink *link, C4ObjectLink *before_link)
{
	// Insert before
	if (before_link)
	{
		link->Prev = before_link->Prev;
		if (before_link->Prev)
		{
			before_link->Prev->Next = link;
		}
		else
		{
			First = link;
		}
		link->Next = before_link;
		before_link->Prev = link;
	}
	// Insert at end
	else
	{
		link->Next = nullptr;
		link->Prev = Last;
		if (Last)
		{
			Last->Next = link;
		}
		else
		{
			First = link;
		}
		Last = link;
	}

	// Adjust iterators
	if (before_link)
	{
		for (iterator* it = FirstIter; it; it = it->Next)
		{
			if (it->link.Obj == before_link->Obj)
			{
				it->link.Prev = link;
			}
		}
	}
}


void C4NotifyingObjectList::InsertLinkBefore(C4ObjectLink *link, C4ObjectLink *before_link)
{
	C4ObjectList::InsertLinkBefore(link, before_link);
	ObjectListChangeListener.OnObjectAdded(this, link);
}

void C4NotifyingObjectList::InsertLink(C4ObjectLink *link, C4ObjectLink *after_link)
{
	C4ObjectList::InsertLink(link, after_link);
	ObjectListChangeListener.OnObjectAdded(this, link);
}

void C4NotifyingObjectList::RemoveLink(C4ObjectLink *link)
{
	C4ObjectList::RemoveLink(link);
	ObjectListChangeListener.OnObjectRemove(this, link);
}

void C4ObjectList::UpdateGraphics(bool graphics_changed)
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			link->Obj->UpdateGraphics(graphics_changed);
		}
	}
}

void C4ObjectList::UpdateFaces(bool update_shapes)
{
	C4ObjectLink *link;
	for (link = First; link; link = link->Next)
	{
		if (link->Obj->Status)
		{
			link->Obj->UpdateFace(update_shapes);
		}
	}
}

void C4ObjectList::DrawSelectMark(C4TargetFacet &cgo) const
{
	C4ObjectLink *link;
	for (link = Last; link; link = link->Prev)
	{
		link->Obj->DrawSelectMark(cgo);
	}
}

void C4ObjectList::CloseMenus()
{
	C4Object *obj;
	C4ObjectLink *link;
	for (link = First; link && (obj = link->Obj); link = link->Next)
	{
		obj->CloseMenu(true);
	}
}

void C4ObjectList::Copy(const C4ObjectList &list)
{
	Clear(); Default();
	C4ObjectLink *link;
	for (link = list.First; link; link = link->Next)
	{
		Add(link->Obj, C4ObjectList::stNone);
	}
}

void C4ObjectList::Default()
{
	First = Last = nullptr;
	Mass = 0;
	pEnumerated = nullptr;
}

bool C4ObjectList::ShiftContents(C4Object *new_first_obj)
{
	// Get link of new first (this ensures list is not empty)
	C4ObjectLink *new_first_link = GetLink(new_first_obj);
	if (!new_first_link)
	{
		return false;
	}
	// Already at front?
	if (new_first_link == First)
	{
		return true;
	}
	// Sort it there:
	// 1. Make cyclic list
	Last->Next = First;
	First->Prev = Last;
	// 2. Re-set first and last
	First = new_first_link;
	Last = new_first_link->Prev;
	// 3. Uncycle list
	First->Prev = Last->Next = nullptr;
	// Done, success
	return true;
}

void C4ObjectList::DeleteObjects()
{
	// Delete links and objects
	while (First)
	{
		C4Object *obj = First->Obj;
		if (obj->Status)
		{
			Game.ClearPointers(obj); // Clear pointers to removed objects that weren't deleted (game end or section change)
		}
		obj->Status = C4OS_DELETED;
		Remove(obj);
		delete obj;
	}
	// Reset mass
	Mass = 0;
}


// -------------------------------------------------
// C4ObjectListIterator

C4Object *C4ObjectListIterator::GetNext(int32_t *piCount)
{
	// End reached?
	if (pCurrID == rList.end())
	{
		return nullptr;
	}
	// Not yet started?
	if (pCurr == rList.end())
	{
		// Then start at first ID list head
		pCurr = pCurrID;
	}
	else if (++pCurr == rList.end()) // Next item
	{
		return nullptr;
	}
	// Next ID section reached?
	if ((*pCurr)->id != (*pCurrID)->id)
	{
		pCurrID = pCurr;
	}
	else
	{
		// Otherwise, it must be checked, whether this is a duplicate item already iterated
		// if so, advance the list
		for (C4ObjectList::iterator pCheck = pCurrID; pCheck != pCurr; ++pCheck)
		{
			if ((*pCheck)->CanConcatPictureWith(*pCurr))
			{
				// Next object of matching category
				if (++pCurr == rList.end())
				{
					return nullptr;
				}
				// Next ID chunk reached?
				if ((*pCurr)->id != (*pCurrID)->id)
				{
					// then break here
					pCurrID = pCurr;
					break;
				}
				// restart check for next object
				pCheck = pCurrID;
			}
		}
	}
	if (piCount)
	{
		// Default count
		*piCount = 1;
		// Add additional objects of same ID to the count
		C4ObjectList::iterator pCheck(pCurr);
		for (++pCheck; pCheck != rList.end() && (*pCheck)->id == (*pCurr)->id; ++pCheck)
		{
			if ((*pCheck)->CanConcatPictureWith(*pCurr))
			{
				++*piCount;
			}
		}
	}
	// return found object
	return *pCurr;
}

void C4ObjectList::UpdateScriptPointers()
{
	for (C4ObjectLink *link = First; link; link = link->Next)
	{
		link->Obj->UpdateScriptPointers();
	}
}

struct C4ObjectListDumpHelper
{
	C4ObjectList *list;
	C4ValueNumbers * numbers;

	void CompileFunc(StdCompiler *pComp) { pComp->Value(mkNamingAdapt(mkParAdapt(*list, numbers), "Objects")); }

	C4ObjectListDumpHelper(C4ObjectList *pLst, C4ValueNumbers * numbers) : list(pLst), numbers(numbers) {}
};

bool C4ObjectList::CheckSort(C4ObjectList *list)
{
	C4ObjectLink *link = First;
	C4ObjectLink *compare_link = list->First;
	while (link && (!link->Obj->Status || link->Obj->Unsorted))
	{
		link = link->Next;
	}

	while (link)
	{
		if (!compare_link)
		{
			Log("CheckSort failure");
			C4ValueNumbers numbers;
			LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper(this, &numbers), "SectorList")).getData());
			LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper(list, &numbers), "MainList")).getData());
			return false;
		}
		else
		{
			if (link->Obj == compare_link->Obj)
			{
				link = link->Next;
				while (link && (!link->Obj->Status || link->Obj->Unsorted))
				{
					link = link->Next;
				}
			}
			compare_link = compare_link->Next;
		}
}
	return true;
}

void C4ObjectList::CheckCategorySort()
{
	// debug: Check whether object list is sorted correctly
	C4ObjectLink *link;
	C4ObjectLink *previous = nullptr;
	for (link = First; link; link = link->Next)
	{
		if (!link->Obj->Unsorted && link->Obj->Status)
		{
			if (previous)
			{
				assert(previous->Obj->GetPlane() >= link->Obj->GetPlane());
			}
			previous = link;
		}
	}
}

C4ObjectList::iterator::iterator(const C4ObjectList & list, const C4ObjectLink * link, bool reverse):
		List(list), link(link ? *link : NULL_LINK), reverse(reverse)
{
	Next=list.AddIter(this);
}
C4ObjectList::iterator::iterator(const C4ObjectList::iterator & iter):
		List(iter.List), link(iter.link), Next(), reverse(iter.reverse)
{
	Next=List.AddIter(this);
}
C4ObjectList::iterator::~iterator()
{
	List.RemoveIter(this);
}
C4ObjectList::iterator& C4ObjectList::iterator::operator++ ()
{
	C4ObjectLink* advance = reverse ? link.Prev : link.Next;
	link = advance ? *advance : NULL_LINK;
	return *this;
}
C4ObjectList::iterator C4ObjectList::iterator::operator++ (int)
{
	iterator old = *this;
	iterator::operator++();
	return old;
}
C4Object * C4ObjectList::iterator::operator* ()
{
	return link.Obj;
}
bool C4ObjectList::iterator::operator== (const iterator & iter) const
{
	return
		&iter.List == &List &&
		iter.link.Obj == link.Obj /* checking for same object should be enough */ &&
		iter.reverse == reverse;
}
bool C4ObjectList::iterator::operator!= (const iterator & iter) const
{
	return !(*this == iter);
}

bool C4ObjectList::iterator::find(C4Object* target)
{
	while (link.Obj)
	{
		if (link.Obj == target)
		{
			return true;
		}
		else
		{
			(*this)++;
		}
	}
	return false;
}

bool C4ObjectList::iterator::atEnd() const
{
	return link.Obj == nullptr;
}

bool C4ObjectList::iterator::reset()
{
	C4ObjectLink* l = reverse ? List.Last : List.First;
	link = l ? *l : NULL_LINK;
	return !atEnd();
}

bool C4ObjectList::iterator::advance()
{
	(*this)++;
	return !atEnd();
}

C4ObjectList::iterator& C4ObjectList::iterator::operator=(const iterator & iter)
{
	// Can only assign iterators into the same list
	assert(&iter.List == &List);

	link = iter.link;
	reverse = iter.reverse;
	return *this;
}

C4ObjectList::iterator C4ObjectList::begin() const
{
	return iterator(*this, First, false);
}
const C4ObjectList::iterator C4ObjectList::end() const
{
	return iterator(*this, nullptr, false);
}
C4ObjectList::iterator * C4ObjectList::AddIter(iterator * iter) const
{
	iterator * r = FirstIter;
	FirstIter = iter;
	return r;
}
void C4ObjectList::RemoveIter(iterator * iter) const
{
	if (iter == FirstIter)
	{
		FirstIter = iter->Next;
	}
	else
	{
		iterator * i = FirstIter;
		while (i->Next && i->Next != iter)
		{
			i = i->Next;
		}
		i->Next = iter->Next;
	}
}

C4ObjectList::iterator C4ObjectList::ReverseView::begin() const
{
	return iterator(list, list.Last, true);
}

C4ObjectList::iterator C4ObjectList::ReverseView::end() const
{
	return iterator(list, nullptr, true);
}
