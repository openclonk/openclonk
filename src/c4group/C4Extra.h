/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Sven Eberhardt
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
// user-customizable multimedia package Extra.c4g

#ifndef INC_C4Extra
#define INC_C4Extra

#include <C4Group.h>

class C4Extra
{
public:
	C4Extra() { Default(); };     // ctor
	~C4Extra() { Clear(); };      // dtor
	void Default(); // zero fields
	void Clear();   // free class members

	bool Init();      // init extra group, using scneario presets
	bool InitGroup(); // open extra group

	std::vector<C4Group*> ExtraGroups; // extra.c4g root folders

protected:
	bool LoadDef(C4Group &hGroup, const char *szName); // load preset for definition
};

#endif
