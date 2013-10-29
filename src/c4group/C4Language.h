/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004  Matthes Bender
 * Copyright (c) 2005-2007  Günther Brammer
 * Copyright (c) 2013  Nicolas Hake
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

/* Language module controlling external language packs */

#ifndef INC_C4Language
#define INC_C4Language

#include "c4group/C4Group.h"
#include "c4group/C4GroupSet.h"
#include "c4group/C4LangStringTable.h"

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

const int C4MaxLanguageInfo = 1024;

class C4LanguageInfo
{
	friend class C4Language;
public:
	char Code[2 + 1];
	char Name[C4MaxLanguageInfo + 1];
	char Info[C4MaxLanguageInfo + 1];
	char Fallback[C4MaxLanguageInfo + 1];
	//char Location[C4MaxLanguageInfo + 1]; ...store group name here
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
	char PackGroupLocation[_MAX_FNAME + 1];
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
	// Encoding conversion functions
	static StdStrBuf IconvClonk(const char * string);
	static StdStrBuf IconvSystem(const char * string);

	inline bool HasStringTable() const { return !C4LangStringTable::GetSystemStringTable().GetDataBuf().isNull(); }

private:
	// Handling of language info loaded from string tables
	void InitInfos();
	void LoadInfos(C4Group &hGroup);
	// Loading of actual resource string table
	bool InitStringTable(const char *strCode);
	bool LoadStringTable(C4Group &hGroup, const char *strCode);
#ifdef HAVE_ICONV
	static iconv_t local_to_host;
	static iconv_t host_to_local;
	static StdStrBuf Iconv(const char * string, iconv_t cd);
#endif
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