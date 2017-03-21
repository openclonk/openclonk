/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

/* Main program entry point */

#include "C4Include.h"
#include "game/C4Application.h"

#include "lib/C4Log.h"
#include "game/C4Game.h"
#include "C4Version.h"
#include "network/C4Network2.h"

#ifdef _WIN32
#include <shellapi.h>

void InstallCrashHandler();

int WINAPI WinMain (HINSTANCE hInst,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszCmdParam,
                    int nCmdShow)
{
#if defined(_DEBUG) && defined(_MSC_VER)
	// enable debugheap!
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// This should be handled in an application manifest, but that is
	// decidedly non-trivial to do portably across compilers and compiler
	// versions, so we do it in code instead.
	// Also we aren't really DPI aware (we'd have to default ingame zoom
	// differently and scale the menus), but this is better than clipping.
	// Fixes #891.
	HMODULE user32 = LoadLibrary(L"user32");
	if (user32)
	{
		typedef BOOL (WINAPI *SETPROCESSDPIAWAREPROC)();
		SETPROCESSDPIAWAREPROC SetProcessDPIAware =
			reinterpret_cast<SETPROCESSDPIAWAREPROC>(GetProcAddress(user32, "SetProcessDPIAware"));
		if (SetProcessDPIAware)
			SetProcessDPIAware();
		FreeLibrary(user32);
	}

	InstallCrashHandler();

	// Split wide command line to wide argv array
	std::vector<char*> argv;
	int argc = 0;
	LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!wargv)
	{
		const char *error = "Internal error: Unable to split command line! Exiting.";
		Log(error);
		Application.MessageDialog(error);
		return C4XRV_Failure;
	}
	argv.reserve(argc);
		
	// Convert args to UTF-8
	LPWSTR *curwarg = wargv;
	while(argc--)
	{
		int arglen = WideCharToMultiByte(CP_UTF8, 0, *curwarg, -1, nullptr, 0, 0, 0);
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
	return WinMain(GetModuleHandle(nullptr), 0, 0, 0);
}

#else // _WIN32

#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_BACKWARD
#include <backward-cpp/backward.hpp>

#elif defined(HAVE_SIGNAL_H)
#include <signal.h>

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

static void crash_handler(int signo, siginfo_t * si, void *)
{
	static unsigned signal_count = 0;
	++signal_count;
	switch (signo)
	{
	case SIGINT: case SIGTERM: case SIGHUP:
		if (signal_count < 2) {
			Application.Quit();
			break;
		} // else/fallthrough
	default:
		int logfd = STDERR_FILENO;
		for (;;)
		{
			// Print out the signal
			write(logfd, C4VERSION ": Caught signal ", sizeof (C4VERSION ": Caught signal ") - 1);
			switch (signo)
			{
			case SIGBUS:  write(logfd, "SIGBUS", sizeof ("SIGBUS") - 1); break;
			case SIGILL:  write(logfd, "SIGILL", sizeof ("SIGILL") - 1); break;
			case SIGSEGV: write(logfd, "SIGSEGV", sizeof ("SIGSEGV") - 1); break;
			case SIGABRT: write(logfd, "SIGABRT", sizeof ("SIGABRT") - 1); break;
			case SIGINT:  write(logfd, "SIGINT", sizeof ("SIGINT") - 1); break;
			case SIGHUP:  write(logfd, "SIGHUP", sizeof ("SIGHUP") - 1); break;
			case SIGFPE:  write(logfd, "SIGFPE", sizeof ("SIGFPE") - 1); break;
			case SIGTERM: write(logfd, "SIGTERM", sizeof ("SIGTERM") - 1); break;
			}
			char hex[sizeof(void *) * 2];
			intptr_t x = reinterpret_cast<intptr_t>(si->si_addr);
			switch (signo)
			{
			case SIGILL: case SIGFPE: case SIGSEGV: case SIGBUS: case SIGTRAP:
				write(logfd, " (0x", sizeof (" (0x") - 1);
				for (int i = sizeof(void *) * 2 - 1; i >= 0; --i)
				{
					if ((x & 0xf) > 9)
						hex[i] = 'a' + (x & 0xf) - 9;
					else
						hex[i] = '0' + (x & 0xf);
					x >>= 4;
				}
				write(logfd, hex, sizeof (hex));
				write(logfd, ")", sizeof (")") - 1);
				break;
			}
			write(logfd, "\n", sizeof ("\n") - 1);
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
#ifdef HAVE_BACKWARD
	backward::SignalHandling sh;
#elif defined(HAVE_SIGNAL_H)
	struct sigaction sa;
	sa.sa_sigaction = crash_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	// Quit the program when asked
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	sigaction(SIGHUP, &sa, nullptr);
	// Set up debugging facilities
	sa.sa_flags |= SA_RESETHAND;
	sigaction(SIGBUS, &sa, nullptr);
	sigaction(SIGILL, &sa, nullptr);
	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGFPE, &sa, nullptr);
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
