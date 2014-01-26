// Events are Windows-specific
#include <C4Include.h>
#include <StdScheduler.h>
#ifdef USE_WIN32_WINDOWS

#include <mmsystem.h>
#include <process.h>

bool StdSchedulerProc::IsSignaled()
{
	return GetEvent() && WaitForSingleObject(GetEvent(), 0) == WAIT_OBJECT_0;
}

CStdNotifyProc::CStdNotifyProc() : Event(true) {}
void CStdNotifyProc::Notify() { Event.Set(); }
bool CStdNotifyProc::CheckAndReset()
{
	if (!Event.WaitFor(0)) return false;
	Event.Reset();
	return true;
}

bool StdScheduler::DoScheduleProcs(int iTimeout)
{
	int i;
	// Collect event handles
	int iEventCnt = 0; HANDLE hEvent;
	StdSchedulerProc *pMessageProc = NULL;
	for (i = 0; i < procs.size(); i++)
	{
		auto proc = procs[i];
		if ( (hEvent = procs[i]->GetEvent()) )
		{
			if (hEvent == STDSCHEDULER_EVENT_MESSAGE)
				pMessageProc = procs[i];
			else
			{
				eventHandles[iEventCnt] = hEvent;
				eventProcs[iEventCnt] = procs[i];
				iEventCnt++;
			}
		}
	}

	// Wait for something to happen
	DWORD ret; DWORD dwMsec = iTimeout < 0 ? INFINITE : iTimeout;
	if (pMessageProc)
		ret = MsgWaitForMultipleObjects(iEventCnt, eventHandles.data(), false, dwMsec, QS_ALLINPUT);
	else
		ret = WaitForMultipleObjects(iEventCnt, eventHandles.data(), false, dwMsec);

	bool fSuccess = true;

	// Event?
	if (ret != WAIT_TIMEOUT)
	{
		// Which event?
		int iEventNr = ret - WAIT_OBJECT_0;

		// Execute the signaled process
		StdSchedulerProc *pProc = iEventNr < iEventCnt ? eventProcs[iEventNr] : pMessageProc;
		if (!pProc->Execute(0))
		{
			OnError(pProc);
			fSuccess = false;
		}

	}

	// Execute all processes with timeout
	auto tNow = C4TimeMilliseconds::Now();
	for (auto proc = procs.begin(); proc != procs.end(); ++proc)
	{
		auto tProcTick = (*proc)->GetNextTick(tNow);
		if (tProcTick <= tNow)
			if (!(*proc)->Execute(0))
			{
				OnError(*proc);
				fSuccess = false;
			}
	}
	return fSuccess;
}

/* CStdMultimediaTimerProc */

int CStdMultimediaTimerProc::iTimePeriod = 0;

CStdMultimediaTimerProc::CStdMultimediaTimerProc(uint32_t iDelay) :
		uCriticalTimerDelay(28),
		idCriticalTimer(0),
		uCriticalTimerResolution(5),
		Event(true)
{

	if (!iTimePeriod)
	{
		// Get resolution caps
		TIMECAPS tc;
		timeGetDevCaps(&tc, sizeof(tc));
		// Establish minimum resolution
		uCriticalTimerResolution = BoundBy(uCriticalTimerResolution, tc.wPeriodMin, tc.wPeriodMax);
		timeBeginPeriod(uCriticalTimerResolution);
	}
	iTimePeriod++;

	SetDelay(iDelay);

}

CStdMultimediaTimerProc::~CStdMultimediaTimerProc()
{
	if (idCriticalTimer)
	{
		timeKillEvent(idCriticalTimer);
		idCriticalTimer = 0;

		iTimePeriod--;
		if (!iTimePeriod)
			timeEndPeriod(uCriticalTimerResolution);
	}
}

void CStdMultimediaTimerProc::SetDelay(uint32_t iDelay)
{

	// Kill old timer (of any)
	if (idCriticalTimer)
		timeKillEvent(idCriticalTimer);

	// Set new delay
	uCriticalTimerDelay = iDelay;

	// Set critical timer
	idCriticalTimer=timeSetEvent(
	                  uCriticalTimerDelay,uCriticalTimerResolution,
	                  (LPTIMECALLBACK) Event.GetEvent(),0,TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);

	if(idCriticalTimer == 0)
		DebugLogF("Creating Critical Timer failed: %d", GetLastError());
}

void StdScheduler::Added(StdSchedulerProc *pProc)
{
	if (procs.size() > eventProcs.size())
	{
		eventProcs.resize(procs.size());
		eventHandles.resize(procs.size()+1);
	}
}

void StdScheduler::Removing(StdSchedulerProc *pProc)
{
}

void StdScheduler::Changed(StdSchedulerProc* pProc)
{
}

bool CStdMultimediaTimerProc::CheckAndReset()
{
	if (!Check()) return false;
	Event.Reset();
	return true;
}

void __cdecl StdThread::_ThreadFunc(void *pPar)
{
	StdThread *pThread = reinterpret_cast<StdThread *>(pPar);
	_endthreadex(pThread->ThreadFunc());
}

#endif