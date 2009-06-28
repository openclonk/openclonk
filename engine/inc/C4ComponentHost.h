/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004  Matthes Bender
 * Copyright (c) 2005, 2007  Günther Brammer
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

/* Holds a single text file component from a group */

#ifndef INC_C4ComponentHost
#define INC_C4ComponentHost

#include "C4GroupSet.h"

class C4ComponentHost
	{
	public:
		C4ComponentHost();
		virtual ~C4ComponentHost();
		const char *GetFilePath() const { return FilePath; }
		void Default();
		void Clear();
		void Open();
		const char *GetData() { return Data.getData(); }
		size_t GetDataSize() { return Data.getLength(); }
		virtual void Close();
		BOOL Load(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL Load(const char *szName, C4GroupSet &hGroupSet, const char *szFilename, const char *szLanguage=NULL);
		BOOL LoadEx(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL LoadAppend(const char *szName, C4Group &hGroup, const char *szFilename, const char *szLanguage=NULL);
		BOOL Set(const char *szData);
		BOOL Save(C4Group &hGroup);
		bool GetLanguageString(const char *szLanguage, class StdStrBuf &rTarget);
		BOOL SetLanguageString(const char *szLanguage, const char *szString);
		void TrimSpaces();
	protected:
		StdCopyStrBuf Data;
		BOOL Modified;
		char Name[_MAX_FNAME+1];
		char Filename[_MAX_FNAME+1];
		char FilePath[_MAX_PATH+1];
		void CopyFilePathFromGroup(const C4Group &hGroup);
#ifdef _WIN32
		HWND hDialog;
		void InitDialog(HWND hDlg);
	friend BOOL CALLBACK ComponentDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
#endif
	};

#endif
