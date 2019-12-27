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

/* Holds crew member information */

#ifndef INC_C4ObjectInfo
#define INC_C4ObjectInfo

#include "graphics/C4FacetEx.h"
#include "graphics/C4Surface.h"
#include "object/C4InfoCore.h"
#include "object/C4Object.h"

class C4ObjectInfo: public C4ObjectInfoCore
{
public:
	C4ObjectInfo();
	~C4ObjectInfo();
public:
	bool WasInAction;
	bool InAction;
	int32_t InActionTime;
	bool HasDied;
	int32_t ControlCount;
	class C4Def *pDef; // definition to ID - only resolved if defs were loaded at object info loading time
	char Filename[_MAX_PATH_LEN];
	C4ObjectInfo *Next;
public:
	void Default();
	void Clear();
	void Evaluate();
	void Retire();
	void Recruit();
	void SetBirthday();
	bool Save(C4Group &hGroup, bool fStoreTiny, C4DefList *pDefs);
	bool Load(C4Group &hGroup);
	bool Load(C4Group &hMother, const char *szEntryname);
};

#endif
