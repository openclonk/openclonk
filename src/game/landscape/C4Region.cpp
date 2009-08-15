/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
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

/* Screen area marked for mouse interaction */

#include <C4Include.h>
#include <C4Region.h>

#ifndef BIG_C4INCLUDE
#include <C4Facet.h>
#endif

C4Region::C4Region()
	{
	Default();
	}

C4Region::~C4Region()
	{
	Clear();
	}

void C4Region::Default()
	{
	X=Y=Wdt=Hgt=0;
	Caption[0]=0;
	Com=RightCom=MoveOverCom=HoldCom=COM_None;
	Data=0;
	id=C4ID_None;
	Target=NULL;
	}

void C4Region::Clear()
	{

	}

C4RegionList::C4RegionList()
	{
	Default();
	}

C4RegionList::~C4RegionList()
	{
	Clear();
	}

void C4RegionList::Default()
	{
	First=NULL;
	AdjustX=AdjustY=0;
	}

void C4RegionList::Clear()
	{
	C4Region *pRgn,*pNext;
	for (pRgn=First; pRgn; pRgn=pNext) { pNext=pRgn->Next; delete pRgn; }
	First=NULL;
	}

bool C4RegionList::Add(int iX, int iY, int iWdt, int iHgt, const char *szCaption,
											 int iCom, C4Object *pTarget, int iMoveOverCom, int iHoldCom, int iData)
	{
	C4Region *pRgn = new C4Region;
	pRgn->Set(iX+AdjustX,iY+AdjustY,iWdt,iHgt,szCaption,iCom,iMoveOverCom,iHoldCom,iData,pTarget);
	pRgn->Next=First;
	First=pRgn;
	return true;
	}

bool C4RegionList::Add(C4Facet &fctArea, const char *szCaption, int iCom, C4Object *pTarget, int iMoveOverCom, int iHoldCom, int iData)
	{
	return Add(fctArea.X,fctArea.Y,fctArea.Wdt,fctArea.Hgt,szCaption,iCom,pTarget,iMoveOverCom,iHoldCom,iData);
	}

bool C4RegionList::Add(C4Region &rRegion)
	{
	C4Region *pRgn = new C4Region;
	*pRgn = rRegion;
	pRgn->X+=AdjustX;
	pRgn->Y+=AdjustY;
	pRgn->Next=First;
	First=pRgn;
	return true;
	}

void C4Region::Set(int iX, int iY, int iWdt, int iHgt, const char *szCaption,
									int iCom, int iMoveOverCom, int iHoldCom, int iData,
									C4Object *pTarget)
	{
	X=iX; Y=iY; Wdt=iWdt; Hgt=iHgt;
	SCopy(szCaption,Caption,C4RGN_MaxCaption);
	Com=iCom; MoveOverCom=iMoveOverCom; HoldCom=iHoldCom;
	Data=iData;
	Target=pTarget;
	}

void C4RegionList::SetAdjust(int iX, int iY)
	{
	AdjustX=iX; AdjustY=iY;
	}

C4Region* C4RegionList::Find(int iX, int iY)
	{
	for (C4Region *pRgn=First; pRgn; pRgn=pRgn->Next)
		if (Inside(iX-pRgn->X,0,pRgn->Wdt-1))
			if (Inside(iY-pRgn->Y,0,pRgn->Hgt-1))
				return pRgn;
	return NULL;
	}

void C4Region::ClearPointers(C4Object *pObj)
	{
	if (Target==pObj) Target=NULL;
	}

void C4RegionList::ClearPointers(C4Object *pObj)
	{
	for (C4Region *pRgn=First; pRgn; pRgn=pRgn->Next)
		pRgn->ClearPointers(pObj);
	}

void C4Region::Set(C4Facet &fctArea, const char *szCaption, C4Object *pTarget)
	{
	X=fctArea.X;
	Y=fctArea.Y;
	Wdt=fctArea.Wdt;
	Hgt=fctArea.Hgt;
	if (szCaption) SCopy(szCaption,Caption,C4RGN_MaxCaption);
	if (pTarget) Target=pTarget;
	}

bool C4RegionList::Add(C4RegionList &rRegionList, bool fAdjust)
	{
	C4Region *pNewFirst=NULL,*pPrev=NULL;
	for (C4Region *cRgn=rRegionList.First; cRgn; cRgn=cRgn->Next)
		{
		C4Region *pRgn = new C4Region;
		*pRgn = *cRgn;
		if (fAdjust) { pRgn->X+=AdjustX; pRgn->Y+=AdjustY; }
		pRgn->Next=First;
		if (!pNewFirst) pNewFirst=pRgn;
		if (pPrev) pPrev->Next=pRgn;
		pPrev=pRgn;
		}
	if (pNewFirst) First=pNewFirst;
	return true;
	}
