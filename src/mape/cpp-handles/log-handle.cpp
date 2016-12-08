/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include "C4Include.h"
#include "lib/C4Log.h"

// This implements the Log engine function such that the first log message
// is stored and can be retrieved later by the C API.
std::string first_log;
unsigned int n_logs = 0;

bool Log(const char *msg)
{
	if(first_log.empty())
	{
		assert(n_logs == 0);
		first_log = msg;
	}

	if(*msg != '\0')
		++n_logs;

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

// C API follows here
extern "C" {

void c4_log_handle_clear()
{
	first_log.clear();
	n_logs = 0;
}

const char* c4_log_handle_get_first_log_message()
{
	if(first_log.empty()) return nullptr;
	return first_log.c_str();
}

unsigned int c4_log_handle_get_n_log_messages()
{
	return n_logs;
}

} // extern "C"
