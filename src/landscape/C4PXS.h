/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Pixel Sprite system for tiny bits of moving material */

#ifndef INC_C4PXS
#define INC_C4PXS

#include "landscape/C4Material.h"

class C4PXS
{
	friend class C4PXSSystem;
protected:
	int32_t Mat{MNone};
	C4Real x{Fix0}, y{Fix0}, xdir{Fix0}, ydir{Fix0};
protected:
	bool Execute();
	void Deactivate();
};

const size_t PXSMax = 10000;

class C4PXSSystem
{
public:
	C4PXSSystem();
	~C4PXSSystem();
public:
	size_t Count;
protected:
	C4PXS PXS[PXSMax];
public:
	void Default();
	void Clear();
	void Execute();
	void Draw(C4TargetFacet &cgo);
	void Cast(int32_t mat, int32_t num, int32_t tx, int32_t ty, int32_t level);
	bool Create(int32_t mat, C4Real ix, C4Real iy, C4Real ixdir=Fix0, C4Real iydir=Fix0);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
	int32_t GetCount() const { return Count; } // count all PXS
	int32_t GetCount(int32_t mat) const; // count PXS of given material
	int32_t GetCount(int32_t mat, int32_t x, int32_t y, int32_t wdt, int32_t hgt) const; // count PXS of given material in given area. mat==-1 for all materials.
protected:
	C4PXS *New();
};

extern C4PXSSystem PXS;
#endif
