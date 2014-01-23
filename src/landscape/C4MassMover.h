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

/* Move liquids in the landscape using individual transport spots */

#ifndef INC_C4MassMover
#define INC_C4MassMover

const int32_t C4MassMoverChunk = 10000;

class C4MassMover
{
	friend class C4MassMoverSet;
protected:
	int32_t Mat,x,y;
protected:
	void Cease();
	bool Execute();
	bool Init(int32_t tx, int32_t ty);
	bool Corrosion(int32_t dx, int32_t dy);
};

class C4MassMoverSet
{
public:
	C4MassMoverSet();
	~C4MassMoverSet();
public:
	int32_t Count;
	int32_t CreatePtr;
protected:
	C4MassMover Set[C4MassMoverChunk];
public:
	void Copy(C4MassMoverSet &rSet);
	void Synchronize();
	void Default();
	void Clear();
	void Draw();
	void Execute();
	bool Create(int32_t x, int32_t y, bool fExecute=false);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
protected:
	void Consolidate();
};

extern C4MassMoverSet MassMover;

#endif
