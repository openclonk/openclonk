/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007, Alexander Post
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
#include "platform/C4App.h"
#include "platform/C4Window.h"

#ifdef WITH_QT_EDITOR
#include "game/C4Application.h"
#include "editor/C4ConsoleQt.h"
#endif

void C4AbstractApp::Run()
{
#ifdef WITH_QT_EDITOR
	if (Application.isEditor)
	{
		// Qt has its own event loop. Use a timer to call our own event handling whenever Qt is done
		// with its events. The alternative (calling Qt's event handling from
		// C4Console::Execute) is too slow, at least on Linux.
		// FIXME: All of this should be happening in a background thread instead of on the UI thread
		QTimer timer;
		QObject::connect(&timer, &QTimer::timeout, [this, &timer]() {
			ScheduleProcs(0);
			if (fQuitMsgReceived)
			{
				QApplication::quit();
				return;
			}
			auto now = C4TimeMilliseconds::Now();
			auto next_tick = GetNextTick(now);
			timer.setInterval(Clamp(next_tick - now, 0, 1000/36));
		});
		timer.setTimerType(Qt::PreciseTimer);
		timer.start();
		QApplication::exec();
		return;
	}
#endif

	// Main message loop
	while (!fQuitMsgReceived)
		ScheduleProcs();
}

bool C4AbstractApp::DoScheduleProcs(int iTimeout)
{
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;
#if defined(USE_SDL_MAINLOOP) || defined(USE_COCOA)
	// Unfortunately, the SDL event loop needs to be polled
	FlushMessages();
#endif
#ifdef WITH_QT_EDITOR
	// Some places, for example the lobby, have a nested event processing loop.
	// To prevent the editor from freezing, call the Qt event loop here.
	if (iTimeout)
		ProcessQtEvents();
#endif
	return StdScheduler::DoScheduleProcs(iTimeout);
}

#ifdef WITH_QT_EDITOR
void C4AbstractApp::ProcessQtEvents()
{
	if (Application.isEditor)
		QApplication::processEvents();
}
#endif

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
