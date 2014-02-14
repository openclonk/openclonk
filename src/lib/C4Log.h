/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* Log file handling */

#ifndef INC_C4Log
#define INC_C4Log

#include <StdBuf.h>

bool OpenLog();
bool CloseLog();

bool Log(const char *szMessage);
bool LogSilent(const char *szMessage);
bool LogF(const char *strMessage, ...) GNUC_FORMAT_ATTRIBUTE;
bool LogSilentF(const char *strMessage, ...) GNUC_FORMAT_ATTRIBUTE;
bool DebugLog(const char *strMessage);
bool DebugLogF(const char *strMessage ...) GNUC_FORMAT_ATTRIBUTE;

bool LogFatal(const char *szMessage); // log message and store it as a fatal error
void ResetFatalError();               // clear any fatal error message
const char *GetFatalError();          // return message that was set as fatal error, if any

size_t GetLogPos(); // get current log position;
bool GetLogSection(size_t iStart, size_t iLength, StdStrBuf &rsOut); // re-read log data from file

// Used to print a backtrace after a crash
int GetLogFD();

#endif
