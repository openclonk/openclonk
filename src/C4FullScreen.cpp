/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2001-2003, 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005-2007  Günther Brammer
 * Copyright (c) 2006-2007  Julian Raschke
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Main class to execute the game fullscreen mode */

#include <C4Include.h>
#include <C4FullScreen.h>

#include <C4Game.h>
#include <C4Application.h>
#include <C4UserMessages.h>
#include <C4Viewport.h>
#include <C4League.h>
#include <C4Language.h>
#include <C4Gui.h>
#include <C4Network2.h>
#include <C4GameDialogs.h>
#include <C4GamePadCon.h>
#include <C4Player.h>
#include <C4GameOverDlg.h>
#include <C4GraphicsSystem.h>
#include <C4MouseControl.h>
#include <C4PlayerList.h>

#ifdef _WIN32

LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool NativeCursorShown = true;
	// Process message
	switch (uMsg)
	{
	case WM_ACTIVATE:
		wParam = (LOWORD(wParam)==WA_ACTIVE || LOWORD(wParam)==WA_CLICKACTIVE);
		// fall through to next case
	case WM_ACTIVATEAPP:
		Application.Active = wParam != 0;
		if (lpDDraw)
		{
			if (Application.Active)
				lpDDraw->TaskIn();
			else
				lpDDraw->TaskOut();
		}
		// redraw background
		::GraphicsSystem.InvalidateBg();
		// Redraw after task switch
		if (Application.Active)
			::GraphicsSystem.Execute();
		// update cursor clip
		::MouseControl.UpdateClip();
		return false;
	case WM_PAINT:
		// Redraw after task switch
		if (Application.Active)
			::GraphicsSystem.Execute();
		break;
	case WM_DESTROY:
		Application.Quit();
		return 0;
	case WM_CLOSE:
		FullScreen.Close();
		return 0;
	case MM_MCINOTIFY:
		if (wParam == MCI_NOTIFY_SUCCESSFUL)
			Application.MusicSystem.NotifySuccess();
		return true;
	case WM_KEYUP:
		if (Game.DoKeyboardInput(wParam, KEYEV_Up, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), false, NULL))
			return 0;
		break;
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), NULL))
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(wParam, KEYEV_Down, Application.IsAltDown(), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), NULL))
			return 0;
		break;
	case WM_CHAR:
	{
		// UTF-8 has 1 to 4 data bytes, and we need a terminating \0
		char c[5] = {0};
		WCHAR wc[1] = {0};
		if(!MultiByteToWideChar(CP_ACP, 0L, reinterpret_cast<LPCSTR>(&wParam), 1, wc, 1))
			return 0;
		if(!WideCharToMultiByte(CP_UTF8, 0L, wc, 1, c, 4, 0, 0))
			return 0;
		// GUI: forward
		if (::pGUI)
			if (::pGUI->CharIn(c))
				return 0;
		return false;
	}
	case WM_USER_LOG:
		if (SEqual2((const char *)lParam, "IDS_"))
			Log(LoadResStr((const char *)lParam));
		else
			Log((const char *)lParam);
		return false;
	case WM_LBUTTONDOWN:
		C4GUI::MouseMove(C4MC_Button_LeftDown,LOWORD(lParam),HIWORD(lParam),wParam, NULL);
		break;
	case WM_LBUTTONUP: C4GUI::MouseMove(C4MC_Button_LeftUp,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_RBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_RightDown,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_RBUTTONUP: C4GUI::MouseMove(C4MC_Button_RightUp,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_LBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_LeftDouble,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_RBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_RightDouble,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_MOUSEWHEEL: C4GUI::MouseMove(C4MC_Button_Wheel,LOWORD(lParam),HIWORD(lParam),wParam, NULL); break;
	case WM_MOUSEMOVE:
		C4GUI::MouseMove(C4MC_Button_None,LOWORD(lParam),HIWORD(lParam),wParam, NULL);
		// Hide cursor in client area
		if (NativeCursorShown)
		{
			NativeCursorShown = false;
			ShowCursor(FALSE);
		}
		break;
	case WM_NCMOUSEMOVE:
		// Show cursor on window frame
		if (!NativeCursorShown)
		{
			NativeCursorShown = true;
			ShowCursor(TRUE);
		}
		break;
	case WM_SIZE:
		// Notify app about render window size change
		switch (wParam)
		{
		case SIZE_RESTORED:
		case SIZE_MAXIMIZED:
			::Application.OnResolutionChanged(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#elif defined(USE_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
void C4FullScreen::HandleMessage (XEvent &e)
{
	// Parent handling
	CStdWindow::HandleMessage(e);

	switch (e.type)
	{
	case KeyPress:
	{
		// Do not take into account the state of the various modifiers and locks
		// we don't need that for keyboard control
		DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
		Game.DoKeyboardInput(key, KEYEV_Down, Application.IsAltDown(), Application.IsControlDown(), Application.IsShiftDown(), false, NULL);
		break;
	}
	case KeyRelease:
	{
		DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
		Game.DoKeyboardInput(key, KEYEV_Up, e.xkey.state & Mod1Mask, e.xkey.state & ControlMask, e.xkey.state & ShiftMask, false, NULL);
		break;
	}
	case ButtonPress:
	{
		static int last_left_click, last_right_click;
		switch (e.xbutton.button)
		{
		case Button1:
			if (timeGetTime() - last_left_click < 400)
			{
				C4GUI::MouseMove(C4MC_Button_LeftDouble,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
				last_left_click = 0;
			}
			else
			{
				C4GUI::MouseMove(C4MC_Button_LeftDown,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
				last_left_click = timeGetTime();
			}
			break;
		case Button2:
			C4GUI::MouseMove(C4MC_Button_MiddleDown,
			                           e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
			break;
		case Button3:
			if (timeGetTime() - last_right_click < 400)
			{
				C4GUI::MouseMove(C4MC_Button_RightDouble,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
				last_right_click = 0;
			}
			else
			{
				C4GUI::MouseMove(C4MC_Button_RightDown,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
				last_right_click = timeGetTime();
			}
			break;
		case Button4:
			C4GUI::MouseMove(C4MC_Button_Wheel,
			                           e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(32) << 16), NULL);
			break;
		case Button5:
			C4GUI::MouseMove(C4MC_Button_Wheel,
			                           e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(-32) << 16), NULL);
			break;
		default:
			break;
		}
	}
	break;
	case ButtonRelease:
		switch (e.xbutton.button)
		{
		case Button1:
			C4GUI::MouseMove(C4MC_Button_LeftUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
			break;
		case Button2:
			C4GUI::MouseMove(C4MC_Button_MiddleUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
			break;
		case Button3:
			C4GUI::MouseMove(C4MC_Button_RightUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
			break;
		default:
			break;
		}
		break;
	case MotionNotify:
		C4GUI::MouseMove(C4MC_Button_None, e.xbutton.x, e.xbutton.y, e.xbutton.state, NULL);
		break;
	case FocusIn:
		Application.Active = true;
		if (lpDDraw) lpDDraw->TaskIn();
		break;
	case FocusOut: case UnmapNotify:
		Application.Active = false;
		if (lpDDraw) lpDDraw->TaskOut();
		break;
	}
}

#elif defined(USE_SDL_MAINLOOP)
// SDL version

namespace
{
	void sdlToC4MCBtn(const SDL_MouseButtonEvent &e,
	                  int32_t& button, DWORD& flags)
	{
		static int lastLeftClick = 0, lastRightClick = 0;
		static int lastX = 0, lastY = 0;
		static const int clickDist = 2;

		button = C4MC_Button_None;
		flags = 0;

		switch (e.button)
		{
		case SDL_BUTTON_LEFT:
			if (e.state == SDL_PRESSED)
				if (timeGetTime() - lastLeftClick < 400 && abs(lastX-e.x) <= clickDist && abs(lastY-e.y) <= clickDist)
				{
					lastLeftClick = 0;
					button = C4MC_Button_LeftDouble;
				}
				else
				{
					lastLeftClick = timeGetTime();
					button = C4MC_Button_LeftDown;
				}
			else
				button = C4MC_Button_LeftUp;
			break;
		case SDL_BUTTON_RIGHT:
			if (e.state == SDL_PRESSED)
				if (timeGetTime() - lastRightClick < 400)
				{
					lastRightClick = 0;
					button = C4MC_Button_RightDouble;
				}
				else
				{
					lastRightClick = timeGetTime();
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

	bool isSpecialKey(unsigned unicode)
	{
		if (unicode >= 0xe00)
			return true;
		if (unicode < 32 || unicode == 127)
			return true;
		return false;
	}
}

#include "StdGL.h"

void C4FullScreen::HandleMessage (SDL_Event &e)
{
	switch (e.type)
	{
	case SDL_KEYDOWN:
	{
#ifdef USE_GL
		if (e.key.keysym.sym == SDLK_f && (e.key.keysym.mod & (KMOD_LMETA | KMOD_RMETA)))
		{
			Config.Graphics.Windowed = !Config.Graphics.Windowed;
			Application.SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.Monitor, !Config.Graphics.Windowed);
			lpDDraw->InvalidateDeviceObjects();
			lpDDraw->RestoreDeviceObjects();

			break;
		}
#endif

		// Only forward real characters to UI. (Nothing outside of "private use" range.)
		// This works without iconv for some reason. Yay!
		// FIXME: convert to UTF-8
		char c[2];
		c[0] = e.key.keysym.unicode;
		c[1] = 0;
		if (::pGUI && !isSpecialKey(e.key.keysym.unicode))
			::pGUI->CharIn(c);
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
}

#endif // _WIN32, USE_X11, USE_SDL_MAINLOOP

void C4FullScreen::CharIn(const char * c) { ::pGUI->CharIn(c); }

C4FullScreen::C4FullScreen()
{
	pMenu = NULL;
}

C4FullScreen::~C4FullScreen()
{
	if (pMenu) delete pMenu;
	if (pSurface) delete pSurface;
}

void C4FullScreen::Close()
{
	if (Game.IsRunning)
		ShowAbortDlg();
	else
		Application.Quit();
}

void C4FullScreen::Clear()
{
	if (pSurface) delete pSurface;
	pSurface = 0;
	CStdWindow::Clear();
}

void C4FullScreen::Execute()
{
	// Execute menu
	if (pMenu) pMenu->Execute();
	// Draw
	::GraphicsSystem.Execute();
}

bool C4FullScreen::ViewportCheck()
{
	int iPlrNum; C4Player *pPlr;
	// Not active
	if (!Active) return false;
	// Determine film mode
	bool fFilm = (Game.C4S.Head.Replay && Game.C4S.Head.Film);
	// Check viewports
	switch (::Viewports.GetViewportCount())
	{
		// No viewports: create no-owner viewport
	case 0:
		iPlrNum = NO_OWNER;
		// Film mode: create viewport for first player (instead of no-owner)
		if (fFilm)
			if ((pPlr = ::Players.First))
				iPlrNum = pPlr->Number;
		// Create viewport
		::Viewports.CreateViewport(iPlrNum, iPlrNum==NO_OWNER);
		// Non-film (observer mode)
		if (!fFilm)
		{
			// Activate mouse control
			::MouseControl.Init(iPlrNum);
			// Display message for how to open observer menu (this message will be cleared if any owned viewport opens)
			StdStrBuf sKey;
			sKey.Format("<c ffff00><%s></c>", Game.KeyboardInput.GetKeyCodeNameByKeyName("FullscreenMenuOpen", false).getData());
			::GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_PRESSORPUSHANYGAMEPADBUTT"), sKey.getData()).getData());
		}
		break;
		// One viewport: do nothing
	case 1:
		break;
		// More than one viewport: remove all no-owner viewports
	default:
		::Viewports.CloseViewport(NO_OWNER, true);
		break;
	}
	// Look for no-owner viewport
	C4Viewport *pNoOwnerVp = ::Viewports.GetViewport(NO_OWNER);
	// No no-owner viewport found
	if (!pNoOwnerVp)
	{
		// Close any open fullscreen menu
		CloseMenu();
	}
	// No-owner viewport present
	else
	{
		// movie mode: player present, and no valid viewport assigned?
		if (Game.C4S.Head.Replay && Game.C4S.Head.Film && (pPlr = ::Players.First))
			// assign viewport to joined player
			pNoOwnerVp->Init(pPlr->Number, true);
	}
	// Done
	return true;
}

bool C4FullScreen::ShowAbortDlg()
{
	// no gui?
	if (!::pGUI) return false;
	// abort dialog already shown
	if (C4AbortGameDialog::IsShown()) return false;
	// not while game over dialog is open
	if (C4GameOverDlg::IsShown()) return false;
	// show abort dialog
	return ::pGUI->ShowRemoveDlg(new C4AbortGameDialog());
}

bool C4FullScreen::ActivateMenuMain()
{
	// Not during game over dialog
	if (C4GameOverDlg::IsShown()) return false;
	// Close previous
	CloseMenu();
	// Open menu
	pMenu = new C4MainMenu();
	return pMenu->ActivateMain(NO_OWNER);
}

void C4FullScreen::CloseMenu()
{
	if (pMenu)
	{
		if (pMenu->IsActive()) pMenu->Close(false);
		delete pMenu;
		pMenu = NULL;
	}
}

bool C4FullScreen::MenuKeyControl(BYTE byCom)
{
	if (pMenu) return pMenu->KeyControl(byCom);
	return false;
}
