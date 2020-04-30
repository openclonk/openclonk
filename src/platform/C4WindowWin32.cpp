/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "platform/C4Window.h"

#include "C4Version.h"
#include "editor/C4Console.h"
#include "editor/C4ViewportWindow.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4DrawGL.h"
#include "gui/C4MouseControl.h"
#include "lib/C4Rect.h"
#include "platform/C4AppWin32Impl.h"
#include "platform/StdRegistry.h"
#include "res/resource.h"

#include "platform/C4windowswrapper.h"
#include <mmsystem.h>
#include <shellapi.h>

#define C4ViewportClassName L"C4Viewport"
#define C4FullScreenClassName L"C4FullScreen"
#define ConsoleDlgClassName L"C4GUIdlg"
#define ConsoleDlgWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX)

/** Convert certain keys to unix scancodes (those that differ from unix scancodes) */
static void ConvertToUnixScancode(WPARAM wParam, C4KeyCode *scancode, bool extended)
{
	C4KeyCode &s = *scancode;

	switch(wParam)
	{
	case VK_HOME:		s = K_HOME; break;
	case VK_END:		s = K_END; break;
	case VK_PRIOR:		s = K_PAGEUP; break;
	case VK_NEXT:		s = K_PAGEDOWN; break;
	case VK_UP:			s = K_UP; break;
	case VK_DOWN:		s = K_DOWN; break;
	case VK_LEFT:		s = K_LEFT; break;
	case VK_RIGHT:		s = K_RIGHT; break;
	case VK_CLEAR:		s = K_CENTER; break;
	case VK_INSERT:		s = K_INSERT; break;
	case VK_DELETE:		s = K_DELETE; break;
	case VK_LWIN:		s = K_WIN_L; break;
	case VK_RWIN:		s = K_WIN_R; break;
	case VK_MENU:		s = K_MENU; break;
	case VK_PAUSE:		s = K_PAUSE; break;
	case VK_PRINT:		s = K_PRINT; break;
	case VK_RCONTROL:	s = K_CONTROL_R; break;
	case VK_CONTROL:	s = (extended ? K_CONTROL_R : K_CONTROL_L); break;
	case VK_NUMLOCK:	s = K_NUM; break;
	case VK_NUMPAD1:	s = K_NUM1; break;
	case VK_NUMPAD2:	s = K_NUM2; break;
	case VK_NUMPAD3:	s = K_NUM3; break;
	case VK_NUMPAD4:	s = K_NUM4; break;
	case VK_NUMPAD5:	s = K_NUM5; break;
	case VK_NUMPAD6:	s = K_NUM6; break;
	case VK_NUMPAD7:	s = K_NUM7; break;
	case VK_NUMPAD8:	s = K_NUM8; break;
	case VK_NUMPAD9:	s = K_NUM9; break;
	case VK_NUMPAD0:	s = K_NUM0; break;
	}
}

LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool NativeCursorShown = true;

	POINT p;
	p.x = GET_X_LPARAM(lParam);
	p.y = GET_Y_LPARAM(lParam);

	// compute scancode
	C4KeyCode scancode = (((unsigned int)lParam) >> 16) & 0xFF;
	bool extended = ((lParam & 0x01000000) != 0);
	ConvertToUnixScancode(wParam, &scancode, extended);

	// Process message
	switch (uMsg)
	{
	case WM_ACTIVATE:
		wParam = (LOWORD(wParam)==WA_ACTIVE || LOWORD(wParam)==WA_CLICKACTIVE);
		// fall through to next case
	case WM_ACTIVATEAPP:
		Application.Active = wParam != 0;
#ifndef USE_CONSOLE
		if (pGL)
		{
			if (Application.Active)
			{
				// restore textures
				if (Application.FullScreenMode())
				{
					Application.SetVideoMode(Application.GetConfigWidth(), Application.GetConfigHeight(), Config.Graphics.RefreshRate, Config.Graphics.Monitor, true);
				}
			}
			else
			{
				if (Application.FullScreenMode())
				{
					::ChangeDisplaySettings(nullptr, 0);
					::ShowWindow(hwnd, SW_MINIMIZE);
				}
			}
		}
#endif
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
		if (Game.DoKeyboardInput(scancode, KEYEV_Up, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, nullptr))
			return 0;
		break;
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), nullptr))
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), nullptr))
		{
			// Remove handled message from queue to prevent Windows "standard" sound for unprocessed system message
			MSG msg;
			PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE);
			return 0;
		}
		break;
	case WM_CHAR:
	{
		// UTF-8 has 1 to 4 data bytes, and we need a terminating \0
		char c[5] = {0,0,0,0,0};
		if(!WideCharToMultiByte(CP_UTF8, 0L, reinterpret_cast<LPCWSTR>(&wParam), 1, c, 4, nullptr, nullptr))
			return 0;
		// GUI: forward
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
		C4GUI::MouseMove(C4MC_Button_LeftDown,p.x,p.y,wParam, nullptr);
		break;
	case WM_LBUTTONUP: C4GUI::MouseMove(C4MC_Button_LeftUp, p.x, p.y, wParam, nullptr); break;
	case WM_RBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_RightDown, p.x, p.y, wParam, nullptr); break;
	case WM_RBUTTONUP: C4GUI::MouseMove(C4MC_Button_RightUp, p.x, p.y, wParam, nullptr); break;
	case WM_LBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_LeftDouble, p.x, p.y, wParam, nullptr); break;
	case WM_RBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_RightDouble, p.x, p.y, wParam, nullptr); break;
	case WM_MOUSEWHEEL:
		// the coordinates are screen-coordinates here (but only on this uMsg),
		// we need to convert them to client area coordinates
		ScreenToClient(hwnd, &p);
		C4GUI::MouseMove(C4MC_Button_Wheel, p.x, p.y, wParam, nullptr);
		break;
	case WM_MOUSEMOVE:
		C4GUI::MouseMove(C4MC_Button_None, p.x, p.y, wParam, nullptr);
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
			::Application.OnResolutionChanged(p.x, p.y);
			if(Application.pWindow) // this might be called from C4Window::Init in which case Application.pWindow is not yet set
				::SetWindowPos(Application.pWindow->renderwnd, nullptr, 0, 0, p.x, p.y, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
			break;
		}
		break;
	case WM_INPUTLANGCHANGE:
		::Application.OnKeyboardLayoutChanged();
		break;
	case WM_SYSCOMMAND:
		// The user pressed Alt to open the system menu. This enters a modal
		// loop which stops us from event processing, so prevent it. Users
		// can still open the system menu by clicking the window's icon.
		if ((wParam & 0xFFF0) == SC_KEYMENU && lParam == 0)
			return 0;
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static C4KeyCode msg2scancode(MSG *msg)
{
	// compute scancode
	C4KeyCode scancode = (((unsigned int)msg->lParam) >> 16) & 0xFF;
	bool extended = ((msg->lParam & 0x01000000) != 0);
	ConvertToUnixScancode(msg->wParam, &scancode, extended);
	return scancode;
}

bool ConsoleHandleWin32KeyboardMessage(MSG *msg)
{
	switch (msg->message)
	{
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(msg2scancode(msg), KEYEV_Down, !!(msg->lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(msg->lParam & 0x40000000), nullptr)) return true;
		break;
	case WM_KEYUP:
		if (Game.DoKeyboardInput(msg2scancode(msg), KEYEV_Up, !!(msg->lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, nullptr)) return false;
		break;
	case WM_SYSKEYDOWN:
		if (msg->wParam == 18) break; // VK_MENU (Alt)
		if (Game.DoKeyboardInput(msg2scancode(msg), KEYEV_Down, !!(msg->lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(msg->lParam & 0x40000000), nullptr)) return false;
		break;
	}
	return false;

}

LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Determine viewport
	C4Viewport *cvp;
	if (!(cvp=::Viewports.GetViewport(hwnd)))
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);

	// compute scancode
	C4KeyCode scancode = (((unsigned int)lParam) >> 16) & 0xFF;
	bool extended = ((lParam & 0x01000000) != 0);
	ConvertToUnixScancode(wParam, &scancode, extended);

	// Process message
	switch (uMsg)
	{
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYDOWN:
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		switch (wParam)
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case VK_SCROLL:
			// key bound to this specific viewport. Don't want to pass this through C4Game...
			cvp->TogglePlayerLock();
			break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		default:
			if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), nullptr)) return 0;
			break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		break;
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYUP:
		if (Game.DoKeyboardInput(scancode, KEYEV_Up, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, nullptr)) return 0;
		break;
		//------------------------------------------------------------------------------------------------------------
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), nullptr)) return 0;
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hwnd, FormatString("Viewport%i",cvp->Player+1).getData(), Config.GetSubkeyPath("Console"));
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_CREATE:
		DragAcceptFiles(hwnd, TRUE);
		break;
	case WM_CLOSE:
		cvp->pWindow->Close();
		break;

		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)(HANDLE) wParam;
		if (!Console.Editing) { Console.Message(LoadResStr("IDS_CNS_NONETEDIT")); return false; }

		int32_t iFileNum = DragQueryFile(hDrop,0xFFFFFFFF,nullptr,0);
		POINT pntPoint;
		DragQueryPoint(hDrop,&pntPoint);
		wchar_t szFilename[500+1];
		for (int32_t cnt=0; cnt<iFileNum; cnt++)
		{
			DragQueryFileW(hDrop,cnt,szFilename,500);
			cvp->DropFile(StdStrBuf(szFilename).getData(), (float)pntPoint.x, (float)pntPoint.y);
		}
		DragFinish(hDrop);
		break;
	}
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_USER_DROPDEF:
		Game.DropDef(C4ID(lParam),cvp->GetViewX()+float(LOWORD(wParam))/cvp->Zoom,cvp->GetViewY()+float(HIWORD(wParam)/cvp->Zoom));
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_SIZE:
	case WM_SIZING:
		cvp->UpdateOutputSize();
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_PAINT:
		::GraphicsSystem.Execute();
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION: cvp->SetViewX(float(HIWORD(wParam))/cvp->Zoom); break;
			case SB_LINELEFT: cvp->ScrollView(-ViewportScrollSpeed, 0.0f); break;
			case SB_LINERIGHT: cvp->ScrollView(+ViewportScrollSpeed, 0.0f); break;
			case SB_PAGELEFT: cvp->ScrollView(-cvp->ViewWdt/cvp->Zoom, 0.0f); break;
			case SB_PAGERIGHT: cvp->ScrollView(+cvp->ViewWdt/cvp->Zoom, 0.0f); break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
			case SB_THUMBTRACK:
			case SB_THUMBPOSITION: cvp->SetViewY(float(HIWORD(wParam))/cvp->Zoom); break;
			case SB_LINEUP: cvp->ScrollView(0.0f,-ViewportScrollSpeed); break;
			case SB_LINEDOWN: cvp->ScrollView(0.0f,+ViewportScrollSpeed); break;
			case SB_PAGEUP: cvp->ScrollView(0.0f,-cvp->ViewWdt/cvp->Zoom); break;
			case SB_PAGEDOWN: cvp->ScrollView(0.0f,+cvp->ViewWdt/cvp->Zoom); break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_ACTIVATE:
		// Keep editing dialogs on top of the current viewport, but don't make them
		// float on other windows (i.e., no HWND_TOPMOST).
		// Also, don't use SetParent, since that activates the window, which in turn
		// posts a new WM_ACTIVATE to us, and so on, ultimately leading to a hang.
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			Console.Win32KeepDialogsFloating();
		}
		else
		{
			// FALLTHROUGH
		case WM_MOUSEACTIVATE:
			// WM_MOUSEACTIVATE is emitted when the user hovers over a window and pushes a mouse button.
			// Setting the window owner here avoids z-order flickering.
			Console.Win32KeepDialogsFloating(hwnd);
		}
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_SYSCOMMAND:
		// The user pressed Alt to open the system menu. This enters a modal
		// loop which stops us from event processing, so prevent it. Users
		// can still open the system menu by clicking the window's icon.
		if ((wParam & 0xFFF0) == SC_KEYMENU && lParam == 0)
			return 0;
		break;
	}

	POINT p;
	p.x = GET_X_LPARAM(lParam);
	p.y = GET_Y_LPARAM(lParam);

	// Viewport mouse control
	if (::MouseControl.IsViewport(cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		switch (uMsg)
		{
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_LeftDown, p.x, p.y, wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONUP: C4GUI::MouseMove(C4MC_Button_LeftUp, p.x, p.y, wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_RightDown, p.x, p.y, wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONUP: C4GUI::MouseMove(C4MC_Button_RightUp, p.x, p.y, wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_LeftDouble, p.x, p.y, wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_RightDouble, p.x, p.y, wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEMOVE:
			if ( Inside<int32_t>(p.x-cvp->DrawX,0,cvp->ViewWdt-1)
			     && Inside<int32_t>(p.y-cvp->DrawY,0,cvp->ViewHgt-1) )
				SetCursor(nullptr);
			C4GUI::MouseMove(C4MC_Button_None, p.x, p.y, wParam, cvp);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEWHEEL:
			ScreenToClient(hwnd, &p);
			C4GUI::MouseMove(C4MC_Button_Wheel, p.x, p.y, wParam, cvp);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------

		}
	}
	// Console edit cursor control
	else
	{
		// The state of the ALT key is not reported in wParam for mouse messages,
		// and for keyboard messages the key states are hidden in lParam. It's a mess. Let's just
		// query GetKeyState().
		DWORD dwKeyState = 0;
		if(GetKeyState(VK_CONTROL) < 0) dwKeyState |= MK_CONTROL;
		if(GetKeyState(VK_SHIFT) < 0) dwKeyState |= MK_SHIFT;
		if(GetKeyState(VK_MENU) < 0) dwKeyState |= MK_ALT;

		switch (uMsg)
		{
		case WM_KEYDOWN:
			Console.EditCursor.KeyDown(scancode, dwKeyState);
			break;
		case WM_KEYUP:
			Console.EditCursor.KeyUp(scancode, dwKeyState);
			break;
		case WM_SYSKEYDOWN:
			Console.EditCursor.KeyDown(scancode, dwKeyState);
			break;
		case WM_SYSKEYUP:
			Console.EditCursor.KeyUp(scancode, dwKeyState);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDOWN:
			// movement update needed before, so target is always up-to-date
			cvp->pWindow->EditCursorMove(p.x, p.y, dwKeyState);
			Console.EditCursor.LeftButtonDown(dwKeyState); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONUP: Console.EditCursor.LeftButtonUp(dwKeyState); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDOWN: Console.EditCursor.RightButtonDown(dwKeyState); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONUP: Console.EditCursor.RightButtonUp(dwKeyState); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEMOVE: cvp->pWindow->EditCursorMove(p.x, p.y, dwKeyState); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT APIENTRY DialogWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Determine dialog
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(hwnd);
	if (!pDlg) return DefWindowProc(hwnd, uMsg, wParam, lParam);

	POINT p;
	p.x = GET_X_LPARAM(lParam);
	p.y = GET_Y_LPARAM(lParam);

	// compute scancode
	C4KeyCode scancode = (((unsigned int)lParam) >> 16) & 0xFF;
	bool extended = ((lParam & 0x01000000) != 0);
	ConvertToUnixScancode(wParam, &scancode, extended);

	// Process message
	switch (uMsg)
	{
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), pDlg)) return 0;
		break;
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYUP:
		if (Game.DoKeyboardInput(scancode, KEYEV_Up, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, pDlg)) return 0;
		break;
		//------------------------------------------------------------------------------------------------------------
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), pDlg)) return 0;
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_DESTROY:
	{
		const char *szID = pDlg->GetID();
		if (szID && *szID)
			StoreWindowPosition(hwnd, FormatString("ConsoleGUI_%s", szID).getData(), Config.GetSubkeyPath("Console"), false);
	}
	break;
	//----------------------------------------------------------------------------------------------------------------------------------
	case WM_CLOSE:
		pDlg->Close(false);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_SIZE:
		// UpdateOutputSize
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_PAINT:
		// 2do: only draw specific dlg?
		break;
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONDOWN: ::pGUI->MouseInput(C4MC_Button_LeftDown, p.x, p.y, wParam, pDlg, nullptr); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONUP: ::pGUI->MouseInput(C4MC_Button_LeftUp, p.x, p.y, wParam, pDlg, nullptr); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONDOWN: ::pGUI->MouseInput(C4MC_Button_RightDown, p.x, p.y, wParam, pDlg, nullptr); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONUP: ::pGUI->MouseInput(C4MC_Button_RightUp, p.x, p.y, wParam, pDlg, nullptr); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONDBLCLK: ::pGUI->MouseInput(C4MC_Button_LeftDouble, p.x, p.y, wParam, pDlg, nullptr); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONDBLCLK: ::pGUI->MouseInput(C4MC_Button_RightDouble, p.x, p.y, wParam, pDlg, nullptr);  break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_MOUSEMOVE:
		::pGUI->MouseInput(C4MC_Button_None, p.x, p.y, wParam, pDlg, nullptr);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_MOUSEWHEEL:
		ScreenToClient(hwnd, &p);
		::pGUI->MouseInput(C4MC_Button_Wheel, p.x, p.y, wParam, pDlg, nullptr);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

C4Window::C4Window () = default;
C4Window::~C4Window () = default;

C4Window * C4Window::Init(C4Window::WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size)
{
	Active = true;
	eKind = windowKind;
	if (windowKind == W_Viewport)
	{
#ifdef WITH_QT_EDITOR
		// embed into editor: Viewport widget creation handled by C4ConsoleQt
		::Console.AddViewport(static_cast<C4ViewportWindow *>(this));
		return this;
#else
		static bool fViewportClassRegistered = false;
		if (!fViewportClassRegistered)
		{
			// Register viewport class
			WNDCLASSEXW WndClass;
			WndClass.cbSize=sizeof(WNDCLASSEX);
			WndClass.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
			WndClass.lpfnWndProc   = ViewportWinProc;
			WndClass.cbClsExtra    = 0;
			WndClass.cbWndExtra    = 0;
			WndClass.hInstance     = pApp->GetInstance();
			WndClass.hCursor       = LoadCursor (nullptr, IDC_ARROW);
			WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
			WndClass.lpszMenuName  = nullptr;
			WndClass.lpszClassName = C4ViewportClassName;
			WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_01_OCS) );
			WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_01_OCS) );
			if (!RegisterClassExW(&WndClass)) return nullptr;
			fViewportClassRegistered = true;
		}
		// Create window
		hWindow = CreateWindowExW (
		            WS_EX_ACCEPTFILES,
		            C4ViewportClassName, GetWideChar(Title), C4ViewportWindowStyle,
		            CW_USEDEFAULT,CW_USEDEFAULT, size->Wdt, size->Hgt,
		            Console.hWindow,nullptr,pApp->GetInstance(),nullptr);
		if(!hWindow) return nullptr;
#endif
		// We don't re-init viewport windows currently, so we don't need a child window
		// for now: Render into main window.
		renderwnd = hWindow;
	}
	else if (windowKind == W_Fullscreen)
	{
		// Register window class
		auto WndClass = WNDCLASSEXW();
		WndClass.cbSize        = sizeof(WNDCLASSEX);
		WndClass.style         = CS_DBLCLKS;
		WndClass.lpfnWndProc   = FullScreenWinProc;
		WndClass.hInstance     = pApp->GetInstance();
		WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		WndClass.lpszClassName = C4FullScreenClassName;
		WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
		WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
		if (!RegisterClassExW(&WndClass)) return nullptr;

		// Create window
		hWindow = CreateWindowExW  (
		            0,
		            C4FullScreenClassName,
		            GetWideChar(Title),
		            WS_OVERLAPPEDWINDOW,
		            CW_USEDEFAULT,CW_USEDEFAULT, size->Wdt, size->Hgt,
		            nullptr,nullptr,pApp->GetInstance(),nullptr);
		if(!hWindow) return nullptr;

		RECT rc;
		GetClientRect(hWindow, &rc);
		renderwnd = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD,
		                                0, 0, rc.right - rc.left, rc.bottom - rc.top,
		                                hWindow, nullptr, pApp->GetInstance(), nullptr);
		if(!renderwnd) { DestroyWindow(hWindow); return nullptr; }
		ShowWindow(renderwnd, SW_SHOW);

	#ifndef USE_CONSOLE
		// Show & focus
		ShowWindow(hWindow,SW_SHOWNORMAL);
		SetFocus(hWindow);
	#endif
	}
	else if (windowKind == W_GuiWindow)
	{
		static bool fDialogClassRegistered = false;
		if (!fDialogClassRegistered)
		{
			// register landscape viewport class
			WNDCLASSEXW WndClass;
			WndClass.cbSize=sizeof(WNDCLASSEX);
			WndClass.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
			WndClass.lpfnWndProc   = DialogWinProc;
			WndClass.cbClsExtra    = 0;
			WndClass.cbWndExtra    = 0;
			WndClass.hInstance     = pApp->GetInstance();
			WndClass.hCursor       = LoadCursor (nullptr, IDC_ARROW); // - always use normal hw cursor
			WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
			WndClass.lpszMenuName  = nullptr;
			WndClass.lpszClassName = ConsoleDlgClassName;
			WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
			WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
			if (!RegisterClassExW(&WndClass))
				return nullptr;
		}
		Active = true;
		// calculate required size
		RECT rtSize;
		rtSize.left = 0;
		rtSize.top = 0;
		rtSize.right = size->Wdt;
		rtSize.bottom = size->Hgt;
		if (!::AdjustWindowRectEx(&rtSize, ConsoleDlgWindowStyle, false, 0))
			return nullptr;
		// create it!
		if (!Title || !*Title) Title = "???";
		hWindow = ::CreateWindowExW(
		            0,
		            ConsoleDlgClassName, GetWideChar(Title),
		            ConsoleDlgWindowStyle,
		            CW_USEDEFAULT,CW_USEDEFAULT,rtSize.right-rtSize.left,rtSize.bottom-rtSize.top,
					::Console.hWindow,nullptr,pApp->GetInstance(),nullptr);
		renderwnd = hWindow;
		return hWindow ? this : nullptr;
	}
	else if (windowKind == W_Control)
	{
		// controlled externally
		hWindow = renderwnd = nullptr;
	}
	return this;
}

bool C4Window::ReInit(C4AbstractApp* pApp)
{
	// We don't need to change anything with the window for any
	// configuration option changes on Windows.

	// However, re-create the render window so that another pixel format can
	// be chosen for it. The pixel format is chosen in CStdGLCtx::Init.

	RECT rc;
	GetClientRect(hWindow, &rc);
	HWND hNewRenderWindow = CreateWindowExW(0, L"STATIC", nullptr, WS_CHILD,
	                                        0, 0, rc.right - rc.left, rc.bottom - rc.top,
	                                        hWindow, nullptr, pApp->hInstance, nullptr);
	if(!hNewRenderWindow) return false;

	ShowWindow(hNewRenderWindow, SW_SHOW);
	DestroyWindow(renderwnd);
	renderwnd = hNewRenderWindow;

	return true;
}

void C4Window::Clear()
{
	// Destroy window if we own it
	if (eKind != W_Control)
	{
		if (renderwnd) DestroyWindow(renderwnd);
		if (hWindow && hWindow != renderwnd) DestroyWindow(hWindow);
	}
#ifdef WITH_QT_EDITOR
	if (eKind == W_Viewport)
	{
		// embed into editor: Viewport widget creation handled by C4ConsoleQt
		::Console.RemoveViewport(static_cast<C4ViewportWindow *>(this));
	}
#endif
	renderwnd = nullptr;
	hWindow = nullptr;
}

bool C4Window::StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize)
{
	return StoreWindowPosition(hWindow, szWindowName, szSubKey, fStoreSize) != 0;
}

bool C4Window::RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden)
{
	if (!RestoreWindowPosition(hWindow, szWindowName, szSubKey, fHidden))
		ShowWindow(hWindow,SW_SHOWNORMAL);
	return true;
}

void C4Window::SetTitle(const char *szToTitle)
{
	if (hWindow) SetWindowTextW(hWindow, GetWideChar(szToTitle ? szToTitle : ""));
}

bool C4Window::GetSize(C4Rect * pRect)
{
	RECT r;
	if (!(hWindow && GetClientRect(hWindow,&r))) return false;
	pRect->x = r.left;
	pRect->y = r.top;
	pRect->Wdt = r.right - r.left;
	pRect->Hgt = r.bottom - r.top;
	return true;
}

void C4Window::SetSize(unsigned int cx, unsigned int cy)
{
	if (hWindow)
	{
		// If bordered, add border size
		RECT rect = { 0, 0, static_cast<LONG>(cx), static_cast<LONG>(cy) };
		::AdjustWindowRectEx(&rect, GetWindowLong(hWindow, GWL_STYLE), FALSE, GetWindowLong(hWindow, GWL_EXSTYLE));
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
		::SetWindowPos(hWindow, nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);

		// Also resize child window
		GetClientRect(hWindow, &rect);
		::SetWindowPos(renderwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void C4Window::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
}

void C4Window::GrabMouse(bool grab)
{
	// TODO
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples, int) const
{
#ifndef USE_CONSOLE
	if(pGL && pGL->pMainCtx)
		samples = pGL->pMainCtx->EnumerateMultiSamples();
#endif
}

/* CStdMessageProc */

bool CStdMessageProc::Execute(int iTimeout, pollfd *)
{
	// Peek messages
	MSG msg;
	while (PeekMessage(&msg,nullptr,0,0,PM_REMOVE))
	{
		// quit?
		if (msg.message == WM_QUIT)
		{
			pApp->fQuitMsgReceived = true;
			return false;
		}
		// Dialog message transfer
		if (!pApp->pWindow || !pApp->pWindow->Win32DialogMessageHandling(&msg))
		{
			TranslateMessage(&msg); DispatchMessage(&msg);
		}
	}
	return true;
}

/* C4AbstractApp */

C4AbstractApp::C4AbstractApp()
{
	ZeroMemory(&dspMode, sizeof(dspMode)); dspMode.dmSize =  sizeof(dspMode);
	ZeroMemory(&OldDspMode, sizeof(OldDspMode)); OldDspMode.dmSize =  sizeof(OldDspMode);
	idMainThread = 0;
#ifdef _WIN32
	MessageProc.SetApp(this);
	Add(&MessageProc);
#endif
}

C4AbstractApp::~C4AbstractApp() = default;

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set instance vars
	idMainThread = ::GetCurrentThreadId();
	// Custom initialization
	return DoInit (argc, argv);
}

void C4AbstractApp::Clear()
{
	idMainThread = 0;
}

void C4AbstractApp::Quit()
{
	PostQuitMessage(0);
}

bool C4AbstractApp::FlushMessages()
{

	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;

	return MessageProc.Execute(0);
}

void C4AbstractApp::SetLastErrorFromOS()
{
	LPWSTR buffer = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	sLastError = WStrToString(buffer);
	LocalFree(buffer);
}

int GLMonitorInfoEnumCount;

static BOOL CALLBACK GLMonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// get to indexed monitor
	if (GLMonitorInfoEnumCount--) return true;
	// store it
	C4AbstractApp *pApp = (C4AbstractApp *) dwData;
	pApp->hMon = hMonitor;
	pApp->MonitorRect = *lprcMonitor;
	return true;
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	// prepare search struct
	DEVMODEW dmode;
	ZeroMemory(&dmode, sizeof(dmode)); dmode.dmSize = sizeof(dmode);
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format(R"(\\.\Display%d)", iMonitor+1);
	// check if indexed mode exists
	if (!EnumDisplaySettingsW(Mon.GetWideChar(), iIndex, &dmode))
	{
		SetLastErrorFromOS();
		return false;
	}
	// mode exists; return it
	if (piXRes) *piXRes = dmode.dmPelsWidth;
	if (piYRes) *piYRes = dmode.dmPelsHeight;
	if (piBitDepth) *piBitDepth = dmode.dmBitsPerPel;
	if (piRefreshRate) *piRefreshRate = dmode.dmDisplayFrequency;
	return true;
}

void C4AbstractApp::RestoreVideoMode()
{
}

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
#ifndef USE_CONSOLE
	SetWindowLong(pWindow->hWindow, GWL_EXSTYLE,
	              GetWindowLong(pWindow->hWindow, GWL_EXSTYLE) | WS_EX_APPWINDOW);
	// change mode
	if (!fFullScreen)
	{

		ChangeDisplaySettings(nullptr, 0);
		SetWindowLong(pWindow->hWindow, GWL_STYLE,
		              GetWindowLong(pWindow->hWindow, GWL_STYLE) | (WS_CAPTION|WS_THICKFRAME|WS_BORDER));
		if(iXRes != -1 && iYRes != -1) {
			pWindow->SetSize(iXRes, iYRes);
			OnResolutionChanged(iXRes, iYRes);
		}
		::SetWindowPos(pWindow->hWindow, nullptr, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_FRAMECHANGED);
	}
	else
	{
		bool fFound=false;
		DEVMODEW dmode;
		// if a monitor is given, search on that instead
		// get monitor infos
		GLMonitorInfoEnumCount = iMonitor;
		hMon = nullptr;
		EnumDisplayMonitors(nullptr, nullptr, GLMonitorInfoEnumProc, (LPARAM) this);
		// no monitor assigned?
		if (!hMon)
		{
			// Okay for primary; then just use a default
			if (!iMonitor)
			{
				MonitorRect.left = MonitorRect.top = 0;
				MonitorRect.right = iXRes;
				MonitorRect.bottom = iYRes;
			}
			else return false;
		}
		StdStrBuf Mon;
		if (iMonitor)
			Mon.Format(R"(\\.\Display%d)", iMonitor+1);

		ZeroMemory(&dmode, sizeof(dmode));
		dmode.dmSize = sizeof(dmode);

		// Get current display settings
		if (!EnumDisplaySettingsW(Mon.GetWideChar(), ENUM_CURRENT_SETTINGS, &dmode))
		{
			SetLastErrorFromOS();
			return false;
		}
		unsigned long orientation = dmode.dmDisplayOrientation;
		if (iXRes == -1 && iYRes == -1)
		{
			dspMode=dmode;
			fFound = true;
		}
		// enumerate modes
		int i=0;
		if (!fFound) while (EnumDisplaySettingsW(Mon.GetWideChar(), i++, &dmode))
				// compare enumerated mode with requested settings
				if (static_cast<int>(dmode.dmPelsWidth) == iXRes && static_cast<int>(dmode.dmPelsHeight) == iYRes && dmode.dmBitsPerPel == C4Draw::COLOR_DEPTH && dmode.dmDisplayOrientation == orientation
				        && (iRefreshRate == 0 || dmode.dmDisplayFrequency == iRefreshRate))
				{
					fFound=true;
					dspMode=dmode;
					break;
				}
		if (!fFound) return false;

		dspMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		if (iRefreshRate != 0)
			dspMode.dmFields |= DM_DISPLAYFREQUENCY;
		LONG rv = ChangeDisplaySettingsExW(iMonitor ? Mon.GetWideChar() : nullptr, &dspMode, nullptr, CDS_FULLSCREEN, nullptr);
		if (rv != DISP_CHANGE_SUCCESSFUL)
		{
			switch (rv)
			{
#define CDSE_ERROR(error) case error: sLastError = LoadResStr("IDS_ERR_" #error); break
				CDSE_ERROR(DISP_CHANGE_BADFLAGS);
				CDSE_ERROR(DISP_CHANGE_BADMODE);
				CDSE_ERROR(DISP_CHANGE_BADPARAM);
				CDSE_ERROR(DISP_CHANGE_RESTART);
				CDSE_ERROR(DISP_CHANGE_FAILED);
#undef CDSE_ERROR
			default:
				sLastError = LoadResStr("IDS_ERR_FAILURE");
				break;
			}
			return false;
		}

		SetWindowLong(pWindow->hWindow, GWL_STYLE,
		              GetWindowLong(pWindow->hWindow, GWL_STYLE) & ~ (WS_CAPTION|WS_THICKFRAME|WS_BORDER));

		pWindow->SetSize(dspMode.dmPelsWidth, dspMode.dmPelsHeight);
		OnResolutionChanged(dspMode.dmPelsWidth, dspMode.dmPelsHeight);
		::SetWindowPos(pWindow->hWindow, nullptr, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_FRAMECHANGED);
	}
	return true;
#endif
}

void C4AbstractApp::MessageDialog(const char * message)
{
	MessageBoxW(nullptr, GetWideChar(message), ADDL(C4ENGINECAPTION), MB_ICONERROR);
}

// Clipboard functions
bool C4AbstractApp::Copy(const std::string &text, bool fClipboard)
{
	if (!fClipboard) return false;
	bool fSuccess = true;
	// gain clipboard ownership
	if (!OpenClipboard(pWindow ? pWindow->hWindow : nullptr)) return false;
	// must empty the global clipboard, so the application clipboard equals the Windows clipboard
	EmptyClipboard();
	int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.size() + 1, nullptr, 0);
	HANDLE hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t));
	if (hglbCopy == nullptr) { CloseClipboard(); return false; }
	// lock the handle and copy the text to the buffer.
	wchar_t *szCopyChar = (wchar_t *) GlobalLock(hglbCopy);
	fSuccess = !!MultiByteToWideChar(CP_UTF8, 0, text.c_str(), text.size() + 1, szCopyChar, size);
	GlobalUnlock(hglbCopy);
	// place the handle on the clipboard.
	fSuccess = fSuccess && !!SetClipboardData(CF_UNICODETEXT, hglbCopy);
	// close clipboard
	CloseClipboard();
	// return whether copying was successful
	return fSuccess;
}

std::string C4AbstractApp::Paste(bool fClipboard)
{
	if (!fClipboard) return std::string();
	// open clipboard
	if (!OpenClipboard(nullptr)) return std::string();
	// get text from clipboard
	HANDLE hglb = GetClipboardData(CF_UNICODETEXT);
	if (!hglb) return std::string();
	std::string text{ WStrToString((wchar_t*)GlobalLock(hglb)) };
	// unlock mem
	GlobalUnlock(hglb);
	// close clipboard
	CloseClipboard();
	return text;
}

bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	if (!fClipboard) return false;
	return !!IsClipboardFormatAvailable(CF_UNICODETEXT);
}

void C4Window::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

