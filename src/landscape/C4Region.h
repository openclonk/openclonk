/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
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

#ifndef INC_C4Region
#define INC_C4Region

#include <C4Id.h>
#include <C4Constants.h>

class C4Object;
class C4Facet;

const int C4RGN_MaxCaption=256;

class C4Region
{
	friend class C4RegionList;
public:
	C4Region();
	~C4Region();
public:
	int X,Y,Wdt,Hgt;
	char Caption[C4RGN_MaxCaption+1];
	int Com,RightCom,MoveOverCom,HoldCom;
	int Data;
	C4ID id;
	C4Object *Target;
protected:
	C4Region *Next;
public:
	void Set(C4Facet &fctArea, const char *szCaption=NULL, C4Object *pTarget=NULL);
	void Clear();
	void Default();
	void Set(int iX, int iY, int iWdt, int iHgt, const char *szCaption, int iCom, int iMoveOverCom, int iHoldCom, int iData, C4Object *pTarget);
protected:
	void ClearPointers(C4Object *pObj);
};

class C4RegionList
{
public:
	C4RegionList();
	~C4RegionList();
protected:
	int AdjustX,AdjustY;
	C4Region *First;
public:
	void ClearPointers(C4Object *pObj);
	void SetAdjust(int iX, int iY);
	void Clear();
	void Default();
	C4Region* Find(int iX, int iY);
	bool Add(int iX, int iY, int iWdt, int iHgt, const char *szCaption=NULL, int iCom=COM_None, C4Object *pTarget=NULL, int iMoveOverCom=COM_None, int iHoldCom=COM_None, int iData=0);
	bool Add(C4Facet &fctArea, const char *szCaption=NULL, int iCom=COM_None, C4Object *pTarget=NULL, int iMoveOverCom=COM_None, int iHoldCom=COM_None, int iData=0);
	bool Add(C4Region &rRegion);
	bool Add(C4RegionList &rRegionList, bool fAdjust=true);
};

#endif
