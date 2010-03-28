/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2006, 2008  Sven Eberhardt
 * Copyright (c) 2004-2006  Peter Wortmann
 * Copyright (c) 2006-2008  Günther Brammer
 * Copyright (c) 2009  Armin Burgmeier
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

/* Dynamic object list */

#include <C4Include.h>
#include <C4ObjectList.h>

#include <C4Object.h>
#include <C4Application.h>
#include <C4Region.h>
#include <C4GraphicsResource.h>
#include <C4Game.h>
#include <C4GameObjects.h>

C4ObjectList::C4ObjectList(): FirstIter(0)
{
	Default();
}

C4ObjectList::C4ObjectList(const C4ObjectList &List): FirstIter(0)
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
	C4ObjectLink *cLnk,*nextLnk;
	for (cLnk=First; cLnk; cLnk=nextLnk)
		{ nextLnk=cLnk->Next; delete cLnk; }
	First=Last=NULL;
	if (pEnumerated) delete pEnumerated; pEnumerated=NULL;
}

const int MaxTempListID = 500;
C4ID TempListID[MaxTempListID];

C4ID C4ObjectList::GetListID(int32_t dwCategory, int Index)
{
	int clid;
	C4ObjectLink *clnk;
	C4Def *cdef;

	// Create a temporary list of all id's and counts
	for (clid=0; clid<MaxTempListID; clid++) TempListID[clid]=C4ID::None;
	for (clnk=First; clnk && clnk->Obj; clnk=clnk->Next)
		if (clnk->Obj->Status)
			if ((dwCategory==C4D_All) || ( (cdef=C4Id2Def(clnk->Obj->Def->id)) && (cdef->Category & dwCategory) ))
				for (clid=0; clid<MaxTempListID; clid++)
				{
					// Already there
					if (TempListID[clid]==clnk->Obj->Def->id) break;
					// End of list, add id
					if (TempListID[clid]==C4ID::None) { TempListID[clid]=clnk->Obj->Def->id; break; }
				}

	// Returns indexed id
	if (Inside(Index,0,MaxTempListID-1)) return TempListID[Index];

	return C4ID::None;
}

int C4ObjectList::ListIDCount(int32_t dwCategory)
{
	int clid;
	C4ObjectLink *clnk;
	C4Def *cdef;

	// Create a temporary list of all id's and counts
	for (clid=0; clid<MaxTempListID; clid++) TempListID[clid]=C4ID::None;
	for (clnk=First; clnk && clnk->Obj; clnk=clnk->Next)
		if (clnk->Obj->Status)
			if ((dwCategory==C4D_All) || ( (cdef=C4Id2Def(clnk->Obj->Def->id)) && (cdef->Category & dwCategory) ))
				for (clid=0; clid<MaxTempListID; clid++)
				{
					// Already there
					if (TempListID[clid]==clnk->Obj->Def->id) break;
					// End of list, add id
					if (TempListID[clid]==C4ID::None) { TempListID[clid]=clnk->Obj->Def->id; break; }
				}

	// Count different id's
	for (clid=0; clid<MaxTempListID; clid++)
		if (TempListID[clid]==C4ID::None)
			return clid;

	return MaxTempListID;
}



bool C4ObjectList::Add(C4Object *nObj, SortType eSort, C4ObjectList *pLstSorted)
{
	C4ObjectLink *nLnk;
	if (!nObj || !nObj->Def || !nObj->Status) return false;

#ifdef _DEBUG
	if (eSort==stMain)
	{
		CheckCategorySort();
		if (pLstSorted)
			assert(CheckSort(pLstSorted));
	}
#endif

	// dbg: don't do double links
	assert (!GetLink(nObj));

	// no self-sort
	assert(pLstSorted != this);

	// Allocate new link
	if (!(nLnk=new C4ObjectLink)) return false;
	// Set link
	nLnk->Obj=nObj;

	// Search insert position (default: end of list)
	C4ObjectLink *cLnk = NULL, *cPrev = Last;

	// Should sort?
	if (eSort == stReverse)
	{
		// reverse sort: Add to beginning of list
		cLnk = First; cPrev = NULL;
	}
	else if (eSort)
	{
		cLnk = NULL; cPrev = Last;

		// Sort override or line? Leave default as is.
		bool fUnsorted = nObj->Unsorted || nObj->Def->Line;
		if (!fUnsorted)
		{

			// Find successor by matching category / id
			// Sort by matching category/id is necessary for inventory shifting.
			// It is not done for static back to allow multiobject outside structure.
			// Unsorted objects are ignored in comparison.
			if (!(nObj->Category & C4D_StaticBack))
				for (cPrev=NULL,cLnk=First; cLnk; cLnk=cLnk->Next)
					if (cLnk->Obj->Status && !cLnk->Obj->Unsorted)
					{
						if ( (cLnk->Obj->Category & C4D_SortLimit)==(nObj->Category & C4D_SortLimit) )
							if ( cLnk->Obj->id == nObj->id )
								break;
						cPrev=cLnk;
					}

			// Find successor by relative category
			if (!cLnk)
				for (cPrev=NULL, cLnk=First; cLnk; cLnk=cLnk->Next)
					if (cLnk->Obj->Status && !cLnk->Obj->Unsorted)
					{
						if ((cLnk->Obj->Category & C4D_SortLimit)<=(nObj->Category & C4D_SortLimit))
							break;
						cPrev=cLnk;
					}

			cLnk = cPrev ? cPrev->Next : First;
		}

		// Sort by master list?
		if (pLstSorted)
		{

			assert(CheckSort(pLstSorted));

			// Unsorted: Always search full list (start with first object in list)
			if (fUnsorted) { cLnk = First; cPrev = NULL; }

			// As cPrev is the last link in front of the first position where the object could be inserted,
			// the object should be after this point in the master list (given it's consistent).
			// If we're about to insert the object at the end of the list, there is obviously nothing to do.
#ifndef _DEBUG
			if (cLnk)
			{
#endif
				C4ObjectLink *cLnk2 = cPrev ? pLstSorted->GetLink(cPrev->Obj)->Next : pLstSorted->First;
				for (; cLnk2; cLnk2 = cLnk2->Next)
					if (cLnk2->Obj == nObj)
						// Position found!
						break;
					else if (cLnk && cLnk2->Obj == cLnk->Obj)
					{
						// So cLnk->Obj is actually in front of nObj. Update insert position
						cPrev = cLnk;
						cLnk = cLnk->Next;
#ifndef _DEBUG
						// At end of list?
						if (!cLnk) break;
#endif
					}

				// No position found? This shouldn't happen with a consistent main list.
				assert(cLnk2);
#ifndef _DEBUG
			}
#endif

		}

	}

	assert(!cPrev || cPrev->Next == cLnk);
	assert(!cLnk || cLnk->Prev == cPrev);

	// Insert new link after predecessor
	InsertLink(nLnk, cPrev);

#ifdef _DEBUG
	// Debug: Check sort
	if (eSort == stMain)
	{
		CheckCategorySort();
		if (pLstSorted)
			assert(CheckSort(pLstSorted));
	}
#endif

	// Add mass
	Mass+=nObj->Mass;

	return true;
}

bool C4ObjectList::Remove(C4Object *pObj)
{
	C4ObjectLink *cLnk;

	// Find link
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj==pObj) break;
	if (!cLnk) return false;

	// Fix iterators
	for (iterator * i = FirstIter; i; i = i->Next)
	{
		if (i->pLink == cLnk) i->pLink = cLnk->Next;
	}

	// Remove link from list
	RemoveLink(cLnk);

	// Deallocate link
	delete cLnk;

	// Remove mass
	Mass-=pObj->Mass; if (Mass<0) Mass=0;

#if defined(_DEBUG)
	if (GetLink(pObj)) BREAKPOINT_HERE;
#endif

	return true;
}

C4Object* C4ObjectList::Find(C4ID id, int owner, DWORD dwOCF)
{
	C4ObjectLink *cLnk;
	// Find link and object
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			if (cLnk->Obj->Def->id==id)
				if ((owner==ANY_OWNER) || (cLnk->Obj->Owner==owner))
					if (dwOCF & cLnk->Obj->OCF)
						return cLnk->Obj;
	return NULL;
}

C4Object* C4ObjectList::FindOther(C4ID id, int owner)
{
	C4ObjectLink *cLnk;
	// Find link and object
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			if (cLnk->Obj->Def->id!=id)
				if ((owner==ANY_OWNER) || (cLnk->Obj->Owner==owner))
					return cLnk->Obj;
	return NULL;
}

C4Object* C4ObjectList::GetObject(int Index)
{
	int cIdx;
	C4ObjectLink *cLnk;
	// Find link and object
	for (cLnk=First,cIdx=0; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
		{
			if (cIdx==Index) return cLnk->Obj;
			cIdx++;
		}
	return NULL;
}

C4ObjectLink* C4ObjectList::GetLink(C4Object *pObj)
{
	if (!pObj) return NULL;
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj==pObj)
			return cLnk;
	return NULL;
}

int C4ObjectList::ObjectCount(C4ID id, int32_t dwCategory) const
{
	C4ObjectLink *cLnk;
	int iCount=0;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			if ( (id==C4ID::None) || (cLnk->Obj->Def->id==id) )
				if ( (dwCategory==C4D_All) || (cLnk->Obj->Category & dwCategory) )
					iCount++;
	return iCount;
}

int C4ObjectList::MassCount()
{
	C4ObjectLink *cLnk;
	int iMass=0;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			iMass+=cLnk->Obj->Mass;
	Mass=iMass;
	return iMass;
}

void C4ObjectList::DrawIDList(C4Facet &cgo, int iSelection,
                              C4DefList &rDefs, int32_t dwCategory,
                              C4RegionList *pRegions, int iRegionCom,
                              bool fDrawOneCounts)
{
	// Calculations & variables
	/*int iSections = cgo.GetSectionCount();
	int iItems = ListIDCount(dwCategory);*/
	//int iFirstItem = BoundBy(iSelection-iSections/2,0,Max(iItems-iSections,0));
	int32_t cSec = 0;
	int32_t iCount;
	C4Facet cgo2;
	C4Object *pFirstObj;
	char szCount[10];
	// objects are sorted in the list already, so just draw them!
	C4ObjectListIterator iter(*this);
	while ((pFirstObj = iter.GetNext(&iCount)))
	{
		// Section
		cgo2 = cgo.GetSection(cSec);
		// draw picture
		pFirstObj->DrawPicture(cgo2, cSec==iSelection);
		// Draw count
		sprintf(szCount,"%dx",iCount);
		if ((iCount!=1) || fDrawOneCounts)
			Application.DDraw->TextOut(szCount, ::GraphicsResource.FontRegular, 1.0, cgo2.Surface,cgo2.X+cgo2.Wdt-1,cgo2.Y+cgo2.Hgt-1-::GraphicsResource.FontRegular.iLineHgt,CStdDDraw::DEFAULT_MESSAGE_COLOR,ARight);
		// Region
		if (pRegions) pRegions->Add(cgo2.X,cgo2.Y,cgo2.Wdt,cgo2.Hgt,pFirstObj->GetName(),iRegionCom,pFirstObj,COM_None,COM_None,pFirstObj->Number);
		// Next section
		cSec++;
	}
	// Draw by list sorted ids
	/* for (cPos=0; c_id=GetListID(dwCategory,cPos); cPos++)
	  if (Inside(cPos,iFirstItem,iFirstItem+iSections-1))
	    {
	    // First object of this type
	    pFirstObj = Find(c_id);
	    // Count
	    iCount=ObjectCount(c_id);
	    // Section
	    cgo2 = cgo.GetSection(cSec);
	    // Draw by definition
	    rDefs.Draw( c_id, cgo2, (cPos==iSelection), pFirstObj->Color, pFirstObj );
	    // Draw count
	    sprintf(szCount,"%dx",iCount);
	    if ((iCount!=1) || fDrawOneCounts)
	      Application.DDraw->TextOut(szCount,cgo2.Surface,cgo2.X+cgo2.Wdt-1,cgo2.Y+cgo2.Hgt-1-Application.DDraw->TextHeight(),FWhite,ARight);
	    // Region
	    if (pRegions) pRegions->Add(cgo2.X,cgo2.Y,cgo2.Wdt,cgo2.Hgt,pFirstObj->GetName(),iRegionCom,pFirstObj,COM_None,COM_None,pFirstObj->id);
	    // Next section
	    cSec++;
	    } */
}

int C4ObjectList::ClearPointers(C4Object *pObj)
{
	int rval=0;
	// Clear all primary list pointers
	while (Remove(pObj)) rval++;
	// Clear all sub pointers
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		cobj->ClearPointers(pObj);
	return rval;
}

void C4ObjectList::DrawAll(C4TargetFacet &cgo, int iPlayer)
{
	C4ObjectLink *clnk;
	// Draw objects (base)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		clnk->Obj->Draw(cgo, iPlayer);
	// Draw objects (top face)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		clnk->Obj->DrawTopFace(cgo, iPlayer);
}

void C4ObjectList::DrawIfCategory(C4TargetFacet &cgo, int iPlayer, uint32_t dwCat, bool fInvert)
{
	C4ObjectLink *clnk;
	// Draw objects (base)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		if (!(clnk->Obj->Category & dwCat) == fInvert)
			clnk->Obj->Draw(cgo, iPlayer);
	// Draw objects (top face)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		if (!(clnk->Obj->Category & dwCat) == fInvert)
			clnk->Obj->DrawTopFace(cgo, iPlayer);
}

void C4ObjectList::Draw(C4TargetFacet &cgo, int iPlayer)
{
	C4ObjectLink *clnk;
	// Draw objects (base)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		if (!(clnk->Obj->Category & C4D_BackgroundOrForeground))
			clnk->Obj->Draw(cgo, iPlayer);
	// Draw objects (top face)
	for (clnk=Last; clnk; clnk=clnk->Prev)
		if (!(clnk->Obj->Category & C4D_BackgroundOrForeground))
			clnk->Obj->DrawTopFace(cgo, iPlayer);
}

void C4ObjectList::Enumerate()
{
	C4ObjectLink *cLnk;
	// Enumerate object pointers
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->EnumeratePointers();
}

bool C4ObjectList::IsContained(C4Object *pObj)
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj==pObj)
			return true;
	return false;
}

bool C4ObjectList::IsClear() const
{
	return (ObjectCount()==0);
}

bool C4ObjectList::DenumerateRead()
{
	if (!pEnumerated) return false;
	// Denumerate all object pointers
	for (std::list<int32_t>::const_iterator pNum = pEnumerated->begin(); pNum != pEnumerated->end(); ++pNum)
		Add(::Objects.ObjectPointer(*pNum), stNone); // Add to tail, unsorted
	// Delete old list
	delete pEnumerated; pEnumerated = NULL;
	return true;
}

bool C4ObjectList::Write(char *szTarget)
{
	char ostr[25];
	szTarget[0]=0;
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk && cLnk->Obj; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
		{
			sprintf(ostr,"%d;",cLnk->Obj->Number);
			SAppend(ostr,szTarget);
		}
	return true;
}

void C4ObjectList::Denumerate()
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->DenumeratePointers();
}

void C4ObjectList::CompileFunc(StdCompiler *pComp, bool fSaveRefs, bool fSkipPlayerObjects)
{
	if (fSaveRefs)
	{
		// this mode not supported
		assert(!fSkipPlayerObjects);
		// (Re)create list
		delete pEnumerated; pEnumerated = new std::list<int32_t>();
		// Decompiling: Build list
		if (!pComp->isCompiler())
			for (C4ObjectLink *pPos = First; pPos; pPos = pPos->Next)
				if (pPos->Obj->Status)
					pEnumerated->push_back(pPos->Obj->Number);
		// Compile list
		pComp->Value(mkSTLContainerAdapt(*pEnumerated, StdCompiler::SEP_SEP2));
		// Decompiling: Delete list
		if (!pComp->isCompiler())
			{ delete pEnumerated; pEnumerated = NULL; }
		// Compiling: Nothing to do - list will e denumerated later
	}
	else
	{
		if (pComp->isDecompiler())
		{
			// skipping player objects would screw object counting in non-naming compilers
			assert(!fSkipPlayerObjects || pComp->hasNaming());
			// Put object count
			int32_t iObjCnt = ObjectCount();
			pComp->Value(mkNamingCountAdapt(iObjCnt, "Object"));
			// Decompile all objects in reverse order
			for (C4ObjectLink *pPos = Last; pPos; pPos = pPos->Prev)
				if (pPos->Obj->Status)
					if (!fSkipPlayerObjects || !pPos->Obj->IsUserPlayerObject())
						pComp->Value(mkNamingAdapt(*pPos->Obj, "Object"));
		}
		else
		{
			// this mode not supported
			assert(!fSkipPlayerObjects);
			// Remove previous data
			Clear();
			// Get "Object" section count
			int32_t iObjCnt;
			pComp->Value(mkNamingCountAdapt(iObjCnt, "Object"));
			// Load objects, add them to the list.
			for (int i = 0; i < iObjCnt; i++)
			{
				C4Object *pObj = NULL;
				try
				{
					pComp->Value(mkNamingAdapt(mkPtrAdaptNoNull(pObj), "Object"));
					Add(pObj, stReverse);
				}
				catch (StdCompiler::Exception *pExc)
				{
					// Failsafe object loading: If an error occurs during object loading, just skip that object and load the next one
					if (!pExc->Pos.getLength())
						LogF("ERROR: Object loading: %s", pExc->Msg.getData());
					else
						LogF("ERROR: Object loading(%s): %s", pExc->Pos.getData(), pExc->Msg.getData());
					delete pExc;
				}
			}
		}
	}
}

StdStrBuf C4ObjectList::GetNameList(C4DefList &rDefs, DWORD dwCategory)
{
	int cpos,idcount;
	C4ID c_id;
	C4Def *cdef;
	StdStrBuf Buf;
	for (cpos=0; (c_id=GetListID(dwCategory,cpos)); cpos++)
		if ((cdef=rDefs.ID2Def(c_id)))
		{
			idcount=ObjectCount(c_id);
			if (cpos>0) Buf.Append(", ");
			Buf.AppendFormat("%dx %s",idcount,cdef->GetName());
		}
	return Buf;
}

bool C4ObjectList::ValidateOwners()
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->ValidateOwner();
	return true;
}

bool C4ObjectList::AssignInfo()
{
	// the list seems to be traced backwards here, to ensure crew objects are added in correct order
	// (or semi-correct, because this will work only if the crew order matches the main object list order)
	// this is obsolete now, because the crew list is stored in the savegame
	C4ObjectLink *cLnk;
	for (cLnk=Last; cLnk; cLnk=cLnk->Prev)
		if (cLnk->Obj->Status)
			cLnk->Obj->AssignInfo();
	return true;
}

void C4ObjectList::ClearInfo(C4ObjectInfo *pInfo)
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->ClearInfo(pInfo);
}

void C4ObjectList::DrawList(C4Facet &cgo, int iSelection, DWORD dwCategory)
{
	int iSections = cgo.GetSectionCount();
	int iObjects = ObjectCount(C4ID::None,dwCategory);
	int iFirstVisible = BoundBy(iSelection-iSections/2,0,Max(iObjects-iSections,0));
	C4Facet cgo2;
	int iObj=0,iSec=0;
	C4ObjectLink *cLnk; C4Object *cObj;
	for (cLnk=First; cLnk && (cObj=cLnk->Obj); cLnk=cLnk->Next)
		if (cObj->Status && (cObj->Category && dwCategory))
		{
			if (Inside(iObj,iFirstVisible,iFirstVisible+iSections-1))
			{
				cgo2 = cgo.GetSection(iSec++);
				cObj->DrawPicture(cgo2,(iObj==iSelection));
			}
			iObj++;
		}
}

void C4ObjectList::Sort()
{
	C4ObjectLink *cLnk;
	bool fSorted;
	// Sort by id
	do
	{
		fSorted = true;
		for (cLnk=First; cLnk && cLnk->Next; cLnk=cLnk->Next)
			if (cLnk->Obj->id > cLnk->Next->Obj->id)
			{
				RemoveLink(cLnk);
				InsertLink(cLnk,cLnk->Next);
				fSorted = false;
				break;
			}
	}
	while (!fSorted);
}

void C4ObjectList::RemoveLink(C4ObjectLink *pLnk)
{
	if (pLnk->Prev) pLnk->Prev->Next=pLnk->Next; else First=pLnk->Next;
	if (pLnk->Next) pLnk->Next->Prev=pLnk->Prev; else Last=pLnk->Prev;
}

void C4ObjectList::InsertLink(C4ObjectLink *pLnk, C4ObjectLink *pAfter)
{
	// Insert after
	if (pAfter)
	{
		pLnk->Prev=pAfter; pLnk->Next=pAfter->Next;
		if (pAfter->Next) pAfter->Next->Prev=pLnk; else Last=pLnk;
		pAfter->Next=pLnk;
	}
	// Insert at head
	else
	{
		pLnk->Prev=NULL; pLnk->Next=First;
		if (First) First->Prev=pLnk; else Last=pLnk;
		First=pLnk;
	}
}

void C4ObjectList::InsertLinkBefore(C4ObjectLink *pLnk, C4ObjectLink *pBefore)
{
	// Insert before
	if (pBefore)
	{
		pLnk->Prev = pBefore->Prev;
		if (pBefore->Prev) pBefore->Prev->Next = pLnk; else First = pLnk;
		pLnk->Next = pBefore; pBefore->Prev = pLnk;
	}
	// Insert at end
	else
	{
		pLnk->Next = NULL; pLnk->Prev = Last;
		if (Last) Last->Next = pLnk; else First = pLnk;
		Last = pLnk;
	}
}


void C4NotifyingObjectList::InsertLinkBefore(C4ObjectLink *pLink, C4ObjectLink *pBefore)
{
	C4ObjectList::InsertLinkBefore(pLink, pBefore);
	ObjectListChangeListener.OnObjectAdded(this, pLink);
}

void C4NotifyingObjectList::InsertLink(C4ObjectLink *pLink, C4ObjectLink *pAfter)
{
	C4ObjectList::InsertLink(pLink, pAfter);
	ObjectListChangeListener.OnObjectAdded(this, pLink);
}

void C4NotifyingObjectList::RemoveLink(C4ObjectLink *pLnk)
{
	C4ObjectList::RemoveLink(pLnk);
	ObjectListChangeListener.OnObjectRemove(this, pLnk);
}

void C4ObjectList::UpdateGraphics(bool fGraphicsChanged)
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->UpdateGraphics(fGraphicsChanged);
}

void C4ObjectList::UpdateFaces(bool bUpdateShapes)
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->UpdateFace(bUpdateShapes);
}

void C4ObjectList::DrawSelectMark(C4TargetFacet &cgo, float Zoom)
{
	C4ObjectLink *cLnk;
	for (cLnk=Last; cLnk; cLnk=cLnk->Prev)
		cLnk->Obj->DrawSelectMark(cgo, Zoom);
}

void C4ObjectList::CloseMenus()
{
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		cobj->CloseMenu(true);
}

void C4ObjectList::Copy(const C4ObjectList &rList)
{
	Clear(); Default();
	C4ObjectLink *cLnk;
	for (cLnk=rList.First; cLnk; cLnk=cLnk->Next) Add(cLnk->Obj, C4ObjectList::stNone);
}

void C4ObjectList::Default()
{
	First=Last=NULL;
	Mass=0;
	pEnumerated=NULL;
}

bool C4ObjectList::OrderObjectBefore(C4Object *pObj1, C4Object *pObj2)
{
	// safety
	if (pObj1->Status != C4OS_NORMAL || pObj2->Status != C4OS_NORMAL) return false;
	// get links (and check whether the objects are part of this list!)
	C4ObjectLink *pLnk1=GetLink(pObj1); if (!pLnk1) return false;
	C4ObjectLink *pLnk2=GetLink(pObj2); if (!pLnk2) return false;
	// check if requirements are already fulfilled
	C4ObjectLink *pLnk=pLnk1;
	while ((pLnk=pLnk->Next)) if (pLnk==pLnk2) break;
	if (pLnk) return true;
	// if not, reorder pLnk1 directly before pLnk2
	// unlink from current position
	// no need to check pLnk1->Prev here, because pLnk1 cannot be first in the list
	// (at least pLnk2 must lie before it!)
	if ((pLnk1->Prev->Next=pLnk1->Next)) pLnk1->Next->Prev=pLnk1->Prev; else Last=pLnk1->Prev;
	// relink into new one
	if ((pLnk1->Prev=pLnk2->Prev)) pLnk2->Prev->Next=pLnk1; else First=pLnk1;
	pLnk1->Next=pLnk2; pLnk2->Prev=pLnk1;
	// done, success
	return true;
}

bool C4ObjectList::OrderObjectAfter(C4Object *pObj1, C4Object *pObj2)
{
	// safety
	if (pObj1->Status != C4OS_NORMAL || pObj2->Status != C4OS_NORMAL) return false;
	// get links (and check whether the objects are part of this list!)
	C4ObjectLink *pLnk1=GetLink(pObj1); if (!pLnk1) return false;
	C4ObjectLink *pLnk2=GetLink(pObj2); if (!pLnk2) return false;
	// check if requirements are already fulfilled
	C4ObjectLink *pLnk=pLnk1;
	while ((pLnk=pLnk->Prev)) if (pLnk==pLnk2) break;
	if (pLnk) return true;
	// if not, reorder pLnk1 directly after pLnk2
	// unlink from current position
	// no need to check pLnk1->Next here, because pLnk1 cannot be last in the list
	// (at least pLnk2 must lie after it!)
	if ((pLnk1->Next->Prev=pLnk1->Prev)) pLnk1->Prev->Next=pLnk1->Next; else First=pLnk1->Next;
	// relink into new one
	if ((pLnk1->Next=pLnk2->Next)) pLnk2->Next->Prev=pLnk1; else Last=pLnk1;
	pLnk1->Prev=pLnk2; pLnk2->Next=pLnk1;
	// done, success
	return true;
}

bool C4ObjectList::ShiftContents(C4Object *pNewFirst)
{
	// get link of new first (this ensures list is not empty)
	C4ObjectLink *pNewFirstLnk = GetLink(pNewFirst);
	if (!pNewFirstLnk) return false;
	// already at front?
	if (pNewFirstLnk == First) return true;
	// sort it there:
	// 1. Make cyclic list
	Last->Next = First; First->Prev = Last;
	// 2. Re-set first and last
	First = pNewFirstLnk;
	Last = pNewFirstLnk->Prev;
	// 3. Uncycle list
	First->Prev = Last->Next = NULL;
	// done, success
	return true;
}

void C4ObjectList::DeleteObjects()
{
	// delete links and objects
	while (First)
	{
		C4Object *pObj = First->Obj;
		Remove(pObj);
		delete pObj;
	}
	// reset mass
	Mass=0;
}


// -------------------------------------------------
// C4ObjectListIterator

C4Object *C4ObjectListIterator::GetNext(int32_t *piCount, uint32_t dwCategory)
{
	// end reached?
	if (pCurrID == rList.end()) return NULL;
	// not yet started?
	if (pCurr == rList.end())
		// then start at first ID list head
		pCurr = pCurrID;
	else
		// next item
		if (++pCurr == rList.end()) return NULL;
	// skip mismatched category
	if (dwCategory)
		while (!((*pCurr)->Category & dwCategory))
			if (++pCurr == rList.end()) return NULL;
	// next ID section reached?
	if ((*pCurr)->id != (*pCurrID)->id)
		pCurrID = pCurr;
	else
	{
		// otherwise, it must be checked, whether this is a duplicate item already iterated
		// if so, advance the list
		for (C4ObjectList::iterator pCheck = pCurrID; pCheck != pCurr; ++pCheck)
			if (!dwCategory || ((*pCheck)->Category & dwCategory))
				if ((*pCheck)->CanConcatPictureWith(*pCurr))
				{
					// next object of matching category
					if (++pCurr == rList.end()) return NULL;
					if (dwCategory)
						while (!((*pCurr)->Category & dwCategory))
							if (++pCurr == rList.end()) return NULL;
					// next ID chunk reached?
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
	if (piCount)
	{
		// default count
		*piCount = 1;
		// add additional objects of same ID to the count
		C4ObjectList::iterator pCheck(pCurr);
		for (++pCheck; pCheck != rList.end() && (*pCheck)->id == (*pCurr)->id; ++pCheck)
			if (!dwCategory || ((*pCheck)->Category & dwCategory))
				if ((*pCheck)->CanConcatPictureWith(*pCurr))
					++*piCount;
	}
	// return found object
	return *pCurr;
}

void C4ObjectList::UpdateScriptPointers()
{
	for (C4ObjectLink *cLnk=First; cLnk; cLnk=cLnk->Next)
		cLnk->Obj->UpdateScriptPointers();
}

struct C4ObjectListDumpHelper
{
	C4ObjectList *pLst;

	void CompileFunc(StdCompiler *pComp) { pComp->Value(mkNamingAdapt(*pLst, "Objects")); }

	C4ObjectListDumpHelper(C4ObjectList *pLst) : pLst(pLst) {}
	ALLOW_TEMP_TO_REF(C4ObjectListDumpHelper)
};

bool C4ObjectList::CheckSort(C4ObjectList *pList)
{
	C4ObjectLink *cLnk = First, *cLnk2 = pList->First;
	while (cLnk && cLnk->Obj->Unsorted) cLnk = cLnk->Next;
	while (cLnk)
		if (!cLnk2)
		{
			Log("CheckSort failure");
			LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper(this), "SectorList")).getData());
			LogSilent(DecompileToBuf<StdCompilerINIWrite>(mkNamingAdapt(C4ObjectListDumpHelper(pList), "MainList")).getData());
			return false;
		}
		else
		{
			if (cLnk->Obj == cLnk2->Obj)
			{
				cLnk = cLnk->Next;
				while (cLnk && cLnk->Obj->Unsorted) cLnk = cLnk->Next;
			}
			cLnk2 = cLnk2->Next;
		}
	return true;
}

void C4ObjectList::CheckCategorySort()
{
	// debug: Check whether object list is sorted correctly
	C4ObjectLink *cLnk, *cPrev=NULL;
	for (cLnk=First; cLnk && cLnk->Next; cLnk=cLnk->Next)
		if (!cLnk->Obj->Unsorted && cLnk->Obj->Status)
		{
			if (cPrev) assert( (cPrev->Obj->Category & C4D_SortLimit) >= (cLnk->Obj->Category & C4D_SortLimit));
			cPrev = cLnk;
		}
}

C4ObjectList::iterator::iterator(C4ObjectList & List):
		List(List), pLink(List.First)
{
	Next = List.AddIter(this);
}
C4ObjectList::iterator::iterator(C4ObjectList & List, C4ObjectLink * pLink):
		List(List), pLink(pLink)
{
	Next=List.AddIter(this);
}
C4ObjectList::iterator::iterator(const C4ObjectList::iterator & iter):
		List(iter.List), pLink(iter.pLink), Next()
{
	Next=List.AddIter(this);
}
C4ObjectList::iterator::~iterator()
{
	List.RemoveIter(this);
}
C4ObjectList::iterator& C4ObjectList::iterator::operator++ ()
{
	pLink = pLink ? pLink->Next : pLink;
	return *this;
}
C4ObjectList::iterator C4ObjectList::iterator::operator++ (int)
{
	iterator old = *this;
	pLink = pLink ? pLink->Next : pLink;
	return old;
}
C4Object * C4ObjectList::iterator::operator* ()
{
	return pLink ? pLink->Obj : 0;
}
bool C4ObjectList::iterator::operator== (const iterator & iter) const
{
	return &iter.List == &List && iter.pLink == pLink;
}
bool C4ObjectList::iterator::operator!= (const iterator & iter) const
{
	return &iter.List != &List || iter.pLink != pLink;
}

C4ObjectList::iterator& C4ObjectList::iterator::operator=(const iterator & iter)
{
	// Can only assign iterators into the same list
	assert(&iter.List == &List);

	pLink = iter.pLink;
	return *this;
}

C4ObjectList::iterator C4ObjectList::begin()
{
	return iterator(*this);
}
const C4ObjectList::iterator C4ObjectList::end()
{
	return iterator(*this, 0);
}
C4ObjectList::iterator * C4ObjectList::AddIter(iterator * iter)
{
	iterator * r = FirstIter;
	FirstIter = iter;
	return r;
}
void C4ObjectList::RemoveIter(iterator * iter)
{
	if (iter == FirstIter)
		FirstIter = iter->Next;
	else
	{
		iterator * i = FirstIter;
		while (i->Next && i->Next != iter)
			i = i->Next;
		i->Next = iter->Next;
	}
}
