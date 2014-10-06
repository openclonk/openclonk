/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
#include <C4Scenario.h>
#include <C4Components.h>
#include <C4ScriptHost.h>

// scenario sections

const char *C4ScenSect_Main = "main";

C4ScenarioSection::C4ScenarioSection(char *szName)
{
	// copy name
	if (szName && !SEqualNoCase(szName, C4ScenSect_Main) && *szName)
	{
		this->szName = new char[strlen(szName)+1];
		SCopy(szName, this->szName);
	}
	else
		this->szName = const_cast<char *>(C4ScenSect_Main);
	// zero fields
	szTempFilename = szFilename = 0;
	fModified = false;
	pObjectScripts = NULL;
	// link into main list
	pNext = Game.pScenarioSections;
	Game.pScenarioSections = this;
}

C4ScenarioSection::~C4ScenarioSection()
{
	// del following scenario sections
	while (pNext)
	{
		C4ScenarioSection *pDel = pNext;
		pNext = pNext->pNext;
		pDel->pNext = NULL;
		delete pDel;
	}
	// del temp file
	if (szTempFilename)
	{
		EraseItem(szTempFilename);
		delete szTempFilename;
	}
	// del filename if assigned
	if (szFilename) delete szFilename;
	// del name if owned
	if (szName != C4ScenSect_Main) delete szName;
}

bool C4ScenarioSection::ScenarioLoad(C4Group &rGrp, char *szFilename)
{
	// safety
	if (this->szFilename || !szFilename) return false;
	// store name
	this->szFilename = new char[strlen(szFilename)+1];
	SCopy(szFilename, this->szFilename, _MAX_FNAME);
	// extract if it's not an open folder
	if (Game.ScenarioFile.IsPacked()) if (!EnsureTempStore(true, true)) return false;
	// load section object script
	if (!SEqualNoCase(szName, C4ScenSect_Main))
	{
		C4Group GrpSect, *pGrp;
		if ((pGrp = GetGroupfile(GrpSect)) && pGrp->FindEntry(C4CFN_ScenarioObjectsScript))
		{
			pObjectScripts = new C4ScenarioObjectsScriptHost();
			pObjectScripts->Reg2List(&::ScriptEngine);
			pObjectScripts->Load(*pGrp, C4CFN_ScenarioObjectsScript, Config.General.LanguageEx, &::Game.ScenarioLangStringTable);
		}
		else
		{
			pObjectScripts = NULL;
		}
	}
	else
	{
		// Object script for main section
		pObjectScripts = ::Game.pScenarioObjectsScript;
	}
	// donce, success
	return true;
}

C4Group *C4ScenarioSection::GetGroupfile(C4Group &rGrp)
{
	// check temp filename
	if (szTempFilename)
	{
		if (rGrp.Open(szTempFilename)) return &rGrp;
		else return NULL;
	}
	// check filename within scenario
	if (szFilename)
	{
		if (rGrp.OpenAsChild(&Game.ScenarioFile, szFilename)) return &rGrp;
		else return NULL;
	}
	// unmodified main section: return main group
	if (SEqualNoCase(szName, C4ScenSect_Main)) return &Game.ScenarioFile;
	// failure
	return NULL;
}

bool C4ScenarioSection::EnsureTempStore(bool fExtractLandscape, bool fExtractObjects)
{
	// if it's temp store already, don't do anything
	if (szTempFilename) return true;
	// make temp filename
	char *szTmp = const_cast<char *>(Config.AtTempPath(szFilename ? GetFilename(szFilename) : szName));
	MakeTempFilename(szTmp);
	// main section: extract section files from main scenario group (create group as open dir)
	if (!szFilename)
	{
		if (!CreatePath(szTmp)) return false;
		C4Group hGroup;
		if (!hGroup.Open(szTmp, true)) { EraseItem(szTmp); return false; }
		// extract all desired section files
		Game.ScenarioFile.ResetSearch();
		char fn[_MAX_FNAME+1]; *fn=0;
		while (Game.ScenarioFile.FindNextEntry(C4FLS_Section, fn))
			if (fExtractLandscape || !WildcardMatch(C4FLS_SectionLandscape, fn))
				if (fExtractObjects || !WildcardMatch(C4FLS_SectionObjects, fn))
					Game.ScenarioFile.ExtractEntry(fn, szTmp);
		hGroup.Close();
	}
	else
	{
		// subsection: simply extract section from main group
		if (!Game.ScenarioFile.ExtractEntry(szFilename, szTmp)) return false;
		// delete undesired landscape/object files
		if (!fExtractLandscape || !fExtractObjects)
		{
			C4Group hGroup;
			if (hGroup.Open(szFilename))
			{
				if (!fExtractLandscape) hGroup.Delete(C4FLS_SectionLandscape);
				if (!fExtractObjects) hGroup.Delete(C4FLS_SectionObjects);
			}
		}
	}
	// copy temp filename
	szTempFilename = new char[strlen(szTmp)+1];
	SCopy(szTmp, szTempFilename, _MAX_PATH);
	// done, success
	return true;
}

