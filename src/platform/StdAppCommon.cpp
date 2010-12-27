/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Mortimer
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
#include "StdAppCommon.h"

#ifdef WITH_GLIB
#include <glib.h>
#endif

void CStdApp::Run()
{
	// Main message loop
	while (!fQuitMsgReceived)
		ScheduleProcs();
}

bool CStdApp::ScheduleProcs(int iTimeout)
{
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;
#if defined(USE_SDL_MAINLOOP)
	// Unfortunately, the SDL event loop needs to be polled
	FlushMessages();
#endif
	return StdScheduler::ScheduleProcs(iTimeout);
}

#if !defined(__APPLE__) && !defined(_WIN32)

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

#endif

void CStdWindow::PerformUpdate()
{
}
