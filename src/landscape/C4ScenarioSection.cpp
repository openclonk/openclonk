/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "landscape/C4Scenario.h"
#include "c4group/C4Components.h"
#include "script/C4ScriptHost.h"

// scenario sections

const char *C4ScenSect_Main = "main";

C4ScenarioSection::C4ScenarioSection(const char *szName)
{
	// copy name
	name.Copy(szName);
	// zero fields
	fModified = false;
	pObjectScripts = nullptr;
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
		pDel->pNext = nullptr;
		delete pDel;
	}
	// del temp file
	if (temp_filename.getData())
	{
		EraseItem(temp_filename.getData());
	}
}

bool C4ScenarioSection::ScenarioLoad(const char *szFilename, bool is_temp_file)
{
	// safety
	if (filename.getData() || !szFilename) return false;
	// store name
	filename.Copy(szFilename);
	if (is_temp_file)
	{
		// temp: Remember temp filename to be deleted on destruction
		temp_filename.Copy(szFilename);
	}
	else
	{
		// non-temp: extract if it's not an open folder
		if (Game.ScenarioFile.IsPacked()) if (!EnsureTempStore(true, true)) return false;
	}
	// load section object script
	if (!SEqualNoCase(name.getData(), C4ScenSect_Main))
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
			pObjectScripts = nullptr;
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
	if (temp_filename.getData())
	{
		if (rGrp.Open(temp_filename.getData())) return &rGrp;
		else return nullptr;
	}
	// check filename within scenario
	if (filename.getData())
	{
		if (rGrp.OpenAsChild(&Game.ScenarioFile, filename.getData())) return &rGrp;
		else return nullptr;
	}
	// unmodified main section: return main group
	if (SEqualNoCase(name.getData(), C4ScenSect_Main)) return &Game.ScenarioFile;
	// failure
	return nullptr;
}

bool C4ScenarioSection::EnsureTempStore(bool fExtractLandscape, bool fExtractObjects)
{
	// if it's temp store already, don't do anything
	if (temp_filename.getData()) return true;
	// make temp filename
	char *szTmp = const_cast<char *>(Config.AtTempPath(filename.getData() ? GetFilename(filename.getData()) : name.getData()));
	MakeTempFilename(szTmp);
	// main section: extract section files from main scenario group (create group as open dir)
	if (!filename.getData())
	{
		if (!CreatePath(szTmp)) return false;
		C4Group hGroup;
		if (!hGroup.Open(szTmp, true)) { EraseItem(szTmp); return false; }
		// extract all desired section files
		Game.ScenarioFile.ResetSearch();
		char fn[_MAX_FNAME_LEN]; *fn=0;
		while (Game.ScenarioFile.FindNextEntry(C4FLS_Section, fn))
			if (fExtractLandscape || !WildcardMatch(C4FLS_SectionLandscape, fn))
				if (fExtractObjects || !WildcardMatch(C4FLS_SectionObjects, fn))
					Game.ScenarioFile.ExtractEntry(fn, szTmp);
		hGroup.Close();
	}
	else
	{
		// subsection: simply extract section from main group
		if (!Game.ScenarioFile.ExtractEntry(filename.getData(), szTmp)) return false;
		// delete undesired landscape/object files
		if (!fExtractLandscape || !fExtractObjects)
		{
			C4Group hGroup;
			if (hGroup.Open(filename.getData()))
			{
				if (!fExtractLandscape) hGroup.Delete(C4FLS_SectionLandscape);
				if (!fExtractObjects) hGroup.Delete(C4FLS_SectionObjects);
			}
		}
	}
	// copy temp filename
	temp_filename.Copy(szTmp);
	// done, success
	return true;
}

