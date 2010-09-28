/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008  Peter Wortmann
 * Copyright (c) 2008  Günther Brammer
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de
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

// An inotify wrapper

#ifndef STD_FILE_MONITOR_H_INC
#define STD_FILE_MONITOR_H_INC

#include <StdScheduler.h>
#include <C4InteractiveThread.h>
#include <map>

#ifdef __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#endif

class C4FileMonitor: public StdSchedulerProc, public C4InteractiveThread::Callback
{

public:

	typedef void (*ChangeNotify)(const char *, const char *);

	C4FileMonitor(ChangeNotify pCallback);
	~C4FileMonitor();

	void StartMonitoring();
	void AddDirectory(const char *szDir);
	void AddTree(const char *szDir);
	//void Remove(const char * file);

	// StdSchedulerProc:
	virtual bool Execute(int iTimeout = -1, pollfd * = 0);

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent();
#else
	virtual void GetFDs(std::vector<struct pollfd> & FDs);
#endif

	// C4InteractiveThread::Callback:
	virtual void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData);

private:

	bool fStarted;
	ChangeNotify pCallback;

#if defined(HAVE_SYS_INOTIFY_H) || defined(HAVE_SYS_SYSCALL_H)
	int fd;
	std::map<int, const char *> watch_descriptors;
#elif defined(_WIN32)

	HANDLE hEvent;

	struct TreeWatch
	{
		HANDLE hDir;
		StdCopyStrBuf DirName;
		OVERLAPPED ov;
		char Buffer[1024];
		TreeWatch *Next;
	};
	TreeWatch *pWatches;

	void HandleNotify(const char *szDir, const struct _FILE_NOTIFY_INFORMATION *pNotify);
#elif defined(__APPLE__)
	FSEventStreamRef eventStream;
	FSEventStreamContext context;
	CFMutableArrayRef watchedDirectories;
	void StartStream();
	void StopStream();
#endif
};

#endif // STD_FILE_MONITOR_H_INC
