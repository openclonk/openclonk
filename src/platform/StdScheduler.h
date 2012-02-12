/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006, 2009  Peter Wortmann
 * Copyright (c) 2006, 2009, 2011  Günther Brammer
 * Copyright (c) 2008  Sven Eberhardt
 * Copyright (c) 2009  Nicolas Hake
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
/* A simple scheduler for ccoperative multitasking */

#ifndef STDSCHEDULER_H
#define STDSCHEDULER_H

#include "StdSync.h"

// Events are Windows-specific
#ifdef _WIN32
#define STDSCHEDULER_USE_EVENTS
#define HAVE_WINTHREAD
#define STDSCHEDULER_EVENT_MESSAGE INVALID_HANDLE_VALUE
struct pollfd;
#ifndef STDSCHEDULER_USE_EVENTS
#include <C4windowswrapper.h>
#include <winsock2.h>
#endif // STDSCHEDULER_USE_EVENTS
#else // _WIN32
#ifdef HAVE_POLL_H
#include <poll.h>
#include <vector>
#else // HAVE_POLL_H
#include <sys/select.h>
#endif // HAVE_POLL_H
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif // HAVE_PTHREAD
#endif // _WIN32

// helper
inline int MaxTimeout(int iTimeout1, int iTimeout2)
{
	return (iTimeout1 == -1 || iTimeout2 == -1) ? -1 : Max(iTimeout1, iTimeout2);
}

typedef struct _GMainLoop GMainLoop;

// Abstract class for a process
class StdSchedulerProc
{
public:
	virtual ~StdSchedulerProc() { }

	// Do whatever the process wishes to do. Should not block longer than the timeout value.
	// Is called whenever the process is signaled or a timeout occurs.
	virtual bool Execute(int iTimeout = -1, pollfd * readyfds = 0) = 0;

	// As Execute, but won't return until the given timeout value has elapsed or a failure occurs
	bool ExecuteUntil(int iTimeout = -1);

	// Signal for calling Execute()
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent() { return 0; }
#else
	virtual void GetFDs(std::vector<struct pollfd> &) { }
#endif

	// Call Execute() after this time has elapsed
	// -1 means no timeout (infinity).
	virtual int GetNextTick(int Now) { return -1; }

	// Is the process signal currently set?
	bool IsSignaled();

	// Is this the expensive game tick?
	virtual bool IsLowPriority() { return false; }

};

// A simple timer proc
class CStdTimerProc : public StdSchedulerProc
{
public:
	CStdTimerProc(uint32_t iDelay) : iLastTimer(0), iDelay(iDelay) { }
	~CStdTimerProc() { }

private:
	uint32_t iLastTimer, iDelay;

public:
	void Set() { iLastTimer = 0; }
	void SetDelay(uint32_t inDelay) { iDelay = inDelay; }
	bool CheckAndReset()
	{
		if (GetTime() < iLastTimer + iDelay) return false;
		// Compensate light drifting
		uint32_t iTime = GetTime();
		uint32_t iDrift = iTime - iLastTimer - iDelay; // >= 0 because of Check()
		iLastTimer = iTime - Min(iDrift, iDelay / 2);
		return true;
	}

	// StdSchedulerProc override
	virtual int GetNextTick(int Now)
	{
		return iLastTimer + iDelay;
	}
};

// A simple alertable proc
class CStdNotifyProc : public StdSchedulerProc
{
public:
	CStdNotifyProc();

	void Notify();
	bool CheckAndReset();

#ifdef STDSCHEDULER_USE_EVENTS
	~CStdNotifyProc() { }

	// StdSchedulerProc override
	virtual HANDLE GetEvent() { return Event.GetEvent(); }

private:
	CStdEvent Event;

#else // STDSCHEDULER_USE_EVENTS
	~CStdNotifyProc();

	// StdSchedulerProc override
	virtual void GetFDs(std::vector<struct pollfd> & checkfds);

private:
	int fds[2];

#endif
};

#ifdef STDSCHEDULER_USE_EVENTS
class CStdMultimediaTimerProc : public CStdNotifyProc
{
public:
	CStdMultimediaTimerProc(uint32_t iDelay);
	~CStdMultimediaTimerProc();

private:
	static int iTimePeriod;
	uint32_t uCriticalTimerDelay;

	UINT idCriticalTimer,uCriticalTimerResolution;
	CStdEvent Event;
	bool Check() { return Event.WaitFor(0); }

public:

	void SetDelay(uint32_t iDelay);
	void Set() { Event.Set(); }
	bool CheckAndReset();

	// StdSchedulerProc overrides
	virtual HANDLE GetEvent() { return Event.GetEvent(); }

};

#elif defined(HAVE_SYS_TIMERFD_H)
// timer proc using a timerfd
class CStdMultimediaTimerProc : public StdSchedulerProc
{
public:
	CStdMultimediaTimerProc(uint32_t iDelay);
	~CStdMultimediaTimerProc();

private:
	int fd;

public:
	void Set();
	void SetDelay(uint32_t inDelay);
	bool CheckAndReset();
	// StdSchedulerProc overrides
	virtual void GetFDs(std::vector<struct pollfd> & checkfds);
};

#else
#define CStdMultimediaTimerProc CStdTimerProc
#endif

// A simple process scheduler
class StdScheduler
{
public:
	StdScheduler();
	virtual ~StdScheduler();

private:
	// Process list
	StdSchedulerProc **ppProcs;
	int iProcCnt, iProcCapacity;

	// Unblocker
	class NoopNotifyProc : public CStdNotifyProc
	{
	public: virtual bool Execute(int, pollfd *) { CheckAndReset(); return true; }
	};
	NoopNotifyProc Unblocker;

	// Dummy lists (preserved to reduce allocs)
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE *pEventHandles;
	StdSchedulerProc **ppEventProcs;
#endif

public:
	int getProcCnt() const { return iProcCnt-1; } // ignore internal NoopNotifyProc
	int getProc(StdSchedulerProc *pProc);
	bool hasProc(StdSchedulerProc *pProc) { return getProc(pProc) >= 0; }

	void Clear();
	void Set(StdSchedulerProc **ppProcs, int iProcCnt);
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool ScheduleProcs(int iTimeout = -1);
	void UnBlock();

protected:
	// overridable
	virtual void OnError(StdSchedulerProc *) { }

private:
	void Enlarge(int iBy);

};

// A simple process scheduler thread
class StdSchedulerThread : public StdScheduler
{
public:
	StdSchedulerThread();
	virtual ~StdSchedulerThread();

private:

	// thread control
	bool fRunThreadRun, fWait;

	bool fThread;
#ifdef HAVE_WINTHREAD
	unsigned long iThread;
#elif defined(HAVE_PTHREAD)
	pthread_t Thread;
#endif

public:
	void Clear();
	void Set(StdSchedulerProc **ppProcs, int iProcCnt);
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);

	bool Start();
	void Stop();

private:

	// thread func
#ifdef HAVE_WINTHREAD
	static void __cdecl _ThreadFunc(void *);
#elif defined(HAVE_PTHREAD)
	static void *_ThreadFunc(void *);
#endif
	unsigned int ThreadFunc();

};

class StdThread
{
private:
	bool fStarted;
	bool fStopSignaled;

#ifdef HAVE_WINTHREAD
	unsigned long iThread;
#elif defined(HAVE_PTHREAD)
	pthread_t Thread;
#endif

public:
	StdThread();
	virtual ~StdThread() { Stop(); }

	bool Start();
	void SignalStop(); // mark thread to stop but don't wait
	void Stop();

	bool IsStarted() { return fStarted; }

protected:
	virtual void Execute() = 0;

	bool IsStopSignaled();

private:
	// thread func
#ifdef HAVE_WINTHREAD
	static void __cdecl _ThreadFunc(void *);
#elif defined(HAVE_PTHREAD)
	static void *_ThreadFunc(void *);
#endif
	unsigned int ThreadFunc();
};

#endif
