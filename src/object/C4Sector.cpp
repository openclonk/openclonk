/*
 * OpenClonk, http://www.openclonk.org
 *
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
// landscape sector base class

#include "C4Include.h"
#include "object/C4Sector.h"

#include "game/C4Game.h"
#include "object/C4Object.h"
#include "lib/C4Log.h"
#include "control/C4Record.h"
#include "object/C4GameObjects.h"

/* sector */

void C4LSector::Init(int ix, int iy)
{
	// clear any previous initialization
	Clear();
	// store class members
	x=ix; y=iy;
}

void C4LSector::Clear()
{
	// clear objects
	ClearObjects();
}

void C4LSector::CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers)
{
	pComp->Value(mkNamingAdapt(mkIntAdapt(x), "x"));
	pComp->Value(mkNamingAdapt(mkIntAdapt(y), "y"));
	pComp->Value(mkNamingAdapt(mkParAdapt(Objects, numbers), "Objects"));
	pComp->Value(mkNamingAdapt(mkParAdapt(ObjectShapes, numbers), "ObjectShapes"));
}

void C4LSector::ClearObjects()
{
	// clear objects
	Objects.Clear();
	ObjectShapes.Clear();
}

/* sector map */

void C4LSectors::Init(int iWdt, int iHgt)
{
	// clear any previous initialization
	Clear();
	// store class members, calc size
	Wdt = ((PxWdt=iWdt)-1)/C4LSectorWdt+1;
	Hgt = ((PxHgt=iHgt)-1)/C4LSectorHgt+1;
	// create sectors
	Sectors = new C4LSector[Size=Wdt*Hgt];
	// init sectors
	C4LSector *sct=Sectors;
	for (int cnt=0; cnt<Size; cnt++, sct++)
		sct->Init(cnt%Wdt, cnt/Wdt);
	SectorOut.Init(-1,-1); // outpos at -1,-1 - MUST NOT intersect with an inside sector!
}

void C4LSectors::Clear()
{
	// clear out-sector
	SectorOut.Clear();
	// free sectors
	delete [] Sectors; Sectors=nullptr;
}

C4LSector *C4LSectors::SectorAt(int ix, int iy)
{
	// check bounds
	if (ix<0 || iy<0 || ix>=PxWdt || iy>=PxHgt)
		return &SectorOut;
	// get sector
	return Sectors+(iy/C4LSectorHgt)*Wdt+(ix/C4LSectorWdt);
}

void C4LSectors::Add(C4Object *pObj, C4ObjectList *pMainList)
{
	assert(Sectors);
	// Add to owning sector
	C4LSector *pSct = SectorAt(pObj->GetX(), pObj->GetY());
	pSct->Objects.Add(pObj, C4ObjectList::stMain, pMainList);
	// Save position
	pObj->old_x = pObj->GetX(); pObj->old_y = pObj->GetY();
	// Add to all sectors in shape area
	pObj->Area.Set(this, pObj);
	for (pSct = pObj->Area.First(); pSct; pSct = pObj->Area.Next(pSct))
	{
		pSct->ObjectShapes.Add(pObj, C4ObjectList::stMain, pMainList);
	}
	if (Config.General.DebugRec)
		pObj->Area.DebugRec(pObj, 'A');
}

void C4LSectors::Update(C4Object *pObj, C4ObjectList *pMainList)
{
	assert(Sectors);
	// Not added yet?
	if (pObj->Area.IsNull())
	{
		Add(pObj, pMainList);
		return;
	}
	C4LSector *pOld, *pNew;
	if (pObj->old_x != pObj->GetX() || pObj->old_y != pObj->GetY())
	{
		// Get involved sectors
		pOld = SectorAt(pObj->old_x, pObj->old_y);
		pNew = SectorAt(pObj->GetX(), pObj->GetY());
		if (pOld != pNew)
		{
			pOld->Objects.Remove(pObj);
			pNew->Objects.Add(pObj, C4ObjectList::stMain, pMainList);
		}
		// Save position
		pObj->old_x = pObj->GetX(); pObj->old_y = pObj->GetY();
	}
	// New area
	C4LArea NewArea(this, pObj);
	if (pObj->Area == NewArea) return;
	// Remove from all old sectors in shape area
	for (pOld = pObj->Area.First(); pOld; pOld = pObj->Area.Next(pOld))
		if (!NewArea.Contains(pOld))
			pOld->ObjectShapes.Remove(pObj);
	// Add to all new sectors in shape area
	for (pNew = NewArea.First(); pNew; pNew = NewArea.Next(pNew))
		if (!pObj->Area.Contains(pNew))
		{
			pNew->ObjectShapes.Add(pObj, C4ObjectList::stMain, pMainList);
		}
	// Update area
	pObj->Area = NewArea;
	if (Config.General.DebugRec)
		pObj->Area.DebugRec(pObj, 'U');
}

void C4LSectors::Remove(C4Object *pObj)
{
	assert(Sectors); assert(pObj);
	// Remove from owning sector
	C4LSector *pSct = SectorAt(pObj->old_x, pObj->old_y);
	if (!pSct->Objects.Remove(pObj))
	{
#ifdef _DEBUG
		LogF("WARNING: Object %d of type %s deleted but not found in pos sector list!", pObj->Number, pObj->id.ToString());
#endif
		// if it was not found in owning sector, it must be somewhere else. yeah...
		bool fFound = false;
		for (pSct = pObj->Area.First(); pSct; pSct = pObj->Area.Next(pSct))
			if (pSct->Objects.Remove(pObj)) { fFound=true; break; }
		// yukh, somewhere else entirely...
		if (!fFound)
		{
			fFound = !!SectorOut.Objects.Remove(pObj);
			if (!fFound)
			{
				pSct = Sectors;
				for (int cnt=0; cnt<Size; cnt++, pSct++)
					if (pSct->Objects.Remove(pObj)) { fFound=true; break; }
			}
			assert(fFound);
		}
	}
	// Remove from all sectors in shape area
	for (pSct = pObj->Area.First(); pSct; pSct = pObj->Area.Next(pSct))
		pSct->ObjectShapes.Remove(pObj);
	if (Config.General.DebugRec)
		pObj->Area.DebugRec(pObj, 'R');
}

void C4LSectors::AssertObjectNotInList(C4Object *pObj)
{
#ifndef NDEBUG
	C4LSector *sct=Sectors;
	for (int cnt=0; cnt<Size; cnt++, sct++)
	{
		assert(!sct->Objects.IsContained(pObj));
		assert(!sct->ObjectShapes.IsContained(pObj));
	}
	assert(!SectorOut.Objects.IsContained(pObj));
	assert(!SectorOut.ObjectShapes.IsContained(pObj));
#endif
}

int C4LSectors::getShapeSum() const
{
	int iSum = 0;
	for (int cnt=0; cnt<Size; cnt++)
		iSum += Sectors[cnt].ObjectShapes.ObjectCount();
	return iSum;
}

void C4LSectors::Dump()
{
	C4ValueNumbers numbers;
	LogSilent(DecompileToBuf<StdCompilerINIWrite>(
	            mkNamingAdapt(
	              mkArrayAdaptMap(Sectors, Size, mkParAdaptMaker(&numbers)),
	              "Sector")).getData());
}

bool C4LSectors::CheckSort()
{
	for (int cnt=0; cnt<Size; cnt++)
		if (!Sectors[cnt].Objects.CheckSort(&::Objects))
			return false;
	if (!SectorOut.Objects.CheckSort(&::Objects)) return false;
	return true;
}

void C4LSectors::ClearObjects()
{
	if (Sectors)
	{
		for (int cnt=0; cnt<Size; cnt++) Sectors[cnt].ClearObjects();
	}
	SectorOut.ClearObjects();
}

/* landscape area */

bool C4LArea::operator == (const C4LArea &Area) const
{
	return pFirst == Area.pFirst &&
	       xL == Area.xL &&
	       yL == Area.yL &&
	       pOut == Area.pOut;
}

void C4LArea::Set(C4LSectors *pSectors, const C4Rect &Rect)
{
	// default: no area
	pFirst=nullptr; pOut=nullptr;
	// check bounds
	C4Rect ClippedRect(Rect),
	Bounds(0, 0, pSectors->PxWdt, pSectors->PxHgt);
	ClippedRect.Normalize();
	if (!Bounds.Contains(ClippedRect))
	{
		ClippedRect.Intersect(Bounds);
		pOut = &pSectors->SectorOut;
	}
	// calc first sector
	pFirst = pSectors->SectorAt(ClippedRect.x, ClippedRect.y);
	// assert the rect isn't degenerated for the following calculations
	// (note this will associate areas that are above landscape bounds with sectors inside)
	if (!ClippedRect.Wdt) ClippedRect.Wdt = 1;
	if (!ClippedRect.Hgt) ClippedRect.Hgt = 1;
	// calc bounds
	xL = (ClippedRect.x + ClippedRect.Wdt - 1) / C4LSectorWdt;
	yL = (ClippedRect.y + ClippedRect.Hgt - 1) / C4LSectorHgt;
	// calc pitch
	dpitch = pSectors->Wdt - (ClippedRect.x + ClippedRect.Wdt - 1) / C4LSectorWdt + ClippedRect.x / C4LSectorWdt;
}

void C4LArea::Set(C4LSectors *pSectors, C4Object *pObj)
{
	// set to object facet rect
	Set(pSectors, C4Rect(pObj->Left(), pObj->Top(), pObj->Width(), pObj->Height()));
}

C4LSector *C4LArea::Next(C4LSector *pPrev) const
{
	// the outside-sector is the last sector that is returned
	if (pPrev == pOut)
		return nullptr;
	// within one line?
	if (pPrev->x<xL)
		return pPrev+1;
	// within the area?
	if (pPrev->y<yL)
		return pPrev+dpitch;
	// end reached - return outside-sector if applicable
	return pOut;
}

bool C4LArea::Contains(C4LSector *pSct) const
{
	assert(pSct);
	// no area
	if (!pFirst) return false;
	// outside?
	if (pSct == pOut) return true;
	if (pFirst == pOut) return false;
	// check bounds
	return (pSct->x>=pFirst->x && pSct->y>=pFirst->y && pSct->x<=xL && pSct->y<=yL);
}

C4ObjectList *C4LArea::NextObjects(C4ObjectList *pPrev, C4LSector **ppSct)
{
	// get next sector
	if (!*ppSct)
		*ppSct = First();
	else
		*ppSct = Next(*ppSct);
	// nothing left?
	if (!*ppSct)
		return nullptr;
	// return object list
	return &(*ppSct)->Objects;
}

C4ObjectList *C4LArea::NextObjectShapes(C4ObjectList *pPrev, C4LSector **ppSct)
{
	// get next sector
	if (!*ppSct)
		*ppSct = First();
	else
		*ppSct = Next(*ppSct);
	// nothing left?
	if (!*ppSct)
		return nullptr;
	// return object list
	return &(*ppSct)->ObjectShapes;
}

void C4LArea::DebugRec(class C4Object *pObj, char cMarker)
{
	C4RCArea rc;
	rc.op = cMarker;
	rc.obj = pObj ? pObj->Number : -1;
	rc.x1 = pFirst ? pFirst->x : -1;
	rc.y1 = pFirst ? pFirst->x /* 2do: y */ : -1;
	rc.xL = xL;
	rc.yL = yL;
	rc.dpitch = dpitch;
	rc.out = !!pOut;
	AddDbgRec(RCT_Area, &rc, sizeof(C4RCArea));
}
