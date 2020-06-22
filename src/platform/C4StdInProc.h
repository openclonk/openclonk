/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4STDINPROC
#define INC_C4STDINPROC

#include "platform/StdScheduler.h"

// A simple alertable proc
class C4StdInProc : public StdSchedulerProc
{
public:
	C4StdInProc();
	~C4StdInProc() override;

	// StdSchedulerProc override
	bool Execute(int iTimeout, pollfd *) override;
#ifdef STDSCHEDULER_USE_EVENTS
	HANDLE GetEvent() override { return GetStdHandle(STD_INPUT_HANDLE); }
#else
	void GetFDs(std::vector<struct pollfd> & checkfds) override
	{
		pollfd pfd = { 0, POLLIN | POLLERR | POLLHUP, 0 };
		checkfds.push_back(pfd);
	}
#endif
private:
	// commands from stdin
	StdCopyStrBuf CmdBuf;
};

#endif /* INC_C4STDINPROC */
