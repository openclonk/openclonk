/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2005, 2007, 2009  Peter Wortmann
 * Copyright (c) 2005-2006, 2008-2009  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
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

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include <Standard.h>
#include <StdRegistry.h>
#ifdef USE_GL
#include <StdGL.h>
#endif
#ifdef USE_DIRECTX
#include <StdD3D.h>
#endif
#include <StdWindow.h>
#include <mmsystem.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>

// multimon.h comes with DirectX, some people don't have DirectX.
#ifdef HAVE_MULTIMON_H

// Lets try this unconditionally so that older windowses get the benefit
// even if the engine was compiled with a newer sdk. Or something.
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

#endif

#include "../../engine/res/resource.h"
#include "../../engine/inc/C4Version.h"

#define C4FullScreenClassName "C4FullScreen"
LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CStdWindow::CStdWindow (): Active(false), hWindow(0) {
}
CStdWindow::~CStdWindow () {
}

BOOL CStdWindow::RegisterWindowClass(HINSTANCE hInst) {
  WNDCLASSEX WndClass;
	WndClass.cbSize=sizeof(WNDCLASSEX);
  WndClass.style         = CS_DBLCLKS;
  WndClass.lpfnWndProc   = FullScreenWinProc;
  WndClass.cbClsExtra    = 0;
  WndClass.cbWndExtra    = 0;
  WndClass.hInstance     = hInst;
  WndClass.hCursor       = NULL;
  WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
  WndClass.lpszMenuName  = NULL;
  WndClass.lpszClassName = C4FullScreenClassName;
	WndClass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE (IDI_00_C4X) );
  WndClass.hIconSm       = LoadIcon (hInst, MAKEINTRESOURCE (IDI_00_C4X) );
	return RegisterClassEx(&WndClass);
}

CStdWindow * CStdWindow::Init(CStdApp * pApp) {
	Active = true;

	// Register window class
	if (!RegisterWindowClass(pApp->hInstance)) return NULL;

	// Create window
	hWindow = CreateWindowEx	(
							0,
							C4FullScreenClassName,
							C4ENGINENAME,
							WS_POPUP,
							CW_USEDEFAULT,CW_USEDEFAULT,0,0,
							NULL,NULL,pApp->hInstance,NULL);

#ifndef USE_CONSOLE
	// Show & focus
	ShowWindow(hWindow,SW_SHOWNORMAL);
	SetFocus(hWindow);
	ShowCursor(FALSE);
#endif

	return this;
}

void CStdWindow::Clear() {
	// Destroy window
	if (hWindow) DestroyWindow(hWindow);
	hWindow = NULL;
}

bool CStdWindow::StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize) {
	return StoreWindowPosition(hWindow, szWindowName, szSubKey, fStoreSize) != 0;
}

bool CStdWindow::RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden) {
	if (!RestoreWindowPosition(hWindow, szWindowName, szSubKey, fHidden))
		ShowWindow(hWindow,SW_SHOWNORMAL);
	return TRUE;
}

void CStdWindow::SetTitle(const char *szToTitle) {
	if (hWindow) SetWindowText(hWindow, szToTitle ? szToTitle : "");
}

bool CStdWindow::GetSize(RECT * pRect) {
	if (!(hWindow && GetClientRect(hWindow,pRect))) return false;
	return true;
}

void CStdWindow::SetSize(unsigned int cx, unsigned int cy) {
	// resize
	if (hWindow) {
		::SetWindowPos(hWindow, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void CStdWindow::FlashWindow()
	{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
	}

/* CStdTimerProc */

int CStdMultimediaTimerProc::iTimePeriod = 0;

CStdMultimediaTimerProc::CStdMultimediaTimerProc(uint32_t iDelay) :
	idCriticalTimer(0),
	uCriticalTimerDelay(28),
	uCriticalTimerResolution(5),
	Event(true)
	{

	if(!iTimePeriod)
		{
		// Get resolution caps
		TIMECAPS tc;
		timeGetDevCaps(&tc, sizeof(tc));
		// Establish minimum resolution
		uCriticalTimerResolution = BoundBy(uCriticalTimerResolution, tc.wPeriodMin, tc.wPeriodMax);
		timeBeginPeriod(uCriticalTimerResolution);
		}
	iTimePeriod++;

	SetDelay(iDelay);

	}

CStdMultimediaTimerProc::~CStdMultimediaTimerProc()
	{
	if (idCriticalTimer)
		{
		timeKillEvent(idCriticalTimer);
		idCriticalTimer = 0;

		iTimePeriod--;
		if (!iTimePeriod)
			timeEndPeriod(uCriticalTimerResolution);
		}
	}

void CStdMultimediaTimerProc::SetDelay(uint32_t iDelay)
	{

	// Kill old timer (of any)
	if(idCriticalTimer)
		timeKillEvent(idCriticalTimer);

	// Set critical timer
	idCriticalTimer=timeSetEvent(
		uCriticalTimerDelay,uCriticalTimerResolution,
		(LPTIMECALLBACK) Event.GetEvent(),0,TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);

	}

bool CStdMultimediaTimerProc::CheckAndReset()
	{
	if(!Check()) return false;
	Event.Reset();
	return true;
	}

/* CStdMessageProc */

bool CStdMessageProc::Execute(int iTimeout, pollfd *)
	{
	// Peek messages
	MSG msg;
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
		// quit?
		if(msg.message == WM_QUIT)
			{
			pApp->fQuitMsgReceived = true;
			return false;
			}
		// Dialog message transfer
		BOOL MsgDone=FALSE;
		if (!pApp->DialogMessageHandling(&msg))
			{
			TranslateMessage(&msg); DispatchMessage(&msg);
			}
		}
	return true;
	}

/* CStdApp */

CStdApp::CStdApp() :
	Active(false), hInstance(NULL), fQuitMsgReceived(false),
	fDspModeSet(false)
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

CStdApp::~CStdApp()
	{
	}

char *LoadResStr(const char *id);

bool CStdApp::Init(HINSTANCE hInst, int nCmdShow, char *szCmdLine) {
	// Set instance vars
	hInstance = hInst;
	this->szCmdLine = szCmdLine;
	hMainThread = ::GetCurrentThread();
	// Custom initialization
	return DoInit ();
}

void CStdApp::Clear() {
	hMainThread = NULL;
}

void CStdApp::Quit() {
	PostQuitMessage(0);
}

bool CStdApp::FlushMessages() {

	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;

	return MessageProc.Execute(0);
}

int GLMonitorInfoEnumCount;

BOOL CALLBACK GLMonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
	// get to indexed monitor
	if (GLMonitorInfoEnumCount--) return TRUE;
	// store it
	CStdApp *pApp = (CStdApp *) dwData;
	pApp->hMon = hMonitor;
	pApp->MonitorRect = *lprcMonitor;
	return TRUE;
	}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor)
	{
	// prepare search struct
	DEVMODE dmode;
	ZeroMemory(&dmode, sizeof(dmode)); dmode.dmSize = sizeof(dmode);
  StdStrBuf Mon;
	if (iMonitor)
    Mon.Format("\\\\.\\Display%d", iMonitor+1);
	// check if indexed mode exists
	if (!EnumDisplaySettings(Mon.getData(), iIndex, &dmode)) return false;
	// mode exists; return it
	if (piXRes) *piXRes = dmode.dmPelsWidth;
	if (piYRes) *piYRes = dmode.dmPelsHeight;
	if (piBitDepth) *piBitDepth = dmode.dmBitsPerPel;
	return true;
	}

void CStdApp::RestoreVideoMode()
	{
	}

bool CStdApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor, bool fFullScreen)
	{
#ifdef USE_DIRECTX
	if (pD3D)
		{
		if(!pD3D->SetVideoMode(iXRes, iYRes, iColorDepth, iMonitor, fFullScreen))
			return false;
		OnResolutionChanged(iXRes, iYRes);
		return true;
		}
#endif
	bool fFound=false;
	DEVMODE dmode;
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
			MonitorRect.right = iXRes; MonitorRect.bottom = iYRes;
			}
		else return false;
		}
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format("\\\\.\\Display%d", iMonitor+1);
	// enumerate modes
	int i=0;
	ZeroMemory(&dmode, sizeof(dmode)); dmode.dmSize = sizeof(dmode);
	while (EnumDisplaySettings(Mon.getData(), i++, &dmode))
		// size and bit depth is OK?
		if (dmode.dmPelsWidth==iXRes && dmode.dmPelsHeight==iYRes && dmode.dmBitsPerPel==iColorDepth)
			{
			// compare with found one
			if (fFound)
				// try getting a mode that is close to 85Hz, rather than taking the one with highest refresh rate
				// (which may set absurd modes on some devices)
				if (Abs<int>(85-dmode.dmDisplayFrequency)>Abs<int>(85-dspMode.dmDisplayFrequency))
					// the previous one was better
					continue;
			// choose this one
			fFound=true;
			dspMode=dmode;
			}

	// change mode
	if (fFullScreen == fDspModeSet) return true;
#ifdef _DEBUG
	SetWindowPos(pWindow->hWindow, HWND_TOP, MonitorRect.left, MonitorRect.top, dspMode.dmPelsWidth, dspMode.dmPelsHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowLong(pWindow->hWindow, GWL_STYLE, (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX));
	return true;
#else
	if (!fFullScreen)
		{
		ChangeDisplaySettings(NULL, CDS_RESET);
		fDspModeSet = false;
		OnResolutionChanged(iXRes, iYRes);
		return true;
		}
	// save original display mode
	fDspModeSet=true;
	// if a monitor is given, use that
	if (iMonitor)
		{
		dspMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		if (ChangeDisplaySettingsEx(Mon.getData(), &dspMode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
			{
			fDspModeSet = false;
			}
		}
	else
		{
		if (ChangeDisplaySettings(&dspMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
			fDspModeSet = false;
			}
		}
	SetWindowPos(pWindow->hWindow, 0, MonitorRect.left, MonitorRect.top, dspMode.dmPelsWidth, dspMode.dmPelsHeight, 0);
	if (fDspModeSet)
		OnResolutionChanged(iXRes, iYRes);
	return fDspModeSet;
#endif
}

bool CStdApp::ReadStdInCommand()
{
	while(_kbhit())
	{
		// Surely not the most efficient way to do it, but we won't have to read much data anyway.
		char c = getch();
		if(c == '\r') {
			if(!CmdBuf.isNull()) {
				OnCommand(CmdBuf.getData()); CmdBuf.Clear();
			}
		} else if(isprint((unsigned char)c))
			CmdBuf.AppendChar(c);
	}
	return true;
}

void CStdApp::MessageDialog(const char * message)
{
	MessageBox(0, message, C4ENGINECAPTION, MB_ICONERROR);
}
