/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2016, The OpenClonk Team and contributors
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

// Proxies the logging functions into a class so we can test that something
// gets logged

#include "TestLog.h"

// Override ALL of the C4SimpleLog.cpp functions here because otherwise MSVC
// pulls the .obj in and will end up with multiple definitions of a symbol.
#define FORWARD_UNFORMATTED(func) bool func(const char *msg) { return TestLog::handler ? TestLog::handler->func(msg) : true; }
FORWARD_UNFORMATTED(Log)
FORWARD_UNFORMATTED(DebugLog)
FORWARD_UNFORMATTED(LogSilent)
FORWARD_UNFORMATTED(LogFatal)
#undef FORWARD_UNFORMATTED

#define FORWARD_FORMATTED(func) bool func(const char *msg, ...) \
	{ \
		va_list args; va_start(args, msg); \
		bool result = TestLog::handler ? TestLog::handler->func(msg, args) : true; \
		va_end(args); \
		return result; \
	}
FORWARD_FORMATTED(DebugLogF)
FORWARD_FORMATTED(LogF)
FORWARD_FORMATTED(LogSilentF)
#undef FORWARD_FORMATTED

TestLog *TestLog::handler = 0;

void TestLog::setHandler(TestLog *new_handler)
{
	handler = new_handler;
}

TestLog::TestLog()
{
	setHandler(this);
}

TestLog::~TestLog()
{
	// Make sure there's no deleted logging handler set
	if (handler == this)
		handler = 0;
}

// Default implementation does nothing
bool TestLog::Log(const char * /* msg */) { return true; }
bool TestLog::DebugLog(const char * /* msg */) { return true; }
bool TestLog::LogFatal(const char * /* msg */) { return true; }
bool TestLog::LogSilent(const char * /* msg */) { return true; }

bool TestLog::LogF(const char * /* msg */, va_list /* args */) { return true; }
bool TestLog::DebugLogF(const char * /* msg */, va_list /* args */) { return true; }
bool TestLog::LogSilentF(const char * /* msg */, va_list /* args */) { return true; }
