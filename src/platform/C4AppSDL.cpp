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
// SDL version

static void sdlToC4MCBtn(const SDL_MouseButtonEvent &e, int32_t& button, DWORD& flags)
{
	C4TimeMilliseconds lastLeftClick = C4TimeMilliseconds::Now();
	C4TimeMilliseconds lastRightClick = C4TimeMilliseconds::Now();
	static int lastX = 0, lastY = 0;
	
	static const int clickDist = 2;
	static const int clickDelay = 400;
	
	button = C4MC_Button_None;
	flags = 0;

	switch (e.button)
	{
	case SDL_BUTTON_LEFT:
		if (e.state == SDL_PRESSED)
			if (C4TimeMilliseconds::Now() - lastLeftClick < clickDelay && abs(lastX-e.x) <= clickDist && abs(lastY-e.y) <= clickDist)
			{
				lastLeftClick = 0;
				button = C4MC_Button_LeftDouble;
			}
			else
			{
				lastLeftClick = C4TimeMilliseconds::Now();
				button = C4MC_Button_LeftDown;
			}
		else
			button = C4MC_Button_LeftUp;
		break;
	case SDL_BUTTON_RIGHT:
		if (e.state == SDL_PRESSED)
			if (C4TimeMilliseconds::Now() - lastRightClick < clickDelay)
			{
				lastRightClick = 0;
				button = C4MC_Button_RightDouble;
			}
			else
			{
				lastRightClick = C4TimeMilliseconds::Now();
				button = C4MC_Button_RightDown;
			}
		else
			button = C4MC_Button_RightUp;
		break;
	case SDL_BUTTON_MIDDLE:
		if (e.state == SDL_PRESSED)
			button = C4MC_Button_MiddleDown;
		else
			button = C4MC_Button_MiddleUp;
		break;
	case SDL_BUTTON_WHEELUP:
		button = C4MC_Button_Wheel;
		flags = (+32) << 16;
		break;
	case SDL_BUTTON_WHEELDOWN:
		button = C4MC_Button_Wheel;
		flags = (-32) << 16;
		break;
	}
	lastX = e.x;
	lastY = e.y;
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

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) < 0)
	{
		Log("Error initializing SDL.");
		return false;
	}

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

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
	// Directly handle QUIT messages.
	switch (e.type)
	{
	case SDL_QUIT:
		Quit();
		break;
	case SDL_KEYDOWN:
	{
#ifndef USE_CONSOLE
		if (e.key.keysym.sym == SDLK_f && (e.key.keysym.mod & (KMOD_LMETA | KMOD_RMETA)))
		{
			Config.Graphics.Windowed = !Config.Graphics.Windowed;
			Application.SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed);
			pDraw->InvalidateDeviceObjects();
			pDraw->RestoreDeviceObjects();

			break;
		}
#endif

		StdStrBuf c;
		c.AppendCharacter(e.key.keysym.unicode);
		::pGUI->CharIn(c.getData());
		Game.DoKeyboardInput(e.key.keysym.sym, KEYEV_Down,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT),
		                     false, NULL);
		break;
	}
	case SDL_KEYUP:
		Game.DoKeyboardInput(e.key.keysym.sym, KEYEV_Up,
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
		DWORD flags;
		sdlToC4MCBtn(e.button, button, flags);
		C4GUI::MouseMove(button, e.button.x, e.button.y, flags, NULL);
		break;
	case SDL_JOYAXISMOTION:
	case SDL_JOYHATMOTION:
	case SDL_JOYBALLMOTION:
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		Application.pGamePadControl->FeedEvent(e);
		break;
	}

#ifdef __APPLE__
	MacUtility::ensureWindowInFront();
#endif
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	// No support for multiple monitors.
	if (iMonitor != 0)
		return false;

	static SDL_Rect** modes = 0;
	static int modeCount = 0;
	if (!modes)
	{
		modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
		// -1 means "all modes allowed". Clonk is not prepared
		// for this; should probably give some random resolutions
		// then.
		assert(reinterpret_cast<intptr_t>(modes) != -1);
		if (!modes)
			modeCount = 0;
		else
			// Count available modes.
			for (SDL_Rect** iter = modes; *iter; ++iter)
				++modeCount;
	}

	if (iIndex >= modeCount)
		return false;

	*piXRes = modes[iIndex]->w;
	*piYRes = modes[iIndex]->h;
	*piBitDepth = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
	return true;
}

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int iColorDepth, unsigned int RefreshRate,  unsigned int iMonitor, bool fFullScreen)
{
	//RECT r;
	//pWindow->GetSize(&r);
	// FIXME: optimize redundant calls away. maybe make all platforms implicitely call SetVideoMode in C4Window::Init?
	// SDL doesn't support multiple monitors.
	if (!SDL_SetVideoMode(iXRes == -1 ? 0 : iXRes, iYRes == -1 ? 0 : iYRes, iColorDepth,
		SDL_OPENGL | (fFullScreen ? SDL_FULLSCREEN : 0)))
	{
		sLastError.Copy(SDL_GetError());
		return false;
	}
	SDL_ShowCursor(SDL_DISABLE);
	const SDL_VideoInfo * info = SDL_GetVideoInfo();
	OnResolutionChanged(info->current_w, info->current_h);
	return true;
}

void C4AbstractApp::RestoreVideoMode()
{
}

bool C4AbstractApp::ApplyGammaRamp(struct _GAMMARAMP& ramp, bool fForce)
{
	return SDL_SetGammaRamp(ramp.red, ramp.green, ramp.blue) != -1;
}

bool C4AbstractApp::SaveDefaultGammaRamp(struct _GAMMARAMP& ramp)
{
	return SDL_GetGammaRamp(ramp.red, ramp.green, ramp.blue) != -1;
}

// For Max OS X, the implementation resides in StdMacApp.mm
#ifndef __APPLE__

// stubs
bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	return false;
}

StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	return StdStrBuf("");
}

bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return false;
}

void C4AbstractApp::MessageDialog(const char * message)
{
}

#endif
