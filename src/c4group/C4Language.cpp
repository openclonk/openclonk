/*
 * OpenClonk, http://www.openclonk.org
 *
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

/*
   Language module
   - handles external language packs
   - provides info on selectable languages by scanning string tables
   - loads and sets a language string table (ResStrTable) based on a specified language sequence

*/

#include "C4Include.h"
#include "c4group/C4Language.h"

#include "game/C4Application.h"
#include "c4group/C4Components.h"

template<size_t iBufferSize>
static bool GetRelativePath(const char *strPath, const char *strRelativeTo, char(&strBuffer)[iBufferSize])
{
	// Specified path is relative to base path
	// Copy relative section
	const char *szCpy;
	SCopy(szCpy = GetRelativePathS(strPath, strRelativeTo), strBuffer, iBufferSize);
	// return whether it was made relative
	return szCpy != strPath;
}

C4Language Languages;

C4Language::C4Language()
{
	Infos = nullptr;
	PackGroupLocation[0] = 0;
}

C4Language::~C4Language()
{
	Clear();
}

bool C4Language::Init()
{
	// Clear (to allow clean re-init)
	Clear();

	// Make sure Language.ocg is unpacked (TODO: This won't work properly if Language.ocg is in system data path)
	// Assume for now that Language.ocg is either at a writable location or unpacked already.
	// TODO: Use all Language.c4gs that we find, and merge them.
	// TODO: Use gettext instead?
	StdStrBuf langPath;
	C4Reloc::iterator iter;
	for(iter = Reloc.begin(); iter != Reloc.end(); ++iter)
	{
		langPath.Copy((*iter).strBuf + DirSep + C4CFN_Languages);
		if(ItemExists(langPath.getData()))
		{
			if(DirectoryExists(langPath.getData()))
				break;
			if(C4Group_UnpackDirectory(langPath.getData()))
				break;
		}
	}

	// Break if no language.ocg found
	if(iter != Reloc.end())
	{
		// Look for available language packs in Language.ocg
		C4Group *pPack;
		char strPackFilename[_MAX_FNAME_LEN], strEntry[_MAX_FNAME_LEN];
		if (PackDirectory.Open(langPath.getData()))
		{
			while (PackDirectory.FindNextEntry("*.ocg", strEntry))
			{
				sprintf(strPackFilename, "%s" DirSep "%s", C4CFN_Languages, strEntry);
				pPack = new C4Group();
				if (pPack->Open(strPackFilename))
				{
					Packs.RegisterGroup(*pPack, true, C4GSCnt_Language, false);
				}
				else
				{
					delete pPack;
				}
			}
		}

		// Now create a pack group for each language pack (these pack groups are child groups
		// that browse along each pack to access requested data)
		for (int iPack = 0; (pPack = Packs.GetGroup(iPack)); iPack++)
			PackGroups.RegisterGroup(*(new C4Group), true, C4GSPrio_Base, C4GSCnt_Language);
	}

	// Load language infos by scanning string tables (the engine doesn't really need this at the moment)
	InitInfos();

	// Done
	return true;
}

void C4Language::Clear()
{
	// Clear pack groups
	PackGroups.Clear();
	// Clear packs
	Packs.Clear();
	// Close pack directory
	PackDirectory.Close();
	// Clear infos
	C4LanguageInfo* pNext;
	while (Infos)
	{
		pNext = Infos->Next;
		delete Infos;
		Infos = pNext;
	}
	Infos = nullptr;
}

int C4Language::GetPackCount()
{
	return Packs.GetGroupCount();
}

int C4Language::GetInfoCount()
{
	int iCount = 0;
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		iCount++;
	return iCount;
}

// Returns a set of groups at the specified relative path within all open language packs.

C4GroupSet C4Language::GetPackGroups(C4Group & hGroup)
{
	// Build a group set containing the provided group and
	// alternative groups for cross-loading from a language pack
	char strRelativePath[_MAX_PATH_LEN];
	char strTargetLocation[_MAX_PATH_LEN];
	char strPackPath[_MAX_PATH_LEN];
	char strPackGroupLocation[_MAX_PATH_LEN];
	char strAdvance[_MAX_PATH_LEN];

	// Store wanted target location
	SCopy(Config.AtRelativePath(hGroup.GetFullName().getData()), strRelativePath, _MAX_PATH);
	SCopy(strRelativePath, strTargetLocation, _MAX_PATH);

	// Adjust location by scenario origin
	if (Game.C4S.Head.Origin.getLength() && SEqualNoCase(GetExtension(Game.C4S.Head.Origin.getData()), "ocs"))
	{
		const char *szScenarioRelativePath = GetRelativePathS(strRelativePath, Config.AtRelativePath(Game.ScenarioFilename));
		if (szScenarioRelativePath != strRelativePath)
		{
			// this is a path within the scenario! Change to origin.
			size_t iRestPathLen = SLen(szScenarioRelativePath);
			if (Game.C4S.Head.Origin.getLength() + 1 + iRestPathLen <= _MAX_PATH)
			{
				SCopy(Game.C4S.Head.Origin.getData(), strTargetLocation);
				if (iRestPathLen)
				{
					SAppendChar(DirectorySeparator, strTargetLocation);
					SAppend(szScenarioRelativePath, strTargetLocation);
				}
			}
		}
	}

	// Process all language packs (and their respective pack groups)
	C4Group *pPack, *pPackGroup;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)) && (pPackGroup = PackGroups.GetGroup(iPack)); iPack++)
	{
		// Get current pack group position within pack
		SCopy(pPack->GetFullName().getData(), strPackPath, _MAX_PATH);
		GetRelativePath(pPackGroup->GetFullName().getData(), strPackPath, strPackGroupLocation);

		// Pack group is at correct position within pack: continue with next pack
		if (SEqualNoCase(strPackGroupLocation, strTargetLocation))
			continue;

		// Try to backtrack until we can reach the target location as a relative child
		while ( strPackGroupLocation[0]
		        && !GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance)
		        && pPackGroup->OpenMother() )
		{
			// Update pack group location
			GetRelativePath(pPackGroup->GetFullName().getData(), strPackPath, strPackGroupLocation);
		}

		// We can reach the target location as a relative child
		if (strPackGroupLocation[0] && GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance))
		{
			// Advance pack group to relative child
			pPackGroup->OpenChild(strAdvance);
		}

		// Cannot reach by advancing: need to close and reopen (rewinding group file)
		else
		{
			// Close pack group (if it is open at all)
			pPackGroup->Close();
			// Reopen pack group to relative position in language pack if possible
			pPackGroup->OpenAsChild(pPack, strTargetLocation);
		}

	}

	// Store new target location
	SCopy(strTargetLocation, PackGroupLocation, _MAX_FNAME);

	C4GroupSet r;
	// Provided group gets highest priority
	r.RegisterGroup(hGroup, false, 1000, C4GSCnt_Component);
	// register currently open pack groups
	r.RegisterGroups(PackGroups, C4GSCnt_Language);
	return r;
}

bool C4Language::LoadComponentHost(C4ComponentHost *host, C4Group &hGroup, const char *szFilename, const char *szLanguage)
{
	assert(host);
	if (!host) return false;
	C4GroupSet hGroups = ::Languages.GetPackGroups(hGroup);
	return host->Load(hGroups, szFilename, szLanguage);
}

void C4Language::InitInfos()
{
	C4Group hGroup;
	// First, look in System.ocg
	if (Reloc.Open(hGroup, C4CFN_System))
	{
		LoadInfos(hGroup);
		hGroup.Close();
	}
	// Now look through the registered packs
	C4Group *pPack;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)); iPack++)
		// Does it contain a System.ocg child group?
		if (hGroup.OpenAsChild(pPack, C4CFN_System))
		{
			LoadInfos(hGroup);
			hGroup.Close();
		}
}

namespace
{
	std::string GetResStr(const char *id, const char *stringtbl)
	{
		// The C++11 standard does not specify whether $ and ^ match
		// the beginning or end of a line, respectively, and it seems
		// like in some implementations they only match the beginning
		// or end of the whole string. See also #1127.
		static std::regex line_pattern("(?:\n|^)([^=]+)=(.*?)\r?(?=\n|$)", static_cast<std::regex::flag_type>(std::regex_constants::optimize | std::regex_constants::ECMAScript));

		assert(stringtbl);
		if (!stringtbl)
		{
			return std::string();
		}

		// Get beginning and end iterators of stringtbl
		const char *begin = stringtbl;
		const char *end = begin + std::char_traits<char>::length(begin);

		for (auto it = std::cregex_iterator(begin, end, line_pattern); it != std::cregex_iterator(); ++it)
		{
			assert(it->size() == 3);
			if (it->size() != 3)
				continue;

			std::string key = (*it)[1];
			if (key != id)
				continue;

			std::string val = (*it)[2];
			return val;
		}

		// If we get here, there was no such string in the string table
		// return the input string so there's at least *something*
		return id;
	}

	template<size_t N>
	void CopyResStr(const char *id, const char *stringtbl, char (&dest)[N])
	{
		std::string value = GetResStr(id, stringtbl);
		std::strncpy(dest, value.c_str(), N);
		dest[N - 1] = '\0';
	}
}

void C4Language::LoadInfos(C4Group &hGroup)
{
	char strEntry[_MAX_FNAME_LEN];
	char *strTable;
	// Look for language string tables
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_Language, strEntry))
		// For now, we will only load info on the first string table found for a given
		// language code as there is currently no handling for selecting different string tables
		// of the same code - the system always loads the first string table found for a given code
		if (!FindInfo(GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2))
			// Load language string table
			if (hGroup.LoadEntry(strEntry, &strTable, nullptr, 1))
			{
				// New language info
				C4LanguageInfo *pInfo = new C4LanguageInfo;
				// Get language code by entry name
				SCopy(GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2, pInfo->Code, 2);
				SCapitalize(pInfo->Code);
				// Get language name, info, fallback from table
				CopyResStr("IDS_LANG_NAME", strTable, pInfo->Name);
				CopyResStr("IDS_LANG_INFO", strTable, pInfo->Info);
				CopyResStr("IDS_LANG_FALLBACK", strTable, pInfo->Fallback);
				// Safety: pipe character is not allowed in any language info string
				SReplaceChar(pInfo->Name, '|', ' ');
				SReplaceChar(pInfo->Info, '|', ' ');
				SReplaceChar(pInfo->Fallback, '|', ' ');
				// Delete table
				delete [] strTable;
				// Add info to list
				pInfo->Next = Infos;
				Infos = pInfo;
			}
}

C4LanguageInfo* C4Language::GetInfo(int iIndex)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (iIndex <= 0)
			return pInfo;
		else
			iIndex--;
	return nullptr;
}

C4LanguageInfo* C4Language::FindInfo(const char *strCode)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (SEqualNoCase(pInfo->Code, strCode, 2))
			return pInfo;
	return nullptr;
}

bool C4Language::LoadLanguage(const char *strLanguages)
{
	// Clear old string table
	ClearLanguage();
	// Try to load string table according to language sequence
	char strLanguageCode[2 + 1];
	for (int i = 0; SCopySegment(strLanguages, i, strLanguageCode, ',', 2, true); i++)
		if (InitStringTable(strLanguageCode))
			return true;
	// No matching string table found: hardcoded fallback to US
	if (InitStringTable("US"))
		return true;
	// No string table present: this is really bad
	Log("Error loading language string table.");
	return false;
}

bool C4Language::InitStringTable(const char *strCode)
{
	C4Group hGroup;
	// First, look in System.ocg
	if (LoadStringTable(Application.SystemGroup, strCode))
		return true;
	// Now look through the registered packs
	C4Group *pPack;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)); iPack++)
		// Does it contain a System.ocg child group?
		if (hGroup.OpenAsChild(pPack, C4CFN_System))
		{
			if (LoadStringTable(hGroup, strCode))
				{ hGroup.Close(); return true; }
			hGroup.Close();
		}
	// No matching string table found
	return false;
}

bool C4Language::LoadStringTable(C4Group &hGroup, const char *strCode)
{
	// Compose entry name
	char strEntry[_MAX_FNAME_LEN];
	sprintf(strEntry, "Language%s.txt", strCode); // ...should use C4CFN_Language here
	// Load string table
	if (!C4LangStringTable::GetSystemStringTable().Load(hGroup, strEntry))
		return false;
	// Success
	return true;
}

void C4Language::ClearLanguage()
{
	// Clear resource string table
	C4LangStringTable::GetSystemStringTable().Clear();
}

// Closes any open language pack that has the specified path.

bool C4Language::CloseGroup(const char *strPath)
{
	// Check all open language packs
	C4Group *pPack;
	for (int iPack = 0; (pPack = Packs.GetGroup(iPack)); iPack++)
		if (ItemIdentical(strPath, pPack->GetFullName().getData()))
		{
			Packs.UnregisterGroup(iPack);
			return true;
		}
	// No pack of that path
	return false;
}
