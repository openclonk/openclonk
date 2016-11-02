/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include "C4Include.h"
#include "platform/C4Window.h"

#include "game/C4Application.h"
#include "graphics/C4DrawGL.h"
#include "editor/C4Console.h"
#include "editor/C4ViewportWindow.h"
#include "gui/C4Gui.h"
#include "platform/StdFile.h"
#include "lib/StdBuf.h"

#include "C4Version.h"
#include "lib/C4Rect.h"
#include "config/C4Config.h"

#ifdef SDL_VIDEO_DRIVER_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <SDL_syswm.h>
#endif

/* C4Window */

C4Window::C4Window ():
		Active(false), pSurface(0), eKind(W_Fullscreen), window(nullptr)
#ifdef WITH_QT_EDITOR
, glwidget(nullptr)
#endif
{
}

C4Window::~C4Window ()
{
	Clear();
}

static void SetMultisamplingAttributes(int samples)
{
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples > 0 ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
}

C4Window * C4Window::Init(WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size)
{
	eKind = windowKind;
#ifdef WITH_QT_EDITOR
	if (windowKind == W_Viewport)
	{
		// embed into editor: Viewport widget creation handled by C4ConsoleQt
		::Console.AddViewport(static_cast<C4ViewportWindow *>(this));
		return this;
	}
#endif
/*	    SDL_GL_MULTISAMPLEBUFFERS,
	    SDL_GL_MULTISAMPLESAMPLES,*/
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, /*REQUESTED_GL_CTX_MAJOR*/ 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, /*REQUESTED_GL_CTX_MINOR*/ 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, (Config.Graphics.DebugOpenGL ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SetMultisamplingAttributes(Config.Graphics.MultiSampling);
	uint32_t flags = SDL_WINDOW_OPENGL;
	if (windowKind == W_Fullscreen && size->Wdt == -1)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (windowKind == W_Fullscreen && !Config.Graphics.Windowed)
		flags |= SDL_WINDOW_FULLSCREEN;
	window = SDL_CreateWindow(Title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size->Wdt, size->Hgt, flags);
	if (!window)
	{
		Log(SDL_GetError());
		return 0;
	}
	SDL_SetWindowData(window, "C4Window", this);
	Active = true;
	SDL_ShowCursor(SDL_DISABLE);
	return this;
}

bool C4Window::ReInit(C4AbstractApp* pApp)
{
	// TODO: Is there some way to do this without requiring a restart?
	// Maybe re-call SDL_SetVideoMode?
	C4GUI::TheScreen.ShowMessage(LoadResStr("IDS_CTL_ANTIALIASING_RESTART_MSG"), LoadResStr("IDS_CTL_ANTIALIASING_RESTART_TITLE"), C4GUI::Ico_Notify);
	return true;
}

void C4Window::Clear()
{
	if (window) SDL_DestroyWindow(window);
	window = nullptr;

#ifdef WITH_QT_EDITOR
	if (eKind == W_Viewport)
	{
		// embed into editor: Viewport widget creation handled by C4ConsoleQt
		::Console.RemoveViewport(static_cast<C4ViewportWindow *>(this));
	}
#endif
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
{
	int max_samples;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
	samples.clear();
	for (int s = 2; s <= max_samples; s *= 2)
	{
		// Not all multisampling options seem to work. Verify by creating a hidden window.
		SetMultisamplingAttributes(s);
		SDL_Window *wnd = SDL_CreateWindow("OpenClonk Test Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 100, 100, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
		if (wnd)
		{
			SDL_DestroyWindow(wnd);
			samples.push_back(s);
		}
		else
			break;
	}
	SetMultisamplingAttributes(Config.Graphics.MultiSampling);
}

bool C4Window::StorePosition(const char *, const char *, bool) { return true; }

bool C4Window::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by C4AbstractApp's display mode management.
// Just remember the size for others to query.

bool C4Window::GetSize(C4Rect * pRect)
{
	pRect->x = pRect->y = 0;
	SDL_GL_GetDrawableSize(window, &pRect->Wdt, &pRect->Hgt);
	return true;
}

void C4Window::SetSize(unsigned int X, unsigned int Y)
{
	SDL_SetWindowSize(window, X, Y);
}

void C4Window::SetTitle(const char * Title)
{
	SDL_SetWindowTitle(window, Title);
}

void C4Window::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

static void SetUrgencyHint(SDL_Window *window, bool urgency_hint)
{
#ifdef SDL_VIDEO_DRIVER_X11
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	if (!SDL_GetWindowWMInfo(window, &wminfo))
	{
		LogF("FlashWindow SDL: %s", SDL_GetError());
		return;
	}

	if (wminfo.subsystem == SDL_SYSWM_X11)
	{
		auto x11 = wminfo.info.x11;
		XWMHints *wmhints = XGetWMHints(x11.display, x11.window);
		if (wmhints == nullptr)
			wmhints = XAllocWMHints();
		// Set the window's urgency hint.
		if (urgency_hint)
			wmhints->flags |= XUrgencyHint;
		else
			wmhints->flags &= ~XUrgencyHint;
		XSetWMHints(x11.display, x11.window, wmhints);
		XFree(wmhints);
	}
#endif
}

void C4Window::FlashWindow()
{
	SetUrgencyHint(window, true);
}

void C4Window::GrabMouse(bool grab)
{
	SDL_SetWindowGrab(window, grab ? SDL_TRUE : SDL_FALSE);
}

void C4Window::HandleSDLEvent(SDL_WindowEvent &e)
{
	switch (e.event)
	{
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		SetUrgencyHint(window, false);
		break;
	case SDL_WINDOWEVENT_RESIZED:
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		Application.OnResolutionChanged(e.data1, e.data2);
		break;
	default:
		// We don't care about most events. We should care about more, though.
		break;
	}
}
