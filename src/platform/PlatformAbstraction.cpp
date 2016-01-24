/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, Sven Eberhardt
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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
	auto wide_filename = GetWideChar(Filename, true); // wide_filename holds the buffer
	SHFILEOPSTRUCTW shs;
	shs.hwnd=0;
	shs.wFunc=FO_DELETE;
	shs.pFrom = wide_filename;
	shs.pTo=NULL;
	shs.fFlags=FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	shs.fAnyOperationsAborted=false;
	shs.hNameMappings=0;
	shs.lpszProgressTitle=NULL;
	auto error = SHFileOperationW(&shs);
	return !error;
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

#ifdef USE_GTK
#include <gtk/gtk.h>
bool OpenURL(const char *szURL)
{
	GError *error = 0;
	if (gtk_show_uri(NULL, szURL, GDK_CURRENT_TIME, &error))
		return true;
	if (error != NULL)
	{
		fprintf (stderr, "Unable to open URL: %s\n", error->message);
		g_error_free (error);
	}
	const char * argv[][3] =
	{
		{ "xdg-open", szURL, 0 },
		{ "sensible-browser", szURL, 0 },
		{ "firefox", szURL, 0 },
		{ "mozilla", szURL, 0 },
		{ "konqueror", szURL, 0 },
		{ "epiphany", szURL, 0 },
		{ 0, 0, 0 }
	};
	for (int i = 0; argv[i][0]; ++i)
	{
		error = 0;
		if (g_spawn_async (g_get_home_dir(), const_cast<char**>(argv[i]), 0, G_SPAWN_SEARCH_PATH, 0, 0, 0, &error))
			return true;
		else
		{
			fprintf(stderr, "%s\n", error->message);
			g_error_free (error);
		}
	}
	return false;
}
#else
bool OpenURL(char const*) {return 0;}
#endif

#endif

