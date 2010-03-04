/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2008-2009  Günther Brammer
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

#include <C4Include.h>
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

#include "MacUtility.h"

/* CStdApp */

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false),
	Location(""), DoNotDelay(false), MainThread(pthread_self()), fDspModeSet(false)
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
#ifdef __APPLE__
	SDL_EnableKeyRepeat(MacUtility::keyRepeatDelay(SDL_DEFAULT_REPEAT_DELAY), MacUtility::keyRepeatInterval(SDL_DEFAULT_REPEAT_INTERVAL));
#else
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif

	// Custom initialization
	return DoInit ();
}


void CStdApp::Clear() {
	SDL_Quit();
	}

void CStdApp::Quit() {
	fQuitMsgReceived = true;
}

bool CStdApp::FlushMessages() {
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;

	// Handle pending SDL messages
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		HandleSDLEvent(event);
	}
	return true;
}

void CStdApp::HandleSDLEvent(SDL_Event& event) {
    // Directly handle QUIT messages.
	switch (event.type) {
		case SDL_QUIT:
		  Quit();
		  break;
	}
	
#ifdef __APPLE__
	MacUtility::ensureWindowInFront();
#endif

    // Everything else goes to the window.
	if (pWindow)
		pWindow->HandleMessage(event);
}

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
	// SDL doesn't support multiple monitors.
	if(!SDL_SetVideoMode(iXRes, iYRes, iColorDepth, SDL_OPENGL | (fFullScreen ? SDL_FULLSCREEN : 0))) {
		sLastError.Copy(SDL_GetError());
		return false;
	}
	SDL_ShowCursor(SDL_DISABLE);
	pWindow->SetSize(iXRes, iYRes);
	OnResolutionChanged(iXRes, iYRes);
	return true;
}

void CStdApp::RestoreVideoMode() {
}

// For Max OS X, the implementation resides in StdMacApp.mm
#ifndef __APPLE__

// stubs
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

void CStdApp::MessageDialog(const char * message)
{
}

#endif

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
