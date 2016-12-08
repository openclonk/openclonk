/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

// object list sectors

#ifndef INC_C4Sector
#define INC_C4Sector

#include "object/C4ObjectList.h"
#include "lib/C4Rect.h"

// class predefs
class C4LSector;
class C4LSectors;
class C4LArea;

// constants
const int32_t C4LSectorWdt = 50,
                             C4LSectorHgt = 50;

// one of those object list sectors
class C4LSector
{
public:
	C4LSector() { } // constructor
	~C4LSector() { Clear(); } // destructor

protected:
	void Init(int ix, int iy);
	void Clear();

public:
	int x, y; // pos

	C4ObjectList Objects; // objects within this sector
	C4ObjectList ObjectShapes; // objects with shapes that overlap this sector

	void CompileFunc(StdCompiler *pComp, C4ValueNumbers * numbers);
	void ClearObjects(); // remove all objects from object lists

	friend class C4LSectors;
};

// the whole map
class C4LSectors
{
public:
	C4LSector *Sectors; // mem holding the sector array
	int PxWdt, PxHgt; // size in px
	int Wdt, Hgt, Size; // sector count

	C4LSector SectorOut; // the sector "outside"

public:
	void Init(int Wdt, int Hgt); // init map sectors
	void Clear(); // free map sectors
	C4LSector *SectorAt(int ix, int iy); // get sector at pos

	void Add(C4Object *pObj, C4ObjectList *pMainList);
	void Update(C4Object *pObj, C4ObjectList *pMainList); // does not update object order!
	void Remove(C4Object *pObj);
	void ClearObjects(); // remove all objects from object lists

	void AssertObjectNotInList(C4Object *pObj); // searches all sector lists for object, and assert if it's inside a list

	int getShapeSum() const;

	void Dump();
	bool CheckSort();
};

// a defined sector-area within the map
class C4LArea
{
public:
	C4LSector *pFirst;
	int xL, yL, dpitch; // bounds / delta-pitch
	C4LSector *pOut; // outside?

	C4LArea() { Clear(); } // default constructor
	C4LArea(C4LSectors *pSectors, int ix, int iy, int iwdt, int ihgt) // initializing constructor
	{ Set(pSectors, C4Rect(ix, iy, iwdt, ihgt)); }
	C4LArea(C4LSectors *pSectors, const C4Rect &rect) // initializing constructor
	{ Set(pSectors, rect); }

	C4LArea(C4LSectors *pSectors, C4Object *pObj) // initializing constructor
	{ Set(pSectors, pObj); }

	inline void Clear() { pFirst=pOut=nullptr; } // zero sector

	bool operator == (const C4LArea &Area) const;

	bool IsNull() const { return !pFirst; }

	void Set(C4LSectors *pSectors, const C4Rect &rect); // set rect, calc bounds and get pitch
	void Set(C4LSectors *pSectors, C4Object *pObj); // set to object facet rect

	C4LSector *First() const { return pFirst; } // get first sector
	C4LSector *Next(C4LSector *pPrev) const; // get next sector within area

	// void MoveObject(C4Object *pObj, const C4LArea &toArea); // store object into new area

	bool Contains(C4LSector *pSct) const; // return whether sector is contained in area

	inline C4ObjectList *FirstObjects(C4LSector **ppSct) // get first object list of this area
	{ *ppSct=nullptr; return NextObjects(nullptr, ppSct); }
	C4ObjectList *NextObjects(C4ObjectList *pPrev, C4LSector **ppSct); // get next object list of this area

	inline C4ObjectList *FirstObjectShapes(C4LSector **ppSct) // get first object shapes list of this area
	{ *ppSct=nullptr; return NextObjectShapes(nullptr, ppSct); }
	C4ObjectList *NextObjectShapes(C4ObjectList *pPrev, C4LSector **ppSct); // get next object shapes list of this area

	void DebugRec(class C4Object *pObj, char cMarker);
};

#endif
