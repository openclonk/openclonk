/*
 * OpenClonk, http://www.openclonk.org
 *
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
// user-customizable multimedia package Extra.ocg

#ifndef INC_C4Extra
#define INC_C4Extra

#include <C4Group.h>

class C4Extra
{
public:
	C4Extra();
	~C4Extra();

	bool Init();      // init extra group, using scneario presets
	bool InitGroup(); // open extra group

	std::vector<std::unique_ptr<C4Group>> ExtraGroups; // extra.ocg root folders

protected:
	bool LoadDef(C4Group &hGroup, const char *szName); // load preset for definition
};

#endif
