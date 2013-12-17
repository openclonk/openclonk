/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007, Alexander Post
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
