/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004  Matthes Bender
 * Copyright (c) 2005-2007  Günther Brammer
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

#include <C4Group.h>
#include <C4GroupSet.h>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

const int C4MaxLanguageInfo = 1024;

class C4Language;

class C4LanguageInfo
{
	friend class C4Language;
	public:
		char Code[2 + 1];
		char Name[C4MaxLanguageInfo + 1];
		char Info[C4MaxLanguageInfo + 1];
		char Fallback[C4MaxLanguageInfo + 1];
		//char Location[C4MaxLanguageInfo + 1]; ...store group name here
	protected:
		C4LanguageInfo* Next;
};

class C4Language
{
  public:
    C4Language();
    ~C4Language();
  protected:
		C4Group PackDirectory;
		C4GroupSet Packs;
		C4GroupSet PackGroups;
		C4LanguageInfo* Infos;
		char PackGroupLocation[_MAX_FNAME + 1];
  public:
	  bool CloseGroup(const char *strPath);
	  void ClearLanguage();
		// Initialization
	  bool Init();
	  void Clear();
		// Handling of external language packs
		int GetPackCount();
		C4GroupSet& GetPackGroups(const char *strRelativePath);
		// Handling of language info loaded from string tables
		int GetInfoCount();
		C4LanguageInfo *GetInfo(int iIndex);
		C4LanguageInfo *FindInfo(const char *strCode);
		// Loading of actual resource string table
		bool LoadLanguage(const char *strLanguages);
		// Encoding conversion functions
		static StdStrBuf IconvClonk(const char * string);
		static StdStrBuf IconvSystem(const char * string);
	protected:
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

#endif
