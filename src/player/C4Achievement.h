/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2017, The OpenClonk Team and contributors
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

#ifndef INC_C4Achievement
#define INC_C4Achievement

// Maps IDs to achievement graphics to be shown beside scenarios (and maybe other things)
class C4AchievementGraphics
{
	std::map<StdCopyStrBuf, C4FacetSurface *> Graphics;
	int32_t idGrp; // ID of group file from which achievements were loaded

public:
	C4AchievementGraphics() : idGrp(0) {}

	// Init will always  load all achievement files from the first group that contains achievements
	bool Init(C4Group &File);
	bool Init(C4GroupSet &Files);
	void Clear();

	C4FacetSurface *FindByName(const char *name) const;
};

#endif // INC_C4Achievement
