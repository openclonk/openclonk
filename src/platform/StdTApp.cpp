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
#include <StdWindow.h>
#include <StdDDraw2.h>
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

/* CStdApp */

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false),
		Location(""), DoNotDelay(false),
		// main thread
#ifdef HAVE_PTHREAD
		MainThread (pthread_self()),
#endif
		fDspModeSet(false)
{
	Add(&InProc);
}

CStdApp::~CStdApp()
{
	Remove(&InProc);
}

bool CStdApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");
	// Try to figure out the location of the executable
	static char dir[PATH_MAX];
	SCopy(argv[0], dir);
	if (dir[0] != '/')
	{
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
		Location = dir;
	}
	else
	{
		Location = dir;
	}

	// Custom initialization
	return DoInit (argc, argv);
}

void CStdApp::Clear()
{
}

void CStdApp::Quit()
{
	fQuitMsgReceived = true;
}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	return false;
}

void CStdApp::RestoreVideoMode() {}

bool CStdApp::SetVideoMode(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, bool) {}

// Copy the text to the clipboard or the primary selection
bool CStdApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	return false;
}

// Paste the text from the clipboard or the primary selection
StdStrBuf CStdApp::Paste(bool fClipboard)
{
	return StdStrBuf("");
}
// Is there something in the clipboard?
bool CStdApp::IsClipboardFull(bool fClipboard)
{
	return false;
}
// Give up Selection ownership
void CStdApp::ClearClipboard(bool fClipboard)
{
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

bool CStdDDraw::SaveDefaultGammaRamp(CStdWindow * pWindow)
{
	return true;
}

void CStdApp::MessageDialog(const char * message)
{
}

bool CStdApp::FlushMessages()
{
	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;
	return true;
}

void CStdWindow::Clear() {}
CStdWindow::CStdWindow() {}
CStdWindow::~CStdWindow() {}
void CStdWindow::EnumerateMultiSamples(std::vector<int, std::allocator<int> >&) const  {}
void CStdWindow::FlashWindow() {}
bool CStdWindow::GetSize(C4Rect*) {return 0;}
CStdWindow* CStdWindow::Init(CStdWindow::WindowKind, CStdApp*, char const*, CStdWindow*, bool) {return this;}
bool CStdWindow::ReInit(CStdApp*) {return 0;}
bool CStdWindow::RestorePosition(char const*, char const*, bool) {return 0;}
void CStdWindow::SetSize(unsigned int, unsigned int) {}
void CStdWindow::SetTitle(char const*) {}

#endif // USE_CONSOLE
