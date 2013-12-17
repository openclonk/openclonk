/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include "C4Include.h"
#include <C4Window.h>

#include <C4Application.h>
#include <C4AppWin32Impl.h>
#include <C4Config.h>
#include <C4Console.h>
#include <C4DrawGL.h>
#include <C4FullScreen.h>
#include <C4GraphicsSystem.h>
#include <C4MouseControl.h>
#include <C4Rect.h>
#include <C4Version.h>
#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <StdRegistry.h>
#include "resource.h"

#include <C4windowswrapper.h>
#include <mmsystem.h>
#include <shellapi.h>

#define C4ViewportClassName L"C4Viewport"
#define C4FullScreenClassName L"C4FullScreen"
#define ConsoleDlgClassName L"C4GUIdlg"
#define ConsoleDlgWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX)

/** Convert certain keys to unix scancodes (those that differ from unix scancodes) */
static void ConvertToUnixScancode(WPARAM wParam, C4KeyCode *scancode)
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
	ConvertToUnixScancode(wParam, &scancode);

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
				if (pTexMgr) pTexMgr->IntUnlock();
				if (Application.FullScreenMode())
				{
					Application.SetVideoMode(Application.GetConfigWidth(), Application.GetConfigHeight(), Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, true);
				}
			}
			else
			{
				if (pTexMgr)
					pTexMgr->IntLock();
				if (pGL)
					pGL->TaskOut();
				if (Application.FullScreenMode())
				{
					::ChangeDisplaySettings(NULL, 0);
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
		if (Game.DoKeyboardInput(scancode, KEYEV_Up, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, NULL))
			return 0;
		break;
	case WM_KEYDOWN:
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), NULL))
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), NULL))
			return 0;
		break;
	case WM_CHAR:
	{
		// UTF-8 has 1 to 4 data bytes, and we need a terminating \0
		char c[5] = {0};
		if(!WideCharToMultiByte(CP_UTF8, 0L, reinterpret_cast<LPCWSTR>(&wParam), 1, c, 4, 0, 0))
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
		C4GUI::MouseMove(C4MC_Button_LeftDown,p.x,p.y,wParam, NULL);
		break;
	case WM_LBUTTONUP: C4GUI::MouseMove(C4MC_Button_LeftUp, p.x, p.y, wParam, NULL); break;
	case WM_RBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_RightDown, p.x, p.y, wParam, NULL); break;
	case WM_RBUTTONUP: C4GUI::MouseMove(C4MC_Button_RightUp, p.x, p.y, wParam, NULL); break;
	case WM_LBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_LeftDouble, p.x, p.y, wParam, NULL); break;
	case WM_RBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_RightDouble, p.x, p.y, wParam, NULL); break;
	case WM_MOUSEWHEEL:
		// the coordinates are screen-coordinates here (but only on this uMsg),
		// we need to convert them to client area coordinates
		ScreenToClient(hwnd, &p);
		C4GUI::MouseMove(C4MC_Button_Wheel, p.x, p.y, wParam, NULL);
		break;
	case WM_MOUSEMOVE:
		C4GUI::MouseMove(C4MC_Button_None, p.x, p.y, wParam, NULL);
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
				::SetWindowPos(Application.pWindow->hRenderWindow, NULL, 0, 0, p.x, p.y, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
			break;
		}
		break;
	case WM_INPUTLANGCHANGE:
		::Application.OnKeyboardLayoutChanged();
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Determine viewport
	C4Viewport *cvp;
	if (!(cvp=::Viewports.GetViewport(hwnd)))
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);

	// compute scancode
	C4KeyCode scancode = (((unsigned int)lParam) >> 16) & 0xFF;
	ConvertToUnixScancode(wParam, &scancode);

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
			if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), NULL)) return 0;
			break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		break;
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYUP:
		if (Game.DoKeyboardInput(scancode, KEYEV_Up, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, false, NULL)) return 0;
		break;
		//------------------------------------------------------------------------------------------------------------
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(scancode, KEYEV_Down, !!(lParam & 0x20000000), GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0, !!(lParam & 0x40000000), NULL)) return 0;
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

		int32_t iFileNum = DragQueryFile(hDrop,0xFFFFFFFF,NULL,0);
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
		Game.DropDef(C4ID(lParam),cvp->ViewX+float(LOWORD(wParam))/cvp->Zoom,cvp->ViewY+float(HIWORD(wParam)/cvp->Zoom));
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_SIZE:
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
		case SB_THUMBTRACK: case SB_THUMBPOSITION: cvp->ViewX=HIWORD(wParam); break;
		case SB_LINELEFT: cvp->ViewX-=ViewportScrollSpeed; break;
		case SB_LINERIGHT: cvp->ViewX+=ViewportScrollSpeed; break;
		case SB_PAGELEFT: cvp->ViewX-=cvp->ViewWdt; break;
		case SB_PAGERIGHT: cvp->ViewX+=cvp->ViewWdt; break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION: cvp->ViewY=HIWORD(wParam); break;
		case SB_LINEUP: cvp->ViewY-=ViewportScrollSpeed; break;
		case SB_LINEDOWN: cvp->ViewY+=ViewportScrollSpeed; break;
		case SB_PAGEUP: cvp->ViewY-=cvp->ViewWdt; break;
		case SB_PAGEDOWN: cvp->ViewY+=cvp->ViewWdt; break;
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
				SetCursor(NULL);
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
	ConvertToUnixScancode(wParam, &scancode);

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
		//::GraphicsSystem.Execute();
		break;
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONDOWN: ::pGUI->MouseInput(C4MC_Button_LeftDown, p.x, p.y, wParam, pDlg, NULL); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONUP: ::pGUI->MouseInput(C4MC_Button_LeftUp, p.x, p.y, wParam, pDlg, NULL); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONDOWN: ::pGUI->MouseInput(C4MC_Button_RightDown, p.x, p.y, wParam, pDlg, NULL); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONUP: ::pGUI->MouseInput(C4MC_Button_RightUp, p.x, p.y, wParam, pDlg, NULL); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_LBUTTONDBLCLK: ::pGUI->MouseInput(C4MC_Button_LeftDouble, p.x, p.y, wParam, pDlg, NULL); break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_RBUTTONDBLCLK: ::pGUI->MouseInput(C4MC_Button_RightDouble, p.x, p.y, wParam, pDlg, NULL);  break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_MOUSEMOVE:
		//SetCursor(NULL);
		::pGUI->MouseInput(C4MC_Button_None, p.x, p.y, wParam, pDlg, NULL);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_MOUSEWHEEL:
		ScreenToClient(hwnd, &p);
		::pGUI->MouseInput(C4MC_Button_Wheel, p.x, p.y, wParam, pDlg, NULL);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

C4Window::C4Window (): Active(false), pSurface(0), hWindow(0)
{
}
C4Window::~C4Window ()
{
}

C4Window * C4Window::Init(C4Window::WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size)
{
	Active = true;
	if (windowKind == W_Viewport)
	{
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
			WndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
			WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
			WndClass.lpszMenuName  = NULL;
			WndClass.lpszClassName = C4ViewportClassName;
			WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_01_OCS) );
			WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_01_OCS) );
			if (!RegisterClassExW(&WndClass)) return NULL;
			fViewportClassRegistered = true;
		}
		// Create window
		hWindow = CreateWindowExW (
		            WS_EX_ACCEPTFILES,
		            C4ViewportClassName, GetWideChar(Title), C4ViewportWindowStyle,
		            CW_USEDEFAULT,CW_USEDEFAULT, size->Wdt, size->Hgt,
		            Console.hWindow,NULL,pApp->GetInstance(),NULL);
		if(!hWindow) return NULL;

		// We don't re-init viewport windows currently, so we don't need a child window
		// for now: Render into main window.
		hRenderWindow = hWindow;
	}
	else if (windowKind == W_Fullscreen)
	{
		// Register window class
		WNDCLASSEXW WndClass = {0};
		WndClass.cbSize        = sizeof(WNDCLASSEX);
		WndClass.style         = CS_DBLCLKS;
		WndClass.lpfnWndProc   = FullScreenWinProc;
		WndClass.hInstance     = pApp->GetInstance();
		WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		WndClass.lpszClassName = C4FullScreenClassName;
		WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
		WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
		if (!RegisterClassExW(&WndClass)) return NULL;

		// Create window
		hWindow = CreateWindowExW  (
		            0,
		            C4FullScreenClassName,
		            GetWideChar(Title),
		            WS_OVERLAPPEDWINDOW,
		            CW_USEDEFAULT,CW_USEDEFAULT, size->Wdt, size->Hgt,
		            NULL,NULL,pApp->GetInstance(),NULL);
		if(!hWindow) return NULL;

		RECT rc;
		GetClientRect(hWindow, &rc);
		hRenderWindow = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD,
		                                0, 0, rc.right - rc.left, rc.bottom - rc.top,
		                                hWindow, NULL, pApp->GetInstance(), NULL);
		if(!hRenderWindow) { DestroyWindow(hWindow); return NULL; }
		ShowWindow(hRenderWindow, SW_SHOW);

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
			WndClass.hCursor       = LoadCursor (NULL, IDC_ARROW); // - always use normal hw cursor
			WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
			WndClass.lpszMenuName  = NULL;
			WndClass.lpszClassName = ConsoleDlgClassName;
			WndClass.hIcon         = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
			WndClass.hIconSm       = LoadIcon (pApp->GetInstance(), MAKEINTRESOURCE (IDI_00_C4X) );
			if (!RegisterClassExW(&WndClass))
				return NULL;
		}
		Active = true;
		// calculate required size
		RECT rtSize;
		rtSize.left = 0;
		rtSize.top = 0;
		rtSize.right = size->Wdt;
		rtSize.bottom = size->Hgt;
		if (!::AdjustWindowRectEx(&rtSize, ConsoleDlgWindowStyle, false, 0))
			return NULL;
		// create it!
		if (!Title || !*Title) Title = "???";
		hWindow = ::CreateWindowExW(
		            0,
		            ConsoleDlgClassName, GetWideChar(Title),
		            ConsoleDlgWindowStyle,
		            CW_USEDEFAULT,CW_USEDEFAULT,rtSize.right-rtSize.left,rtSize.bottom-rtSize.top,
					::Console.hWindow,NULL,pApp->GetInstance(),NULL);
		hRenderWindow = hWindow;
		return hWindow ? this : 0;
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
	HWND hNewRenderWindow = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD,
	                                        0, 0, rc.right - rc.left, rc.bottom - rc.top,
	                                        hWindow, NULL, pApp->hInstance, NULL);
	if(!hNewRenderWindow) return false;

	ShowWindow(hNewRenderWindow, SW_SHOW);
	DestroyWindow(hRenderWindow);
	hRenderWindow = hNewRenderWindow;

	return true;
}

void C4Window::Clear()
{
	// Destroy window
	if (hRenderWindow) DestroyWindow(hRenderWindow);
	if (hWindow && hWindow != hRenderWindow) DestroyWindow(hWindow);
	hRenderWindow = NULL;
	hWindow = NULL;
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
	if (hWindow) SetWindowTextW(hWindow, szToTitle ? GetWideChar(szToTitle) : L"");
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
		RECT rect = {0, 0, cx, cy};
		::AdjustWindowRectEx(&rect, GetWindowLong(hWindow, GWL_STYLE), FALSE, GetWindowLong(hWindow, GWL_EXSTYLE));
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
		::SetWindowPos(hWindow, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);

		// Also resize child window
		GetClientRect(hWindow, &rect);
		::SetWindowPos(hRenderWindow, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void C4Window::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
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
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
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

C4AbstractApp::C4AbstractApp() :
		Active(false), pWindow(NULL), fQuitMsgReceived(false),
		hInstance(NULL), fDspModeSet(false)
{
	ZeroMemory(&pfd, sizeof(pfd)); pfd.nSize = sizeof(pfd);
	ZeroMemory(&dspMode, sizeof(dspMode)); dspMode.dmSize =  sizeof(dspMode);
	ZeroMemory(&OldDspMode, sizeof(OldDspMode)); OldDspMode.dmSize =  sizeof(OldDspMode);
	hMainThread = NULL;
#ifdef _WIN32
	MessageProc.SetApp(this);
	Add(&MessageProc);
#endif
}

C4AbstractApp::~C4AbstractApp()
{
}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set instance vars
	hMainThread = ::GetCurrentThread();
	// Custom initialization
	return DoInit (argc, argv);
}

void C4AbstractApp::Clear()
{
	hMainThread = NULL;
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
	LPWSTR buffer = 0;
	DWORD rv = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&buffer), 0, 0);
	sLastError.Take(StdStrBuf(buffer));
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
		Mon.Format("\\\\.\\Display%d", iMonitor+1);
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

bool C4AbstractApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
#ifndef USE_CONSOLE
	SetWindowLong(pWindow->hWindow, GWL_EXSTYLE,
	              GetWindowLong(pWindow->hWindow, GWL_EXSTYLE) | WS_EX_APPWINDOW);
	// change mode
	if (!fFullScreen)
	{

		ChangeDisplaySettings(NULL, 0);
		SetWindowLong(pWindow->hWindow, GWL_STYLE,
		              GetWindowLong(pWindow->hWindow, GWL_STYLE) | (WS_CAPTION|WS_THICKFRAME|WS_BORDER));
		if(iXRes != -1 && iYRes != -1) {
			pWindow->SetSize(iXRes, iYRes);
			OnResolutionChanged(iXRes, iYRes);
		}
		::SetWindowPos(pWindow->hWindow, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_FRAMECHANGED);
	}
	else
	{
		bool fFound=false;
		DEVMODEW dmode;
		// if a monitor is given, search on that instead
		// get monitor infos
		GLMonitorInfoEnumCount = iMonitor;
		hMon = NULL;
		EnumDisplayMonitors(NULL, NULL, GLMonitorInfoEnumProc, (LPARAM) this);
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
			Mon.Format("\\\\.\\Display%d", iMonitor+1);

		ZeroMemory(&dmode, sizeof(dmode));
		dmode.dmSize = sizeof(dmode);

		// Get current display settings
		if (!EnumDisplaySettingsW(Mon.GetWideChar(), ENUM_CURRENT_SETTINGS, &dmode))
		{
			SetLastErrorFromOS();
			return false;
		}
		int orientation = dmode.dmDisplayOrientation;
		if (iXRes == -1 && iYRes == -1)
		{
			dspMode=dmode;
			fFound = true;
		}
		// enumerate modes
		int i=0;
		if (!fFound) while (EnumDisplaySettingsW(Mon.GetWideChar(), i++, &dmode))
				// compare enumerated mode with requested settings
				if (dmode.dmPelsWidth==iXRes && dmode.dmPelsHeight==iYRes && dmode.dmBitsPerPel==iColorDepth && dmode.dmDisplayOrientation==orientation
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
		DWORD rv = ChangeDisplaySettingsExW(iMonitor ? Mon.GetWideChar() : NULL, &dspMode, NULL, CDS_FULLSCREEN, NULL);
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
		::SetWindowPos(pWindow->hWindow, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_FRAMECHANGED);
	}
	return true;
#endif
}

bool C4AbstractApp::SaveDefaultGammaRamp(_GAMMARAMP &ramp)
{
	HDC hDC = GetDC(pWindow->hWindow);
	if (hDC)
	{
		bool r = !!GetDeviceGammaRamp(hDC, &ramp);
		if (!r)
		{
			Log("  Error getting default gamma ramp; using standard");
		}
		ReleaseDC(pWindow->hWindow, hDC);
		return r;
	}
	return false;
}

bool C4AbstractApp::ApplyGammaRamp(_GAMMARAMP &ramp, bool fForce)
{
	if (!Active && !fForce) return false;
	HDC hDC = GetDC(pWindow->hWindow);
	if (hDC)
	{
		bool r = !!SetDeviceGammaRamp(hDC, &ramp);
		ReleaseDC(pWindow->hWindow, hDC);
		return r;
	}
	return false;
}

void C4AbstractApp::MessageDialog(const char * message)
{
	MessageBoxW(0, GetWideChar(message), ADDL(C4ENGINECAPTION), MB_ICONERROR);
}

// Clipboard functions
bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	if (!fClipboard) return false;
	bool fSuccess = true;
	// gain clipboard ownership
	if (!OpenClipboard(pWindow ? pWindow->hWindow : NULL)) return false;
	// must empty the global clipboard, so the application clipboard equals the Windows clipboard
	EmptyClipboard();
	int size = MultiByteToWideChar(CP_UTF8, 0, text.getData(), text.getSize(), 0, 0);
	HANDLE hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t));
	if (hglbCopy == NULL) { CloseClipboard(); return false; }
	// lock the handle and copy the text to the buffer.
	wchar_t *szCopyChar = (wchar_t *) GlobalLock(hglbCopy);
	fSuccess = !!MultiByteToWideChar(CP_UTF8, 0, text.getData(), text.getSize(), szCopyChar, size);
	GlobalUnlock(hglbCopy);
	// place the handle on the clipboard.
	fSuccess = fSuccess && !!SetClipboardData(CF_UNICODETEXT, hglbCopy);
	// close clipboard
	CloseClipboard();
	// return whether copying was successful
	return fSuccess;
}

StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	if (!fClipboard) return StdStrBuf();
	// open clipboard
	if (!OpenClipboard(NULL)) return StdStrBuf();
	// get text from clipboard
	HANDLE hglb = GetClipboardData(CF_UNICODETEXT);
	if (!hglb) return StdStrBuf();
	StdStrBuf text((LPCWSTR) GlobalLock(hglb));
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

