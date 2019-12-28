/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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

/* Language module controlling external language packs */

#ifndef INC_C4Language
#define INC_C4Language

#include "c4group/C4Group.h"
#include "c4group/C4GroupSet.h"
#include "c4group/C4LangStringTable.h"

const int C4MaxLanguageInfo = 1024;

class C4LanguageInfo
{
	friend class C4Language;
public:
	char Code[2 + 1];
	char Name[C4MaxLanguageInfo + 1];
	char Info[C4MaxLanguageInfo + 1];
	char Fallback[C4MaxLanguageInfo + 1];
private:
	C4LanguageInfo* Next;
};

class C4Language
{
public:
	C4Language();
	~C4Language();
private:
	C4Group PackDirectory;
	C4GroupSet Packs;
	C4GroupSet PackGroups;
	class C4LanguageInfo* Infos;
	char PackGroupLocation[_MAX_FNAME_LEN];
public:
	bool CloseGroup(const char *strPath);
	void ClearLanguage();
	// Initialization
	bool Init();
	void Clear();
	// Handling of external language packs
	int GetPackCount();
	C4GroupSet GetPackGroups(C4Group &);
	// Load a C4ComponentHost from all loaded language packs
	static bool LoadComponentHost(C4ComponentHost *host, C4Group &hGroup, const char *szFilename, const char *szLanguage);

	// Handling of language info loaded from string tables
	int GetInfoCount();
	C4LanguageInfo *GetInfo(int iIndex);
	C4LanguageInfo *FindInfo(const char *strCode);
	// Loading of actual resource string table
	bool LoadLanguage(const char *strLanguages);

	inline bool HasStringTable() const { return !C4LangStringTable::GetSystemStringTable().GetDataBuf().isNull(); }

private:
	// Handling of language info loaded from string tables
	void InitInfos();
	void LoadInfos(C4Group &hGroup);
	// Loading of actual resource string table
	bool InitStringTable(const char *strCode);
	bool LoadStringTable(C4Group &hGroup, const char *strCode);
};

extern C4Language Languages;

inline const char *LoadResStr(const char *id)
{
	try
	{
		return C4LangStringTable::GetSystemStringTable().Translate(id).c_str();
	}
	catch (C4LangStringTable::NoSuchTranslation &)
	{
		return id;
	}
}
const char *LoadResStrNoAmp(const char *id);

#endif
