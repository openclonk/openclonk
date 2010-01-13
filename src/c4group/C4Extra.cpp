/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004  Sven Eberhardt
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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
// user-customizable multimedia package Extra.c4g

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
	ExtraSysGrp.Close();
	ExtraUserGrp.Close();
	}

bool C4Extra::InitGroup()
	{
	// register extra root into game group set
	if (ItemExists(Config.AtSystemDataPath(C4CFN_Extra)) && ExtraSysGrp.Open(Config.AtSystemDataPath(C4CFN_Extra)))
		Game.GroupSet.RegisterGroup(ExtraSysGrp, false, C4GSPrio_ExtraRoot, C4GSCnt_ExtraRoot);
	// Add user Extra.c4g
	if (ItemExists(Config.AtUserDataPath(C4CFN_Extra)) && ExtraUserGrp.Open(Config.AtUserDataPath(C4CFN_Extra)))
		Game.GroupSet.RegisterGroup(ExtraUserGrp, false, C4GSPrio_ExtraRoot, C4GSCnt_ExtraRoot);

	// done, success
	return true;
	}

bool C4Extra::Init()
	{
	// no group: OK
	if (!ExtraSysGrp.IsOpen() && !ExtraUserGrp.IsOpen()) return true;
	// load from all definitions that are activated
	// add first definition first, so the priority will be lowest
	// (according to definition load/overload order)
	char szSegment[_MAX_PATH+1];
	bool fAnythingLoaded=false;
	for (int cseg=0; SCopySegment(Game.DefinitionFilenames,cseg,szSegment,';',_MAX_PATH); cseg++)
		if (LoadDef(ExtraUserGrp,GetFilename(szSegment)) || LoadDef(ExtraSysGrp,GetFilename(szSegment)))
			fAnythingLoaded=true;
	// done, success
	return true;
	}

bool C4Extra::LoadDef(C4Group &hGroup, const char *szName)
	{
	// check if file exists
	if (!hGroup.FindEntry(szName)) return false;
	// log that extra group is loaded
	LogF(LoadResStr("IDS_PRC_LOADEXTRA"), ExtraSysGrp.GetName(), szName);
	// open and add group to set
	C4Group *pGrp = new C4Group;
	if (!pGrp->OpenAsChild(&hGroup, szName)) { Log(LoadResStr("IDS_ERR_FAILURE")); delete pGrp; return false; }
	Game.GroupSet.RegisterGroup(*pGrp, true, C4GSPrio_Extra, C4GSCnt_Extra);
	// done, success
	return true;
	}
