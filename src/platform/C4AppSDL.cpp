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
	/*case SDL_BUTTON_WHEELUP:
		button = C4MC_Button_Wheel;
		flags = (+32) << 16;
		break;
	case SDL_BUTTON_WHEELDOWN:
		button = C4MC_Button_Wheel;
		flags = (-32) << 16;
		break;*/
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

static C4KeyCode sdl_scancode_to_keycode(SDL_Scancode scancode)
{
	switch (scancode)
	{
	case SDL_SCANCODE_LSHIFT: return K_SHIFT_L;
	case SDL_SCANCODE_RSHIFT: return K_SHIFT_R;
	case SDL_SCANCODE_LCTRL: return K_CONTROL_L;
	case SDL_SCANCODE_RCTRL: return K_CONTROL_R;
	case SDL_SCANCODE_LALT: return K_ALT_L;
	case SDL_SCANCODE_RALT: return K_ALT_R;
	case SDL_SCANCODE_F1: return K_F1;
	case SDL_SCANCODE_F2: return K_F2;
	case SDL_SCANCODE_F3: return K_F3;
	case SDL_SCANCODE_F4: return K_F4;
	case SDL_SCANCODE_F5: return K_F5;
	case SDL_SCANCODE_F6: return K_F6;
	case SDL_SCANCODE_F7: return K_F7;
	case SDL_SCANCODE_F8: return K_F8;
	case SDL_SCANCODE_F9: return K_F9;
	case SDL_SCANCODE_F10: return K_F10;
	case SDL_SCANCODE_F11: return K_F11;
	case SDL_SCANCODE_F12: return K_F12;
	case SDL_SCANCODE_KP_PLUS: return K_ADD;
	case SDL_SCANCODE_KP_MINUS: return K_SUBTRACT;
	case SDL_SCANCODE_KP_MULTIPLY: return K_MULTIPLY;
	case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
	case SDL_SCANCODE_PAUSE: return K_PAUSE;
	case SDL_SCANCODE_TAB: return K_TAB;
	case SDL_SCANCODE_RETURN: return K_RETURN;
	case SDL_SCANCODE_DELETE: return K_DELETE;
	case SDL_SCANCODE_INSERT: return K_INSERT;
	case SDL_SCANCODE_BACKSPACE: return K_BACK;
	case SDL_SCANCODE_SPACE: return K_SPACE;
	case SDL_SCANCODE_UP: return K_UP;
	case SDL_SCANCODE_DOWN: return K_DOWN;
	case SDL_SCANCODE_LEFT: return K_LEFT;
	case SDL_SCANCODE_RIGHT: return K_RIGHT;
	case SDL_SCANCODE_HOME: return K_HOME;
	case SDL_SCANCODE_END: return K_END;
	case SDL_SCANCODE_SCROLLLOCK: return K_SCROLL;
	case SDL_SCANCODE_MENU: return K_MENU;
	case SDL_SCANCODE_PAGEUP: return K_PAGEUP;
	case SDL_SCANCODE_PAGEDOWN: return K_PAGEDOWN;
	case SDL_SCANCODE_1: return K_1;
	case SDL_SCANCODE_2: return K_2;
	case SDL_SCANCODE_3: return K_3;
	case SDL_SCANCODE_4: return K_4;
	case SDL_SCANCODE_5: return K_5;
	case SDL_SCANCODE_6: return K_6;
	case SDL_SCANCODE_7: return K_7;
	case SDL_SCANCODE_8: return K_8;
	case SDL_SCANCODE_9: return K_9;
	case SDL_SCANCODE_0: return K_0;
	case SDL_SCANCODE_A: return K_A;
	case SDL_SCANCODE_B: return K_B;
	case SDL_SCANCODE_C: return K_C;
	case SDL_SCANCODE_D: return K_D;
	case SDL_SCANCODE_E: return K_E;
	case SDL_SCANCODE_F: return K_F;
	case SDL_SCANCODE_G: return K_G;
	case SDL_SCANCODE_H: return K_H;
	case SDL_SCANCODE_I: return K_I;
	case SDL_SCANCODE_J: return K_J;
	case SDL_SCANCODE_K: return K_K;
	case SDL_SCANCODE_L: return K_L;
	case SDL_SCANCODE_M: return K_M;
	case SDL_SCANCODE_N: return K_N;
	case SDL_SCANCODE_O: return K_O;
	case SDL_SCANCODE_P: return K_P;
	case SDL_SCANCODE_Q: return K_Q;
	case SDL_SCANCODE_R: return K_R;
	case SDL_SCANCODE_S: return K_S;
	case SDL_SCANCODE_T: return K_T;
	case SDL_SCANCODE_U: return K_U;
	case SDL_SCANCODE_V: return K_V;
	case SDL_SCANCODE_W: return K_W;
	case SDL_SCANCODE_X: return K_X;
	case SDL_SCANCODE_Y: return K_Y;
	case SDL_SCANCODE_Z: return K_Z;
	case SDL_SCANCODE_MINUS: return K_MINUS;
	case SDL_SCANCODE_EQUALS: return K_EQUAL;
	case SDL_SCANCODE_LEFTBRACKET: return K_LEFT_BRACKET;
	case SDL_SCANCODE_RIGHTBRACKET: return K_RIGHT_BRACKET;
	case SDL_SCANCODE_SEMICOLON: return K_SEMICOLON;
	case SDL_SCANCODE_APOSTROPHE: return K_APOSTROPHE;
	case SDL_SCANCODE_GRAVE: return K_GRAVE_ACCENT;
	case SDL_SCANCODE_BACKSLASH: return K_BACKSLASH;
	case SDL_SCANCODE_COMMA: return K_COMMA;
	case SDL_SCANCODE_PERIOD: return K_PERIOD;
	case SDL_SCANCODE_SLASH: return K_SLASH;
	case SDL_SCANCODE_CAPSLOCK: return K_CAPS;
	case SDL_SCANCODE_NUMLOCKCLEAR: return K_NUM;
	case SDL_SCANCODE_KP_7: return K_NUM7;
	case SDL_SCANCODE_KP_8: return K_NUM8;
	case SDL_SCANCODE_KP_9: return K_NUM9;
	case SDL_SCANCODE_KP_4: return K_NUM4;
	case SDL_SCANCODE_KP_5: return K_NUM5;
	case SDL_SCANCODE_KP_6: return K_NUM6;
	case SDL_SCANCODE_KP_1: return K_NUM1;
	case SDL_SCANCODE_KP_2: return K_NUM2;
	case SDL_SCANCODE_KP_3: return K_NUM3;
	case SDL_SCANCODE_KP_0: return K_NUM0;
	case SDL_SCANCODE_KP_PERIOD: return K_DECIMAL;
	case SDL_SCANCODE_NONUSBACKSLASH: return K_86;
	case SDL_SCANCODE_KP_ENTER: return K_NUM_RETURN;
	case SDL_SCANCODE_KP_DIVIDE: return K_DIVIDE;
	case SDL_SCANCODE_LGUI: return K_WIN_L;
	case SDL_SCANCODE_RGUI: return K_WIN_R;
	case SDL_SCANCODE_PRINTSCREEN: return K_PRINT;
	}
	return 0;
}

void C4AbstractApp::HandleSDLEvent(SDL_Event& e)
{
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
		Game.DoKeyboardInput(sdl_scancode_to_keycode(e.key.keysym.scancode), KEYEV_Down,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT),
		                     false, NULL);
		break;
	}
	case SDL_KEYUP:
		Game.DoKeyboardInput(sdl_scancode_to_keycode(e.key.keysym.scancode), KEYEV_Up,
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
