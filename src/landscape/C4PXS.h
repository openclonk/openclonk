/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005  Sven Eberhardt
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

/* Pixel Sprite system for tiny bits of moving material */

#ifndef INC_C4PXS
#define INC_C4PXS

#include <C4Material.h>

class C4PXS
{
	C4PXS(): Mat(MNone), x(Fix0), y(Fix0), xdir(Fix0), ydir(Fix0) {}
	friend class C4PXSSystem;
protected:
	int32_t Mat;
	C4Real x,y,xdir,ydir;
protected:
	void Execute();
	void Deactivate();
};

const size_t PXSChunkSize=500,PXSMaxChunk=20;

class C4PXSSystem
{
public:
	C4PXSSystem();
	~C4PXSSystem();
public:
	int32_t Count;
protected:
	C4PXS *Chunk[PXSMaxChunk];
	size_t iChunkPXS[PXSMaxChunk];
public:
	void Delete(C4PXS *pPXS);
	void Default();
	void Clear();
	void Execute();
	void Draw(C4TargetFacet &cgo);
	void Synchronize();
	void SyncClearance();
	void Cast(int32_t mat, int32_t num, int32_t tx, int32_t ty, int32_t level);
	bool Create(int32_t mat, C4Real ix, C4Real iy, C4Real ixdir=Fix0, C4Real iydir=Fix0);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
protected:
	C4PXS *New();
};

extern C4PXSSystem PXS;
#endif
