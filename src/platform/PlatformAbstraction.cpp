/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, Sven Eberhardt
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
#include "game/C4Application.h"

#ifdef _WIN32
#include "platform/C4windowswrapper.h"
#include <shellapi.h>
bool OpenURL(const char *szURL)
{
	return (intptr_t)ShellExecuteW(nullptr, L"open", GetWideChar(szURL), nullptr, nullptr, SW_SHOW) > 32;
}

bool EraseItemSafe(const char *szFilename)
{
	char Filename[_MAX_PATH+1];
	SCopy(szFilename, Filename, _MAX_PATH);
	Filename[SLen(Filename)+1]=0;
	auto wide_filename = GetWideChar(Filename, true); // wide_filename holds the buffer
	SHFILEOPSTRUCTW shs;
	shs.hwnd=nullptr;
	shs.wFunc=FO_DELETE;
	shs.pFrom = wide_filename;
	shs.pTo=nullptr;
	shs.fFlags=FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
	shs.fAnyOperationsAborted=false;
	shs.hNameMappings=nullptr;
	shs.lpszProgressTitle=nullptr;
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
	if (strstr(setlocale(LC_MESSAGES, nullptr), "de"))
		return true;
	else
		return false;
}

bool EraseItemSafe(const char *szFilename)
{
	return false;
}

#if defined(WITH_QT_EDITOR)
#undef LineFeed
#include <QDesktopServices>
#include <QUrl>
bool OpenURL(char const* url)
{
	return QDesktopServices::openUrl(QUrl::fromUserInput(url));
}
#else
bool OpenURL(char const*) {return 0;}
#endif

#endif


bool RestartApplication(std::vector<const char *> parameters)
{
	// restart with given parameters
	bool success = false;
#ifdef _WIN32
	wchar_t buf[_MAX_PATH];
	DWORD sz = ::GetModuleFileName(::GetModuleHandle(nullptr), buf, _MAX_PATH);
	if (sz)
	{
		StdStrBuf params;
		for (auto p : parameters)
		{
			params += R"(")";
			params += p;
			params += R"(" )";
		}
		intptr_t iError = (intptr_t)::ShellExecute(nullptr, nullptr, buf, params.GetWideChar(), Config.General.ExePath.GetWideChar(), SW_SHOW);
		if (iError > 32) success = true;
	}
#else
	pid_t pid;
	switch (pid = fork())
	{
	case -1: break; // error message shown below
	case 0:
	{
		std::vector<const char*> params = {Application.argv0};
		params.insert(params.end(), parameters.begin(), parameters.end());
		params.push_back(nullptr);
#ifdef PROC_SELF_EXE
		execv(PROC_SELF_EXE, const_cast<char *const *>(params.data()));
		perror("editor launch via " PROC_SELF_EXE " failed");
#endif
		execvp(Application.argv0, const_cast<char *const *>(params.data()));
		perror("editor launch via argv[0] failed");
		exit(1);
	}
	default:
		success = true;
	}
#endif
	// must quit ourselves for new instance to be shown
	if (success) Application.Quit();
	return success;
}
