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

/* Special regions to extend the pathfinder */

#include "C4Include.h"
#include "landscape/C4TransferZone.h"

#include "graphics/C4FacetEx.h"
#include "graphics/C4Draw.h"
#include "landscape/C4Landscape.h"
#include "object/C4GameObjects.h"
#include "lib/StdColors.h"

C4TransferZone::C4TransferZone()
{
	Object = nullptr;
	X = Y = Wdt = Hgt = 0;
	Next = nullptr;
	Used = false;
}

C4TransferZone::~C4TransferZone()
{
}

C4TransferZones::C4TransferZones()
{
	Default();
}

C4TransferZones::~C4TransferZones()
{
	Clear();
}

void C4TransferZones::Default()
{
	First=nullptr;
}

void C4TransferZones::Clear()
{
	C4TransferZone *pZone,*pNext;
	for (pZone=First; pZone; pZone=pNext) { pNext=pZone->Next; delete pZone; }
	First=nullptr;
}

void C4TransferZones::ClearPointers(C4Object *pObj)
{
	// Clear object pointers
	for (C4TransferZone *pZone=First; pZone; pZone=pZone->Next)
		if (pZone->Object==pObj)
			pZone->Object=nullptr;
	// Remove cleared zones immediately
	RemoveNullZones();
}

bool C4TransferZones::Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj)
{
	C4TransferZone *pZone;
	// Empty zone: clear existing object zones
	if (!iWdt || !iHgt) { ClearPointers(pObj); return true; }
	// Update existing zone
	if ((pZone=Find(pObj)))
	{
		pZone->X=iX; pZone->Y=iY;
		pZone->Wdt=iWdt; pZone->Hgt=iHgt;
	}
	// Allocate and add new zone
	else
		Add(iX,iY,iWdt,iHgt,pObj);
	// Success
	return true;
}

bool C4TransferZones::Add(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj)
{
	C4TransferZone *pZone;
	// Allocate and add new zone
	pZone = new C4TransferZone;
	pZone->X=iX; pZone->Y=iY;
	pZone->Wdt=iWdt; pZone->Hgt=iHgt;
	pZone->Object=pObj;
	pZone->Next=First;
	First=pZone;
	// Success
	return true;
}

void C4TransferZones::Synchronize()
{
	Clear();
	::Objects.OnSynchronized();
}

C4TransferZone* C4TransferZones::Find(int32_t iX, int32_t iY)
{
	for (C4TransferZone *pZone=First; pZone; pZone=pZone->Next)
		if (Inside<int32_t>(iX-pZone->X,0,pZone->Wdt-1))
			if (Inside<int32_t>(iY-pZone->Y,0,pZone->Hgt-1))
				return pZone;
	return nullptr;
}

void C4TransferZones::Draw(C4TargetFacet &cgo)
{
	for (C4TransferZone *pZone=First; pZone; pZone=pZone->Next)
		pZone->Draw(cgo);
}

void C4TransferZone::Draw(C4TargetFacet &cgo, bool fHighlight)
{
	if (Used) fHighlight=true;
	pDraw->DrawFrameDw(cgo.Surface,
	                   int(cgo.X+X-cgo.TargetX),int(cgo.Y+Y-cgo.TargetY),
	                   int(cgo.X+X-cgo.TargetX+Wdt-1),int(cgo.Y+Y-cgo.TargetY+Hgt-1),
	                   fHighlight ? C4RGB(0, 0xca, 0) : C4RGB(0xca, 0, 0));
}

bool C4TransferZone::At(int32_t iX, int32_t iY)
{
	return (Inside<int32_t>(iX-X,0,Wdt-1) && Inside<int32_t>(iY-Y,0,Hgt-1));
}

int32_t C4TransferZones::RemoveNullZones()
{
	int32_t iResult=0;
	C4TransferZone *pZone,*pNext,*pPrev=nullptr;
	for (pZone=First; pZone; pZone=pNext)
	{
		pNext=pZone->Next;
		if (!pZone->Object)
		{
			delete pZone;
			if (pPrev) pPrev->Next=pNext;
			else First=pNext;
			iResult++;
		}
		else
			pPrev=pZone;
	}
	return iResult;
}

void AdjustMoveToTarget(int32_t &rX, int32_t &rY, bool fFreeMove, int32_t iShapeHgt);

bool C4TransferZone::GetEntryPoint(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY)
{
	// Target inside zone: move outside horizontally
	if (Inside<int32_t>(iToX-X,0,Wdt-1) && Inside<int32_t>(iToY-Y,0,Hgt-1))
	{
		if (iToX<X+Wdt/2) iToX=X-1;
		else iToX=X+Wdt;
	}
	// Get closest adjacent point
	rX=Clamp<int32_t>(iToX,X-1,X+Wdt);
	rY=Clamp<int32_t>(iToY,Y-1,Y+Hgt);
	// Search around zone for free
	int32_t iX1=rX,iY1=rY,iX2=rX,iY2=rY;
	int32_t iXIncr1=0,iYIncr1=-1,iXIncr2=0,iYIncr2=+1;
	int32_t cnt;
	for (cnt=0; cnt<2*Wdt+2*Hgt; cnt++)
	{
		// Found free
		if (!GBackSolid(iX1,iY1)) { rX=iX1; rY=iY1; break; }
		if (!GBackSolid(iX2,iY2)) { rX=iX2; rY=iY2; break; }
		// Advance
		iX1+=iXIncr1; iY1+=iYIncr1;
		iX2+=iXIncr2; iY2+=iYIncr2;
		// Corners
		if (iY1<Y-1) { iY1=Y-1; iXIncr1=+1; iYIncr1=0; }
		if (iX1>X+Wdt) { iX1=X+Wdt; iXIncr1=0; iYIncr1=+1; }
		if (iY1>Y+Hgt) { iY1=Y+Hgt; iXIncr1=-1; iYIncr1=0; }
		if (iX1<X-1) { iX1=X-1; iXIncr1=0; iYIncr1=-1; }
		if (iY2<Y-1) { iY2=Y-1; iXIncr2=-1; iYIncr2=0; }
		if (iX2>X+Wdt) { iX2=X+Wdt; iXIncr2=0; iYIncr2=-1; }
		if (iY2>Y+Hgt) { iY2=Y+Hgt; iXIncr2=+1; iYIncr2=0; }
		if (iX2<X-1) { iX2=X-1; iXIncr2=0; iYIncr2=+1; }
	}
	// No free found
	if (cnt>=2*Wdt+2*Hgt) return false;
	// Vertical walk-to adjust (only if at the side of zone)
	if (!Inside<int32_t>(rX-X,0,Wdt-1))
		AdjustMoveToTarget(rX,rY,false,20);
	// Success
	return true;
}

void C4TransferZones::ClearUsed()
{
	for (C4TransferZone *pZone=First; pZone; pZone=pZone->Next)
		pZone->Used=false;
}

C4TransferZone* C4TransferZones::Find(C4Object *pObj)
{
	for (C4TransferZone *pZone=First; pZone; pZone=pZone->Next)
		if (pZone->Object==pObj)
			return pZone;
	return nullptr;
}
