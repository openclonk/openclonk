/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2008  Peter Wortmann
 * Copyright (c) 2006-2008  Günther Brammer
 * Copyright (c) 2007  Sven Eberhardt
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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
#include <C4Include.h>
#include <C4FindObject.h>

#include <C4Object.h>
#include <C4Game.h>
#include <C4Random.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>

// *** C4FindObject

C4FindObject::~C4FindObject()
{
	delete pSort;
}

C4FindObject *C4FindObject::CreateByValue(const C4Value &DataVal, C4SortObject **ppSortObj)
{
	// Must be an array
	C4ValueArray *pArray = C4Value(DataVal).getArray();
	if (!pArray) return NULL;

	const C4ValueArray &Data = *pArray;
	int32_t iType = Data[0].getInt();
	if (Inside<int32_t>(iType, C4SO_First, C4SO_Last))
	{
		// this is not a FindObject but a sort condition!
		// sort condition not desired here?
		if (!ppSortObj) return NULL;
		// otherwise, create it!
		*ppSortObj = C4SortObject::CreateByValue(iType, Data);
		// done
		return NULL;
	}

	switch (iType)
	{
	case C4FO_Not:
	{
		// Create child condition
		C4FindObject *pCond = C4FindObject::CreateByValue(Data[1]);
		if (!pCond) return NULL;
		// wrap
		return new C4FindObjectNot(pCond);
	}

	case C4FO_And: case C4FO_Or:
	{
		// Trivial case (one condition)
		if (Data.GetSize() == 2)
			return C4FindObject::CreateByValue(Data[1]);
		// Create all childs
		int32_t i;
		C4FindObject **ppConds = new C4FindObject *[Data.GetSize() - 1];
		for (i = 0; i < Data.GetSize() - 1; i++)
			ppConds[i] = C4FindObject::CreateByValue(Data[i+1]);
		// Count real entries, move them to start of list
		int32_t iSize = 0;
		for (i = 0; i < Data.GetSize() - 1; i++)
			if (ppConds[i])
				if (iSize++ != i)
					ppConds[iSize-1] = ppConds[i];
		// Create
		if (iType == C4FO_And)
			return new C4FindObjectAnd(iSize, ppConds);
		else
			return new C4FindObjectOr(iSize, ppConds);
	}

	case C4FO_Exclude:
		return new C4FindObjectExclude(Data[1].getObj());

	case C4FO_ID:
		return new C4FindObjectID(Data[1].getC4ID());

	case C4FO_InRect:
		return new C4FindObjectInRect(C4Rect(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(), Data[4].getInt()));

	case C4FO_AtPoint:
		return new C4FindObjectAtPoint(Data[1].getInt(), Data[2].getInt());

	case C4FO_AtRect:
		return new C4FindObjectAtRect(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(),Data[4].getInt());

	case C4FO_OnLine:
		return new C4FindObjectOnLine(Data[1].getInt(), Data[2].getInt(), Data[3].getInt(), Data[4].getInt());

	case C4FO_Distance:
		return new C4FindObjectDistance(Data[1].getInt(), Data[2].getInt(), Data[3].getInt());

	case C4FO_OCF:
		return new C4FindObjectOCF(Data[1].getInt());

	case C4FO_Category:
		return new C4FindObjectCategory(Data[1].getInt());

	case C4FO_Action:
	{
		C4String *pStr = Data[1].getStr();
		if (!pStr) return NULL;
		// Don't copy, it should be safe
		return new C4FindObjectAction(pStr->GetCStr());
	}

	case C4FO_Func:
	{
		// Get function name
		C4String *pStr = Data[1].getStr();
		if (!pStr) return NULL;
		// Construct
		C4FindObjectFunc *pFO = new C4FindObjectFunc(pStr->GetCStr());
		// Add parameters
		for (int i = 2; i < Data.GetSize(); i++)
			pFO->SetPar(i - 2, Data[i]);
		// Done
		return pFO;
	}

	case C4FO_ActionTarget:
	{
		int index = 0;
		if (Data.GetSize() >= 3)
			index = BoundBy(Data[2].getInt(), 0, 1);
		return new C4FindObjectActionTarget(Data[1].getObj(), index);
	}

	case C4FO_Procedure:
		return new C4FindObjectProcedure(Data[1].getInt());

	case C4FO_Container:
		return new C4FindObjectContainer(Data[1].getObj());

	case C4FO_AnyContainer:
		return new C4FindObjectAnyContainer();

	case C4FO_Owner:
		return new C4FindObjectOwner(Data[1].getInt());

	case C4FO_Controller:
		return new C4FindObjectController(Data[1].getInt());

	case C4FO_Layer:
		return new C4FindObjectLayer(Data[1].getObj());

	}
	return NULL;
}

int32_t C4FindObject::Count(const C4ObjectList &Objs)
{
	// Trivial cases
	if (IsImpossible())
		return 0;
	if (IsEnsured())
		return Objs.ObjectCount();
	// Count
	int32_t iCount = 0;
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
				iCount++;
	return iCount;
}

C4Object *C4FindObject::Find(const C4ObjectList &Objs)
{
	// Trivial case
	if (IsImpossible())
		return NULL;
	// Search
	// Double-check object status, as object might be deleted after Check()!
	C4Object *pBestResult = NULL;
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
				if (pLnk->Obj->Status)
				{
					// no sorting: Use first object found
					if (!pSort) return pLnk->Obj;
					// Sorting: Check if found object is better
					if (!pBestResult || pSort->Compare(pLnk->Obj, pBestResult) > 0)
						if (pLnk->Obj->Status)
							pBestResult = pLnk->Obj;
				}
	return pBestResult;
}

C4ValueArray *C4FindObject::FindMany(const C4ObjectList &Objs)
{
	// Trivial case
	if (IsImpossible())
		return new C4ValueArray();
	// Set up array
	C4ValueArray *pArray = new C4ValueArray(32);
	int32_t iSize = 0;
	// Search
	for (C4ObjectLink *pLnk = Objs.First; pLnk; pLnk = pLnk->Next)
		if (pLnk->Obj->Status)
			if (Check(pLnk->Obj))
			{
				// Grow the array, if neccessary
				if (iSize >= pArray->GetSize())
					pArray->SetSize(iSize * 2);
				// Add object
				(*pArray)[iSize++] = C4VObj(pLnk->Obj);
			}
	// Shrink array
	pArray->SetSize(iSize);
	// Recheck object status (may shrink array again)
	CheckObjectStatus(pArray);
	// Apply sorting
	if (pSort) pSort->SortObjects(pArray);
	return pArray;
}

int32_t C4FindObject::Count(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial cases
	if (IsImpossible())
		return 0;
	if (IsEnsured())
		return Objs.ObjectCount();
	// Check bounds
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return Count(Objs);
	else if (UseShapes())
	{
		// Get area
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		C4ObjectList *pLst = Area.FirstObjectShapes(&pSct);
		// Check if a single-sector check is enough
		if (!Area.Next(pSct))
			return Count(pSct->ObjectShapes);
		// Create marker, count over all areas
		unsigned int iMarker = ++::Objects.LastUsedMarker;
		int32_t iCount = 0;
		for (; pLst; pLst=Area.NextObjectShapes(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (pLnk->Obj->Marker != iMarker)
					{
						pLnk->Obj->Marker = iMarker;
						if (Check(pLnk->Obj))
							iCount++;
					}
		return iCount;
	}
	else
	{
		// Count objects per area
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		int32_t iCount = 0;
		for (C4ObjectList *pLst=Area.FirstObjects(&pSct); pLst; pLst=Area.NextObjects(pLst, &pSct))
			iCount += Count(*pLst);
		return iCount;
	}
}

C4Object *C4FindObject::Find(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial case
	if (IsImpossible())
		return NULL;
	C4Object *pBestResult = NULL;
	// Check bounds
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return Find(Objs);
	// Traverse areas, return first matching object w/o sort or best with sort
	else if (UseShapes())
	{
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		C4Object *pObj;
		for (C4ObjectList *pLst=Area.FirstObjectShapes(&pSct); pLst; pLst=Area.NextObjectShapes(pLst, &pSct))
			if ((pObj = Find(*pLst)))
			{
				if (!pSort)
					return pObj;
				else if (!pBestResult || pSort->Compare(pObj, pBestResult) > 0)
					if (pObj->Status)
						pBestResult = pObj;
			}
	}
	else
	{
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		C4Object *pObj;
		for (C4ObjectList *pLst=Area.FirstObjects(&pSct); pLst; pLst=Area.NextObjects(pLst, &pSct))
		{
			if ((pObj = Find(*pLst)))
			{
				if (!pSort)
					return pObj;
				else if (!pBestResult || pSort->Compare(pObj, pBestResult) > 0)
					if (pObj->Status)
						pBestResult = pObj;
			}
		}
	}
	return pBestResult;
}

C4ValueArray *C4FindObject::FindMany(const C4ObjectList &Objs, const C4LSectors &Sct)
{
	// Trivial case
	if (IsImpossible())
		return new C4ValueArray();
	C4Rect *pBounds = GetBounds();
	if (!pBounds)
		return FindMany(Objs);
	// Prepare for array that may be generated
	C4ValueArray *pArray; int32_t iSize;
	// Check shape lists?
	if (UseShapes())
	{
		// Get area
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		C4ObjectList *pLst = Area.FirstObjectShapes(&pSct);
		// Check if a single-sector check is enough
		if (!Area.Next(pSct))
			return FindMany(pSct->ObjectShapes);
		// Set up array
		pArray = new C4ValueArray(32); iSize = 0;
		// Create marker, search all areas
		unsigned int iMarker = ++::Objects.LastUsedMarker;
		for (; pLst; pLst=Area.NextObjectShapes(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (pLnk->Obj->Marker != iMarker)
					{
						pLnk->Obj->Marker = iMarker;
						if (Check(pLnk->Obj))
						{
							// Grow the array, if neccessary
							if (iSize >= pArray->GetSize())
								pArray->SetSize(iSize * 2);
							// Add object
							(*pArray)[iSize++] = C4VObj(pLnk->Obj);
						}
					}
	}
	else
	{
		// Set up array
		pArray = new C4ValueArray(32); iSize = 0;
		// Search
		C4LArea Area(&::Objects.Sectors, *pBounds); C4LSector *pSct;
		for (C4ObjectList *pLst=Area.FirstObjects(&pSct); pLst; pLst=Area.NextObjects(pLst, &pSct))
			for (C4ObjectLink *pLnk = pLst->First; pLnk; pLnk = pLnk->Next)
				if (pLnk->Obj->Status)
					if (Check(pLnk->Obj))
					{
						// Grow the array, if neccessary
						if (iSize >= pArray->GetSize())
							pArray->SetSize(iSize * 2);
						// Add object
						(*pArray)[iSize++] = C4VObj(pLnk->Obj);
					}
	}
	// Shrink array
	pArray->SetSize(iSize);
	// Recheck object status (may shrink array again)
	CheckObjectStatus(pArray);
	// Apply sorting
	if (pSort) pSort->SortObjects(pArray);
	return pArray;
}

void C4FindObject::CheckObjectStatus(C4ValueArray *pArray)
{
	// Recheck object status
	for (int32_t i = 0; i < pArray->GetSize(); i++)
		if (!pArray->GetItem(i).getObj()->Status)
		{
			// This shouldn't happen really, so this is done as a seperate loop.
			int32_t j = i; i++;
			for (; i < pArray->GetSize(); i++)
				if (pArray->GetItem(i).getObj()->Status)
					pArray->GetItem(j++) = pArray->GetItem(i);
			// Set new size
			pArray->SetSize(j);
			break;
		}
}

void C4FindObject::SetSort(C4SortObject *pToSort)
{
	delete pSort;
	pSort = pToSort;
}


// *** C4FindObjectNot

C4FindObjectNot::~C4FindObjectNot()
{
	delete pCond;
}

bool C4FindObjectNot::Check(C4Object *pObj)
{
	return !pCond->Check(pObj);
}

// *** C4FindObjectAnd

C4FindObjectAnd::C4FindObjectAnd(int32_t inCnt, C4FindObject **ppConds, bool fFreeArray)
		: iCnt(inCnt), ppConds(ppConds), fFreeArray(fFreeArray), fUseShapes(false), fHasBounds(false)
{
	// Filter ensured entries
	int32_t i;
	for (i = 0; i < iCnt; )
		if (ppConds[i]->IsEnsured())
		{
			delete ppConds[i];
			iCnt--;
			for (int32_t j = i; j < iCnt; j++)
				ppConds[j] = ppConds[j + 1];
		}
		else
			i++;
	// Intersect all child bounds
	for (i = 0; i < iCnt; i++)
	{
		C4Rect *pChildBounds = ppConds[i]->GetBounds();
		if (pChildBounds)
		{
			// some objects might be in an rect and at a point not in that rect
			// so do not intersect an atpoint bound with an rect bound
			fUseShapes = ppConds[i]->UseShapes();
			if (fUseShapes)
			{
				Bounds = *pChildBounds;
				fHasBounds = true;
				break;
			}
			if (fHasBounds)
				Bounds.Intersect(*pChildBounds);
			else
			{
				Bounds = *pChildBounds;
				fHasBounds = true;
			}
		}
	}
}

C4FindObjectAnd::~C4FindObjectAnd()
{
	for (int32_t i = 0; i < iCnt; i++)
		delete ppConds[i];
	if (fFreeArray)
		delete [] ppConds;
}

bool C4FindObjectAnd::Check(C4Object *pObj)
{
	for (int32_t i = 0; i < iCnt; i++)
		if (!ppConds[i]->Check(pObj))
			return false;
	return true;
}

bool C4FindObjectAnd::IsImpossible()
{
	for (int32_t i = 0; i < iCnt; i++)
		if (ppConds[i]->IsImpossible())
			return true;
	return false;
}

// *** C4FindObjectOr

C4FindObjectOr::C4FindObjectOr(int32_t inCnt, C4FindObject **ppConds)
		: iCnt(inCnt), ppConds(ppConds), fHasBounds(false)
{
	// Filter impossible entries
	int32_t i;
	for (i = 0; i < iCnt; )
		if (ppConds[i]->IsImpossible())
		{
			delete ppConds[i];
			iCnt--;
			for (int32_t j = i; j < iCnt; j++)
				ppConds[j] = ppConds[j + 1];
		}
		else
			i++;
	// Sum up all child bounds
	for (i = 0; i < iCnt; i++)
	{
		C4Rect *pChildBounds = ppConds[i]->GetBounds();
		if (!pChildBounds) { fHasBounds = false; break; }
		// Do not optimize atpoint: It could lead to having to search multiple
		// sectors. An object's shape can be in multiple sectors. We do not want
		// to find the same object twice.
		if (ppConds[i]->UseShapes())
			{ fHasBounds = false; break; }
		if (fHasBounds)
			Bounds.Add(*pChildBounds);
		else
		{
			Bounds = *pChildBounds;
			fHasBounds = true;
		}
	}
}

C4FindObjectOr::~C4FindObjectOr()
{
	for (int32_t i = 0; i < iCnt; i++)
		delete ppConds[i];
	delete [] ppConds;
}

bool C4FindObjectOr::Check(C4Object *pObj)
{
	for (int32_t i = 0; i < iCnt; i++)
		if (ppConds[i]->Check(pObj))
			return true;
	return false;
}

bool C4FindObjectOr::IsEnsured()
{
	for (int32_t i = 0; i < iCnt; i++)
		if (ppConds[i]->IsEnsured())
			return true;
	return false;
}

// *** C4FindObject* (primitive conditions)

bool C4FindObjectExclude::Check(C4Object *pObj)
{
	return pObj != pExclude;
}

bool C4FindObjectID::Check(C4Object *pObj)
{
	return pObj->id == id;
}

bool C4FindObjectID::IsImpossible()
{
	C4Def * pDef = C4Id2Def(id);
	return !pDef || !pDef->Count;
}

bool C4FindObjectInRect::Check(C4Object *pObj)
{
	return rect.Contains(pObj->GetX(), pObj->GetY());
}

bool C4FindObjectInRect::IsImpossible()
{
	return !rect.Wdt || !rect.Hgt;
}

bool C4FindObjectAtPoint::Check(C4Object *pObj)
{
	return pObj->Shape.Contains(bounds.x - pObj->GetX(), bounds.y - pObj->GetY());
}

bool C4FindObjectAtRect::Check(C4Object *pObj)
{
	C4Rect rcShapeBounds = pObj->Shape;
	rcShapeBounds.x += pObj->GetX(); rcShapeBounds.y += pObj->GetY();
	return !!rcShapeBounds.Overlap(bounds);
}

bool C4FindObjectOnLine::Check(C4Object *pObj)
{
	return pObj->Shape.IntersectsLine(x - pObj->GetX(), y - pObj->GetY(), x2 - pObj->GetX(), y2 - pObj->GetY());
}

bool C4FindObjectDistance::Check(C4Object *pObj)
{
	return (pObj->GetX() - x) * (pObj->GetX() - x) + (pObj->GetY() - y) * (pObj->GetY() - y) <= r2;
}

bool C4FindObjectOCF::Check(C4Object *pObj)
{
	return !! (pObj->OCF & ocf);
}

bool C4FindObjectOCF::IsImpossible()
{
	return !ocf;
}

bool C4FindObjectCategory::Check(C4Object *pObj)
{
	return !! (pObj->Category & iCategory);
}

bool C4FindObjectCategory::IsEnsured()
{
	return !iCategory;
}

bool C4FindObjectAction::Check(C4Object *pObj)
{
	return SEqual(pObj->GetAction()->GetName(), szAction);
}

bool C4FindObjectActionTarget::Check(C4Object *pObj)
{
	assert(index >= 0 && index <= 1);
	if (index == 0)
		return pObj->Action.Target == pActionTarget;
	else if (index == 1)
		return pObj->Action.Target2 == pActionTarget;
	else
		return false;
}

bool C4FindObjectProcedure::Check(C4Object *pObj)
{
	C4Value v;
	pObj->GetAction()->GetPropertyVal(P_Procedure, v);
	return v != C4VNull && v.getInt() == procedure;
}

bool C4FindObjectProcedure::IsImpossible()
{
	return procedure < DFA_NONE || procedure >= C4D_MaxDFA;
}

bool C4FindObjectContainer::Check(C4Object *pObj)
{
	return pObj->Contained == pContainer;
}

bool C4FindObjectAnyContainer::Check(C4Object *pObj)
{
	return !! pObj->Contained;
}

bool C4FindObjectOwner::Check(C4Object *pObj)
{
	return pObj->Owner == iOwner;
}

bool C4FindObjectOwner::IsImpossible()
{
	return iOwner != NO_OWNER && !ValidPlr(iOwner);
}

bool C4FindObjectController::Check(C4Object *pObj)
{
	return pObj->Controller == controller;
}

bool C4FindObjectController::IsImpossible()
{
	return controller != NO_OWNER && !ValidPlr(controller);
}

// *** C4FindObjectFunc

void C4FindObjectFunc::SetPar(int i, const C4Value &Par)
{
	// Over limit?
	if (i >= C4AUL_MAX_Par) return;
	// Set parameter
	Pars[i] = Par;
}

bool C4FindObjectFunc::Check(C4Object *pObj)
{
	// Function not found?
	if (!Name) return false;
	// Search same-name-list for appropriate function
	C4AulFunc *pCallFunc = pObj->Def->Script.GetFuncRecursive(Name);
	if (!pCallFunc) return false;
	// Call
	return !! pCallFunc->Exec(pObj, &Pars);
}

bool C4FindObjectFunc::IsImpossible()
{
	return !Name;
}

// *** C4FindObjectLayer

bool C4FindObjectLayer::Check(C4Object *pObj)
{
	return pObj->pLayer == pLayer;
}

bool C4FindObjectLayer::IsImpossible()
{
	return false;
}

// *** C4SortObject

C4SortObject *C4SortObject::CreateByValue(const C4Value &DataVal)
{
	// Must be an array
	const C4ValueArray *pArray = C4Value(DataVal).getArray();
	if (!pArray) return NULL;
	const C4ValueArray &Data = *pArray;
	int32_t iType = Data[0].getInt();
	return CreateByValue(iType, Data);
}

C4SortObject *C4SortObject::CreateByValue(int32_t iType, const C4ValueArray &Data)
{
	switch (iType)
	{
	case  C4SO_Reverse:
	{
		// create child sort
		C4SortObject *pChildSort = C4SortObject::CreateByValue(Data[1]);
		if (!pChildSort) return NULL;
		// wrap
		return new C4SortObjectReverse(pChildSort);
	}

	case C4SO_Multiple:
	{
		// Trivial case (one sort)
		if (Data.GetSize() == 2)
		{
			return C4SortObject::CreateByValue(Data[1]);
		}
		// Create all children
		int32_t i;
		C4SortObject **ppSorts = new C4SortObject *[Data.GetSize() - 1];
		for (i = 0; i < Data.GetSize() - 1; i++)
		{
			ppSorts[i] = C4SortObject::CreateByValue(Data[i+1]);
		}
		// Count real entries, move them to start of list
		int32_t iSize = 0;
		for (i = 0; i < Data.GetSize() - 1; i++)
			if (ppSorts[i])
				if (iSize++ != i)
					ppSorts[iSize-1] = ppSorts[i];
		// Create
		return new C4SortObjectMultiple(iSize, ppSorts);
	}

	case C4SO_Distance:
		return new C4SortObjectDistance(Data[1].getInt(), Data[2].getInt());

	case C4SO_Random:
		return new C4SortObjectRandom();

	case C4SO_Speed:
		return new C4SortObjectSpeed();

	case C4SO_Mass:
		return new C4SortObjectMass();

	case C4SO_Value:
		return new C4SortObjectValue();

	case C4SO_Func:
	{
		// Get function name
		C4String *pStr = Data[1].getStr();
		if (!pStr) return NULL;
		// Construct
		C4SortObjectFunc *pSO = new C4SortObjectFunc(pStr->GetCStr());
		// Add parameters
		for (int i = 2; i < Data.GetSize(); i++)
			pSO->SetPar(i - 2, Data[i]);
		// Done
		return pSO;
	}

	}
	return NULL;
}

void C4SortObject::SortObjects(C4ValueArray *pArray)
{
	pArray->Sort(*this);
}

// *** C4SortObjectByValue

C4SortObjectByValue::C4SortObjectByValue()
		: C4SortObject(), pVals(NULL), iSize(0)
{
}

C4SortObjectByValue::~C4SortObjectByValue()
{
	delete [] pVals; pVals = NULL;
}

bool C4SortObjectByValue::PrepareCache(const C4ValueArray *pObjs)
{
	// Clear old cache
	delete [] pVals; pVals = NULL; iSize = 0;
	// Create new cache
	iSize = pObjs->GetSize(); pVals = new int32_t [iSize];
	for (int32_t i = 0; i < iSize; i++)
		pVals[i] = CompareGetValue(pObjs->GetItem(i)._getObj());
	// Okay
	return true;
}

int32_t C4SortObjectByValue::Compare(C4Object *pObj1, C4Object *pObj2)
{
	// this is rather slow, should only be called in special cases

	// make sure to hardcode the call order, as it might lead to desyncs otherwise [Icewing]
	int32_t iValue1 = CompareGetValue(pObj1);
	int32_t iValue2 = CompareGetValue(pObj2);
	return iValue2 - iValue1;
}

int32_t C4SortObjectByValue::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	assert(pVals); assert(iObj1 >= 0 && iObj1 < iSize); assert(iObj2 >= 0 && iObj2 < iSize);
	// Might overflow for large values...!
	return pVals[iObj2] - pVals[iObj1];
}

C4SortObjectReverse::~C4SortObjectReverse()
{
	delete pSort;
}

int32_t C4SortObjectReverse::Compare(C4Object *pObj1, C4Object *pObj2)
{
	return pSort->Compare(pObj2, pObj1);
}

bool C4SortObjectReverse::PrepareCache(const C4ValueArray *pObjs)
{
	return pSort->PrepareCache(pObjs);
}

int32_t C4SortObjectReverse::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	return pSort->CompareCache(iObj2, iObj1, pObj2, pObj1);
}

C4SortObjectMultiple::~C4SortObjectMultiple()
{
	for (int32_t i=0; i<iCnt; ++i) delete ppSorts[i];
	if (fFreeArray) delete [] ppSorts;
}

int32_t C4SortObjectMultiple::Compare(C4Object *pObj1, C4Object *pObj2)
{
	// return first comparison that's nonzero
	int32_t iCmp;
	for (int32_t i=0; i<iCnt; ++i)
		if ((iCmp = ppSorts[i]->Compare(pObj1, pObj2)))
			return iCmp;
	// all comparisons equal
	return 0;
}

bool C4SortObjectMultiple::PrepareCache(const C4ValueArray *pObjs)
{
	bool fCaches = false;
	for (int32_t i=0; i<iCnt; ++i)
		fCaches |= ppSorts[i]->PrepareCache(pObjs);
	// return wether a sort citerion uses a cache
	return fCaches;
}

int32_t C4SortObjectMultiple::CompareCache(int32_t iObj1, int32_t iObj2, C4Object *pObj1, C4Object *pObj2)
{
	// return first comparison that's nonzero
	int32_t iCmp;
	for (int32_t i=0; i<iCnt; ++i)
		if ((iCmp = ppSorts[i]->CompareCache(iObj1, iObj2, pObj1, pObj2)))
			return iCmp;
	// all comparisons equal
	return 0;
}

int32_t C4SortObjectDistance::CompareGetValue(C4Object *pFor)
{
	int32_t dx=pFor->GetX()-iX, dy=pFor->GetY()-iY;
	return dx*dx+dy*dy;
}

int32_t C4SortObjectRandom::CompareGetValue(C4Object *pFor)
{
	return Random(1 << 16);
}

int32_t C4SortObjectSpeed::CompareGetValue(C4Object *pFor)
{
	return pFor->xdir*pFor->xdir + pFor->ydir*pFor->ydir;
}

int32_t C4SortObjectMass::CompareGetValue(C4Object *pFor)
{
	return pFor->Mass;
}

int32_t C4SortObjectValue::CompareGetValue(C4Object *pFor)
{
	return pFor->GetValue(NULL, NO_OWNER);
}

void C4SortObjectFunc::SetPar(int i, const C4Value &Par)
{
	// Over limit?
	if (i >= C4AUL_MAX_Par) return;
	// Set parameter
	Pars[i] = Par;
}

int32_t C4SortObjectFunc::CompareGetValue(C4Object *pObj)
{
	// Function not found?
	if (!Name) return false;
	// Search same-name-list for appropriate function
	C4AulFunc *pCallFunc = pObj->Def->Script.GetFuncRecursive(Name);
	if (!pCallFunc) return false;
	// Call
	return pCallFunc->Exec(pObj, &Pars).getInt();
}
