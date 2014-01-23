/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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
#ifndef INC_STD_X_PRIVATE_H
#define INC_STD_X_PRIVATE_H

class C4GLibProc: public StdSchedulerProc
{
public:
	C4GLibProc(GMainContext *context): context(context), query_time(C4TimeMilliseconds::NegativeInfinity) { fds.resize(1); g_main_context_ref(context); }
	~C4GLibProc()
	{
		g_main_context_unref(context);
	}

	GMainContext *context;
	std::vector<pollfd> fds;
	C4TimeMilliseconds query_time;
	int timeout;
	int max_priority;

private:
	// Obtain the timeout and FDs from the glib mainloop. We then pass them
	// to the StdScheduler in GetFDs() and GetNextTick() so that it can
	// poll the file descriptors, along with the file descriptors from
	// other sources that it might have.
	void query(C4TimeMilliseconds Now)
	{
		// If Execute() has not yet been called, then finish the current iteration first.
		// Note that we cannot simply ignore the query() call, as new
		// FDs or Timeouts may have been added to the Glib loop in the meanwhile
		if (!query_time.IsInfinite())
		{
			//g_main_context_check(context, max_priority, fds.empty() ? NULL : (GPollFD*) &fds[0], fds.size());
			Execute();
		}

		g_main_context_prepare (context, &max_priority);
		unsigned int fd_count;
		if (fds.empty()) fds.resize(1);
		while ((fd_count = g_main_context_query(context, max_priority, &timeout, (GPollFD*) &fds[0], fds.size())) > fds.size())
		{
			fds.resize(fd_count);
		}
		// Make sure we don't report more FDs than there are available
		fds.resize(fd_count);
		query_time = Now;
	}

public:
	// Iterate the Glib main loop until all pending events have been
	// processed. Don't use g_main_context_pending() directly as the
	// C4GLibProc might have initiated a loop iteration already.
	// This is mainly used to update the log in the editor window while
	// a scenario is being loaded.
	void IteratePendingEvents()
	{
		// TODO: I think we can also iterate the context manually,
		// without g_main_context_iteration. This might be less hacky.

		// Finish current iteration first
		C4TimeMilliseconds old_query_time = C4TimeMilliseconds::NegativeInfinity;
		if (!query_time.IsInfinite())
		{
			old_query_time = query_time;
			//g_main_context_check(context, max_priority, fds.empty() ? NULL : (GPollFD*) &fds[0], fds.size());
			//query_time = C4TimeMilliseconds::NegativeInfinity;
			Execute();
		}

		// Run the loop
		while (g_main_context_pending(context))
			g_main_context_iteration(context, false);

		// Return to original state
		if (!old_query_time.IsInfinite())
			query(old_query_time);
	}

	// StdSchedulerProc override
	virtual void GetFDs(std::vector<struct pollfd> & rfds)
	{
		if (query_time.IsInfinite()) query(C4TimeMilliseconds::Now());
		rfds.insert(rfds.end(), fds.begin(), fds.end());
	}
	virtual C4TimeMilliseconds GetNextTick(C4TimeMilliseconds Now)
	{
		query(Now);
		if (timeout < 0) return C4TimeMilliseconds::PositiveInfinity;
		return query_time + timeout;
	}
	virtual bool Execute(int iTimeout = -1, pollfd * readyfds = 0)
	{
		if (query_time.IsInfinite()) return true;
		g_main_context_check(context, max_priority, fds.empty() ? NULL : readyfds ? (GPollFD*) readyfds : (GPollFD*) &fds[0], fds.size());

		// g_main_context_dispatch makes callbacks from the main loop.
		// We allow the callback to iterate the mainloop via
		// IteratePendingEvents so reset query_time before to not call
		// g_main_context_check() twice for the current iteration.
		// This would otherwise lead to a freeze since
		// g_main_context_check() seems to block when called twice.
		query_time = C4TimeMilliseconds::NegativeInfinity;
		g_main_context_dispatch(context);
		return true;
	}
};

class C4X11AppImpl
{
public:
	C4GLibProc GLibProc;
	C4X11AppImpl(C4AbstractApp *pApp):
			GLibProc(g_main_context_default()),
			gammasize(0),
			xrandr_major_version(-1), xrandr_minor_version(-1),
			xrandr_oldmode(-1),
			xrandr_rot(0),
			xrandr_event(-1),
			argc(0), argv(0)
	{
	}

	int gammasize; // Size of gamma ramps

	int xrandr_major_version, xrandr_minor_version;
	int xrandr_oldmode;
	unsigned short xrandr_rot;
	int xrandr_event;

	int argc; char ** argv;
};

#endif // INC_STD_X_PRIVATE_H
