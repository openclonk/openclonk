/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
#include "StdScheduler.h"

#ifdef HAVE_POLL_H
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_SHARE_H
#include <share.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <map>

// Is this process currently signaled?
bool StdSchedulerProc::IsSignaled()
{
	// Initialize file descriptor sets
	std::vector<struct pollfd> fds;

	// Get file descriptors
	GetFDs(fds);

	// Test
	return poll(&fds[0], fds.size(), 0) > 0;
}

namespace
{
	void Fail(const char* msg)
	{
		Log(msg);
	}
}

#ifdef HAVE_SYS_EVENTFD_H
#include <sys/eventfd.h>

CStdNotifyProc::CStdNotifyProc()
{
	fds[0] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (fds[0] == -1)
		Fail("eventfd failed");
}
CStdNotifyProc::~CStdNotifyProc()
{
	close(fds[0]);
}
void CStdNotifyProc::Notify()
{
	uint64_t n = 1;
	if (write(fds[0], &n, 8) == -1)
		Fail("write failed");
}
bool CStdNotifyProc::CheckAndReset()
{
	uint64_t n;
	return (read(fds[0], &n, 8) != -1);
}
#else
CStdNotifyProc::CStdNotifyProc()
{
	if (pipe(fds) == -1)
		Fail("pipe failed");
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);
	fcntl(fds[0], F_SETFD, FD_CLOEXEC);
	fcntl(fds[1], F_SETFD, FD_CLOEXEC);
}
CStdNotifyProc::~CStdNotifyProc()
{
	close(fds[0]);
	close(fds[1]);
}
void CStdNotifyProc::Notify()
{
	char c = 42;
	if (write(fds[1], &c, 1) == -1)
		Fail("write failed");
}
bool CStdNotifyProc::CheckAndReset()
{
	bool r = false;
	while (1)
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
void CStdNotifyProc::GetFDs(std::vector<struct pollfd> & checkfds)
{
	pollfd pfd = { fds[0], POLLIN, 0 };
	checkfds.push_back(pfd);
}

bool StdScheduler::DoScheduleProcs(int iTimeout)
{
	// Initialize file descriptor sets
	std::vector<struct pollfd> fds;
	std::map<StdSchedulerProc *, std::pair<unsigned int, unsigned int> > fds_for_proc;

	// Collect file descriptors
	for (auto proc : procs)
	{
		unsigned int os = fds.size();
		proc->GetFDs(fds);
		if (os != fds.size())
			fds_for_proc[proc] = std::pair<unsigned int, unsigned int>(os, fds.size());
	}

	// Wait for something to happen
	int cnt = poll(&fds[0], fds.size(), iTimeout);

	bool fSuccess = true;

	if (cnt >= 0)
	{
		bool any_executed = false;
		auto tNow = C4TimeMilliseconds::Now();
		// Which process?
		for (size_t i = 0; i < procs.size(); i++)
		{
			auto proc = procs[i];
			auto tProcTick = proc->GetNextTick(tNow);
			if (tProcTick <= tNow)
			{
				struct pollfd * pfd = 0;
				if (fds_for_proc.find(proc) != fds_for_proc.end())
					pfd = &fds[fds_for_proc[proc].first];
				if (!proc->Execute(0, pfd))
				{
					OnError(proc);
					fSuccess = false;
				}
				any_executed = true;
				continue;
			}
			// no fds?
			if (fds_for_proc.find(proc) == fds_for_proc.end())
				continue;
			// Check intersection
			unsigned int begin = fds_for_proc[proc].first;
			unsigned int end = fds_for_proc[proc].second;
			for (unsigned int j = begin; j < end; ++j)
			{
				if (fds[j].events & fds[j].revents)
				{
					if (any_executed && proc->IsLowPriority())
						break;
					if (!proc->Execute(0, &fds[begin]))
					{
						OnError(proc);
						fSuccess = false;
					}
					any_executed = true;
					// the list of procs might have been changed, but procs must be in both ppProcs and
					// fds_for_proc to be executed, which prevents execution of any proc not polled this round
					// or deleted. Some procs might be skipped or executed twice, but that should be save.
					break;
				}
			}
		}
	}
	else if (cnt < 0 && errno != EINTR)
	{
		printf("StdScheduler::%s: poll failed: %s\n",__func__,strerror(errno));
	}
	return fSuccess;
}

#if defined(HAVE_SYS_TIMERFD_H)
#include <sys/timerfd.h>
CStdMultimediaTimerProc::CStdMultimediaTimerProc(uint32_t iDelay)
{
	fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd == -1)
		Log("timerfd_create failed");
	SetDelay(iDelay);
}

CStdMultimediaTimerProc::~CStdMultimediaTimerProc()
{
	close(fd);
}

void CStdMultimediaTimerProc::SetDelay(uint32_t inDelay)
{
	struct itimerspec nv, ov;
	nv.it_interval.tv_sec = inDelay / 1000;
	nv.it_interval.tv_nsec = (inDelay % 1000) * 1000000;
	nv.it_value = nv.it_interval;
	timerfd_settime(fd, 0, &nv, &ov);
}

void CStdMultimediaTimerProc::Set()
{
	struct itimerspec nv, ov;
	timerfd_gettime(fd, &nv);
	nv.it_value.tv_sec = 0;
	nv.it_value.tv_nsec = 1;
	timerfd_settime(fd, 0, &nv, &ov);
}

bool CStdMultimediaTimerProc::CheckAndReset()
{
	uint64_t n;
	return read(fd, &n, 8) != -1;
}

void CStdMultimediaTimerProc::GetFDs(std::vector<struct pollfd> & checkfds)
{
	pollfd pfd = { fd, POLLIN, 0 };
	checkfds.push_back(pfd);
}
#endif // HAVE_SYS_TIMERFD_H

#if !defined(USE_COCOA)
void StdScheduler::Added(StdSchedulerProc *pProc) {}
void StdScheduler::Removing(StdSchedulerProc *pProc) {}
void StdScheduler::Changed(StdSchedulerProc* pProc) {}
void StdScheduler::StartOnCurrentThread() {}
#endif
#endif // HAVE_POLL_H
