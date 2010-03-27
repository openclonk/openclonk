/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004  Peter Wortmann
 * Copyright (c) 2005, 2007  Günther Brammer
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
/* synchronization helper classes */

#ifndef INC_StdSync
#define INC_StdSync

#ifdef _WIN32

class CStdCSec
{
public:
	CStdCSec() { InitializeCriticalSection(&sec); }
	virtual ~CStdCSec() { DeleteCriticalSection(&sec); }

protected:
	CRITICAL_SECTION sec;

public:
	virtual void Enter() { EnterCriticalSection(&sec); }
	virtual void Leave() { LeaveCriticalSection(&sec); }
};

class CStdEvent
{
public:
	CStdEvent(bool fManualReset) { hEvent = CreateEvent(NULL, fManualReset, false, NULL); }
	~CStdEvent() { CloseHandle(hEvent); }

protected:
	HANDLE hEvent;

public:
	void Set() { SetEvent(hEvent); }
	void Pulse() { PulseEvent(hEvent); }
	void Reset() { ResetEvent(hEvent); }
	bool WaitFor(int iMillis) { return WaitForSingleObject(hEvent, iMillis) == WAIT_OBJECT_0; }

	HANDLE GetEvent() { return hEvent; }
};

#elif defined(HAVE_PTHREAD)

#include <pthread.h>

// Value to specify infinite wait.
#ifndef INFINITE
#define INFINITE (~0u)
#endif

class CStdCSec
{
public:
	CStdCSec() {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mutex, &attr);
	}
	virtual ~CStdCSec() { pthread_mutex_destroy(&mutex); }

protected:
	pthread_mutex_t mutex;

public:
	virtual void Enter() { pthread_mutex_lock(&mutex); }
	virtual void Leave() { pthread_mutex_unlock(&mutex); }
};

class CStdEvent
{
public:
	CStdEvent(bool fManualReset) : fManualReset(fManualReset), fSet(false)
	{
		pthread_cond_init(&cond, NULL);
		pthread_mutex_init(&mutex, NULL);
	}
	~CStdEvent()
	{
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
	}

protected:
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	bool fManualReset, fSet;

public:
	void Set()
	{
		pthread_mutex_lock(&mutex);
		fSet = true;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}
	void Pulse()
	{
		pthread_cond_broadcast(&cond);
	}
	void Reset()
	{
		pthread_mutex_lock(&mutex);
		fSet = false;
		pthread_mutex_unlock(&mutex);
	}
	bool WaitFor(unsigned int iMillis)
	{
		pthread_mutex_lock(&mutex);
		// Already set?
		while(!fSet)
		{
			// Use pthread_cond_wait or pthread_cond_timedwait depending on wait length. Check return value.
			// Note this will temporarily unlock the mutex, so no deadlock should occur.
			timespec ts = { iMillis / 1000, (iMillis % 1000) * 1000000 };
			if(0 != (iMillis != INFINITE ? pthread_cond_timedwait(&cond, &mutex, &ts) : pthread_cond_wait(&cond, &mutex)))
			{
				pthread_mutex_unlock(&mutex);
				return false;
			}
		}
		// Reset flag, release mutex, done.
		if (!fManualReset) fSet = false;
		pthread_mutex_unlock(&mutex);
		return true;
	}
};

#if defined __GNUC__ && ((__GNUC__ >= 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ >= 5))
inline long InterlockedIncrement(long * p) { return __sync_add_and_fetch(p, 1); }
inline long InterlockedDecrement(long * p) { return __sync_sub_and_fetch(p, 1); }
#else
// Defined in Standard.cpp
long InterlockedIncrement(long *);
long InterlockedDecrement(long *);
#define NEED_FALLBACK_ATOMIC_FUNCS 1
#endif

#else
// Some stubs to silence the compiler
class CStdCSec
{
public:
	CStdCSec() { }
	virtual ~CStdCSec() { }
	virtual void Enter() { }
	virtual void Leave() { }
};
class CStdEvent
{
public:
	CStdEvent(bool) { }
	~CStdEvent() { }
	void Set() { }
	void Pulse() { }
	void Reset() { }
	bool WaitFor(int) { return false; }
};
#endif // HAVE_PTHREAD

class CStdLock
{
public:
	CStdLock(CStdCSec *pSec) : sec(pSec)
		{ sec->Enter(); }
	~CStdLock()
		{ Clear(); }

protected:
	CStdCSec *sec;

public:
	void Clear()
		{ if(sec) sec->Leave(); sec = NULL; }
};

class CStdCSecExCallback
{
public:
	// is called with CSec exlusive locked!
	virtual void OnShareFree(class CStdCSecEx *pCSec) = 0;
	virtual ~CStdCSecExCallback() {}
};

class CStdCSecEx : public CStdCSec
{
public:
	CStdCSecEx()
		: lShareCnt(0), ShareFreeEvent(false), pCallbClass(NULL)
	{ }
	CStdCSecEx(CStdCSecExCallback *pCallb)
		: lShareCnt(0), ShareFreeEvent(false), pCallbClass(pCallb)
	{ }
	~CStdCSecEx()
	{ }

protected:
	// share counter
	long lShareCnt;
	// event: exclusive access permitted
	CStdEvent ShareFreeEvent;
	// callback
	CStdCSecExCallback *pCallbClass;

public:

	// (cycles forever if shared locked by calling thread!)
	void Enter()
	{
		// lock
		CStdCSec::Enter();
		// wait for share-free
		while(lShareCnt)
		{
			// reset event
			ShareFreeEvent.Reset();
			// leave section for waiting
			CStdCSec::Leave();
			// wait
			ShareFreeEvent.WaitFor(INFINITE);
			// reenter section
			CStdCSec::Enter();
		}
	}

	void Leave()
	{
		// set event
		ShareFreeEvent.Set();
		// unlock
		CStdCSec::Leave();
	}

	void EnterShared()
	{
		// lock
		CStdCSec::Enter();
		// add share
		lShareCnt++;
		// unlock
		CStdCSec::Leave();
	}

	void LeaveShared()
	{
		// lock
		CStdCSec::Enter();
		// remove share
		if(!--lShareCnt)
		{
			// do callback
			if(pCallbClass)
				pCallbClass->OnShareFree(this);
			// set event
			ShareFreeEvent.Set();
		}
		// unlock
		CStdCSec::Leave();
	}
};

class CStdShareLock
{
public:
	CStdShareLock(CStdCSecEx *pSec) : sec(pSec)
		{ sec->EnterShared(); }
	~CStdShareLock()
		{ Clear(); }

protected:
	CStdCSecEx *sec;

public:
	void Clear()
		{ if(sec) sec->LeaveShared(); sec = NULL; }
};

#endif // INC_StdSync
