/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007, 2009-2010  Günther Brammer
 * Copyright (c) 2005, 2009  Peter Wortmann
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2006, 2008  Armin Burgmeier
 * Copyright (c) 2007  Alex
 * Copyright (c) 2010  Mortimer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* A wrapper class to OS dependent event and window interfaces */

#ifndef INC_STDWINDOW
#define INC_STDWINDOW

#include <StdBuf.h>
#include <StdScheduler.h>
#include <StdSync.h>

#ifdef _WIN32
const int SEC1_TIMER=1,SEC1_MSEC=1000;
#endif

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef _WIN32
#define K_ALT VK_MENU
#define K_ESCAPE VK_ESCAPE
#define K_PAUSE VK_PAUSE
#define K_TAB VK_TAB
#define K_RETURN VK_RETURN
#define K_DELETE VK_DELETE
#define K_INSERT VK_INSERT
#define K_BACK VK_BACK
#define K_SPACE VK_SPACE
#define K_F1 VK_F1
#define K_F2 VK_F2
#define K_F3 VK_F3
#define K_F4 VK_F4
#define K_F5 VK_F5
#define K_F6 VK_F6
#define K_F7 VK_F7
#define K_F8 VK_F8
#define K_F9 VK_F9
#define K_F10 VK_F10
#define K_F11 VK_F11
#define K_F12 VK_F12
#define K_ADD VK_ADD
#define K_SUBTRACT VK_SUBTRACT
#define K_MULTIPLY VK_MULTIPLY
#define K_UP VK_UP
#define K_DOWN VK_DOWN
#define K_LEFT VK_LEFT
#define K_RIGHT VK_RIGHT
#define K_HOME VK_HOME
#define K_END VK_END
#define K_SCROLL VK_SCROLL
#define K_MENU VK_APPS
#define K_PAGEUP VK_PRIOR
#define K_PAGEDOWN VK_NEXT
#define KEY_A ((WORD) 'A') // select all in GUI-editbox
#define KEY_C ((WORD) 'C') // copy in GUI-editbox
#define KEY_I ((WORD) 'I') // console mode control key
#define KEY_M ((WORD) 'M') // console mode control key
#define KEY_T ((WORD) 'T') // console mode control key
#define KEY_V ((WORD) 'V') // paste in GUI-editbox
#define KEY_W ((WORD) 'W') // console mode control key
#define KEY_X ((WORD) 'X') // cut from GUI-editbox
#elif defined(USE_X11)
#include <X11/keysym.h>
#include <sys/time.h>
#define K_F1 XK_F1
#define K_F2 XK_F2
#define K_F3 XK_F3
#define K_F4 XK_F4
#define K_F5 XK_F5
#define K_F6 XK_F6
#define K_F7 XK_F7
#define K_F8 XK_F8
#define K_F9 XK_F9
#define K_F10 XK_F10
#define K_F11 XK_F11
#define K_F12 XK_F12
#define K_ADD XK_KP_Add
#define K_SUBTRACT XK_KP_Subtract
#define K_MULTIPLY XK_KP_Multiply
#define K_ESCAPE XK_Escape
#define K_PAUSE XK_Pause
#define K_TAB XK_Tab
#define K_RETURN XK_Return
#define K_DELETE XK_Delete
#define K_INSERT XK_Insert
#define K_BACK XK_BackSpace
#define K_SPACE XK_space
#define K_UP XK_Up
#define K_DOWN XK_Down
#define K_LEFT XK_Left
#define K_RIGHT XK_Right
#define K_HOME XK_Home
#define K_END XK_End
#define K_SCROLL XK_Scroll_Lock
#define K_MENU XK_Menu
#define K_PAGEUP XK_Page_Up
#define K_PAGEDOWN XK_Page_Down
#define KEY_A XK_a // select all in GUI-editbox
#define KEY_C XK_c // copy in GUI-editbox
#define KEY_I XK_i // console mode control key
#define KEY_M XK_m // console mode control key
#define KEY_T XK_t // console mode control key
#define KEY_V XK_v // paste in GUI-editbox
#define KEY_W XK_w // console mode control key
#define KEY_X XK_x // cut from GUI-editbox
// from X.h:
//#define ShiftMask   (1<<0)
//#define ControlMask   (1<<2)
#define MK_CONTROL (1<<2)
#define MK_SHIFT (1<<0)
#elif defined(USE_SDL_MAINLOOP)
#include <SDL.h>
#define K_F1 SDLK_F1
#define K_F2 SDLK_F2
#define K_F3 SDLK_F3
#define K_F4 SDLK_F4
#define K_F5 SDLK_F5
#define K_F6 SDLK_F6
#define K_F7 SDLK_F7
#define K_F8 SDLK_F8
#define K_F9 SDLK_F9
#define K_F10 SDLK_F10
#define K_F11 SDLK_F11
#define K_F12 SDLK_F12
#define K_ADD SDLK_KP_PLUS
#define K_SUBTRACT SDLK_KP_MINUS
#define K_MULTIPLY SDLK_KP_MULTIPLY
#define K_ESCAPE SDLK_ESCAPE
#define K_PAUSE SDLK_PAUSE
#define K_TAB SDLK_TAB
#define K_RETURN SDLK_RETURN
#define K_DELETE SDLK_DELETE
#define K_INSERT SDLK_INSERT
#define K_BACK SDLK_BACKSPACE
#define K_SPACE SDLK_SPACE
#define K_UP SDLK_UP
#define K_DOWN SDLK_DOWN
#define K_LEFT SDLK_LEFT
#define K_RIGHT SDLK_RIGHT
#define K_HOME SDLK_HOME
#define K_END SDLK_END
#define K_SCROLL SDLK_SCROLLOCK
#define K_MENU SDLK_MENU
#define K_PAGEUP SDLK_PAGEUP
#define K_PAGEDOWN SDLK_PAGEDOWN
#define KEY_M SDLK_m
#define KEY_T SDLK_t
#define KEY_W SDLK_w
#define KEY_I SDLK_i
#define KEY_C SDLK_c
#define KEY_V SDLK_v
#define KEY_X SDLK_x
#define KEY_A SDLK_a
#define MK_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define MK_CONTROL (KMOD_LCTRL | KMOD_RCTRL)
#elif defined(USE_CONSOLE)
#define K_F1 0
#define K_F2 0
#define K_F3 0
#define K_F4 0
#define K_F5 0
#define K_F6 0
#define K_F7 0
#define K_F8 0
#define K_F9 0
#define K_F10 0
#define K_F11 0
#define K_F12 0
#define K_ADD 0
#define K_SUBTRACT 0
#define K_MULTIPLY 0
#define K_ESCAPE 0
#define K_PAUSE 0
#define K_TAB 0
#define K_RETURN 0
#define K_DELETE 0
#define K_INSERT 0
#define K_BACK 0
#define K_SPACE 0
#define K_UP 0
#define K_DOWN 0
#define K_LEFT 0
#define K_RIGHT 0
#define K_HOME 0
#define K_END 0
#define K_SCROLL 0
#define K_MENU 0
#define K_PAGEUP 0
#define K_PAGEDOWN 0
#define KEY_M 0
#define KEY_T 0
#define KEY_W 0
#define KEY_I 0
#define KEY_C 0
#define KEY_V 0
#define KEY_X 0
#define KEY_A 0
#define MK_SHIFT 0
#define MK_CONTROL 0
#elif defined(USE_COCOA)
// declare as extern variables and initialize them in StdMacWindow.mm so as to not include objc headers
const int CocoaKeycodeOffset = 300;
extern int K_F1;
extern int K_F2;
extern int K_F3;
extern int K_F4;
extern int K_F5;
extern int K_F6;
extern int K_F7;
extern int K_F8;
extern int K_F9;
extern int K_F10;
extern int K_F11;
extern int K_F12;
extern int K_ADD;
extern int K_SUBTRACT;
extern int K_MULTIPLY;
extern int K_ESCAPE;
extern int K_PAUSE;
extern int K_TAB;
extern int K_RETURN;
extern int K_DELETE;
extern int K_INSERT;
extern int K_BACK;
extern int K_SPACE;
extern int K_UP;
extern int K_DOWN;
extern int K_LEFT;
extern int K_RIGHT;
extern int K_HOME;
extern int K_END;
extern int K_SCROLL;
extern int K_MENU;
extern int K_PAGEUP;
extern int K_PAGEDOWN;
extern int KEY_M;
extern int KEY_T;
extern int KEY_W;
extern int KEY_I;
extern int KEY_C;
extern int KEY_V;
extern int KEY_X;
extern int KEY_A;
extern int MK_SHIFT;
extern int MK_CONTROL;
#endif

enum C4AppHandleResult
{
	HR_Timeout,
	HR_Message,         // handled a message
	HR_Timer,           // got timer event
	HR_Failure          // error, or quit message received
};

class CStdApp;
#ifdef USE_X11
// Forward declarations because xlib.h is evil
typedef union _XEvent XEvent;
typedef struct _XDisplay Display;
#endif

class CStdWindow
{
public:
	enum WindowKind
	{
		W_GuiWindow,
		W_Viewport,
		W_Fullscreen
	};
public:
	CStdWindow ();
	virtual ~CStdWindow ();
	bool Active;
	CSurface * pSurface;
	virtual void Clear();
	// Only when the wm requests a close
	// For example, when the user clicks the little x in the corner or uses Alt-F4
	virtual void Close() = 0;
	// Keypress(es) translated to a char
	virtual void CharIn(const char * c) { }
	virtual CStdWindow * Init(CStdApp * pApp);
#ifndef _WIN32
	virtual CStdWindow * Init(WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent = 0, bool HideCursor = true);
#endif
	bool StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize = true);
	bool RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden = false);
	bool GetSize(RECT * pRect);
	void SetSize(unsigned int cx, unsigned int cy); // resize
	void SetTitle(const char * Title);
	void FlashWindow();

#ifdef _WIN32
public:
	HWND hWindow;
protected:
	bool RegisterWindowClass(HINSTANCE hInst);
	virtual bool Win32DialogMessageHandling(MSG * msg) { return false; };
#elif defined(USE_X11)
protected:
	bool FindInfo();

	unsigned long wnd;
	unsigned long renderwnd;
	Display * dpy;
	virtual void HandleMessage (XEvent &);
	// The currently set window hints
	void * Hints;
	bool HasFocus; // To clear urgency hint
	// The XVisualInfo the window was created with
	void * Info;
#elif defined(USE_SDL_MAINLOOP)
private:
	int width, height;
protected:
	virtual void HandleMessage(SDL_Event&) {}
#elif defined(USE_COCOA)
protected:
	/*ClonkWindowController*/void* controller;
	virtual void HandleMessage(/*NSEvent*/void*);
public:	
	/*ClonkWindowController*/void* GetController() {return controller;}
#endif
public:
	// request that this window be redrawn in the near future (including immediately)
	virtual void RequestUpdate();
	// Invokes actual drawing code - should not be called directly
	virtual void PerformUpdate();
public:
	friend class CStdDDraw;
	friend class CStdGL;
	friend class CStdGLCtx;
	friend class CStdApp;
	friend class CStdGtkWindow;
};

#ifdef _WIN32
class CStdMultimediaTimerProc : public CStdNotifyProc
{
public:
	CStdMultimediaTimerProc(uint32_t iDelay);
	~CStdMultimediaTimerProc();

private:
	static int iTimePeriod;
	uint32_t uCriticalTimerDelay;

	UINT idCriticalTimer,uCriticalTimerResolution;
	CStdEvent Event;

public:

	void SetDelay(uint32_t iDelay);
	void Set() { Event.Set(); }
	bool Check() { return Event.WaitFor(0); }
	bool CheckAndReset();

	// StdSchedulerProc overrides
	virtual HANDLE GetEvent() { return Event.GetEvent(); }

};

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
#else
#define CStdMultimediaTimerProc CStdTimerProc
#endif

#ifdef USE_CONSOLE
// A simple alertable proc
class CStdInProc : public StdSchedulerProc
{
public:
	CStdInProc();
	~CStdInProc() { }

public:
	void Notify();
	bool Check();
	bool CheckAndReset();
public:
	// StdSchedulerProc override
	virtual bool Execute(int iTimeout, pollfd *);
	virtual void GetFDs(std::vector<struct pollfd> & checkfds)
	{
		pollfd pfd = { 0, POLLIN, 0 };
		checkfds.push_back(pfd);
	}
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

	bool GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor);
	bool SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor, bool fFullScreen);
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
	void NotifyUserIfInactive()
	{
#ifdef _WIN32
		if (!Active && pWindow) pWindow->FlashWindow();
#else
		if (pWindow) pWindow->FlashWindow();
#endif
	}
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
	HWND GetWindowHandle() { return pWindow ? pWindow->hWindow : NULL; }
	void SetInstance(HINSTANCE hInst) { hInstance = hInst; }
	HINSTANCE GetInstance() const { return hInstance; }
	bool DialogMessageHandling(MSG *pMsg) { return pWindow ? pWindow->Win32DialogMessageHandling(pMsg) : false; }
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
	DEVMODE dspMode, OldDspMode;// display mode for fullscreen
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
	void OnStdInInput();
protected:
#  ifdef USE_X11
	class CStdAppPrivate * Priv;
	void HandleXMessage();
#  endif
	unsigned int KeyMask;
#endif
protected:
	int argc; char ** argv;
#ifdef USE_CONSOLE
	CStdInProc InProc;
#endif
	StdStrBuf sLastError;
	bool fDspModeSet;           // true if display mode was changed
	virtual bool DoInit(int argc, char * argv[]) = 0;

	// commands from stdin (console only)
	StdCopyStrBuf CmdBuf;
	bool ReadStdInCommand();

	friend class CStdGL;
	friend class CStdWindow;
	friend class CStdGtkWindow;
};

#endif // INC_STDWINDOW
