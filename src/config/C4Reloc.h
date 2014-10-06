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

#ifndef C4RELOC_H
#define C4RELOC_H

#include <vector>

class C4Reloc
{
public:
	enum PathType
	{
		PATH_Regular,
		PATH_PreferredInstallationLocation
	};
	struct PathInfo
	{
		StdCopyStrBuf strBuf;
		PathType pathType;
		PathInfo(const StdCopyStrBuf buf, PathType pathType): strBuf(buf), pathType(pathType) {}
		bool operator==(const PathInfo&other) {return pathType==other.pathType && strBuf==other.strBuf;}
		operator const char*() {return strBuf.getData();}
	};
	typedef std::vector<PathInfo> PathList;
	typedef PathList::const_iterator iterator;

	// Can also be used for re-init, drops custom paths added with AddPath.
	// Make sure to call after Config.Load.
	void Init();

	bool AddPath(const char* path, PathType pathType = PATH_Regular);

	iterator begin() const;
	iterator end() const;

	bool Open(C4Group& hGroup, const char* filename) const;
	bool LocateItem(const char* filename, StdStrBuf& str) const;
private:
	PathList Paths;
};

extern C4Reloc Reloc;

#endif // C4RELOC_H
