/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011 Armin Burgmeier
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

#ifndef C4RELOC_H
#define C4RELOC_H

#include <vector>

class C4Reloc
{
public:
	typedef std::vector<StdCopyStrBuf> PathList;
	typedef PathList::const_iterator iterator;

	// Can also be used for re-init, drops custom paths added with AddPath.
	// Make sure to call after Config.Load.
	void Init();

	bool AddPath(const char* path);

	iterator begin() const;
	iterator end() const;

	bool Open(C4Group& hGroup, const char* filename) const;
	bool LocateItem(const char* filename, StdStrBuf& str) const;
private:
	C4Reloc::PathList Paths;
};

extern C4Reloc Reloc;

#endif // C4RELOC_H
