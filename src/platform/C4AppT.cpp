/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2006, 2008-2009, 2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
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

#include <C4Include.h>
#ifdef USE_CONSOLE
#include <C4Window.h>
#include <C4Draw.h>
#include <C4Application.h>

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  endif
static void readline_callback (char *);
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  endif
#endif /* HAVE_READLINE_HISTORY */

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

bool C4AbstractApp::SetVideoMode(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, bool)
{
	return true;
}

// Copy the text to the clipboard or the primary selection
bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	return false;
}

// Paste the text from the clipboard or the primary selection
StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	return StdStrBuf("");
}
// Is there something in the clipboard?
bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return false;
}

CStdInProc::CStdInProc()
{
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install (">", readline_callback);
#endif
}

CStdInProc::~CStdInProc()
{
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
}

bool CStdInProc::Execute(int iTimeout, pollfd *)
{
#ifdef _WIN32
	while (_kbhit())
	{
		// Surely not the most efficient way to do it, but we won't have to read much data anyway.
		char c = getch();
		if (c == '\r')
		{
			if (!CmdBuf.isNull())
			{
				Application.OnCommand(CmdBuf.getData());
				CmdBuf.Clear();
			}
		}
		else if (isprint((unsigned char)c))
			CmdBuf.AppendChar(c);
	}
	// FIXME: handle stdin-close
	return true;
#elif defined(HAVE_LIBREADLINE)
	rl_callback_read_char();
	return true;
#else
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
	char c;
	if (read(0, &c, 1) != 1)
	{
		Application.Quit();
		return false;
	}
	if (c == '\n')
	{
		if (!CmdBuf.isNull())
		{
			Application.OnCommand(CmdBuf.getData());
			CmdBuf.Clear();
		}
	}
	else if (isprint((unsigned char)c))
		CmdBuf.AppendChar(c);
	return true;
#endif
}

#if HAVE_LIBREADLINE
static void readline_callback (char * line)
{
	if (!line)
	{
		Application.Quit();
	}
	else
	{
		Application.OnCommand(line);
	}
#if HAVE_READLINE_HISTORY
	if (line && *line)
	{
		add_history (line);
	}
#endif
	free(line);
}
#endif

bool C4AbstractApp::ApplyGammaRamp(_D3DGAMMARAMP&, bool) { return true; }
bool C4AbstractApp::SaveDefaultGammaRamp(_D3DGAMMARAMP&) { return true; }
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
bool C4Window::GetSize(C4Rect*) {return 0;}
C4Window* C4Window::Init(C4Window::WindowKind, C4AbstractApp*, char const*, const C4Rect *) {return this;}
bool C4Window::ReInit(C4AbstractApp*) {return 0;}
bool C4Window::RestorePosition(char const*, char const*, bool) {return 0;}
void C4Window::RequestUpdate() {}
void C4Window::SetSize(unsigned int, unsigned int) {}
void C4Window::SetTitle(char const*) {}

#endif // USE_CONSOLE
