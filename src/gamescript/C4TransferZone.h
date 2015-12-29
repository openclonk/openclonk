/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#ifndef INC_C4TransferZone
#define INC_C4TransferZone

class C4TransferZone
{
	friend class C4TransferZones;
public:
	C4TransferZone();
	~C4TransferZone();
public:
	C4Object *Object;
	int32_t X,Y,Wdt,Hgt;
	bool Used;
protected:
	C4TransferZone *Next;
public:
	bool GetEntryPoint(int32_t &rX, int32_t &rY, int32_t iToX, int32_t iToY);
	void Draw(C4TargetFacet &cgo, bool fHighlight=false);
	bool At(int32_t iX, int32_t iY);
};

class C4TransferZones
{
public:
	C4TransferZones();
	~C4TransferZones();
protected:
	int32_t RemoveNullZones();
	C4TransferZone *First;
public:
	void Default();
	void Clear();
	void ClearUsed();
	void ClearPointers(C4Object *pObj);
	void Draw(C4TargetFacet &cgo);
	void Synchronize();
	C4TransferZone* Find(C4Object *pObj);
	C4TransferZone* Find(int32_t iX, int32_t iY);
	bool Add(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj);
	bool Set(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4Object *pObj);
};

#endif
