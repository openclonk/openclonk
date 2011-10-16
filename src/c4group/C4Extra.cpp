/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2011  Armin Burgmeier
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
// user-customizable multimedia package Extra.ocg

#include <C4Include.h>
#include <C4Extra.h>

#include <C4Config.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4Log.h>

void C4Extra::Default()
{
	// zero fields
}

void C4Extra::Clear()
{
	// free class members
	for(unsigned int i = 0; i < ExtraGroups.size(); ++i)
		delete ExtraGroups[i];
	ExtraGroups.clear();
}

bool C4Extra::InitGroup()
{
	// register extra root into game group set
	for(C4Reloc::iterator iter = Reloc.begin(); iter != Reloc.end(); ++iter)
	{
		std::auto_ptr<C4Group> pGroup(new C4Group);
		if(pGroup->Open( ((*iter).strBuf + DirSep + C4CFN_Extra).getData()))
			ExtraGroups.push_back(pGroup.release());
	}

	// done, success
	return true;
}

bool C4Extra::Init()
{
	// no group: OK
	if (ExtraGroups.empty()) return true;
	// load from all definitions that are activated
	// add first definition first, so the priority will be lowest
	// (according to definition load/overload order)
	char szSegment[_MAX_PATH+1];
	for (int cseg=0; SCopySegment(Game.DefinitionFilenames,cseg,szSegment,';',_MAX_PATH); cseg++)
	{
		for(unsigned int i = 0; i < ExtraGroups.size(); ++i)
		{
			if(LoadDef(*ExtraGroups[i], GetFilename(szSegment)))
			{
				break;
			}
		}
	}
	// done, success
	return true;
}

bool C4Extra::LoadDef(C4Group &hGroup, const char *szName)
{
	// check if file exists
	if (!hGroup.FindEntry(szName)) return false;
	// log that extra group is loaded
	LogF(LoadResStr("IDS_PRC_LOADEXTRA"), hGroup.GetName(), szName);
	// open and add group to set
	C4Group *pGrp = new C4Group;
	if (!pGrp->OpenAsChild(&hGroup, szName)) { Log(LoadResStr("IDS_ERR_FAILURE")); delete pGrp; return false; }
	Game.GroupSet.RegisterGroup(*pGrp, true, C4GSPrio_Extra, C4GSCnt_Extra);
	// done, success
	return true;
}
