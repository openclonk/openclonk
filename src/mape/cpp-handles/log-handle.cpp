/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009 Armin Burgmeier
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

#include <C4Include.h>
#include <C4Log.h>

// This implements the Log engine function such that the first log message
// is stored and can be retrieved later by the C API.
std::string first_log;
bool Log(const char *msg)
{
	if(first_log.empty())
		first_log = msg;
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
}

const char* c4_log_handle_get_first_log_message()
{
	if(first_log.empty()) return NULL;
	return first_log.c_str();
}

} // extern "C"
