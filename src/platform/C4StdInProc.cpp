/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, Peter Wortmann
 * Copyright (c) 2005-2006, Günther Brammer
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include "C4StdInProc.h"

#include <C4Application.h>

#ifdef HAVE_LIBREADLINE
#include <readline.h>
#include <history.h>

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
	if (line && *line)
	{
		add_history (line);
	}
	free(line);
}

C4StdInProc::C4StdInProc()
{
	rl_callback_handler_install ("", readline_callback);
}

C4StdInProc::~C4StdInProc()
{
	rl_callback_handler_remove();
}

bool C4StdInProc::Execute(int iTimeout, pollfd *)
{
	rl_callback_read_char();
	return true;
}

#else

C4StdInProc::C4StdInProc() { }

C4StdInProc::~C4StdInProc() { }

bool C4StdInProc::Execute(int iTimeout, pollfd *)
{
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
}

#endif /* HAVE_LIBREADLINE */
