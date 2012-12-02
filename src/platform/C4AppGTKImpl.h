/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, 2008-2010  Günther Brammer
 * Copyright (c) 2009-2010  Armin Burgmeier
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
#ifndef INC_STD_X_PRIVATE_H
#define INC_STD_X_PRIVATE_H

class C4GLibProc: public StdSchedulerProc
{
public:
	C4GLibProc(GMainContext *context): context(context), query_time(-1) { fds.resize(1); g_main_context_ref(context); }
	~C4GLibProc() { g_main_context_unref(context); }

	GMainContext *context;
	std::vector<pollfd> fds;
	int query_time;
	int timeout;
	int max_priority;

private:
	// Obtain the timeout and FDs from the glib mainloop. We then pass them
	// to the StdScheduler in GetFDs() and GetNextTick() so that it can
	// poll the file descriptors, along with the file descriptors from
	// other sources that it might have.
	void query(int Now)
	{
		// If Execute() has not yet been called, then finish the current iteration first.
		// Note that we cannot simply ignore the query() call, as new
		// FDs or Timeouts may have been added to the Glib loop in the meanwhile
		if (query_time >= 0)
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
		int old_query_time = query_time;
		if (query_time >= 0)
		{
			//g_main_context_check(context, max_priority, fds.empty() ? NULL : (GPollFD*) &fds[0], fds.size());
			//query_time = -1;
			Execute();
		}

		// Run the loop
		while (g_main_context_pending(context))
			g_main_context_iteration(context, false);

		// Return to original state
		if (old_query_time >= 0)
			query(old_query_time);
	}

	// StdSchedulerProc override
	virtual void GetFDs(std::vector<struct pollfd> & rfds)
	{
		if (query_time < 0) query(GetTime());
		rfds.insert(rfds.end(), fds.begin(), fds.end());
	}
	virtual int GetNextTick(int Now)
	{
		query(Now);
		if (timeout < 0) return timeout;
		return query_time + timeout;
	}
	virtual bool Execute(int iTimeout = -1, pollfd * readyfds = 0)
	{
		if (query_time < 0) return true;
		g_main_context_check(context, max_priority, fds.empty() ? NULL : readyfds ? (GPollFD*) readyfds : (GPollFD*) &fds[0], fds.size());

		// g_main_context_dispatch makes callbacks from the main loop.
		// We allow the callback to iterate the mainloop via
		// IteratePendingEvents so reset query_time before to not call
		// g_main_context_check() twice for the current iteration.
		// This would otherwise lead to a freeze since
		// g_main_context_check() seems to block when called twice.
		query_time = -1;
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
