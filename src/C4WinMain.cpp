/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005, 2007-2008, 2010  Günther Brammer
 * Copyright (c) 2005, 2008  Peter Wortmann
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* Main program entry point */

#include <C4Include.h>
#include <C4Application.h>

#include <C4Log.h>
#include <C4Game.h>
#include <C4Version.h>
#include "C4Network2.h"

#ifdef _WIN32
#include <shellapi.h>

#ifdef GENERATE_MINI_DUMP

// Dump generation on crash
#include <specstrings.h>
#include <dbghelp.h>
#include <fcntl.h>

static bool FirstCrash = true;

LONG WINAPI GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
	if (!FirstCrash) return EXCEPTION_EXECUTE_HANDLER;
	FirstCrash = false;

	// Open dump file
	const char *szFilename = Config.AtExePath("Clonk.dmp");
	HANDLE file = CreateFile(szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);

	// Write dump
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;
	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = true;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
	                  file, MiniDumpNormal, &ExpParam, NULL, NULL);

	// (Try to) log it
	LogF("FATAL: Clonk crashed! Some developer might be interested in Clonk.dmp...");

	// Pass exception
	return EXCEPTION_EXECUTE_HANDLER;
}

#endif // GENERATE_MINI_DUMP

int WINAPI WinMain (HINSTANCE hInst,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszCmdParam,
                    int nCmdShow)
{
#if defined(_DEBUG) && defined(_MSC_VER)
	// enable debugheap!
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#if defined(GENERATE_MINI_DUMP)
	SetUnhandledExceptionFilter(GenerateDump);
#endif
	// Split wide command line to wide argv array
	std::vector<char*> argv;
	int argc = 0;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!wargv)
		throw std::runtime_error("Unable to split command line");
	argv.reserve(argc);
		
	// Convert args to UTF-8
	LPWSTR *curwarg = wargv;
	while(argc--)
	{
		int arglen = WideCharToMultiByte(CP_UTF8, 0, *curwarg, -1, NULL, 0, 0, 0);
		char *utf8arg = new char[arglen ? arglen : 1];
		WideCharToMultiByte(CP_UTF8, 0, *curwarg, -1, utf8arg, arglen, 0, 0);
		argv.push_back(utf8arg);
		++curwarg;
	}
	LocalFree(wargv);

	// Init application
	Application.SetInstance(hInst);
	if (!Application.Init(argv.size(), &argv[0]))
	{
		Application.Clear();
		return C4XRV_Failure;
	}

	// Run it
	Application.Run();
	Application.Clear();

	// delete arguments
	for(std::vector<char*>::const_iterator it = argv.begin(); it != argv.end(); ++it)
		delete[] *it;
	argv.clear();
	// Return exit code
	if (!Game.GameOver) return C4XRV_Aborted;
	return C4XRV_Completed;
}

int main()
{
	return WinMain(GetModuleHandle(NULL), 0, 0, 0);
}

#else // _WIN32

#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

static void crash_handler(int signo)
{
	int logfd = STDERR_FILENO;
	ssize_t ignore;
	for (;;)
	{
		// Print out the signal
		ignore = write(logfd, C4VERSION ": Caught signal ", sizeof (C4VERSION ": Caught signal ") - 1);
		switch (signo)
		{
		case SIGBUS:  ignore = write(logfd, "SIGBUS", sizeof ("SIGBUS") - 1); break;
		case SIGILL:  ignore = write(logfd, "SIGILL", sizeof ("SIGILL") - 1); break;
		case SIGSEGV: ignore = write(logfd, "SIGSEGV", sizeof ("SIGSEGV") - 1); break;
		case SIGABRT: ignore = write(logfd, "SIGABRT", sizeof ("SIGABRT") - 1); break;
		case SIGINT:  ignore = write(logfd, "SIGINT", sizeof ("SIGINT") - 1); break;
		case SIGQUIT: ignore = write(logfd, "SIGQUIT", sizeof ("SIGQUIT") - 1); break;
		case SIGFPE:  ignore = write(logfd, "SIGFPE", sizeof ("SIGFPE") - 1); break;
		case SIGTERM: ignore = write(logfd, "SIGTERM", sizeof ("SIGTERM") - 1); break;
		}
		ignore = write(logfd, "\n", sizeof ("\n") - 1);
		if (logfd == STDERR_FILENO) logfd = GetLogFD();
		else break;
		if (logfd < 0) break;
	}
#ifdef HAVE_EXECINFO_H
	// Get the backtrace
	void *stack[100];
	int count = backtrace(stack, 100);
	// Print it out
	backtrace_symbols_fd (stack, count, STDERR_FILENO);
	// Also to the log file
	if (logfd >= 0)
		backtrace_symbols_fd (stack, count, logfd);
#endif
	// Bye.
	_exit(C4XRV_Failure);
}
#endif // HAVE_SIGNAL_H

static void restart(char * argv[])
{
	// Close all file descriptors except stdin, stdout, stderr
	int open_max = sysconf (_SC_OPEN_MAX);
	for (int fd = 4; fd < open_max; fd++)
		fcntl (fd, F_SETFD, FD_CLOEXEC);
	// Execute the new engine
	execlp(argv[0], argv[0], static_cast<char *>(0));
}

int main (int argc, char * argv[])
{
	if (!geteuid())
	{
		printf("Do not run %s as root!\n", argc ? argv[0] : "this program");
		return C4XRV_Failure;
	}
#ifdef HAVE_SIGNAL_H
	// Set up debugging facilities
	signal(SIGBUS, crash_handler);
	signal(SIGILL, crash_handler);
	signal(SIGSEGV, crash_handler);
	signal(SIGABRT, crash_handler);
	signal(SIGINT, crash_handler);
	signal(SIGQUIT, crash_handler);
	signal(SIGFPE, crash_handler);
	signal(SIGTERM, crash_handler);
#endif

	// Init application
	if (!Application.Init(argc, argv))
	{
		Application.Clear();
		return C4XRV_Failure;
	}
	// Execute application
	Application.Run();
	// free app stuff
	Application.Clear();
	if (Application.restartAtEnd) restart(argv);
	// Return exit code
	if (!Game.GameOver) return C4XRV_Aborted;
	return C4XRV_Completed;
}
#endif
