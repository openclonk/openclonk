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

#include <C4Include.h>
#include <C4Reloc.h>

#include <C4Config.h>

C4Reloc Reloc; // singleton

void C4Reloc::Init()
{
	Paths.clear();

	// TODO: On Linux, we might not always want to have ExePath (= working directory)
	// here. It's handy for development so that we can easily run the engine in planet/
	// but for distribution it might make sense to disable it.
	// TODO: We might also want to add ExePath/planet if it exists, so that we don't
	// need to run the engine in planet/.
	AddPath(Config.General.ExePath.getData());
	AddPath(Config.General.UserDataPath);
	AddPath(Config.General.SystemDataPath);
}

bool C4Reloc::AddPath(const char* path)
{
	if(!IsGlobalPath(path))
		return false;

	if(std::find(Paths.begin(), Paths.end(), path) != Paths.end())
		return false;

	Paths.push_back(StdCopyStrBuf(path));
	return true;
}

C4Reloc::iterator C4Reloc::begin() const
{
	return Paths.begin();
}

C4Reloc::iterator C4Reloc::end() const
{
	return Paths.end();
}

bool C4Reloc::Open(C4Group& hGroup, const char* filename) const
{
	if(IsGlobalPath(filename)) return hGroup.Open(filename);

	for(iterator iter = begin(); iter != end(); ++iter)
		if(hGroup.Open((*iter + DirSep + filename).getData()))
			return true;

	return false;
}

bool C4Reloc::LocateItem(const char* filename, StdStrBuf& str) const
{
	if(IsGlobalPath(filename))
	{
		str.Copy(filename);
		return true;
	}

	for(iterator iter = begin(); iter != end(); ++iter)
	{
		str.Copy(*iter + DirSep + filename);
		if(ItemExists(str.getData()))
			return true;
	}

	return false;
}
