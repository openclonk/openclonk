/*
 * OpenClonk, http://www.openclonk.org
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
#include "StdAppCommon.h"

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

void CStdWindow::PerformUpdate()
{
}