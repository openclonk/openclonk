/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Alexander Post
 * Copyright (c) 2010  Martin Plicht
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
#include "C4App.h"
#include "C4Window.h"

void C4AbstractApp::Run()
{
	// Main message loop
	while (!fQuitMsgReceived)
		ScheduleProcs();
}

bool C4AbstractApp::ScheduleProcs(int iTimeout)
{
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;
#if defined(USE_SDL_MAINLOOP) || defined(USE_COCOA)
	// Unfortunately, the SDL event loop needs to be polled
	FlushMessages();
#endif
	return StdScheduler::ScheduleProcs(iTimeout);
}

void C4Window::PerformUpdate()
{
}

void C4AbstractApp::NotifyUserIfInactive()
{
#ifdef _WIN32
		if (!Active && pWindow) pWindow->FlashWindow();
#else
		if (pWindow) pWindow->FlashWindow();
#endif
}
