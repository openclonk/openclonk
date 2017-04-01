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
#include "C4Include.h"
#include "player/C4Achievement.h"
#include "c4group/C4Components.h"
#include "graphics/C4FacetEx.h"


/* C4AchievementGraphics */

bool C4AchievementGraphics::Init(C4Group &File)
{
	// Load all graphics matching achievement filename and register them to map
	char FileName[_MAX_FNAME];
	File.ResetSearch();
	while (File.FindNextEntry(C4CFN_Achievements, FileName))
	{
		C4FacetSurface *new_fct = new C4FacetSurface();
		if (!new_fct->Load(File, FileName, C4FCT_Height, C4FCT_Full, false, 0))
		{
			delete new_fct;
			LogF(LoadResStr("IDS_PRC_NOGFXFILE"), FileName, LoadResStr("IDS_ERR_NOFILE"));
			return false;
		}
		// Register under filename excluding the leading "Achv" part. Delete any existing file with same name.
		RemoveExtension(FileName);
		int32_t id_offset = SCharPos('*', C4CFN_Achievements); assert(id_offset>=0);
		StdCopyStrBuf sFileName(FileName + id_offset);
		auto i = Graphics.find(sFileName);
		if (i != Graphics.end()) delete i->second;
		Graphics[sFileName] = new_fct;
	}
	// done. success no matter how many files were loaded.
	return true;
}

bool C4AchievementGraphics::Init(C4GroupSet &Files)
{
	int32_t idNewGrp=0;
	C4Group *pGrp = Files.FindEntry(C4CFN_Achievements, nullptr, &idNewGrp);
	if (!pGrp) return true; // no achievement gfx. That's OK.
	if (idNewGrp == idGrp) return true; // no update
	idGrp = idNewGrp;
	// OK, load from this group
	return Init(*pGrp);
}

void C4AchievementGraphics::Clear()
{
	for (auto i = Graphics.begin(); i != Graphics.end(); ++i)
		delete i->second;
	Graphics.clear();
	idGrp = 0;
}

C4FacetSurface *C4AchievementGraphics::FindByName(const char *name) const
{
	auto i = Graphics.find(StdCopyStrBuf(name));
	if (i != Graphics.end()) return i->second; else return nullptr;
}
