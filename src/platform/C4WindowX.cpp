/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, 2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006, 2008, 2010  Armin Burgmeier
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

/* A wrapper class to OS dependent event and window interfaces, X11 version */

#include <C4Include.h>

#ifdef USE_X11
#include <C4Window.h>

#include <C4App.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <C4Config.h>
#include <C4Rect.h>
#include "C4Version.h"

#include "c4x.xpm"
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>

#include <string>
#include <map>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "C4AppXImpl.h"
/* C4Window */
void C4Window::FlashWindow()
{
	// This tries to implement flashing via
	// _NET_WM_STATE_DEMANDS_ATTENTION, but it simply does not work for me.
	// -ck.
#if 0
	XEvent e;
	e.xclient.type = ClientMessage;
	e.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", True);
	e.xclient.window = wnd;
	e.xclient.display = dpy;
	e.xclient.format = 32;
	e.xclient.data.l[0] = 1;
	e.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", True);
	e.xclient.data.l[2] = 0l;
	e.xclient.data.l[3] = 0l;
	e.xclient.data.l[4] = 0l;

	XSendEvent(dpy, DefaultRootWindow(dpy), false, SubstructureNotifyMask | SubstructureRedirectMask, &e);
#endif

	if (!HasFocus)
	{
		XWMHints * wm_hint = static_cast<XWMHints*>(Hints);
		wm_hint->flags |= XUrgencyHint;
		XSetWMHints(dpy, wnd, wm_hint);
	}
}

void C4Window::HandleMessage(XEvent& event)
{
	if (event.type == FocusIn)
	{
		HasFocus = true;

		// Clear urgency flag
		XWMHints * wm_hint = static_cast<XWMHints*>(Hints);
		if (wm_hint->flags & XUrgencyHint)
		{
			wm_hint->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, wnd, wm_hint);
		}
	}
	else if (event.type == FocusOut /*|| event.type == UnmapNotify*/)
	{
		int detail = reinterpret_cast<XFocusChangeEvent*>(&event)->detail;

		// StdGtkWindow gets two FocusOut events, one of which comes
		// directly after a FocusIn event even when the window has
		// focus. For these FocusOut events, detail is set to
		// NotifyInferior which is why we are ignoring it here.
		if (detail != NotifyInferior)
		{
			HasFocus = false;
		}
	}
}
#endif // USE_X11
