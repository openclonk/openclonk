/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include <C4Reloc.h>

#include <C4Config.h>
#include <C4Application.h>

C4Reloc Reloc; // singleton

void C4Reloc::Init()
{
	Paths.clear();

	// The system folder (i.e. installation path) has higher priority than the user path
	// Although this is counter-intuitive (the user may want to overload system files in the user path),
	// people had trouble when they downloaded e.g. an Objects.ocd file in a network lobby and that copy permanently
	// ruined their OpenClonk installation with no obvious way to fix it.
	// Not even reinstalling would fix the problem because reinstallation does not overwrite user data.
	// We currently don't have any valid case where overloading system files would make sense so just give higher priority to the system path for now.
#ifndef __APPLE__
	// Add planet subfolder with highest priority because it's used when starting directly from the repository with binaries in the root folder
	StdCopyStrBuf planet(Config.General.ExePath);
	planet.AppendBackslash();
	planet.Append("planet");
	AddPath(planet.getData());
#endif
	// Add main system path
	AddPath(Config.General.SystemDataPath);
	// Add user path for additional data (player files, user scenarios, etc.)
	AddPath(Config.General.UserDataPath, PATH_PreferredInstallationLocation);
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
