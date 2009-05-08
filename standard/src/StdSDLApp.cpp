/*
 * OpenClonk, http://www.openclonk.org
 *
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include <Standard.h>
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

/* CStdApp */

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false),
	Location(""), DoNotDelay(false), MainThread(pthread_self()), fDspModeSet(false),
	// 36 FPS
	Delay(27777)
{
}

CStdApp::~CStdApp() {
}

bool CStdApp::Init(int argc, char * argv[]) {
	// Set locale
	setlocale(LC_ALL,"");

	// SDLmain.m copied the executable path into argv[0];
    // just copy it (not sure if original buffer is guaranteed
    // to be permanent).
	this->argc=argc; this->argv=argv;
	static char dir[PATH_MAX];
	SCopy(argv[0], dir);
	Location = dir;

	// Build command line.
	static std::string s("\"");
	for (int i = 1; i < argc; ++i) {
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        Log("Error initializing SDL.");
		return false;
    }

    SDL_EnableUNICODE(1);

	// create pipe
	if(pipe(this->Pipe) != 0) {
		Log("Error creating Pipe");
		return false;
	}

	// Custom initialization
	return DoInit ();
}

bool CStdApp::InitTimer() { gettimeofday(&LastExecute, 0); return true; }

void CStdApp::Clear() {
	SDL_Quit();
	}

void CStdApp::Quit() {
	fQuitMsgReceived = true;
}

void CStdApp::Execute () {
	time_t seconds = LastExecute.tv_sec;
	timeval tv;
	gettimeofday(&tv, 0);
	// Too slow?
	if(DoNotDelay) {
		DoNotDelay = false;
		LastExecute = tv;
	} else if (LastExecute.tv_sec < tv.tv_sec - 2) {
		LastExecute = tv;
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
		// nothing to do
	} else if (fCheckTimer) {
		gettimeofday(&tv, 0);
		tv.tv_usec = LastExecute.tv_usec - tv.tv_usec + Delay
			- 1000000 * (tv.tv_sec - LastExecute.tv_sec);
		// Check if the given timeout comes first
		// (don't call Execute then, because it assumes it has been called because of a timer event!)
		if (iTimeout != INFINITE && iTimeout * 1000 < tv.tv_usec) {
			tv.tv_usec = iTimeout * 1000;
			do_execute = false;
		}
		if (tv.tv_usec < 0)
			tv.tv_usec = 0;
		tv.tv_sec = 0;
	} else {
		tv.tv_usec = iTimeout * 1000;
	}

	// Saaaafeeeetyyyyy!!!!!11
	if (tv.tv_usec >= 999999)
		tv.tv_usec = 999999;

	// Handle pending SDL messages
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		HandleSDLEvent(event);
	}

	// Watch dpy to see when it has input.
	int max_fd = 0;
	fd_set rfds;
	FD_ZERO(&rfds);

	// And for events from the network thread
	FD_SET(this->Pipe[0], &rfds);
	max_fd = Max(this->Pipe[0], max_fd);
	//printf("%d %d\n", tv.tv_sec, tv.tv_usec);
	switch (select(max_fd + 1, &rfds, NULL, NULL, &tv)) {
		// error
		case -1:
		if (errno == EINTR) return HR_Timeout; //Whatever, probably never needed
		Log("select error:");
		Log(strerror(errno));
		return HR_Failure;

		// timeout
		case 0:
		if (do_execute) {
			Execute();
			return HR_Timer;
		}
		return HR_Timeout;

		default:
		// flush pipe
		if(FD_ISSET(this->Pipe[0], &rfds)) {
			OnPipeInput();
		}
		return HR_Message;
	}
}

void CStdApp::HandleSDLEvent(SDL_Event& event) {
    // Directly handle QUIT messages.
	switch (event.type) {
		case SDL_QUIT:
		  Quit();
		  break;
	}

    // Everything else goes to the window.
	if (pWindow)
		pWindow->HandleMessage(event);
}

// The three following functions, together, perform a mode switch.
// GetIndexedDisplayMode is used to enumerate them, FindDisplayMode
// remembers a mode (if possible) and SetFullScreen switches to the
// last found mode.
// At least that's how I understand it. It certainly doesn't make sense.

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor) {
    // No support for multiple monitors.
    if (iMonitor != 0)
        return false;

    static SDL_Rect** modes = 0;
    static unsigned modeCount = 0;
    if (!modes) {
        modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
        // -1 means "all modes allowed". Clonk is not prepared
        // for this; should probably give some random resolutions
        // then.
        assert(reinterpret_cast<intptr_t>(modes) != -1);
        if (!modes)
            modeCount = 0;
        else
            // Count available modes.
            for (SDL_Rect** iter = modes; *iter; ++iter)
                ++modeCount;
    }

    if (iIndex >= modeCount)
        return false;

    *piXRes = modes[iIndex]->w;
    *piYRes = modes[iIndex]->h;
    *piBitDepth = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
    return true;
}

bool CStdApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iMonitor, bool fFullScreen) {
// Multi-monitor support not implemented.
// As far as I can tell, SDL doesn't support it to begin with.
	SDL_SetVideoMode(iXRes, iYRes, iColorDepth, SDL_OPENGL | (fFullScreen ? SDL_FULLSCREEN : 0));
	SDL_ShowCursor(SDL_DISABLE);
}

void CStdApp::RestoreVideoMode()
	{
	}

// Clipboard not implemented.

void CStdApp::Copy(const StdStrBuf & text, bool fClipboard) {
}
StdStrBuf CStdApp::Paste(bool fClipboard) {
	return StdStrBuf(0);
}
bool CStdApp::IsClipboardFull(bool fClipboard) {
	return false;
}
void CStdApp::ClearClipboard(bool fClipboard) {
}

// Event-pipe-whatever stuff I do not understand.

bool CStdApp::ReadStdInCommand() {
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
}

bool CStdApp::SignalNetworkEvent() {
	char c = 1;
	write(this->Pipe[1], &c, 1);
	return true;
}

void CStdApp::OnPipeInput() {
	char c;
	::read(this->Pipe[0], &c, 1);
	// call network class to handle it
	OnNetworkEvents();
}

void CStdApp::MessageDialog(const char * message)
{
// FIXME: At least for macosx something could be done here
}
