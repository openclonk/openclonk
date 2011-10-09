/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2005-2006, 2010-2011  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Peter Wortmann
 * Copyright (c) 2010  Martin Plicht
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

#ifndef INC_STDAPP
#define INC_STDAPP

#include <StdScheduler.h>
#include <StdSync.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef _WIN32
#include <C4windowswrapper.h>

#elif defined(USE_X11)
// do not include xlib.h
typedef struct _XDisplay Display;
// from X.h:
//#define ShiftMask   (1<<0)
//#define ControlMask   (1<<2)
#define MK_CONTROL (1<<2)
#define MK_SHIFT (1<<0)

#elif defined(USE_SDL_MAINLOOP)
#include <SDL.h>
#define MK_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define MK_CONTROL (KMOD_LCTRL | KMOD_RCTRL)

#elif defined(USE_CONSOLE)
#define MK_SHIFT 0
#define MK_CONTROL 0

#elif defined(USE_COCOA)
// declare as extern variables and initialize them in StdMacWindow.mm so as to not include objc headers
extern int MK_SHIFT;
extern int MK_CONTROL;
#endif

#ifdef _WIN32
class CStdMessageProc : public StdSchedulerProc
{
public:
	CStdMessageProc() : pApp(NULL) { }
	~CStdMessageProc() { }

private:
	CStdApp *pApp;

public:
	void SetApp(CStdApp *pnApp) { pApp = pnApp; }

	// StdSchedulerProc overrides
	virtual bool Execute(int iTimeout = -1, pollfd *dummy=0);
	virtual HANDLE GetEvent() { return STDSCHEDULER_EVENT_MESSAGE; }

};
#endif

#ifdef USE_CONSOLE
// A simple alertable proc
class CStdInProc : public StdSchedulerProc
{
public:
	CStdInProc();
	~CStdInProc();

	// StdSchedulerProc override
	virtual bool Execute(int iTimeout, pollfd *);
	virtual void GetFDs(std::vector<struct pollfd> & checkfds)
	{
		pollfd pfd = { 0, POLLIN, 0 };
		checkfds.push_back(pfd);
	}
private:
	// commands from stdin
	StdCopyStrBuf CmdBuf;
};
#endif

class CStdApp : public StdScheduler
{
public:
	CStdApp ();
	virtual ~CStdApp ();

	bool Active;

	virtual void Clear();

	bool Init(int argc, char * argv[]);
	void Run();
	virtual void Quit();

	bool GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor);
	bool SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen);
	void RestoreVideoMode();
	bool ScheduleProcs(int iTimeout = -1);
	bool FlushMessages();
	CStdWindow * pWindow;
	bool fQuitMsgReceived; // if true, a quit message has been received and the application should terminate

	// Copy the text to the clipboard or the primary selection
	bool Copy(const StdStrBuf & text, bool fClipboard = true);
	// Paste the text from the clipboard or the primary selection
	StdStrBuf Paste(bool fClipboard = true);
	// Is there something in the clipboard?
	bool IsClipboardFull(bool fClipboard = true);
	// Give up Selection ownership
	void ClearClipboard(bool fClipboard = true);
	// a command from stdin
	virtual void OnCommand(const char *szCmd) = 0; // callback
	// Callback from SetVideoMode
	virtual void OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) = 0;
	// notify user to get back to the program
	void NotifyUserIfInactive();
	void MessageDialog(const char * message);
	const char *GetLastError() { return sLastError.getData(); }
	void Error(const char * m) { sLastError.Copy(m); }
#ifdef _WIN32

private:
	HINSTANCE hInstance;
	HANDLE hMainThread; // handle to main thread that initialized the app
	CStdMessageProc MessageProc;

public:
	bool IsShiftDown() { return GetKeyState(VK_SHIFT) < 0; }
	bool IsControlDown() { return GetKeyState(VK_CONTROL) < 0; }
	bool IsAltDown() { return GetKeyState(VK_MENU) < 0; }
	void SetInstance(HINSTANCE hInst) { hInstance = hInst; }
	HINSTANCE GetInstance() const { return hInstance; }
	bool AssertMainThread()
	{
#  ifdef _DEBUG
		if (hMainThread && hMainThread != ::GetCurrentThread())
		{
			assert(false);
			return false;
		}
#  endif
		return true;
	}
	PIXELFORMATDESCRIPTOR &GetPFD() { return pfd; }
	HMONITOR hMon; // monitor handle of used monitor
	RECT MonitorRect;     // output window rect
protected:
	PIXELFORMATDESCRIPTOR pfd;  // desired pixel format
	DEVMODEW dspMode, OldDspMode;// display mode for fullscreen
#else
#  if defined(USE_X11)
	Display * dpy;
	int xf86vmode_major_version, xf86vmode_minor_version;
	int xrandr_major_version, xrandr_minor_version;
#  endif

#  if defined(USE_SDL_MAINLOOP)
	void HandleSDLEvent(SDL_Event& event);
#  endif
#ifdef USE_COCOA
	void HandleNSEvent(/*NSEvent*/void* event);
	StdStrBuf GetGameDataPath();
#endif
	const char * Location;
	pthread_t MainThread;
	bool DoNotDelay;
	bool IsShiftDown() { return KeyMask & MK_SHIFT; }
	bool IsControlDown() { return KeyMask & MK_CONTROL; }
	bool IsAltDown() { return KeyMask & (1<<3); }
	bool AssertMainThread()
	{
		assert(MainThread == pthread_self());
		return MainThread == pthread_self();
	}
	// These must be public to be callable from callback functions from
	// the glib main loop that are in an anonymous namespace in
	// StdXApp.cpp.
	void OnXInput();
protected:
#  ifdef USE_X11
	class CStdAppPrivate * Priv;
	void HandleXMessage();
#  endif
	unsigned int KeyMask;
#endif
protected:
#ifdef USE_CONSOLE
	CStdInProc InProc;
#endif
	StdStrBuf sLastError;
	bool fDspModeSet;           // true if display mode was changed
	virtual bool DoInit(int argc, char * argv[]) = 0;;

	friend class CStdGL;
	friend class CStdGLCtx;
	friend class CStdWindow;
	friend class CStdGtkWindow;
};

#endif // INC_STDAPP
