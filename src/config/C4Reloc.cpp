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
#include <C4Application.h>

C4Reloc Reloc; // singleton

void C4Reloc::Init()
{
	Paths.clear();

#ifndef __APPLE__
	StdCopyStrBuf planet(Config.General.ExePath);
	planet.AppendBackslash();
	planet.Append("planet");
	AddPath(planet.getData());
#endif

	AddPath(Config.General.UserDataPath, PATH_PreferredInstallationLocation);
	AddPath(Config.General.SystemDataPath);
}

bool C4Reloc::AddPath(const char* path, PathType pathType)
{
	if(!IsGlobalPath(path))
		return false;

	if(std::find(Paths.begin(), Paths.end(), path) != Paths.end())
		return false;

	Paths.push_back(PathInfo(StdCopyStrBuf(path), pathType));
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
		if(hGroup.Open(((*iter).strBuf + DirSep + filename).getData()))
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
		str.Copy((*iter).strBuf + DirSep + filename);
		if(ItemExists(str.getData()))
			return true;
	}

	return false;
}