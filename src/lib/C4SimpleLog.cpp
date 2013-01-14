/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, 2011  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010-2011  Armin Burgmeier
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

// Implement a simplified version of Log so that we don't get undefined
// references when e.g. StdFile attempts to call it - we are not compiling
// C4Log.cpp into the small utility programs because it pulls in a whole
// lot of other dependencies.

#include <C4Include.h>
#include <C4Log.h>

bool fQuiet = false;

bool Log(const char *msg)
{
	if (!fQuiet)
		printf("%s\n", msg);
	return true;
}
bool DebugLog(const char *strMessage) { return Log(strMessage); }
bool LogFatal(const char *strMessage) { return Log(strMessage); }

#define IMPLEMENT_LOGF(func) \
  bool func(const char *msg, ...) { \
    va_list args; va_start(args, msg); \
    StdStrBuf Buf; \
    Buf.FormatV(msg, args); \
    return Log(Buf.getData()); \
  }

IMPLEMENT_LOGF(DebugLogF)
IMPLEMENT_LOGF(LogF)
IMPLEMENT_LOGF(LogSilentF)

