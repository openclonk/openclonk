/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, Sven Eberhardt
 * Copyright (c) 2005-2006, Günther Brammer
 * Copyright (c) 2006, Armin Burgmeier
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

#ifndef INC_STDAPP
#define INC_STDAPP

#include "C4ForbidLibraryCompilation.h"
#include "platform/StdScheduler.h"
#include "platform/StdSync.h"
#include "platform/C4StdInProc.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#ifdef USE_SDL_MAINLOOP
#include <SDL.h>

const char* KeycodeToString(C4KeyCode code);
#endif

#ifdef USE_WIN32_WINDOWS
class CStdMessageProc : public StdSchedulerProc
{
public:
	CStdMessageProc() : pApp(nullptr) { }
	~CStdMessageProc() { }

private:
	C4AbstractApp *pApp;

public:
	void SetApp(C4AbstractApp *pnApp) { pApp = pnApp; }

	// StdSchedulerProc overrides
	virtual bool Execute(int iTimeout = -1, pollfd *dummy=0);
	virtual HANDLE GetEvent() { return STDSCHEDULER_EVENT_MESSAGE; }

};
#endif

class C4AbstractApp : public StdScheduler
{
public:
	C4AbstractApp ();
	virtual ~C4AbstractApp ();

	bool Active;

	virtual void Clear();

	bool Init(int argc, char * argv[]);
	void Run();
	virtual void Quit();

	bool GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor);
	bool SetVideoMode(int iXRes, int iYRes, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen);
	void RestoreVideoMode();

	virtual bool DoScheduleProcs(int iTimeout);
	bool FlushMessages();
#ifdef WITH_QT_EDITOR
	void ProcessQtEvents();
#endif
	C4Window * pWindow;
	bool fQuitMsgReceived; // if true, a quit message has been received and the application should terminate

	// Copy the text to the clipboard or the primary selection
	bool Copy(const std::string &text, bool fClipboard = true);
	// Paste the text from the clipboard or the primary selection
	std::string Paste(bool fClipboard = true);
	// Is there something in the clipboard?
	bool IsClipboardFull(bool fClipboard = true);
	// a command from stdin
	virtual void OnCommand(const char *szCmd) = 0; // callback
	// Callback from SetVideoMode
	virtual void OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) = 0;
	// Keyboard layout changed
	virtual void OnKeyboardLayoutChanged() = 0;
	// notify user to get back to the program
	void NotifyUserIfInactive();
	void MessageDialog(const char * message);
	const char *GetLastError() { return sLastError.c_str(); }
	void Error(const char * m) { sLastError = m; }

#ifdef _WIN32
private:
	HINSTANCE hInstance;
	DWORD idMainThread; // ID of main thread that initialized the app

	void SetLastErrorFromOS();

public:
	void SetInstance(HINSTANCE hInst) { hInstance = hInst; }
	HINSTANCE GetInstance() const { return hInstance; }
	bool AssertMainThread()
	{
#  ifdef _DEBUG
		if (idMainThread && idMainThread != ::GetCurrentThreadId())
		{
			assert(false);
			return false;
		}
#  endif
		return true;
	}
#else
	bool AssertMainThread()
	{
		assert(MainThread == pthread_self());
		return MainThread == pthread_self();
	}
	pthread_t MainThread;
#endif

#if defined(USE_SDL_MAINLOOP)
public:
	void HandleSDLEvent(SDL_Event& event);

#elif defined(USE_CONSOLE)
protected:
	C4StdInProc InProc;
#endif

#ifdef __APPLE__
public:
	std::string GetGameDataPath();
#endif

#ifdef USE_WIN32_WINDOWS
private:
	CStdMessageProc MessageProc;
public:
	HMONITOR hMon; // monitor handle of used monitor
	RECT MonitorRect;     // output window rect
protected:
	DEVMODEW dspMode, OldDspMode;// display mode for fullscreen
#endif

protected:
	std::string sLastError;
	bool fDspModeSet;           // true if display mode was changed
	virtual bool DoInit(int argc, char * argv[]) = 0;

	friend class CStdGL;
	friend class CStdGLCtx;
	friend class C4Window;
	friend class C4GtkWindow;
};

#endif // INC_STDAPP
