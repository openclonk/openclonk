/*
 * OpenClonk, http://www.openclonk.org
 *
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

class CX11Proc: public StdSchedulerProc {

public:
	CX11Proc(CStdApp *pApp): pApp(pApp) { }
	~CX11Proc() { }

	CStdApp *pApp;

	// StdSchedulerProc override
	virtual void GetFDs(std::vector<struct pollfd> & fds) {
		pollfd pfd = { XConnectionNumber(pApp->dpy), POLLIN, 0 };
		fds.push_back(pfd);
	}
	virtual bool Execute(int iTimeout = -1, pollfd * readyfds = 0) {
		pApp->OnXInput();
		return true;
	}
};

#ifdef WITH_GLIB
class CGLibProc: public StdSchedulerProc
{
public:
	CGLibProc(GMainContext *context): context(context), checked(true) { fds.resize(1); g_main_context_ref(context); }
	~CGLibProc() { g_main_context_unref(context); }

	GMainContext *context;
	std::vector<pollfd> fds;
	bool checked;
	int timeout;
	int max_priority;

	void query()
	{
		if (!checked) return;
		g_main_context_prepare (context, &max_priority);
		unsigned int fd_count;
		while ((fd_count = g_main_context_query(context, max_priority, &timeout, (GPollFD*) &fds[0], fds.size())) > fds.size())
		{
			fds.resize(fd_count);
		}
		checked = false;
	}
	// StdSchedulerProc override
	virtual void GetFDs(std::vector<struct pollfd> & rfds)
	{
		query();
		rfds.insert(rfds.end(), fds.begin(), fds.end());
	}
	virtual int GetNextTick(int Now)
	{
		query();
		return Now + timeout;
	}
	virtual bool Execute(int iTimeout = -1, pollfd * readyfds = 0) {
		if (checked) return true;
		g_main_context_check(context, max_priority, readyfds ? (GPollFD*) readyfds : (GPollFD*) &fds[0], fds.size());
		g_main_context_dispatch(context);
		checked = true;
		return true;
	}
};
#endif // WITH_GLIB

class CStdAppPrivate {
	public:
#ifdef WITH_GLIB
	GMainLoop* loop;

	// IOChannels required to wake up the main loop
	GIOChannel* pipe_channel;
	GIOChannel* x_channel;
	GIOChannel* stdin_channel;
	CGLibProc GLibProc;
#endif

	CStdAppPrivate(CStdApp *pApp):
#ifdef USE_X11
		PrimarySelection(), ClipboardSelection(),
		LastEventTime(CurrentTime), tasked_out(false), pending_desktop(false),
		xim(0), xic(0), X11Proc(pApp),
#endif
#ifdef WITH_GLIB
		GLibProc(g_main_context_default()),
#endif // WITH_GLIB
		argc(0), argv(0) { }
	static CStdWindow * GetWindow(unsigned long wnd);
	static void SetWindow(unsigned long wnd, CStdWindow * pWindow);
#ifdef USE_X11
	bool SwitchToFullscreen(CStdApp * pApp, Window wnd);
	void SwitchToDesktop(CStdApp * pApp, Window wnd);
	void SetEWMHFullscreen (CStdApp * pApp, bool fFullScreen, Window wnd);
	struct ClipboardData {
		StdStrBuf Text;
		unsigned long AcquirationTime;
	} PrimarySelection, ClipboardSelection;
	unsigned long LastEventTime;
	typedef std::map<unsigned long, CStdWindow *> WindowListT;
	static WindowListT WindowList;
	XF86VidModeModeInfo xf86vmode_oldmode, xf86vmode_targetmode;
	int xrandr_oldmode;
	unsigned short xrandr_rot;
	int xrandr_event;
	bool tasked_out; int wdt; int hgt;
	bool pending_desktop;
	static const int PENDING_DESKTOP_DELAY = 3;
	XIM xim;
	XIC xic;
	Bool detectable_autorepeat_supported;
	CX11Proc X11Proc;
#endif
	int argc; char ** argv;
	// Used to signal a network event
	int Pipe[2];
};

#endif // INC_STD_X_PRIVATE_H
