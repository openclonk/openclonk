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
#include "C4Include.h"
#include "platform/StdScheduler.h"

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

#ifdef HAVE_UNISTD_H
// For pipe()
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#endif

// *** StdSchedulerProc

// Keep calling Execute until timeout has elapsed
bool StdSchedulerProc::ExecuteUntil(int iTimeout)
{
	// Infinite?
	if (iTimeout < 0)
		for (;;)
			if (!Execute())
				return false;
	// Calculate endpoint
	C4TimeMilliseconds tStopTime = C4TimeMilliseconds::Now() + iTimeout;
	for (;;)
	{
		// Call execute with given timeout
		if (!Execute(std::max(iTimeout, 0)))
			return false;
		// Calculate timeout
		C4TimeMilliseconds tTime = C4TimeMilliseconds::Now();
		if (tTime >= tStopTime)
			break;
		iTimeout = tStopTime - tTime;
	}
	// All ok.
	return true;
}

// *** StdScheduler

StdScheduler::StdScheduler() : isInManualLoop(false)
{
	Add(&Unblocker);
}

StdScheduler::~StdScheduler()
{
	Clear();
}

void StdScheduler::Clear()
{
	while (procs.size() > 0)
		Remove(procs[procs.size()-1]);
}

void StdScheduler::Set(StdSchedulerProc **ppnProcs, int inProcCnt)
{
	// Remove previous data
	Clear();
	// Copy new
	for (int i = 0; i < inProcCnt; i++)
		Add(ppnProcs[i]);
}

void StdScheduler::Add(StdSchedulerProc *pProc)
{
	// Already added to some scheduler
	if (pProc->scheduler)
		return;
	// Add
	procs.push_back(pProc);
	pProc->scheduler = this;
	
	Added(pProc);
}

void StdScheduler::Remove(StdSchedulerProc *pProc)
{
	// :o ?
	if (pProc->scheduler != this)
		return;
	Removing(pProc);
	pProc->scheduler = nullptr;
	auto pos = std::find(procs.begin(), procs.end(), pProc);
	if (pos != procs.end())
		procs.erase(pos);
}

void StdSchedulerProc::Changed()
{
	auto s = scheduler;
	if (s)
		s->Changed(this);
}

C4TimeMilliseconds StdSchedulerProc::GetNextTick(C4TimeMilliseconds tNow)
{
	return C4TimeMilliseconds::PositiveInfinity;
}

C4TimeMilliseconds StdScheduler::GetNextTick(C4TimeMilliseconds tNow)
{
	C4TimeMilliseconds tProcTick = C4TimeMilliseconds::PositiveInfinity;
	for (auto proc : procs)
	{
		tProcTick = std::min(tProcTick, proc->GetNextTick(tNow));
	}
	return tProcTick;
}

bool StdScheduler::ScheduleProcs(int iTimeout)
{
	// Needs at least one process to work properly
	if (!procs.size()) return false;

	// Get timeout
	C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
	C4TimeMilliseconds tProcTick = GetNextTick(tNow);
	if (iTimeout == -1 || tNow + iTimeout > tProcTick)
	{
		iTimeout = std::max<decltype(iTimeout)>(tProcTick - tNow, 0);
	}
	
	bool old = isInManualLoop;
	isInManualLoop = true;
	bool res = DoScheduleProcs(iTimeout);
	isInManualLoop = old;
	return res;
}

void StdScheduler::UnBlock()
{
	Unblocker.Notify();
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
	if (fThread) Stop();
	// Clear scheduler
	StdScheduler::Clear();
}

void StdSchedulerThread::Set(StdSchedulerProc **ppProcs, int iProcCnt)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if (fGotThread) Stop();
	// Set
	StdScheduler::Set(ppProcs, iProcCnt);
	// Restart
	if (fGotThread) Start();
}

void StdSchedulerThread::Add(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if (fGotThread) Stop();
	// Set
	StdScheduler::Add(pProc);
	// Restart
	if (fGotThread) Start();
}

void StdSchedulerThread::Remove(StdSchedulerProc *pProc)
{
	// Thread is running? Stop it first
	bool fGotThread = fThread;
	if (fGotThread) Stop();
	// Set
	StdScheduler::Remove(pProc);
	// Restart
	if (fGotThread) Start();
}

bool StdSchedulerThread::Start()
{
	// already running? stop
	if (fThread) Stop();
	// begin thread
	fRunThreadRun = true;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fThread = (iThread != -1);
#elif defined(HAVE_PTHREAD)
	fThread = !pthread_create(&Thread, nullptr, _ThreadFunc, this);
#endif
	// success?
	return fThread;
}

void StdSchedulerThread::Stop()
{
	// Not running?
	if (!fThread) return;
	// Set flag
	fRunThreadRun = false;
	// Unblock
	UnBlock();
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if (WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill it in case it refuses to do so
		TerminateThread(hThread, -1);
#elif defined(HAVE_PTHREAD)
	// wait for thread to terminate itself
	// (without security - let's trust these unwashed hackers for once)
	pthread_join(Thread, nullptr);
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
void __cdecl StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}
#elif defined(HAVE_PTHREAD)
void *StdSchedulerThread::_ThreadFunc(void *pPar)
{
	StdSchedulerThread *pThread = reinterpret_cast<StdSchedulerThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
void *StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	return reinterpret_cast<void *>(pThread->ThreadFunc());
}
#endif

unsigned int StdSchedulerThread::ThreadFunc()
{
	StartOnCurrentThread();
	// Keep calling Execute until someone gets fed up and calls StopThread()
	while (fRunThreadRun)
		ScheduleProcs(1000);
	return(0);
}



StdThread::StdThread() : fStarted(false), fStopSignaled(false)
{

}

bool StdThread::Start()
{
	// already running? stop
	if (fStarted) Stop();
	// begin thread
	fStopSignaled = false;
#ifdef HAVE_WINTHREAD
	iThread = _beginthread(_ThreadFunc, 0, this);
	fStarted = (iThread != -1);
#elif defined(HAVE_PTHREAD)
	fStarted = !pthread_create(&Thread, nullptr, _ThreadFunc, this);
#endif
	// success?
	return fStarted;
}

void StdThread::SignalStop()
{
	// Not running?
	if (!fStarted) return;
	// Set flag
	fStopSignaled = true;
}

void StdThread::Stop()
{
	// Not running?
	if (!fStarted) return;
	// Set flag
	fStopSignaled = true;
#ifdef HAVE_WINTHREAD
	// Wait for thread to terminate itself
	HANDLE hThread = reinterpret_cast<HANDLE>(iThread);
	if (WaitForSingleObject(hThread, 10000) == WAIT_TIMEOUT)
		// ... or kill it in case it refuses to do so
		TerminateThread(hThread, -1);
#elif defined(HAVE_PTHREAD)
	// wait for thread to terminate itself
	// (whithout security - let's trust these unwashed hackers for once)
	pthread_join(Thread, nullptr);
#endif
	fStarted = false;
	// ok
	return;
}

unsigned int StdThread::ThreadFunc()
{
	// Keep calling Execute until someone gets fed up and calls Stop()
	while (!IsStopSignaled())
		Execute();
	// Handle deletion
	if (IsSelfDestruct())
	{
		fStarted = false; // reset start flag to avoid Stop() call
		delete this;
	}
	return(0);
}

bool StdThread::IsStopSignaled()
{
	return fStopSignaled;
}
