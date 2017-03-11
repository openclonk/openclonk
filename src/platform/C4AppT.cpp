/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* A wrapper class to OS dependent event and window interfaces, Text version */

#include "C4Include.h"
#include "platform/C4App.h"

#include "platform/C4Window.h"
#include "graphics/C4Draw.h"
#include "game/C4Application.h"

/* C4AbstractApp */

C4AbstractApp::C4AbstractApp(): Active(false), fQuitMsgReceived(false),
		// main thread
#ifdef HAVE_PTHREAD
		MainThread (pthread_self()),
#endif
		fDspModeSet(false)
{
	Add(&InProc);
}

C4AbstractApp::~C4AbstractApp()
{
	Remove(&InProc);
}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");

	// Custom initialization
	return DoInit (argc, argv);
}

void C4AbstractApp::Clear()
{
}

void C4AbstractApp::Quit()
{
	fQuitMsgReceived = true;
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	return false;
}

void C4AbstractApp::RestoreVideoMode() {}

bool C4AbstractApp::SetVideoMode(int, int, unsigned int, unsigned int, bool)
{
	return true;
}

// Copy the text to the clipboard or the primary selection
bool C4AbstractApp::Copy(const std::string &text, bool fClipboard)
{
	return false;
}

// Paste the text from the clipboard or the primary selection
std::string C4AbstractApp::Paste(bool fClipboard)
{
	return std::string();
}
// Is there something in the clipboard?
bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return false;
}

void C4AbstractApp::MessageDialog(const char * message) {}

bool C4AbstractApp::FlushMessages()
{
	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;
	return true;
}

void C4Window::Clear() {}
C4Window::C4Window() {}
C4Window::~C4Window() {}
void C4Window::EnumerateMultiSamples(std::vector<int, std::allocator<int> >&) const  {}
void C4Window::FlashWindow() {}
void C4Window::GrabMouse(bool) {}
bool C4Window::GetSize(C4Rect*) {return 0;}
C4Window* C4Window::Init(C4Window::WindowKind, C4AbstractApp*, char const*, const C4Rect *) {return this;}
bool C4Window::ReInit(C4AbstractApp*) {return 0;}
bool C4Window::RestorePosition(char const*, char const*, bool) {return 0;}
void C4Window::RequestUpdate() {}
void C4Window::SetSize(unsigned int, unsigned int) {}
void C4Window::SetTitle(char const*) {}
