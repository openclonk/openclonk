/*
 * OpenClonk, http://www.openclonk.org
 *
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
/* A simple scheduler for ccoperative multitasking */

#ifndef STDSCHEDULER_H
#define STDSCHEDULER_H

#include "platform/StdSync.h"

// Events are Windows-specific
#ifdef _WIN32
#define STDSCHEDULER_USE_EVENTS
#define HAVE_WINTHREAD
#define STDSCHEDULER_EVENT_MESSAGE INVALID_HANDLE_VALUE
struct pollfd;
#ifndef STDSCHEDULER_USE_EVENTS
#include "platform/C4windowswrapper.h"
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
#ifdef __APPLE__
#include <sched.h>
#endif
#endif // _WIN32


#include <vector>

typedef struct _GMainLoop GMainLoop;

// Abstract class for a process
class StdSchedulerProc
{
private:
	class StdScheduler *scheduler;
protected:
	void Changed();
public:

	StdSchedulerProc(): scheduler(nullptr) {}
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
	virtual C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow);

	// Is the process signal currently set?
	bool IsSignaled();

	// Is this the expensive game tick?
	virtual bool IsLowPriority() { return false; }
	virtual bool IsNotify() { return false; }
	virtual uint32_t TimerInterval() { return 0; }

	friend class StdScheduler;

};

// A simple timer proc
class CStdTimerProc : public StdSchedulerProc
{
public:
	CStdTimerProc(uint32_t iDelay) : tLastTimer(C4TimeMilliseconds::NegativeInfinity), iDelay(iDelay) { }
	~CStdTimerProc() { Set(); }

private:
	C4TimeMilliseconds tLastTimer;
	uint32_t iDelay;

public:
	void Set()
	{
		tLastTimer = C4TimeMilliseconds::NegativeInfinity;
	}
	void SetDelay(uint32_t inDelay) { iDelay = inDelay; Changed(); }
	bool CheckAndReset()
	{
		C4TimeMilliseconds tTime = C4TimeMilliseconds::Now();
		if (tTime < tLastTimer + iDelay) return false;
		// Compensate light drifting
		int32_t iDrift = tTime - (tLastTimer + iDelay); // a positive time difference because of above check
		tLastTimer = tTime - std::min(iDrift, (int32_t) iDelay / 2);
		return true;
	}

	// StdSchedulerProc override
	virtual C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow)
	{
		return tLastTimer + iDelay;
	}
	virtual uint32_t TimerInterval() { return iDelay; }
};

class C4ApplicationSec1Timer : protected CStdTimerProc
{
public:
	C4ApplicationSec1Timer() : CStdTimerProc(1000) { }
	virtual void OnSec1Timer() = 0;
protected:
	virtual bool Execute(int, pollfd *)
	{
		if (CheckAndReset())
			OnSec1Timer();
		return true;
	}
};

// A simple alertable proc
class CStdNotifyProc : public StdSchedulerProc
{
public:
	CStdNotifyProc();

	void Notify();
	bool CheckAndReset();
	virtual bool IsNotify() { return true; }

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
	std::vector<StdSchedulerProc*> procs;
	bool isInManualLoop;

	// Unblocker
	class NoopNotifyProc : public CStdNotifyProc
	{
	public: virtual bool Execute(int, pollfd *) { CheckAndReset(); return true; }
	};
	NoopNotifyProc Unblocker;

	// Dummy lists (preserved to reduce allocs)
#ifdef STDSCHEDULER_USE_EVENTS
	std::vector<HANDLE> eventHandles;
	std::vector<StdSchedulerProc*> eventProcs;
#endif

public:
	int getProcCnt() const { return procs.size()-1; } // ignore internal NoopNotifyProc
	bool hasProc(StdSchedulerProc *pProc) { return std::find(procs.begin(), procs.end(), pProc) != procs.end(); }
	bool IsInManualLoop() { return isInManualLoop; }

	void Clear();
	void Set(StdSchedulerProc **ppProcs, int iProcCnt);
	void Add(StdSchedulerProc *pProc);
	void Remove(StdSchedulerProc *pProc);
	
	// extra events for above Add/Remove methods
	void Added(StdSchedulerProc *pProc);
	void Removing(StdSchedulerProc *pProc);
	// called by StdSchedulerProcs when something important about their configuration changed
	void Changed(StdSchedulerProc *pProc);
	// needs to be called on thread tasks for this scheduler are meant to be run on
	void StartOnCurrentThread();

	C4TimeMilliseconds GetNextTick(C4TimeMilliseconds tNow);
	bool ScheduleProcs(int iTimeout = 1000/36);
	void UnBlock();

protected:
	// overridable
	virtual void OnError(StdSchedulerProc *) { }
	virtual bool DoScheduleProcs(int iTimeout);
};

// A simple process scheduler thread
class StdSchedulerThread : public StdScheduler
{
public:
	StdSchedulerThread();
	virtual ~StdSchedulerThread();

private:

	// thread control
	bool fRunThreadRun;

	bool fThread;
#ifdef HAVE_WINTHREAD
	uintptr_t iThread;
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
	uintptr_t iThread;
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
	virtual bool IsSelfDestruct() { return false; } // whether thread should delete itself after execution finished

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
