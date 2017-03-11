/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "platform/C4App.h"

#include "platform/C4Window.h"
#include "graphics/C4DrawGL.h"
#include "platform/StdFile.h"
#include "lib/StdBuf.h"
#include "gui/C4MouseControl.h"
#include "game/C4Application.h"
#include "gui/C4Gui.h"
#include "platform/C4GamePadCon.h"
#include "C4Version.h"

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
			button = e.clicks == 2 ? C4MC_Button_MiddleDouble : C4MC_Button_MiddleDown;
		else
			button = C4MC_Button_MiddleUp;
		break;
	case SDL_BUTTON_X1:
		if (e.state == SDL_PRESSED)
			button = e.clicks == 2 ? C4MC_Button_X1Double : C4MC_Button_X1Down;
		else
			button = C4MC_Button_X1Up;
		break;
	case SDL_BUTTON_X2:
		if (e.state == SDL_PRESSED)
			button = e.clicks == 2 ? C4MC_Button_X2Double : C4MC_Button_X2Down;
		else
			button = C4MC_Button_X2Up;
		break;
	}
}

/* C4AbstractApp */

C4AbstractApp::C4AbstractApp():
	Active(false), pWindow(nullptr), fQuitMsgReceived(false),
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

#define SDL_SCANCODE_KEYCODE \
	X(SDL_SCANCODE_LSHIFT, K_SHIFT_L) \
	X(SDL_SCANCODE_RSHIFT, K_SHIFT_R) \
	X(SDL_SCANCODE_LCTRL, K_CONTROL_L) \
	X(SDL_SCANCODE_RCTRL, K_CONTROL_R) \
	X(SDL_SCANCODE_LALT, K_ALT_L) \
	X(SDL_SCANCODE_RALT, K_ALT_R) \
	X(SDL_SCANCODE_F1, K_F1) \
	X(SDL_SCANCODE_F2, K_F2) \
	X(SDL_SCANCODE_F3, K_F3) \
	X(SDL_SCANCODE_F4, K_F4) \
	X(SDL_SCANCODE_F5, K_F5) \
	X(SDL_SCANCODE_F6, K_F6) \
	X(SDL_SCANCODE_F7, K_F7) \
	X(SDL_SCANCODE_F8, K_F8) \
	X(SDL_SCANCODE_F9, K_F9) \
	X(SDL_SCANCODE_F10, K_F10) \
	X(SDL_SCANCODE_F11, K_F11) \
	X(SDL_SCANCODE_F12, K_F12) \
	X(SDL_SCANCODE_KP_PLUS, K_ADD) \
	X(SDL_SCANCODE_KP_MINUS, K_SUBTRACT) \
	X(SDL_SCANCODE_KP_MULTIPLY, K_MULTIPLY) \
	X(SDL_SCANCODE_ESCAPE, K_ESCAPE) \
	X(SDL_SCANCODE_PAUSE, K_PAUSE) \
	X(SDL_SCANCODE_TAB, K_TAB) \
	X(SDL_SCANCODE_RETURN, K_RETURN) \
	X(SDL_SCANCODE_DELETE, K_DELETE) \
	X(SDL_SCANCODE_INSERT, K_INSERT) \
	X(SDL_SCANCODE_BACKSPACE, K_BACK) \
	X(SDL_SCANCODE_SPACE, K_SPACE) \
	X(SDL_SCANCODE_UP, K_UP) \
	X(SDL_SCANCODE_DOWN, K_DOWN) \
	X(SDL_SCANCODE_LEFT, K_LEFT) \
	X(SDL_SCANCODE_RIGHT, K_RIGHT) \
	X(SDL_SCANCODE_HOME, K_HOME) \
	X(SDL_SCANCODE_END, K_END) \
	X(SDL_SCANCODE_SCROLLLOCK, K_SCROLL) \
	X(SDL_SCANCODE_MENU, K_MENU) \
	X(SDL_SCANCODE_PAGEUP, K_PAGEUP) \
	X(SDL_SCANCODE_PAGEDOWN, K_PAGEDOWN) \
	X(SDL_SCANCODE_1, K_1) \
	X(SDL_SCANCODE_2, K_2) \
	X(SDL_SCANCODE_3, K_3) \
	X(SDL_SCANCODE_4, K_4) \
	X(SDL_SCANCODE_5, K_5) \
	X(SDL_SCANCODE_6, K_6) \
	X(SDL_SCANCODE_7, K_7) \
	X(SDL_SCANCODE_8, K_8) \
	X(SDL_SCANCODE_9, K_9) \
	X(SDL_SCANCODE_0, K_0) \
	X(SDL_SCANCODE_A, K_A) \
	X(SDL_SCANCODE_B, K_B) \
	X(SDL_SCANCODE_C, K_C) \
	X(SDL_SCANCODE_D, K_D) \
	X(SDL_SCANCODE_E, K_E) \
	X(SDL_SCANCODE_F, K_F) \
	X(SDL_SCANCODE_G, K_G) \
	X(SDL_SCANCODE_H, K_H) \
	X(SDL_SCANCODE_I, K_I) \
	X(SDL_SCANCODE_J, K_J) \
	X(SDL_SCANCODE_K, K_K) \
	X(SDL_SCANCODE_L, K_L) \
	X(SDL_SCANCODE_M, K_M) \
	X(SDL_SCANCODE_N, K_N) \
	X(SDL_SCANCODE_O, K_O) \
	X(SDL_SCANCODE_P, K_P) \
	X(SDL_SCANCODE_Q, K_Q) \
	X(SDL_SCANCODE_R, K_R) \
	X(SDL_SCANCODE_S, K_S) \
	X(SDL_SCANCODE_T, K_T) \
	X(SDL_SCANCODE_U, K_U) \
	X(SDL_SCANCODE_V, K_V) \
	X(SDL_SCANCODE_W, K_W) \
	X(SDL_SCANCODE_X, K_X) \
	X(SDL_SCANCODE_Y, K_Y) \
	X(SDL_SCANCODE_Z, K_Z) \
	X(SDL_SCANCODE_MINUS, K_MINUS) \
	X(SDL_SCANCODE_EQUALS, K_EQUAL) \
	X(SDL_SCANCODE_LEFTBRACKET, K_LEFT_BRACKET) \
	X(SDL_SCANCODE_RIGHTBRACKET, K_RIGHT_BRACKET) \
	X(SDL_SCANCODE_SEMICOLON, K_SEMICOLON) \
	X(SDL_SCANCODE_APOSTROPHE, K_APOSTROPHE) \
	X(SDL_SCANCODE_GRAVE, K_GRAVE_ACCENT) \
	X(SDL_SCANCODE_BACKSLASH, K_BACKSLASH) \
	X(SDL_SCANCODE_COMMA, K_COMMA) \
	X(SDL_SCANCODE_PERIOD, K_PERIOD) \
	X(SDL_SCANCODE_SLASH, K_SLASH) \
	X(SDL_SCANCODE_CAPSLOCK, K_CAPS) \
	X(SDL_SCANCODE_NUMLOCKCLEAR, K_NUM) \
	X(SDL_SCANCODE_KP_7, K_NUM7) \
	X(SDL_SCANCODE_KP_8, K_NUM8) \
	X(SDL_SCANCODE_KP_9, K_NUM9) \
	X(SDL_SCANCODE_KP_4, K_NUM4) \
	X(SDL_SCANCODE_KP_5, K_NUM5) \
	X(SDL_SCANCODE_KP_6, K_NUM6) \
	X(SDL_SCANCODE_KP_1, K_NUM1) \
	X(SDL_SCANCODE_KP_2, K_NUM2) \
	X(SDL_SCANCODE_KP_3, K_NUM3) \
	X(SDL_SCANCODE_KP_0, K_NUM0) \
	X(SDL_SCANCODE_KP_PERIOD, K_DECIMAL) \
	X(SDL_SCANCODE_NONUSBACKSLASH, K_86) \
	X(SDL_SCANCODE_KP_ENTER, K_NUM_RETURN) \
	X(SDL_SCANCODE_KP_DIVIDE, K_DIVIDE) \
	X(SDL_SCANCODE_LGUI, K_WIN_L) \
	X(SDL_SCANCODE_RGUI, K_WIN_R) \
	X(SDL_SCANCODE_PRINTSCREEN, K_PRINT) \

static C4KeyCode sdl_scancode_to_keycode(SDL_Scancode scancode)
{
	switch (scancode)
	{
#define X(sdl, oc) case sdl: return oc;
	SDL_SCANCODE_KEYCODE
#undef X
		default: return 0; // silence warnings.
	}
}

const char* KeycodeToString(C4KeyCode code)
{
	SDL_Scancode scancode;
	switch (code)
	{
#define X(sdl, oc) case oc: scancode = sdl; break;
	SDL_SCANCODE_KEYCODE
#undef X
	default:
		return nullptr;
	}
	return SDL_GetScancodeName(scancode);
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
		Game.DoKeyboardInput(sdl_scancode_to_keycode(e.key.keysym.scancode), KEYEV_Down,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT),
		                     e.key.repeat > 0, nullptr);
		break;
	}
	case SDL_KEYUP:
		Game.DoKeyboardInput(sdl_scancode_to_keycode(e.key.keysym.scancode), KEYEV_Up,
		                     e.key.keysym.mod & (KMOD_LALT | KMOD_RALT),
		                     e.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL),
		                     e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT), false, nullptr);
		break;
	case SDL_MOUSEMOTION:
		C4GUI::MouseMove(C4MC_Button_None, e.motion.x, e.motion.y, 0, nullptr);
		break;
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		int32_t button;
		sdlToC4MCBtn(e.button, button, flags);
		C4GUI::MouseMove(button, e.button.x, e.button.y, flags, nullptr);
		break;
	case SDL_MOUSEWHEEL:
		flags = e.wheel.y > 0 ? (+32) << 16 : (DWORD) (-32) << 16;
		flags += SDL_GetModState();
		int x, y;
		SDL_GetMouseState(&x, &y);
		C4GUI::MouseMove(C4MC_Button_Wheel, x, y, flags, nullptr);
		break;
	case SDL_CONTROLLERAXISMOTION:
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		Application.pGamePadControl->FeedEvent(e, C4GamePadControl::FEED_BUTTONS);
		break;
	case SDL_JOYDEVICEADDED:
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_CONTROLLERDEVICEREMOVED:
		Application.pGamePadControl->CheckGamePad(e);
		break;
	case SDL_WINDOWEVENT:
		// Forward to C4Window instance.
		auto window = static_cast<C4Window*>(SDL_GetWindowData(SDL_GetWindowFromID(e.window.windowID), "C4Window"));
		window->HandleSDLEvent(e.window);
		break;
	}
}

static int modeCount = 0;

static int bits_per_pixel(int format)
{
	// C4Draw::BITS_PER_PIXEL is 32, and other parts of the code expect
	// the mode's bpp to match exactly. 24 is fully compatible, so just
	// pretend it's 32 in that case to adhere to the expected interface.
	int bbp = SDL_BITSPERPIXEL(format);
	if (bbp == 24) bbp = 32;
	return bbp;
}

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
	*piBitDepth = bits_per_pixel(mode.format);
	if (piRefreshRate) *piRefreshRate = mode.refresh_rate;
	return true;
}

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int RefreshRate,  unsigned int iMonitor, bool fFullScreen)
{
	int res;
	if (!fFullScreen)
	{
		if (iXRes == -1)
		{
			SDL_DisplayMode desktop_mode;
			res = SDL_GetDesktopDisplayMode(iMonitor, &desktop_mode);
			if (res)
			{
				Error(SDL_GetError());
				LogF("SDL_GetDesktopDisplayMode: %s", SDL_GetError());
				return false;
			}

			iXRes = desktop_mode.w;
			iYRes = desktop_mode.h;
		}

		res = SDL_SetWindowFullscreen(pWindow->window, 0);
		if (res)
		{
			Error(SDL_GetError());
			LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
			return false;
		}

		pWindow->SetSize(iXRes, iYRes);
		OnResolutionChanged(iXRes, iYRes);
		return true;
	}
	SDL_DisplayMode mode;
	if (iXRes < 0 || iYRes < 0)
	{
		res = SDL_SetWindowFullscreen(pWindow->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		if (res)
		{
			Error(SDL_GetError());
			LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
			return false;
		}
		res = SDL_GetDesktopDisplayMode(iMonitor, &mode);
		if (res)
		{
			Error(SDL_GetError());
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
			Error(SDL_GetError());
			LogF("SDL_GetDisplayMode: %s", SDL_GetError());
			return false;
		}

		if (mode.w == iXRes && mode.h == iYRes && (RefreshRate == 0 || mode.refresh_rate == RefreshRate) && bits_per_pixel(mode.format) == C4Draw::COLOR_DEPTH)
		{
			res = SDL_SetWindowDisplayMode(pWindow->window, &mode);
			if (res)
			{
				Error(SDL_GetError());
				LogF("SDL_SetWindowDisplayMode: %s", SDL_GetError());
				return false;
			}
			res = SDL_SetWindowFullscreen(pWindow->window, SDL_WINDOW_FULLSCREEN);
			if (res)
			{
				Error(SDL_GetError());
				LogF("SDL_SetWindowFullscreen: %s", SDL_GetError());
				return false;
			}
			OnResolutionChanged(mode.w, mode.h);
			return true;
		}
	}

	Error("No such resolution available");
	return false;
}

void C4AbstractApp::RestoreVideoMode()
{
	if (pWindow && pWindow->window)
		SDL_SetWindowFullscreen(pWindow->window, 0);
}

bool C4AbstractApp::Copy(const std::string &text, bool fClipboard)
{
	return SDL_SetClipboardText(text.c_str()) == 0;
}

std::string C4AbstractApp::Paste(bool fClipboard)
{
	char * text = SDL_GetClipboardText();
	std::string buf(text);
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
