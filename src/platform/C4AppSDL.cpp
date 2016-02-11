/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include "C4App.h"

#include <C4Window.h>
#include <C4DrawGL.h>
#include <StdFile.h>
#include <StdBuf.h>
#include <C4MouseControl.h>
#include <C4Application.h>
#include <C4Gui.h>
#include <C4GamePadCon.h>
#include <C4Version.h>

static void sdlToC4MCBtn(const SDL_MouseButtonEvent &e, int32_t& button, DWORD& flags)
{
	button = C4MC_Button_None;
	flags = SDL_GetModState();

	switch (e.button)
	{
	case SDL_BUTTON_LEFT:
		if (e.state == SDL_PRESSED)
			button = e.clicks == 2 ? C4MC_Button_LeftDouble : C4MC_Button_LeftDown;
		else
			button = C4MC_Button_LeftUp;
		break;
	case SDL_BUTTON_RIGHT:
		if (e.state == SDL_PRESSED)
			button = e.clicks == 2 ? C4MC_Button_RightDouble : C4MC_Button_RightDown;
		else
			button = C4MC_Button_RightUp;
		break;
	case SDL_BUTTON_MIDDLE:
		if (e.state == SDL_PRESSED)
			button = C4MC_Button_MiddleDown;
		else
			button = C4MC_Button_MiddleUp;
		break;
	}
}

/* C4AbstractApp */

C4AbstractApp::C4AbstractApp():
	Active(false), pWindow(NULL), fQuitMsgReceived(false),
	// main thread
#ifdef HAVE_PTHREAD
	MainThread (pthread_self()),
#endif
#ifdef _WIN32
	idMainThread(0),
#endif
	fDspModeSet(false)
{
}

C4AbstractApp::~C4AbstractApp()
{
}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		LogF("SDL_Init: %s", SDL_GetError());
		return false;
	}

	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	// Custom initialization
	return DoInit (argc, argv);
}


void C4AbstractApp::Clear()
{
	SDL_Quit();
}

void C4AbstractApp::Quit()
{
	fQuitMsgReceived = true;
}

bool C4AbstractApp::FlushMessages()
{
	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;

	// Handle pending SDL messages
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		HandleSDLEvent(event);
	}
	return true;
}

void C4AbstractApp::HandleSDLEvent(SDL_Event& e)
{
	DWORD flags;
	// Directly handle QUIT messages.
	switch (e.type)
	{
	case SDL_QUIT:
		Quit();
		break;
	case SDL_TEXTINPUT:
		::pGUI->CharIn(e.text.text);
		break;
	case SDL_KEYDOWN:
	{
		Game.DoKeyboardInput(e.key.keysym.scancode, KEYEV_Down,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT),
		                     false, NULL);
		break;
	}
	case SDL_KEYUP:
		Game.DoKeyboardInput(e.key.keysym.scancode, KEYEV_Up,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT), false, NULL);
		break;
	case SDL_MOUSEMOTION:
		C4GUI::MouseMove(C4MC_Button_None, e.motion.x, e.motion.y, 0, NULL);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		int32_t button;
		sdlToC4MCBtn(e.button, button, flags);
		C4GUI::MouseMove(button, e.button.x, e.button.y, flags, NULL);
		break;
	case SDL_MOUSEWHEEL:
		flags = e.wheel.y > 0 ? (+32) << 16 : (DWORD) (-32) << 16;
		flags += SDL_GetModState();
		int x, y;
		SDL_GetMouseState(&x, &y);
		C4GUI::MouseMove(C4MC_Button_Wheel, x, y, flags, NULL);
		break;
	case SDL_JOYAXISMOTION:
	case SDL_JOYHATMOTION:
	case SDL_JOYBALLMOTION:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		Application.pGamePadControl->FeedEvent(e);
		break;
	}
}

static int modeCount = 0;

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	if (!modeCount)
	{
		modeCount = SDL_GetNumDisplayModes(iMonitor);
	}

	if (iIndex >= modeCount)
		return false;

	SDL_DisplayMode mode;
	SDL_GetDisplayMode(iMonitor, iIndex, &mode);
	*piXRes = mode.w;
	*piYRes = mode.h;
	*piBitDepth = SDL_BITSPERPIXEL(mode.format);
	*piRefreshRate = mode.refresh_rate;
	return true;
}

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int RefreshRate,  unsigned int iMonitor, bool fFullScreen)
{
	int res;
	if (!fFullScreen)
	{
		res = SDL_SetWindowFullscreen(pWindow->window, 0);
		if (res)
		{
			LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
			return false;
		}
		if (iXRes != -1)
			pWindow->SetSize(iXRes, iYRes);
		C4Rect r;
		pWindow->GetSize(&r);
		OnResolutionChanged(r.Wdt, r.Hgt);
		return true;
	}
	SDL_DisplayMode mode;
	if (iXRes < 0 || iYRes < 0)
	{
		res = SDL_SetWindowFullscreen(pWindow->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		if (res)
		{
			LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
			return false;
		}
		res = SDL_GetDesktopDisplayMode(iMonitor, &mode);
		if (res)
		{
			LogF("SDL_GetDesktopDisplayMode: %s", SDL_GetError());
			return false;
		}
		OnResolutionChanged(mode.w, mode.h);
		return true;
	}
	for (int i = 0; i < modeCount; ++i)
	{
		res = SDL_GetDisplayMode(iMonitor, i, &mode);
		if (res)
		{
			LogF("SDL_GetDisplayMode: %s", SDL_GetError());
			return false;
		}
		if (mode.w == iXRes && mode.h == iYRes && mode.refresh_rate == RefreshRate && SDL_BITSPERPIXEL(mode.format) == C4Draw::COLOR_DEPTH)
		{
			res = SDL_SetWindowDisplayMode(pWindow->window, &mode);
			if (res)
			{
				LogF("SDL_SetWindowDisplayMode: %s", SDL_GetError());
				return false;
			}
			res = SDL_SetWindowFullscreen(pWindow->window, SDL_WINDOW_FULLSCREEN);
			if (res)
			{
				LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
				return false;
			}
			OnResolutionChanged(mode.w, mode.h);
			return true;
		}
	}
	return false;
}

void C4AbstractApp::RestoreVideoMode()
{
	if (pWindow && pWindow->window)
		SDL_SetWindowFullscreen(pWindow->window, 0);
}

bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	return SDL_SetClipboardText(text.getData()) == 0;
}

StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	char * text = SDL_GetClipboardText();
	StdStrBuf buf;
	buf.Copy(text);
	SDL_free(text);
	return buf;
}

bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return SDL_HasClipboardText();
}

void C4AbstractApp::MessageDialog(const char * message)
{
	SDL_ShowSimpleMessageBox(0, C4ENGINECAPTION, message, pWindow ? pWindow->window : 0);
}
