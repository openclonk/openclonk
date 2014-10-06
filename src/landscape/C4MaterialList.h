/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* A primitive list to store one amount value per mapped material */

#ifndef INC_C4MaterialList
#define INC_C4MaterialList

#include <C4Landscape.h>

class C4MaterialList
{
public:
	C4MaterialList();
	~C4MaterialList();
public:
	int32_t Amount[C4MaxMaterial];
public:
	void Default();
	void Clear();
	void Reset();
	int32_t Get(int32_t iMaterial);
	void Add(int32_t iMaterial, int32_t iAmount);
	void Set(int32_t iMaterial, int32_t iAmount);
};

#endif // INC_C4MaterialList
