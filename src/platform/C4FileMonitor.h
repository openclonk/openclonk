/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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

// An inotify wrapper

#ifndef STD_FILE_MONITOR_H_INC
#define STD_FILE_MONITOR_H_INC

#include "network/C4InteractiveThread.h"
#include "platform/StdScheduler.h"

#ifdef __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#import "platform/ObjectiveCAssociated.h"
#endif

class C4FileMonitor: public StdSchedulerProc, public C4InteractiveThread::Callback
#ifdef __APPLE__
, public ObjectiveCAssociated
#endif
{

public:

	typedef void (*ChangeNotify)(const char *, const char *);

	C4FileMonitor(ChangeNotify pCallback);
	~C4FileMonitor() override;

	void StartMonitoring();
	void AddDirectory(const char *szDir);

	// StdSchedulerProc:
	bool Execute(int iTimeout = -1, pollfd * = nullptr) override;

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override;
#else
	void GetFDs(std::vector<struct pollfd> & FDs) override;
#endif

	// C4InteractiveThread::Callback:
	void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) override;

private:

	bool fStarted;
	ChangeNotify pCallback;

#ifdef HAVE_SYS_INOTIFY_H
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
	void StartStream();
	void StopStream();
#endif
};

#endif // STD_FILE_MONITOR_H_INC
