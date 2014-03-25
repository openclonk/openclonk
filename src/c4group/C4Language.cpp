/*
 * OpenClonk, http://www.openclonk.org
 *
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

/*
   Language module
   - handles external language packs
   - provides info on selectable languages by scanning string tables
   - loads and sets a language string table (ResStrTable) based on a specified language sequence

*/

#include <C4Include.h>
#include <C4Language.h>

#include <C4Application.h>
#include <C4Components.h>
#include <C4Log.h>
#include <C4Config.h>
#include <C4Game.h>

#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#include <errno.h>
iconv_t C4Language::host_to_local = iconv_t(-1);
iconv_t C4Language::local_to_host = iconv_t(-1);
#endif

C4Language Languages;

//char strLog[2048 + 1];

C4Language::C4Language()
{
	Infos = NULL;
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

	// Look for available language packs in Language.ocg          Opening Language.ocg as a group and
	/*C4Group *pPack;                                             the packs as children is no good -
	char strPackFilename[_MAX_FNAME + 1];                         C4Group simply cannot handle it. So
	Log("Registering languages...");                              we need to open the pack group files
	if (PackDirectory.Open(C4CFN_Languages))                      directly...
	  while (PackDirectory.FindNextEntry("*.ocg", strPackFilename))
	  {
	    pPack = new C4Group();
	    if (pPack->OpenAsChild(&PackDirectory, strPackFilename))
	    {
	      sprintf(strLog, "  %s...", strPackFilename); Log(strLog);
	      Packs.RegisterGroup(*pPack, true, C4GSCnt_Language, false);
	    }
	    else
	    {
	      sprintf(strLog, "Could not open language pack %s...", strPackFilename); Log(strLog);
	      delete pPack;
	    }
	  }*/

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
		char strPackFilename[_MAX_FNAME + 1], strEntry[_MAX_FNAME + 1];
		//Log("Registering languages...");
		if (PackDirectory.Open(langPath.getData()))
		{
			while (PackDirectory.FindNextEntry("*.ocg", strEntry))
			{
				sprintf(strPackFilename, "%s" DirSep "%s", C4CFN_Languages, strEntry);
				pPack = new C4Group();
				if (pPack->Open(strPackFilename))
				{
					//sprintf(strLog, "  %s...", strPackFilename); Log(strLog);
					Packs.RegisterGroup(*pPack, true, C4GSCnt_Language, false);
				}
				else
				{
					//sprintf(strLog, "Could not open language pack %s...", strPackFilename); Log(strLog);
					delete pPack;
				}
			}
		}

		// Log
		//sprintf(strLog, "%d external language packs registered.", GetPackCount()); Log(strLog);

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
	Infos = NULL;
#ifdef HAVE_ICONV
	if (local_to_host != iconv_t(-1))
	{
		iconv_close(local_to_host);
		local_to_host = iconv_t(-1);
	}
	if (host_to_local != iconv_t(-1))
	{
		iconv_close(host_to_local);
		host_to_local = iconv_t(-1);
	}
#endif
}

#ifdef HAVE_ICONV
StdStrBuf C4Language::Iconv(const char * string, iconv_t cd)
{
	if (cd == iconv_t(-1))
	{
		return StdStrBuf(string, true);
	}
	StdStrBuf r;
	if (!string) return r;
	size_t inlen = strlen(string);
	size_t outlen = strlen(string);
	r.SetLength(inlen);
	const char * inbuf = string;
	char * outbuf = r.getMData();
	while (inlen > 0)
	{
		// Hope that iconv does not change the inbuf...
		if ((size_t)(-1) == iconv(cd, const_cast<ICONV_CONST char * *>(&inbuf), &inlen, &outbuf, &outlen))
		{
			switch (errno)
			{
				// There is not sufficient room at *outbuf.
			case E2BIG:
			{
				size_t done = outbuf - r.getMData();
				r.Grow(inlen * 2);
				outbuf = r.getMData() + done;
				outlen += inlen * 2;
				break;
			}
			// An invalid multibyte sequence has been encountered in the input.
			case EILSEQ:
				++inbuf;
				--inlen;
				break;
				// An incomplete multibyte sequence has been encountered in the input.
			case EINVAL:
			default:
				if (outlen) r.Shrink(outlen);
				return r;
			}
		}
	}
	if (outlen) r.Shrink(outlen);
	// StdStrBuf has taken care of the terminating zero
	return r;
}
StdStrBuf C4Language::IconvSystem(const char * string)
{
	return Iconv(string, local_to_host);
}
StdStrBuf C4Language::IconvClonk(const char * string)
{
	return Iconv(string, host_to_local);
}
#else
StdStrBuf C4Language::IconvSystem(const char * string)
{
	// Just copy through
	return StdStrBuf(string, true);
}
StdStrBuf C4Language::IconvClonk(const char * string)
{
	// Just copy through
	return StdStrBuf(string, true);
}
#endif

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
	char strRelativePath[_MAX_PATH + 1];
	char strTargetLocation[_MAX_PATH + 1];
	char strPackPath[_MAX_PATH + 1];
	char strPackGroupLocation[_MAX_PATH + 1];
	char strAdvance[_MAX_PATH + 1];

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

	//if (SEqualNoCase(strTargetLocation, PackGroupLocation))
		//LogF("Reloading for %s", strTargetLocation);

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
			// Log
			//sprintf(strLog, "%s < %s", pPack->GetName(), strPackGroupLocation); Log(strLog);
			//sprintf(strLog, "Backtracking to child group %s in %s", strPackGroupLocation, pPack->GetName()); Log(strLog);
		}

		// We can reach the target location as a relative child
		if (strPackGroupLocation[0] && GetRelativePath(strTargetLocation, strPackGroupLocation, strAdvance))
		{
			// Advance pack group to relative child
			if (pPackGroup->OpenChild(strAdvance))
			{
				// Log
				//sprintf(strLog, "%s > %s", pPack->GetName(), strTargetLocation); Log(strLog);
				//sprintf(strLog, "Advancing to child group %s in %s", strTargetLocation, pPack->GetName()); Log(strLog);
			}
		}

		// Cannot reach by advancing: need to close and reopen (rewinding group file)
		else
		{
			// Close pack group (if it is open at all)
			pPackGroup->Close();
			// Reopen pack group to relative position in language pack if possible
			pPackGroup->OpenAsChild(pPack, strTargetLocation);
			/*if (pPackGroup->OpenAsChild(pPack, strTargetLocation)) // Slow one...
			{
			  sprintf(strLog, "%s - %s", pPack->GetName(), strTargetLocation); Log(strLog);
			}
			else
			{
			  sprintf(strLog, "%s ! %s", pPack->GetName(), strTargetLocation); Log(strLog);
			}*/
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
		static re::regex line_pattern("^([^=]+)=(.*?)\r?$", static_cast<re::regex::flag_type>(re::regex_constants::optimize | re::regex_constants::ECMAScript));
		assert(stringtbl);
		if (!stringtbl)
		{
			return std::string();
		}

		// Get beginning and end iterators of stringtbl
		const char *begin = stringtbl;
		const char *end = begin + std::char_traits<char>::length(begin);

		for (auto it = re::cregex_iterator(begin, end, line_pattern); it != re::cregex_iterator(); ++it)
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
	char strEntry[_MAX_FNAME + 1];
	char *strTable;
	// Look for language string tables
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_Language, strEntry))
		// For now, we will only load info on the first string table found for a given
		// language code as there is currently no handling for selecting different string tables
		// of the same code - the system always loads the first string table found for a given code
		if (!FindInfo(GetFilenameOnly(strEntry) + SLen(GetFilenameOnly(strEntry)) - 2))
			// Load language string table
			if (hGroup.LoadEntry(strEntry, &strTable, 0, 1))
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
				// Log
				//sprintf(strLog, "Language info loaded from %s", strEntry); Log(strLog);
			}
}

C4LanguageInfo* C4Language::GetInfo(int iIndex)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (iIndex <= 0)
			return pInfo;
		else
			iIndex--;
	return NULL;
}

C4LanguageInfo* C4Language::FindInfo(const char *strCode)
{
	for (C4LanguageInfo *pInfo = Infos; pInfo; pInfo = pInfo->Next)
		if (SEqualNoCase(pInfo->Code, strCode, 2))
			return pInfo;
	return NULL;
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
	char strEntry[_MAX_FNAME + 1];
	sprintf(strEntry, "Language%s.txt", strCode); // ...should use C4CFN_Language here
	// Load string table
	if (!C4LangStringTable::GetSystemStringTable().Load(hGroup, strEntry))
		return false;
	
#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
	const char * const to_set = nl_langinfo(CODESET);
	if (local_to_host == iconv_t(-1))
		local_to_host = iconv_open (to_set ? to_set : "ASCII", "UTF-8");
	if (host_to_local == iconv_t(-1))
		host_to_local = iconv_open ("UTF-8",
		                            to_set ? to_set : "ASCII");
#else
	const char * const to_set = "";
#endif
#endif
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
