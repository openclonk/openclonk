/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2009  Peter Wortmann
 * Copyright (c) 2006-2007, 2009  Günther Brammer
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
#include "StdScheduler.h"
#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include <vector>

#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_SHARE_H
#include <share.h>
#endif
#ifdef _WIN32
#include <process.h>
#include <mmsystem.h>

static int pipe(int *phandles)
{
	// This doesn't work with select(), rendering the non-event-solution
	// unusable for Win32. Oh well, it isn't desirable performance-wise, anyway.
	return _pipe(phandles, 10, O_BINARY);
}
#endif

#ifdef HAVE_UNISTD_H
// For pipe()
#include <unistd.h>
#endif

// *** StdSchedulerProc

// Keep calling Execute until timeout has elapsed
bool StdSchedulerProc::ExecuteUntil(int iTimeout)
{
	// Infinite?
	if(iTimeout < 0)
		for(;;)
			if(!Execute())
				return false;
	// Calculate endpoint
	unsigned int iStopTime = timeGetTime() + iTimeout;
	for(;;)
	{
		// Call execute with given timeout
		if(!Execute(Max(iTimeout, 0)))
			return false;
		// Calculate timeout
		unsigned int iTime = timeGetTime();
		if(iTime >= iStopTime)
			break;
		iTimeout = int(iStopTime - iTime);
	}
	// All ok.
	return true;
}

// Is this process currently signaled?
bool StdSchedulerProc::IsSignaled()
{
#ifdef STDSCHEDULER_USE_EVENTS
	return GetEvent() && WaitForSingleObject(GetEvent(), 0) == WAIT_OBJECT_0;
#else
	// Initialize file descriptor sets
  	std::vector<struct pollfd> fds;

	// Get file descriptors
	GetFDs(fds);

	// Test
	return poll(&fds[0], fds.size(), 0) > 0;
#endif
}

// *** StdScheduler

StdScheduler::StdScheduler()
	: ppProcs(NULL), iProcCnt(0), iProcCapacity(0)
{
#ifdef STDSCHEDULER_USE_EVENTS
	pEventHandles = NULL;
	ppEventProcs = NULL;
#endif
	Add(&Unblocker);
}

StdScheduler::~StdScheduler()
{
	Clear();
}


int StdScheduler::getProc(StdSchedulerProc *pProc)
{
	for(int i = 0; i < iProcCnt; i++)
		if(ppProcs[i] == pProc)
			return i;
	return -1;
}

void StdScheduler::Clear()
{
	delete[] ppProcs; ppProcs = NULL;
#ifdef STDSCHEDULER_USE_EVENTS
	delete[] pEventHandles; pEventHandles = NULL;
	delete[] ppEventProcs; ppEventProcs = NULL;
#endif
	iProcCnt = iProcCapacity = 0;
}

void StdScheduler::Set(StdSchedulerProc **ppnProcs, int inProcCnt)
{
	// Remove previous data
	Clear();
	// Set size
	Enlarge(inProcCnt - iProcCapacity);
	// Copy new
	iProcCnt = inProcCnt;
	for(int i = 0; i < iProcCnt; i++)
		ppProcs[i] = ppnProcs[i];
}

void StdScheduler::Add(StdSchedulerProc *pProc)
{
	// Alrady in list?
	if(hasProc(pProc)) return;
	// Enlarge
	if(iProcCnt >= iProcCapacity) Enlarge(10);
	// Add
	ppProcs[iProcCnt] = pProc;
	iProcCnt++;
}

void StdScheduler::Remove(StdSchedulerProc *pProc)
{
	// Search
	int iPos = getProc(pProc);
	// Not found?
	if(iPos < 0 || iPos >= iProcCnt) return;
	// Remove
	for(int i = iPos + 1; i < iProcCnt; i++)
		ppProcs[i-1] = ppProcs[i];
	iProcCnt--;
}

bool StdScheduler::ScheduleProcs(int iTimeout)
{
	// Needs at least one process to work properly
	if(!iProcCnt) return false;

	// Get timeout
	int i; int iProcTick; int Now = timeGetTime();
	for(i = 0; i < iProcCnt; i++)
		if((iProcTick = ppProcs[i]->GetNextTick(Now)) >= 0)
			if(iTimeout == -1 || iTimeout + Now > iProcTick)
				iTimeout = Max(iProcTick - Now, 0);

#ifdef STDSCHEDULER_USE_EVENTS

	// Collect event handles
	int iEventCnt = 0; HANDLE hEvent;
	StdSchedulerProc *pMessageProc = NULL;
	for(i = 0; i < iProcCnt; i++)
		if(hEvent = ppProcs[i]->GetEvent())
			if(hEvent == STDSCHEDULER_EVENT_MESSAGE)
				pMessageProc = ppProcs[i];
			else
			{
				pEventHandles[iEventCnt] = hEvent;
				ppEventProcs[iEventCnt] = ppProcs[i];
				iEventCnt++;
			}

	// Wait for something to happen
	DWORD ret; DWORD dwMsec = iTimeout < 0 ? INFINITE : iTimeout;
	if(pMessageProc)
		ret = MsgWaitForMultipleObjects(iEventCnt, pEventHandles, FALSE, dwMsec, QS_ALLEVENTS);
	else
		ret = WaitForMultipleObjects(iEventCnt, pEventHandles, FALSE, dwMsec);

	bool fSuccess = true;

	// Event?
	if(ret != WAIT_TIMEOUT)
		{
		// Which event?
		int iEventNr = ret - WAIT_OBJECT_0;

		// Execute the signaled process
		StdSchedulerProc *pProc = iEventNr < iEventCnt ? ppEventProcs[iEventNr] : pMessageProc;
		if(!pProc->Execute(0))
			{
			OnError(pProc);
			fSuccess = false;
			}
		}

#else

	// Initialize file descriptor sets
	std::vector<struct pollfd> fds;

	// Collect file descriptors
	for(i = 0; i < iProcCnt; i++)
		ppProcs[i]->GetFDs(fds);

	// Wait for something to happen
	int cnt = poll(&fds[0], fds.size(), iTimeout);

	bool fSuccess = true;

	if(cnt > 0)
	{
		// Which process?
		std::vector<struct pollfd> test_fds;
		test_fds.reserve(fds.size());
		for(i = 0; i < iProcCnt; i++)
		{
			// Get FDs for this process alone
			int prev_fds = test_fds.size();
			ppProcs[i]->GetFDs(test_fds);

			// Check intersection
			for (int j = prev_fds; j < test_fds.size(); ++j) if (fds[j].events & fds[j].revents)
			{
				if(!ppProcs[i]->Execute(0))
				{
					OnError(ppProcs[i]);
					fSuccess = false;
				}
				break;
			}
		}
	}
	else if (cnt < 0)
	{
		printf("StdScheduler::Execute: poll failed %s\n",strerror(errno));
	}
#endif

	// Execute all processes with timeout
	Now = timeGetTime();
	for(i = 0; i < iProcCnt; i++)
	{
		iProcTick = ppProcs[i]->GetNextTick(Now);
		if(iProcTick >= 0 && iProcTick <= Now)
			if(!ppProcs[i]->Execute(0))
			{
				OnError(ppProcs[i]);
				fSuccess = false;
			}
	}

	return fSuccess;
}

void StdScheduler::UnBlock()
{
	Unblocker.Notify();
}

void StdScheduler::Enlarge(int iBy)
{
	iProcCapacity += iBy;
	// Realloc
	StdSchedulerProc **ppnProcs = new StdSchedulerProc *[iProcCapacity];
	// Set data
	for(int i = 0; i < iProcCnt; i++)
		ppnProcs[i] = ppProcs[i];
	delete[] ppProcs;
	ppProcs = ppnProcs;
#ifdef STDSCHEDULER_USE_EVENTS
	// Allocate dummy arrays (one handle neede for unlocker!)
	delete[] pEventHandles; pEventHandles = new HANDLE[iProcCapacity + 1];
	delete[] ppEventProcs;  ppEventProcs = new StdSchedulerProc *[iProcCapacity];
#endif
}

// *** StdSchedulerThread

StdSchedulerThread::StdSchedulerThread()
	: fThread(false)
{

}

StdSchedulerThread::~StdSchedulerThread()
{
	Clear();
}

void StdSchedulerThread::Clear()
{
	// Stop thread
	if(fThread) Stop();
	// Clear scheduler
	StdScheduler::Clear();
}

void StdSchedulerThread::Set(StdSchedulerProc **ppProcs, int iProcCnt)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
	StdScheduler::Set(ppProcs, iProcCnt);
	// Restart
	if(fGotThread) Start();
}

void StdSchedulerThread::Add(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
	StdScheduler::Add(pProc);
	// Restart
	if(fGotThread) Start();
}

void StdSchedulerThread::Remove(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if(fGotThread) Stop();
	// Set
  StdScheduler::Remove(pProc);
	// Restart
	if(fGotThread) Start();
}

bool StdSchedulerThread::Start()
{
	// already running? stop
	if(fThread) Stop();
	// begin thread
	fRunThreadRun = true;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fThread = (iThread != -1);
#elif defined(HAVE_PTHREAD)
	fThread = !pthread_create(&Thread, NULL, _ThreadFunc, this);
#endif
	// success?
	return fThread;
}

void StdSchedulerThread::Stop()
{
	// Not running?
	if(!fThread) return;
	// Set flag
	fRunThreadRun = false;
	// Unblock
	UnBlock();
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if(WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill it in case it refuses to do so
		TerminateThread(hThread, -1);
#elif defined(HAVE_PTHREAD)
	// wait for thread to terminate itself
	// (without security - let's trust these unwashed hackers for once)
	pthread_join(Thread, NULL);
#endif
	fThread = false;
	// ok
	return;
}

#ifdef HAVE_WINTHREAD
void __cdecl StdSchedulerThread::_ThreadFunc(void *pPar)
{
	StdSchedulerThread *pThread = reinterpret_cast<StdSchedulerThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}
#elif defined(HAVE_PTHREAD)
void *StdSchedulerThread::_ThreadFunc(void *pPar)
{
	StdSchedulerThread *pThread = reinterpret_cast<StdSchedulerThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
#endif

unsigned int StdSchedulerThread::ThreadFunc()
{
	// Keep calling Execute until someone gets fed up and calls StopThread()
	while(fRunThreadRun)
		ScheduleProcs();
	return(0);
}



StdThread::StdThread() : fStarted(false), fStopSignaled(false)
{

}

bool StdThread::Start()
{
	// already running? stop
	if(fStarted) Stop();
	// begin thread
	fStopSignaled = false;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fStarted = (iThread != -1);
#elif defined(HAVE_PTHREAD)
	fStarted = !pthread_create(&Thread, NULL, _ThreadFunc, this);
#endif
	// success?
	return fStarted;
}

void StdThread::SignalStop()
{
	// Not running?
	if(!fStarted) return;
	// Set flag
	fStopSignaled = true;
}

void StdThread::Stop()
{
	// Not running?
	if(!fStarted) return;
	// Set flag
	fStopSignaled = true;
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if(WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill him in case he refuses to do so
		TerminateThread(hThread, -1);
#elif defined(HAVE_PTHREAD)
	// wait for thread to terminate itself
	// (whithout security - let's trust these unwashed hackers for once)
	pthread_join(Thread, NULL);
#endif
	fStarted = false;
	// ok
	return;
}

#ifdef HAVE_WINTHREAD
void __cdecl StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}
#elif defined(HAVE_PTHREAD)
void *StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
#endif

unsigned int StdThread::ThreadFunc()
{
	// Keep calling Execute until someone gets fed up and calls Stop()
	while(!IsStopSignaled())
		Execute();
	return(0);
}

bool StdThread::IsStopSignaled()
{
	return fStopSignaled;
}

#ifdef STDSCHEDULER_USE_EVENTS
CStdNotifyProc::CStdNotifyProc() : Event(true) {}
void CStdNotifyProc::Notify() { Event.Set(); }
bool CStdNotifyProc::Check() { return Event.WaitFor(0); }
bool CStdNotifyProc::CheckAndReset()
	{
	if(!Check()) return false;
	Event.Reset();
	return true;
	}
#else // STDSCHEDULER_USE_EVENTS
CStdNotifyProc::CStdNotifyProc()
	{
	pipe(fds);
	// Experimental castration of the pipe.
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
	}
void CStdNotifyProc::Notify()
	{
	char c = 42;
	write(fds[1], &c, 1);
	}
bool CStdNotifyProc::Check()
	{
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(fds[0], &fdset);
	timeval to = { 0, 0 };
	return select(fds[0] + 1, &fdset, NULL, NULL, &to);
	}
bool CStdNotifyProc::CheckAndReset()
	{
	bool r = false;
	while(1)
		{
		char c;
		if (read(fds[0], &c, 1) <= 0)
			break;
		else
			r = true;
		}
	return r;
	}
#endif
