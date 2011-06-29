/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
 * Copyright (c) 2009  Günther Brammer
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

/* Special regions to extend the pathfinder */

#ifndef INC_C4TransferZone
#define INC_C4TransferZone

class C4Object;
class C4TargetFacet;
class C4TransferZones;

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
	void Default();
	void Clear();
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
