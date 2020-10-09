/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "config/C4Reloc.h"

#include "game/C4Application.h"

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
	if (DirectoryExists(planet.getData()))
	{
		// Only add planet if it's a valid contents folder.
		// Because users may create a folder "planet" in their source repos.
		StdCopyStrBuf planet_system_check(planet);
		planet_system_check.AppendBackslash();
		planet_system_check.Append(C4CFN_System);
		if (ItemExists(planet_system_check.getData()))
		{
			AddPath(planet.getData());
		}
	}
#endif
	// Add main system path (unless it's using planet/ anyway, in which we would just slow down scenario enumeration by looking throug hthe whole source folder)
	if (!Paths.size())
	{
		AddPath(Config.General.SystemDataPath);
	}
	// Add mods directory before the user path.
	AddPath(Config.General.ModsDataPath, PATH_IncludingSubdirectories);
	// Add user path for additional data (player files, user scenarios, etc.)
	AddPath(Config.General.UserDataPath, PATH_PreferredInstallationLocation);
}

bool C4Reloc::AddPath(const char* path, PathType pathType)
{
	if (!IsGlobalPath(path))
	{
		return false;
	}

	if (std::find(Paths.begin(), Paths.end(), path) != Paths.end())
	{
		return false;
	}

	Paths.emplace_back(StdCopyStrBuf(path), pathType);
	return true;
}


C4Reloc::const_iterator& C4Reloc::const_iterator::operator++()
{
	if (subdirIters.empty())
	{
		if ((*pathListIter).pathType == C4Reloc::PathType::PATH_IncludingSubdirectories)
		{
			DirectoryIterator subdir((*pathListIter).strBuf.getData());
			if (*subdir)
				subdirIters.emplace(subdir);
			else
				++pathListIter;
		}
		else
		{
			++pathListIter;
		}
	}
	else // Currently in a subdir?
	{
		DirectoryIterator &currentSubdir = subdirIters.top();

		if ((!*currentSubdir) || !*(++currentSubdir))
		{
			subdirIters.pop();
			if (subdirIters.empty())
				++pathListIter;
		}
		else
		{
			// Go deeper?
			if (DirectoryExists(*currentSubdir))
			{
				DirectoryIterator subdir(*currentSubdir);
				if (*subdir) // Make sure there is at least one file/subdir.
					subdirIters.emplace(subdir);
			}
		}
	}
	return *this;
}
const C4Reloc::PathInfo & C4Reloc::const_iterator::operator*() const
{
	if (!subdirIters.empty())
	{
		const DirectoryIterator &currentSubdir = subdirIters.top();
		temporaryPathInfo.reset(new C4Reloc::PathInfo(StdStrBuf(*currentSubdir), PathType::PATH_Regular));
		return *temporaryPathInfo;
	}
	return *pathListIter;
}

bool operator==(const C4Reloc::const_iterator& a, const C4Reloc::const_iterator& b)
{
	if (!a.subdirIters.empty()) return false;
	if (!b.subdirIters.empty()) return false;
	return a.pathListIter == b.pathListIter;
}

bool operator!=(const C4Reloc::const_iterator& a, const C4Reloc::const_iterator& b)
{
	return !(a == b);
}

C4Reloc::iterator C4Reloc::begin() const
{
	C4Reloc::iterator iter;
	iter.pathListIter = Paths.begin();
	return std::move(iter);
}

C4Reloc::iterator C4Reloc::end() const
{
	C4Reloc::iterator iter;
	iter.pathListIter = Paths.end();
	return std::move(iter);
}

bool C4Reloc::Open(C4Group& group, const char* filename) const
{
	if (IsGlobalPath(filename))
	{
		return group.Open(filename);
	}

	for (const auto & iter : *this)
	{
		if (group.Open((iter.strBuf + DirSep + filename).getData()))
		{
			return true;
		}
	}

	return false;
}

bool C4Reloc::LocateItem(const char* filename, StdStrBuf& str) const
{
	if (IsGlobalPath(filename))
	{
		str.Copy(filename);
		return true;
	}

	for(const auto & iter : *this)
	{
		str.Copy(iter.strBuf + DirSep + filename);
		if (ItemExists(str.getData()))
		{
			return true;
		}
	}

	return false;
}
