/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006, 2008-2009  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* A wrapper class to OS dependent event and window interfaces, Text version */

#include <Standard.h>
#ifdef USE_CONSOLE
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <string>
#include <sstream>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  endif
static void readline_callback (char *);
static CStdApp * readline_callback_use_this_app = 0;
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  endif
#endif /* HAVE_READLINE_HISTORY */

#include "StdXPrivate.h"

/* CStdApp */

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false), Priv(new CStdAppPrivate()),
	Location(""), DoNotDelay(false),
	// main thread
#ifdef HAVE_PTHREAD
	MainThread (pthread_self()),
#endif
	fDspModeSet(false),
	// 36 FPS
	Delay(27777)
{
}

CStdApp::~CStdApp() {
	delete Priv;
}

bool CStdApp::Init(int argc, char * argv[]) {
	// Set locale
	setlocale(LC_ALL,"");
	// Try to figure out the location of the executable
	Priv->argc=argc; Priv->argv=argv;
	static char dir[PATH_MAX];
	SCopy(argv[0], dir);
	if (dir[0] != '/') {
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
		Location = dir;
	} else {
		Location = dir;
	}
	// botch arguments
	static std::string s("\"");
	for (int i = 1; i < argc; ++i) {
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install (">", readline_callback);
	readline_callback_use_this_app = this;
#endif
	// create pipe
	if(pipe(Priv->Pipe) != 0) {
		Log("Error creating Pipe");
		return false;
	}

	// Custom initialization
	return DoInit ();
}

bool CStdApp::InitTimer() { gettimeofday(&LastExecute, 0); return true; }

void CStdApp::Clear() {
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
	// close pipe
	close(Priv->Pipe[0]);
	close(Priv->Pipe[1]);
}

void CStdApp::Quit() {
	fQuitMsgReceived = true;
}

void CStdApp::Execute () {
	time_t seconds = LastExecute.tv_sec;
	timeval tv;
	gettimeofday(&tv, 0);
	// Too slow?
	if (LastExecute.tv_sec < tv.tv_sec + 2) {
		gettimeofday(&LastExecute, 0);
	} else {
		LastExecute.tv_usec += Delay;
		if (LastExecute.tv_usec > 1000000) {
			++LastExecute.tv_sec;
			LastExecute.tv_usec -= 1000000;
		}
	}
	// This will make the FPS look "prettier" in some situations
	// But who cares...
	if (seconds != LastExecute.tv_sec) {
		pWindow->Sec1Timer();
	}
}
void CStdApp::NextTick(bool fYield) {
	DoNotDelay = true;
}

void CStdApp::Run() {
	// Main message loop
	while (true) if (HandleMessage(INFINITE, true) == HR_Failure) return;
}

void CStdApp::ResetTimer(unsigned int d) { Delay = 1000 * d; }

C4AppHandleResult CStdApp::HandleMessage(unsigned int iTimeout, bool fCheckTimer) {
	// quit check for nested HandleMessage-calls
	if (fQuitMsgReceived) return HR_Failure;
	bool do_execute = fCheckTimer;
	// Wait Delay microseconds.
	timeval tv = { 0, 0 };
	if (DoNotDelay) {
		DoNotDelay = false;
	} else if (fCheckTimer) {
		gettimeofday(&tv, 0);
		//printf("tv %d %d\n", tv.tv_sec, tv.tv_usec);
		//printf("le %d %d\n", LastExecute.tv_sec, LastExecute.tv_usec);
		tv.tv_usec = LastExecute.tv_usec - tv.tv_usec + Delay
			- 1000000 * (tv.tv_sec - LastExecute.tv_sec);
		if (iTimeout != INFINITE && iTimeout * 1000 < tv.tv_usec) tv.tv_usec = iTimeout * 1000;
		if (tv.tv_usec < 0)
			tv.tv_usec = 0;
		tv.tv_sec = 0;
	} else {
		tv.tv_usec = iTimeout * 1000;
	}

	// Watch dpy to see when it has input.
	int max_fd = 0;
	fd_set rfds;
	FD_ZERO(&rfds);

#ifdef USE_CONSOLE
	// Wait for commands from stdin
	FD_SET(0, &rfds);
#endif
	// And for events from the network thread
	FD_SET(Priv->Pipe[0], &rfds);
	max_fd = Max(Priv->Pipe[0], max_fd);
	//printf("%d %d\n", tv.tv_sec, tv.tv_usec);
	switch (select(max_fd + 1, &rfds, NULL, NULL, &tv)) {
		// error
		case -1:
		if (errno == EINTR) return HR_Timeout; //Whatever, probably never needed
		Log(strerror(errno));
		Log("select error.");
		return HR_Failure;

		// timeout
		case 0:
		if (do_execute) {
			Execute();
			return HR_Timer;
		}
		return HR_Timeout;

		default:
#ifdef USE_CONSOLE
		// handle commands
		if(FD_ISSET(0, &rfds))
		{
			// Do not call OnStdInInput to be able to return
			// HR_Failure when ReadStdInCommand returns false
			if(!ReadStdInCommand())
				return HR_Failure;
		}
#endif
		return HR_Message;
	}
}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor) {
	return false;
}

void CStdApp::RestoreVideoMode() {}

bool CStdApp::SetVideoMode(unsigned int, unsigned int, unsigned int, unsigned int, bool) {}

// Copy the text to the clipboard or the primary selection
void CStdApp::Copy(const StdStrBuf & text, bool fClipboard) {
}

// Paste the text from the clipboard or the primary selection
StdStrBuf CStdApp::Paste(bool fClipboard) {
	return StdStrBuf("");
}
// Is there something in the clipboard?
bool CStdApp::IsClipboardFull(bool fClipboard) {
	return false;
}
// Give up Selection ownership
void CStdApp::ClearClipboard(bool fClipboard) {
}
/*
CStdWindow * CStdAppPrivate::GetWindow(unsigned long wnd) {
	WindowListT::iterator i = WindowList.find(wnd);
	if (i != WindowList.end()) return i->second;
	return 0;
}
void CStdAppPrivate::SetWindow(unsigned long wnd, CStdWindow * pWindow) {
	if (!pWindow) {
		WindowList.erase(wnd);
	} else {
		WindowList[wnd] = pWindow;
	}
}*/

bool CStdApp::ReadStdInCommand() {
#if HAVE_LIBREADLINE
	rl_callback_read_char();
	return true;
#else
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
	char c;
	if(read(0, &c, 1) != 1)
		return false;
	if(c == '\n') {
		if(!CmdBuf.isNull()) {
			OnCommand(CmdBuf.getData()); CmdBuf.Clear();
		}
	} else if(isprint((unsigned char)c))
		CmdBuf.AppendChar(c);
	return true;
#endif
}
#if HAVE_LIBREADLINE
static void readline_callback (char * line) {
	if (!line) {
		readline_callback_use_this_app->Quit();
	} else {
		readline_callback_use_this_app->OnCommand(line);
	}
#if HAVE_READLINE_HISTORY
	if (line && *line) {
		add_history (line);
	}
#endif
	free(line);
}
#endif

bool CStdDDraw::SaveDefaultGammaRamp(CStdWindow * pWindow)
	{
	return true;
	}

void CStdApp::OnStdInInput()
{
	if(!ReadStdInCommand())
	{
		// TODO: This should only cause HandleMessage to return
		// HR_Failure...
		Quit();
	}
}

void CStdApp::MessageDialog(const char * message)
{
}
#endif // USE_CONSOLE
