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
#include "network/C4InteractiveThread.h"
#include "game/C4Application.h"
#include "lib/C4Log.h"

#include "game/C4Game.h"

// *** C4InteractiveThread

C4InteractiveThread::C4InteractiveThread()
{
	// Add head-item
	pFirstEvent = pLastEvent = new Event();
	pFirstEvent->Type = Ev_None;
	pFirstEvent->Next = nullptr;
	// reset event handlers
	ZeroMem(&pCallbacks, sizeof(pCallbacks));
	// Set notify proc
	NotifyProc.SetNotify(this);
	Application.Add(&NotifyProc);
}

C4InteractiveThread::~C4InteractiveThread()
{
	CStdLock PushLock(&EventPushCSec), PopLock(&EventPopCSec);
	// Remove all items. This may leak data, if pData was allocated on the heap.
	while (PopEvent(nullptr, nullptr)) {}
	// Delete head-item
	delete pFirstEvent;
	pFirstEvent = pLastEvent = nullptr;
	// Unregister notify
	Application.Remove(&NotifyProc);
}

bool C4InteractiveThread::AddProc(StdSchedulerProc *pProc)
{
	bool fFirst = !Scheduler.getProcCnt();
	// Add the proc
	Scheduler.Add(pProc);
	// Not started yet?
	if (fFirst)
		if (!Scheduler.Start())
			return false;
	return true;
}

void C4InteractiveThread::RemoveProc(StdSchedulerProc *pProc)
{
	// Process not in list?
	if (!Scheduler.hasProc(pProc))
		return;
	// Last proc to be removed?
	if (Scheduler.getProcCnt() == 1)
		Scheduler.Stop();
	// Remove
	Scheduler.Remove(pProc);
}

bool C4InteractiveThread::PushEvent(C4InteractiveEventType eEvent, void *pData)
{
	CStdLock PushLock(&EventPushCSec);
	if (!pLastEvent) return false;
	// create event
	Event *pEvent = new Event;
	pEvent->Type = eEvent;
	pEvent->Data = pData;
#ifdef _DEBUG
	pEvent->Time = C4TimeMilliseconds::Now();
#endif
	pEvent->Next = nullptr;
	// add item (at end)
	pLastEvent->Next = pEvent;
	pLastEvent = pEvent;
	PushLock.Clear();
	// notify main thread
	NotifyProc.Notify();
	return true;
}

#ifdef _DEBUG
double AvgNetEvDelay = 0;
#endif

bool C4InteractiveThread::PopEvent(C4InteractiveEventType *pEventType, void **ppData) // (by main thread)
{
	CStdLock PopLock(&EventPopCSec);
	if (!pFirstEvent) return false;
	// get event
	Event *pEvent = pFirstEvent->Next;
	if (!pEvent) return false;
	// return
	if (pEventType)
		*pEventType = pEvent->Type;
	if (ppData)
		*ppData = pEvent->Data;
#ifdef _DEBUG
	if (Game.IsRunning)
		AvgNetEvDelay += ((C4TimeMilliseconds::Now() - pEvent->Time) - AvgNetEvDelay) / 100;
#endif
	// remove
	delete pFirstEvent;
	pFirstEvent = pEvent;
	pFirstEvent->Type = Ev_None;
	return true;
}


void C4InteractiveThread::ProcessEvents() // by main thread
{
	C4InteractiveEventType eEventType; void *pEventData;
	while (PopEvent(&eEventType, &pEventData))
		switch (eEventType)
		{
			// Logging
		case Ev_Log: case Ev_LogSilent: case Ev_LogFatal: case Ev_LogDebug:
		{
			// Reconstruct the StdStrBuf which allocated the data.
			StdStrBuf pLog;
			pLog.Take(reinterpret_cast<char *>(pEventData));
			switch (eEventType)
			{
			case Ev_Log:
				Log(pLog.getData()); break;
			case Ev_LogSilent:
				LogSilent(pLog.getData()); break;
			case Ev_LogFatal:
				LogFatal(pLog.getData()); break;
			case Ev_LogDebug:
				DebugLog(pLog.getData()); break;
			default: assert(eEventType == Ev_Log || eEventType == Ev_LogSilent || eEventType == Ev_LogFatal || eEventType == Ev_LogDebug); // obviously will not happen, but someone tell gcc
			}

		}
		break;

	case Ev_Function:
		{
			std::unique_ptr<std::function<void ()> > func(static_cast<std::function<void()>*>(pEventData));
			(*func)();
		}

		// Other events: check for a registered handler
		default:
			if (eEventType >= Ev_None && eEventType <= Ev_Last)
				if (pCallbacks[eEventType])
					pCallbacks[eEventType]->OnThreadEvent(eEventType, pEventData);
			// Note that memory might leak if the event wasn't processed....
		}
}


bool C4InteractiveThread::ThreadLog(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_Log, Msg.GrabPointer());
}

bool C4InteractiveThread::ThreadLogFatal(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_LogFatal, Msg.GrabPointer());
}

bool C4InteractiveThread::ThreadLogS(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_LogSilent, Msg.GrabPointer());
}

bool C4InteractiveThread::ThreadLogDebug(const char *szMessage, ...)
{
	// format message
	va_list lst; va_start(lst, szMessage);
	StdStrBuf Msg = FormatStringV(szMessage, lst);
	// send to main thread
	return PushEvent(Ev_LogDebug, Msg.GrabPointer());
}

bool C4InteractiveThreadNotifyProc::Execute(int, pollfd*)
{
	if (CheckAndReset())
		pNotify->ProcessEvents();
	return true;
}
