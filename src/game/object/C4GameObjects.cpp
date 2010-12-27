/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2001-2008  Sven Eberhardt
 * Copyright (c) 2004, 2006-2008  Peter Wortmann
 * Copyright (c) 2005-2010  Günther Brammer
 * Copyright (c) 2010  Maikel de Vries
 * Copyright (c) 2010  Benjamin Herr
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
// game object lists

#include <C4Include.h>
#include <C4GameObjects.h>

#include <C4Object.h>
#include <C4ObjectCom.h>
#include <C4Physics.h>
#include <C4Random.h>
#include <C4Network2Stats.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4PlayerList.h>
#include <C4Record.h>

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
	BackObjects.Default();
	ForeObjects.Default();
}

void C4GameObjects::Init(int32_t iWidth, int32_t iHeight)
{
	// init sectors
	Sectors.Init(iWidth, iHeight);
}


void C4GameObjects::CompileFunc(StdCompiler *pComp, bool fSkipPlayerObjects)
{
	// "Object" section count
	int32_t iObjCnt = ObjectCount();
	pComp->Value(mkNamingCountAdapt(iObjCnt, "Object"));
	// "Def" section count
	//int32_t iDefCnt = ::Definitions.GetDefCount();
	//pComp->Value(mkNamingCountAdapt(iDefCnt, "Def"));
	// "PropList" section count
	int32_t iPropListCnt = PropLists.GetSize() - iObjCnt /*- iDefCnt*/;
	pComp->Value(mkNamingCountAdapt(iPropListCnt, "PropList"));
	// skipping player objects would screw object counting in non-naming compilers
	assert(!fSkipPlayerObjects || pComp->hasNaming());
	if (pComp->isDecompiler())
	{
		// Decompile all objects in reverse order
		for (C4ObjectLink *pPos = Last; pPos; pPos = pPos->Prev)
			if (pPos->Obj->Status)
				if (!fSkipPlayerObjects || !pPos->Obj->IsUserPlayerObject())
					pComp->Value(mkNamingAdapt(*pPos->Obj, "Object"));
		/*for(C4PropList * const * ppPropList = PropLists.First(); ppPropList; ppPropList = PropLists.Next(ppPropList))
		  if (dynamic_cast<C4Def *>(*ppPropList))
		    {
		    pComp->Name("Def");
		    pComp->Value(mkNamingAdapt(mkC4IDAdapt(dynamic_cast<C4Def *>(*ppPropList)->id), "id", C4ID_None));
		    pComp->Value(mkNamingAdapt((*ppPropList)->Number, "Number", -1));
		    pComp->NameEnd();
		    }*/
		for (C4PropListNumbered * const * ppPropList = PropLists.First(); ppPropList; ppPropList = PropLists.Next(ppPropList))
			if ((*ppPropList)->IsScriptPropList())
			{
				pComp->Value(mkNamingAdapt(**ppPropList, "PropList"));
			}
	}
	else
	{
		// this mode not supported
		assert(!fSkipPlayerObjects);
		// Remove previous data
		Clear();
		//assert(PropLists.GetSize() == ::Definitions.GetDefCount());
		// Load objects, add them to the list.
		for (int i = 0; i < iObjCnt; i++)
		{
			C4Object *pObj = NULL;
			try
			{
				pComp->Value(mkNamingAdapt(mkPtrAdaptNoNull(pObj), "Object"));
				C4ObjectList::Add(pObj, stReverse);
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
		// Load object numbers of the definitions
		/*for(int i = 0; i < iDefCnt; i++)
		  {
		  try
		    {
		    pComp->Name("Def");
		    C4ID id;
		    pComp->Value(mkNamingAdapt(mkC4IDAdapt(id), "id", C4ID_None));
		    C4Def * pDef = ::Definitions.ID2Def(id);
		    if (!pDef)
		      { pComp->excNotFound(LoadResStr("IDS_PRC_UNDEFINEDOBJECT"),C4IdText(id)); continue; }
		    PropLists.Remove(static_cast<C4PropList *>(pDef));
		    pComp->Value(mkNamingAdapt(pDef->Number, "Number", -1));
		    PropLists.Add(static_cast<C4PropList *>(pDef));
		    }
		  catch (StdCompiler::Exception *pExc)
		    {
		    // Failsafe object loading: If an error occurs during object loading, just skip that object and load the next one
		    if(!pExc->Pos.getLength())
		      LogF("ERROR: Definition loading: %s", pExc->Msg.getData());
		    else
		      LogF("ERROR: Definition loading(%s): %s", pExc->Pos.getData(), pExc->Msg.getData());
		    delete pExc;
		      pComp->NameEnd(true);
		    }
		  pComp->NameEnd();
		  }*/
		// Load proplists
		for (int i = 0; i < iPropListCnt; i++)
		{
			C4PropListScript *pPropList = NULL;
			try
			{
				pComp->Value(mkNamingAdapt(mkPtrAdaptNoNull(pPropList), "PropList"));
			}
			catch (StdCompiler::Exception *pExc)
			{
				// Failsafe object loading: If an error occurs during object loading, just skip that object and load the next one
				if (!pExc->Pos.getLength())
					LogF("ERROR: PropList loading: %s", pExc->Msg.getData());
				else
					LogF("ERROR: PropList loading(%s): %s", pExc->Pos.getData(), pExc->Msg.getData());
				delete pExc;
			}
		}
	}
}

bool C4GameObjects::Add(C4Object *nObj)
{
	// add inactive objects to the inactive list only
	if (nObj->Status == C4OS_INACTIVE)
		return InactiveObjects.Add(nObj, C4ObjectList::stMain);
	// if this is a background object, add it to the list
	if (nObj->Category & C4D_Background)
		::Objects.BackObjects.Add(nObj, C4ObjectList::stMain);
	// if this is a foreground object, add it to the list
	if (nObj->Category & C4D_Foreground)
		::Objects.ForeObjects.Add(nObj, C4ObjectList::stMain);
	// manipulate main list
	if (!C4ObjectList::Add(nObj, C4ObjectList::stMain))
		return false;
	// add to sectors
	Sectors.Add(nObj, this);
	return true;
}


bool C4GameObjects::Remove(C4Object *pObj)
{
	// if it's an inactive object, simply remove from the inactiv elist
	if (pObj->Status == C4OS_INACTIVE) return InactiveObjects.Remove(pObj);
	// remove from sectors
	Sectors.Remove(pObj);
	// remove from backlist
	::Objects.BackObjects.Remove(pObj);
	// remove from forelist
	::Objects.ForeObjects.Remove(pObj);
	// manipulate main list
	return C4ObjectList::Remove(pObj);
}

C4ObjectList &C4GameObjects::ObjectsAt(int ix, int iy)
{
	return Sectors.SectorAt(ix, iy)->ObjectShapes;
}

void C4GameObjects::CrossCheck() // Every Tick1 by ExecObjects
{
	C4Object *obj1 = NULL,*obj2 = NULL;
	DWORD ocf1,ocf2,focf,tocf;

	// AtObject-Check: Checks for first match of obj1 at obj2

	// Checks for this frame
	focf=tocf=OCF_None;
	// Very low level: Incineration
	if (!::Game.iTick35)
		{ focf|=OCF_OnFire; tocf|=OCF_Inflammable; }

	if (focf && tocf)
		for (C4ObjectList::iterator iter=begin(); iter != end() && (obj1=*iter); ++iter)
			if (obj1->Status && !obj1->Contained)
				if (obj1->OCF & focf)
				{
					ocf1=obj1->OCF; ocf2=tocf;
					if ((obj2=AtObject(obj1->GetX(),obj1->GetY(),ocf2,obj1)))
						// Incineration
						if ((ocf1 & OCF_OnFire) && (ocf2 & OCF_Inflammable))
							if (!Random(obj2->Def->ContactIncinerate))
								{ obj2->Incinerate(obj1->GetFireCausePlr(), false, obj1); continue; }
				}

	// Reverse area check: Checks for all obj2 at obj1

	focf=tocf=OCF_None;
	// High level: Collection, Hit
	if (!::Game.iTick3)
		{ focf|=OCF_Collection; tocf|=OCF_Carryable; }
	focf|=OCF_Alive;      tocf|=OCF_HitSpeed2;

	if (focf && tocf)
		for (C4ObjectList::iterator iter=begin(); iter != end() && (obj1=*iter); ++iter)
			if (obj1->Status && !obj1->Contained && (obj1->OCF & focf))
			{
				unsigned int Marker = ++LastUsedMarker;
				C4LSector *pSct;
				for (C4ObjectList *pLst=obj1->Area.FirstObjects(&pSct); pLst; pLst=obj1->Area.NextObjects(pLst, &pSct))
					for (C4ObjectList::iterator iter2=pLst->begin(); iter2 != pLst->end() && (obj2=*iter2); ++iter2)
						if (obj2->Status && !obj2->Contained && (obj2!=obj1) && (obj2->OCF & tocf))
							if (Inside<int32_t>(obj2->GetX()-(obj1->GetX()+obj1->Shape.x),0,obj1->Shape.Wdt-1))
								if (Inside<int32_t>(obj2->GetY()-(obj1->GetY()+obj1->Shape.y),0,obj1->Shape.Hgt-1))
									if (obj1->Layer == obj2->Layer)
									{
										// handle collision only once
										if (obj2->Marker == Marker) continue;
										obj2->Marker = Marker;
										// Only hit if target is alive and projectile is an object
										if ((obj1->OCF & OCF_Alive) && (obj2->Category & C4D_Object))
										{
											C4Real dXDir = obj2->xdir - obj1->xdir, dYDir = obj2->ydir - obj1->ydir;
											C4Real speed = dXDir * dXDir + dYDir * dYDir;
											// Only hit if obj2's speed and relative speeds are larger than HitSpeed2
											if ((obj2->OCF & OCF_HitSpeed2) && speed > HitSpeed2)
												if (!obj1->Call(PSF_QueryCatchBlow, &C4AulParSet(C4VObj(obj2))))
												{
													int32_t iHitEnergy = fixtoi(speed * obj2->Mass / 5 );
													// Hit energy reduced to 1/3rd, but do not drop to zero because of this division
													iHitEnergy = Max<int32_t>(iHitEnergy/3, !!iHitEnergy); 
													obj1->DoEnergy(-iHitEnergy/5, false, C4FxCall_EngObjHit, obj2->Controller);
													int tmass=Max<int32_t>(obj1->Mass,50);
													C4PropList* pActionDef = obj1->GetAction();
													if (!::Game.iTick3 || (pActionDef && pActionDef->GetPropertyP(P_Procedure) != DFA_FLIGHT))
														obj1->Fling(obj2->xdir*50/tmass,-Abs(obj2->ydir/2)*50/tmass, false);
													obj1->Call(PSF_CatchBlow,&C4AulParSet(C4VInt(-iHitEnergy/5),
																						  C4VObj(obj2)));
													// obj1 might have been tampered with
													if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
														goto out1;
													continue;
												}
										}
										// Collection
										if ((obj1->OCF & OCF_Collection) && (obj2->OCF & OCF_Carryable))
											if (Inside<int32_t>(obj2->GetX()-(obj1->GetX()+obj1->Def->Collection.x),0,obj1->Def->Collection.Wdt-1))
												if (Inside<int32_t>(obj2->GetY()-(obj1->GetY()+obj1->Def->Collection.y),0,obj1->Def->Collection.Hgt-1))
												{
													//if(!pLst->First) BREAKPOINT_HERE;
													obj1->Collect(obj2);
													//if(!pLst->First)  BREAKPOINT_HERE;
													// obj1 might have been tampered with
													if (!obj1->Status || obj1->Contained || !(obj1->OCF & focf))
														goto out1;
												}
									}
out1: ;
			}
}

C4Object* C4GameObjects::AtObject(int ctx, int cty, DWORD &ocf, C4Object *exclude)
{
	DWORD cocf;
	C4Object *cObj; C4ObjectLink *clnk;

	for (clnk=ObjectsAt(ctx,cty).First; clnk && (cObj=clnk->Obj); clnk=clnk->Next)
		if (!exclude || (cObj!=exclude && exclude->Layer == cObj->Layer)) if (cObj->Status)
			{
				cocf=ocf | OCF_Exclusive;
				if (cObj->At(ctx,cty,cocf))
				{
					// Search match
					if (cocf & ocf) { ocf=cocf; return cObj; }
					// EXCLUSIVE block
					else return NULL;
				}
			}
	return NULL;
}

void C4GameObjects::Synchronize()
{
	// synchronize unsorted objects
	ResortUnsorted();
	// synchronize solidmasks
	UpdateSolidMasks();
}

C4Object *C4GameObjects::ObjectPointer(int32_t iNumber)
{
	// search own list
	C4PropList *pObj = PropLists.Get(iNumber);
	if (pObj) return pObj->GetObject();
	return 0;
}

C4PropList *C4GameObjects::PropListPointer(int32_t iNumber)
{
	return PropLists.Get(iNumber);
}

int32_t C4GameObjects::ObjectNumber(C4PropList *pObj)
{
	if (!pObj) return 0;
	C4PropListNumbered * const * p = PropLists.First();
	while (p)
	{
		if (*p == pObj)
		{
			if ((*p)->GetPropListNumbered()) return (*p)->GetPropListNumbered()->Number;
			return 0;
		}
		p = PropLists.Next(p);
	}
	return 0;
}

C4Object *C4GameObjects::SafeObjectPointer(int32_t iNumber)
{
	C4Object *pObj = ObjectPointer(iNumber);
	if (pObj) if (!pObj->Status) return NULL;
	return pObj;
}

void C4GameObjects::UpdateSolidMasks()
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->UpdateSolidMask(false);
}

void C4GameObjects::DeleteObjects(bool fDeleteInactive)
{
	C4ObjectList::DeleteObjects();
	Sectors.ClearObjects();
	BackObjects.Clear();
	ForeObjects.Clear();
	if (fDeleteInactive) InactiveObjects.DeleteObjects();
}

void C4GameObjects::Clear(bool fClearInactive)
{
	DeleteObjects(fClearInactive);
	if (fClearInactive)
		InactiveObjects.Clear();
	LastUsedMarker = 0;
}

#define C4CV_Section_Object "[Object]"

int C4GameObjects::Load(C4Group &hGroup, bool fKeepInactive)
{
	Clear(!fKeepInactive);

	// Load data component
	StdStrBuf Source;
	if (!hGroup.LoadEntryString(C4CFN_ScenarioObjects, Source))
		return 0;

	// Compile
	StdStrBuf Name = hGroup.GetFullName() + DirSep C4CFN_ScenarioObjects;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
	      mkParAdapt(*this, false),
	      Source,
	      Name.getData()))
		return 0;

	// Process objects
	C4ObjectLink *cLnk;
	C4Object *pObj;
	bool fObjectNumberCollision = false;
	int32_t iMaxObjectNumber = 0;
	for (cLnk = Last; cLnk; cLnk = cLnk->Prev)
	{
		C4Object *pObj = cLnk->Obj;
		// check object number collision with inactive list
		if (fKeepInactive)
		{
			for (C4ObjectLink *clnk = InactiveObjects.First; clnk; clnk=clnk->Next)
				if (clnk->Obj->Number == pObj->Number) fObjectNumberCollision = true;
		}
		// keep track of numbers
		iMaxObjectNumber = Max<long>(iMaxObjectNumber, pObj->Number);
		// add to list of backobjects
		if (pObj->Category & C4D_Background)
			::Objects.BackObjects.Add(pObj, C4ObjectList::stMain, this);
		// add to list of foreobjects
		if (pObj->Category & C4D_Foreground)
			::Objects.ForeObjects.Add(pObj, C4ObjectList::stMain, this);
		// Unterminate end
	}

	// Denumerate pointers
	// if object numbers collideded, numbers will be adjusted afterwards
	// so fake inactive object list empty meanwhile
	C4ObjectLink *pInFirst = NULL;
	if (fObjectNumberCollision) { pInFirst = InactiveObjects.First; InactiveObjects.First = NULL; }
	// denumerate pointers
	Denumerate();
	for (C4PropListNumbered * const * ppPropList = PropLists.First(); ppPropList; ppPropList = PropLists.Next(ppPropList))
		if ((*ppPropList)->IsScriptPropList()) (*ppPropList)->DenumeratePointers();
	// update object enumeration index now, because calls like UpdateTransferZone might create objects
	Game.ObjectEnumerationIndex = Max(Game.ObjectEnumerationIndex, iMaxObjectNumber);
	// end faking and adjust object numbers
	if (fObjectNumberCollision)
	{
		InactiveObjects.First=pInFirst;
		// simply renumber all inactive objects
		for (cLnk=InactiveObjects.First; cLnk; cLnk=cLnk->Next)
			if ((pObj=cLnk->Obj)->Status)
				pObj->Number = ++Game.ObjectEnumerationIndex;
	}

	// special checks:
	// -contained/contents-consistency
	// -StaticBack-objects zero speed
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if ((pObj=cLnk->Obj)->Status)
		{
			// staticback must not have speed
			if (pObj->Category & C4D_StaticBack)
			{
				pObj->xdir = pObj->ydir = 0;
			}
			// contained must be in contents list
			if (pObj->Contained)
				if (!pObj->Contained->Contents.GetLink(pObj))
				{
					DebugLogF("Error in Objects.txt: Container of #%d is #%d, but not found in contents list!", pObj->Number, pObj->Contained->Number);
					pObj->Contained->Contents.Add(pObj, C4ObjectList::stContents);
				}
			// all contents must have contained set; otherwise, remove them!
			C4Object *pObj2;
			for (C4ObjectLink *cLnkCont=pObj->Contents.First; cLnkCont; cLnkCont=cLnkCont->Next)
			{
				// check double links
				if (pObj->Contents.GetLink(cLnkCont->Obj) != cLnkCont)
				{
					DebugLogF("Error in Objects.txt: Double containment of #%d by #%d!", cLnkCont->Obj->Number, pObj->Number);
					// this remove-call will only remove the previous (dobuled) link, so cLnkCont should be save
					pObj->Contents.Remove(cLnkCont->Obj);
					// contents checked already
					continue;
				}
				// check contents/contained-relation
				if ((pObj2=cLnkCont->Obj)->Status)
					if (pObj2->Contained != pObj)
					{
						DebugLogF("Error in Objects.txt: Object #%d not in container #%d as referenced!", pObj2->Number, pObj->Number);
						pObj2->Contained = pObj;
					}
			}
		}
	// sort out inactive objects
	C4ObjectLink *cLnkNext;
	for (cLnk=First; cLnk; cLnk=cLnkNext)
	{
		cLnkNext = cLnk->Next;
		if (cLnk->Obj->Status == C4OS_INACTIVE)
		{
			if (cLnk->Prev) cLnk->Prev->Next=cLnkNext; else First=cLnkNext;
			if (cLnkNext) cLnkNext->Prev=cLnk->Prev; else Last=cLnk->Prev;
			if ((cLnk->Prev = InactiveObjects.Last))
				InactiveObjects.Last->Next = cLnk;
			else
				InactiveObjects.First = cLnk;
			InactiveObjects.Last = cLnk; cLnk->Next = NULL;
			Mass-=cLnk->Obj->Mass;
		}
	}

	{
		C4DebugRecOff DBGRECOFF; // - script callbacks that would kill DebugRec-sync for runtime start
		// update graphics
		UpdateGraphics(false);
		// Update faces
		UpdateFaces(false);
		// Update ocf
		SetOCF();
	}

	// make sure list is sorted by category - after sorting out inactives, because inactives aren't sorted into the main list
	FixObjectOrder();

	//Sectors.Dump();

	// misc updates
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if ((pObj=cLnk->Obj)->Status)
		{
			// add to plrview
			pObj->PlrFoWActualize();
			// update flipdir (for old objects.txt with no flipdir defined)
			// assigns Action.DrawDir as well
			pObj->UpdateFlipDir();
			// initial OCF update
			pObj->SetOCF();
		}
	// Done
	return ObjectCount();
}

bool C4GameObjects::Save(C4Group &hGroup, bool fSaveGame, bool fSaveInactive)
{
	// Save to temp file
	char szFilename[_MAX_PATH+1]; SCopy( Config.AtTempPath(C4CFN_ScenarioObjects), szFilename );
	if (!Save(szFilename,fSaveGame,fSaveInactive)) return false;

	// Move temp file to group
	hGroup.Move(szFilename, NULL); // check?
	// Success
	return true;
}

bool C4GameObjects::Save(const char *szFilename, bool fSaveGame, bool fSaveInactive)
{
	// Enumerate
	Enumerate();
	InactiveObjects.Enumerate();

	// Decompile objects to buffer
	StdStrBuf Buffer;
	bool fSuccess = DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(*this, !fSaveGame), &Buffer, szFilename);

	// Decompile inactives
	if (fSaveInactive)
	{
		StdStrBuf InactiveBuffer;
		fSuccess &= DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(InactiveObjects, false, !fSaveGame), &InactiveBuffer, szFilename);
		Buffer.Append("\r\n");
		Buffer.Append(InactiveBuffer);
	}

	// Denumerate
	InactiveObjects.Denumerate();
	Denumerate();

	// Error?
	if (!fSuccess)
		return false;

	// Write
	return Buffer.SaveToFile(szFilename);
}

void C4GameObjects::UpdateScriptPointers()
{
	// call in sublists
	C4ObjectList::UpdateScriptPointers();
	InactiveObjects.UpdateScriptPointers();
	// adjust global effects
	if (Game.pGlobalEffects) Game.pGlobalEffects->ReAssignAllCallbackFunctions();
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
	// fixes the object order so it matches the global object order sorting constraints
	C4ObjectLink *pLnk0=First, *pLnkL=Last;
	while (pLnk0 != pLnkL)
	{
		C4ObjectLink *pLnk1stUnsorted=NULL, *pLnkLastUnsorted=NULL, *pLnkPrev=NULL, *pLnk;
		C4Object *pLastWarnObj = NULL;
		// forward fix
		int lastPlane = 2147483647; //INT32_MAX;
		for (pLnk = pLnk0; pLnk!=pLnkL->Next; pLnk=pLnk->Next)
		{
			C4Object *pObj = pLnk->Obj;
			if (pObj->Unsorted || !pObj->Status) continue;
			int currentPlane = pObj->GetPlane();
			// must have nonzero Plane
			if (!currentPlane)
			{
				DebugLogF("Objects.txt: Object #%d has zero Plane!", (int) pObj->Number);
				pObj->SetPlane(lastPlane); currentPlane = lastPlane;
			}
			// fix order
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
				lastPlane = currentPlane;
			pLnkPrev = pLnk;
		}
		if (!pLnkLastUnsorted) break; // done
		pLnkL = pLnkLastUnsorted;
		// backwards fix
		lastPlane = -2147483647-1; //INT32_MIN;
		for (pLnk = pLnkL; pLnk!=pLnk0->Prev; pLnk=pLnk->Prev)
		{
			C4Object *pObj = pLnk->Obj;
			if (pObj->Unsorted || !pObj->Status) continue;
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
				lastPlane = currentPlane;
			pLnkPrev = pLnk;
		}
		if (!pLnk1stUnsorted) break; // done
		pLnk0 = pLnk1stUnsorted;
	}
	// objects fixed!
}

void C4GameObjects::ResortUnsorted()
{
	C4ObjectLink *clnk=First; C4Object *cObj;
	while (clnk && (cObj=clnk->Obj))
	{
		clnk=clnk->Next;
		if (cObj->Unsorted)
		{
			// readd to main object list
			Remove(cObj);
			cObj->Unsorted=false;
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
	// validate in sublists
	bool fSucc = true;
	if (!C4ObjectList::ValidateOwners()) fSucc = false;
	if (!InactiveObjects.ValidateOwners()) fSucc = false;
	return fSucc;
}

bool C4GameObjects::AssignInfo()
{
	// assign in sublists
	bool fSucc = true;
	if (!C4ObjectList::AssignInfo()) fSucc = false;
	if (!InactiveObjects.AssignInfo()) fSucc = false;
	return fSucc;
}

void C4GameObjects::AssignPlrViewRange()
{
	C4ObjectLink *cLnk;
	for (cLnk=Last; cLnk; cLnk=cLnk->Prev)
		if (cLnk->Obj->Status)
			cLnk->Obj->AssignPlrViewRange();
}

void C4GameObjects::SyncClearance()
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj)
			cLnk->Obj->SyncClearance();
}

void C4GameObjects::UpdateTransferZones()
{
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		cobj->Call(PSF_UpdateTransferZone);
}

void C4GameObjects::ResetAudibility()
{
	C4Object *cobj; C4ObjectLink *clnk;
	for (clnk=First; clnk && (cobj=clnk->Obj); clnk=clnk->Next)
		cobj->Audible=cobj->AudiblePan=0;
}

void C4GameObjects::SetOCF()
{
	C4ObjectLink *cLnk;
	for (cLnk=First; cLnk; cLnk=cLnk->Next)
		if (cLnk->Obj->Status)
			cLnk->Obj->SetOCF();
}
