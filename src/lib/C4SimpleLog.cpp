/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, Günther Brammer
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

