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
