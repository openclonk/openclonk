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

#include <C4Include.h>
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

/* CStdApp */

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false),
		Location(""), DoNotDelay(false),
		// main thread
#ifdef HAVE_PTHREAD
		MainThread (pthread_self()),
#endif
		fDspModeSet(false)
{
}

CStdApp::~CStdApp()
{
}

bool CStdApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");
	// Try to figure out the location of the executable
	this->argc=argc; this->argv=argv;
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
	// botch arguments
	static std::string s("\"");
	for (int i = 1; i < argc; ++i)
	{
		s.append(argv[i]);
		s.append("\" \"");
	}
	s.append("\"");
	szCmdLine = s.c_str();

#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install (">", readline_callback);
	readline_callback_use_this_app = this;
#endif
	// Custom initialization
	return DoInit ();
}

void CStdApp::Clear()
{
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
}

void CStdApp::Quit()
{
	fQuitMsgReceived = true;
}
/*
    // handle commands
    if(FD_ISSET(0, &rfds))
    {
      // Do not call OnStdInInput to be able to return
      // HR_Failure when ReadStdInCommand returns false
      if(!ReadStdInCommand())
        return HR_Failure;
    }
*/
bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, uint32_t iMonitor)
{
	return false;
}

void CStdApp::RestoreVideoMode() {}

bool CStdApp::SetVideoMode(unsigned int, unsigned int, unsigned int, unsigned int, bool) {}

// Copy the text to the clipboard or the primary selection
void CStdApp::Copy(const StdStrBuf & text, bool fClipboard)
{
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

bool CStdApp::ReadStdInCommand()
{
#if HAVE_LIBREADLINE
	rl_callback_read_char();
	return true;
#else
	// Surely not the most efficient way to do it, but we won't have to read much data anyway.
	char c;
	if (read(0, &c, 1) != 1)
		return false;
	if (c == '\n')
	{
		if (!CmdBuf.isNull())
		{
			OnCommand(CmdBuf.getData()); CmdBuf.Clear();
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
		readline_callback_use_this_app->Quit();
	}
	else
	{
		readline_callback_use_this_app->OnCommand(line);
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

void CStdApp::OnStdInInput()
{
	if (!ReadStdInCommand())
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
