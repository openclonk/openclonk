/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2010  Martin Plicht
 * Copyright (c) 2012  Günther Brammer
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

#include <C4Include.h>

#ifdef _WIN32
#include <C4windowswrapper.h>
#include <shellapi.h>
bool OpenURL(const char *szURL)
{
	return (intptr_t)ShellExecuteW(NULL, L"open", GetWideChar(szURL), NULL, NULL, SW_SHOW) > 32;
}

bool EraseItemSafe(const char *szFilename)
{
	char Filename[_MAX_PATH+1];
	SCopy(szFilename, Filename, _MAX_PATH);
	Filename[SLen(Filename)+1]=0;
	SHFILEOPSTRUCTW shs;
	shs.hwnd=0;
	shs.wFunc=FO_DELETE;
	shs.pFrom=GetWideChar(Filename);
	shs.pTo=NULL;
	shs.fFlags=FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	shs.fAnyOperationsAborted=false;
	shs.hNameMappings=0;
	shs.lpszProgressTitle=NULL;
	return !SHFileOperationW(&shs);
}

bool IsGermanSystem()
{
	return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_GERMAN;
}
#elif !defined(__APPLE__)

bool IsGermanSystem()
{
	if (strstr(setlocale(LC_MESSAGES, 0), "de"))
		return true;
	else
		return false;
}

bool EraseItemSafe(const char *szFilename)
{
	return false;
}

#if !defined(USE_X11)
bool OpenURL(char const*) {return 0;}
#endif

#endif

