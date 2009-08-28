/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2006, 2008  Armin Burgmeier
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

#include <Standard.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include "C4Version.h"

#ifdef USE_X11
#define bool _BOOL
#include "c4x.xpm"
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>
#undef bool
#endif

#include <string>
#include <map>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "StdXPrivate.h"

/* CStdWindow */

CStdWindow::CStdWindow ():
	Active(false)
#ifdef USE_X11
	,wnd(0), renderwnd(0), dpy(0), Info(0), Hints(0), HasFocus(false)
#endif
{
}
CStdWindow::~CStdWindow () {
	Clear();
}
CStdWindow * CStdWindow::Init(CStdApp * pApp) {
	return Init(pApp, C4ENGINENAME);
}

CStdWindow * CStdWindow::Init(CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor) {
#ifndef USE_X11
	return this;
#else
	Active = true;
	dpy = pApp->dpy;

	if(!FindInfo() ) return 0;

// Various properties
	XSetWindowAttributes attr;
	attr.border_pixel = 0;
	attr.background_pixel = 0;
	// Which events we want to receive
	attr.event_mask =
		//EnterWindowMask |
		//LeaveWindowMask |
		StructureNotifyMask |
		FocusChangeMask |
		KeyPressMask |
		KeyReleaseMask |
		PointerMotionMask |
		ButtonPressMask |
		ButtonReleaseMask;
	attr.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), ((XVisualInfo*)Info)->visual, AllocNone);
	unsigned long attrmask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	Pixmap bitmap = 0;
	if (HideCursor) {
		// Hide the mouse cursor
		XColor cursor_color;
		// We do not care what color the invisible cursor has
		memset(&cursor_color, 0, sizeof(cursor_color));
		bitmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), "\000", 1, 1);
		if (bitmap) {
			attr.cursor = XCreatePixmapCursor(dpy, bitmap, bitmap, &cursor_color, &cursor_color, 0, 0);
			if (attr.cursor)
				attrmask |= CWCursor;
			else
				Log("Error creating cursor.");
		} else {
			Log("Error creating bitmap for cursor.");
			attr.cursor = 0;
		}
	} else {
		attr.cursor = 0;
	}

	wnd = XCreateWindow(dpy, DefaultRootWindow(dpy),
		0, 0, 640, 480, 0, ((XVisualInfo*)Info)->depth, InputOutput, ((XVisualInfo*)Info)->visual,
		attrmask, &attr);
	if (attr.cursor)
		XFreeCursor(dpy, attr.cursor);
	if (bitmap)
		XFreePixmap(dpy, bitmap);
	if (!wnd) {
		Log("Error creating window.");
		return 0;
	}
	// Update the XWindow->CStdWindow-Map
	CStdAppPrivate::SetWindow(wnd, this);
	if (!pApp->Priv->xic && pApp->Priv->xim) {
		pApp->Priv->xic = XCreateIC(pApp->Priv->xim,
			XNClientWindow, wnd,
			XNFocusWindow, wnd,
			XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			NULL);
		if (!pApp->Priv->xic) {
			Log("Failed to create input context.");
			XCloseIM(pApp->Priv->xim);
			pApp->Priv->xim=0;
		} else {
			long ic_event_mask;
			if (XGetICValues(pApp->Priv->xic, XNFilterEvents, &ic_event_mask, NULL) == NULL)
				attr.event_mask |= ic_event_mask;
			XSelectInput(dpy, wnd, attr.event_mask);
		}
	}
	// We want notification of closerequests and be killed if we hang
	Atom WMProtocols[2];
	const char * WMProtocolnames[] = { "WM_DELETE_WINDOW", "_NET_WM_PING" };
	XInternAtoms(dpy, const_cast<char **>(WMProtocolnames), 2, false, WMProtocols);
	XSetWMProtocols(dpy, wnd, WMProtocols, 2);
	// Let the window manager know our pid so it can kill us
	Atom PID = XInternAtom(pApp->dpy, "_NET_WM_PID", false);
	int32_t pid = getpid();
	if (PID != None) XChangeProperty(pApp->dpy, wnd, PID, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(&pid), 1);
	// State and Icon
	XWMHints * wm_hint = XAllocWMHints();
	wm_hint->flags = StateHint | IconPixmapHint | IconMaskHint;
	wm_hint->initial_state = NormalState;
	// Trust XpmCreatePixmapFromData to not modify the xpm...
	XpmCreatePixmapFromData (dpy, wnd, const_cast<char **>(c4x_xpm), &wm_hint->icon_pixmap, &wm_hint->icon_mask, 0);
	// Window class
	XClassHint * class_hint = XAllocClassHint();
	class_hint->res_name = const_cast<char *>(C4ENGINENAME);
	class_hint->res_class = const_cast<char *>(C4ENGINENAME);
	Xutf8SetWMProperties(dpy, wnd, const_cast<char*>(Title), const_cast<char*>(Title), pApp->Priv->argv, pApp->Priv->argc, 0, wm_hint, class_hint);
	// Set "parent". Clonk does not use "real" parent windows, but multiple toplevel windows.
	if (pParent) XSetTransientForHint(dpy, wnd, pParent->wnd);
	// Show window
	XMapWindow (dpy, wnd);
	// Clean up
	// The pixmap has to stay as long as the window exists, so it does not hurt to never free it.
	//XFreePixmap(dpy,xwmh->icon_pixmap);
	//XFreePixmap(dpy,xwmh->icon_mask);
	Hints = wm_hint;
	XFree(class_hint);

	// Render into whole window
	renderwnd = wnd;

	return this;
#endif // USE_X11
}

void CStdWindow::Clear() {
#ifdef USE_X11
	// Destroy window
	if (wnd) {
		CStdAppPrivate::SetWindow(wnd, 0);
		XUnmapWindow(dpy, wnd);
		XDestroyWindow(dpy, wnd);
		if(Info) XFree (Info);
		if(Hints) XFree(Hints);

		// Might be necessary when the last window is closed
		XFlush(dpy);
	}
	wnd = renderwnd = 0;
#endif
}

#ifdef USE_X11
bool CStdWindow::FindInfo()
{
#ifdef USE_GL
	// get an appropriate visual
	// attributes for a single buffered visual in RGBA format with at least 4 bits per color
	static int attrListSgl[] = { GLX_RGBA,
		GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
		GLX_DEPTH_SIZE, 8,
		None };
	// attributes for a double buffered visual in RGBA format with at least 4 bits per color
	static int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_DEPTH_SIZE, 8,
		None };
	// doublebuffered is the best
	Info = glXChooseVisual(dpy, DefaultScreen(dpy), attrListDbl);
	if (!Info)
		{
		Log("  gl: no doublebuffered visual.");
		// a singlebuffered is probably better than the default
		Info = glXChooseVisual(dpy, DefaultScreen(dpy), attrListSgl);
		}
#endif // USE_GL
	if (!Info)
		{
		Log("  gl: no singlebuffered visual, either.");
		// just try to get the default
		XVisualInfo vitmpl; int blub;
		vitmpl.visual = DefaultVisual(dpy, DefaultScreen(dpy));
		vitmpl.visualid = XVisualIDFromVisual(vitmpl.visual);
		Info = XGetVisualInfo(dpy, VisualIDMask, &vitmpl, &blub);
		}
	if (!Info)
		{
		Log("  gl: no visual at all.");
		return false;
		}

	return true;
}
#endif // USE_X11

bool CStdWindow::StorePosition(const char *, const char *, bool) { return true; }

bool CStdWindow::RestorePosition(const char *, const char *, bool) {
	// The Windowmanager is responsible for window placement.
    return true;
}

bool CStdWindow::GetSize(RECT * pRect) {
#ifdef USE_X11
	Window winDummy;
	unsigned int borderDummy;
	int x, y;
	unsigned int width, height;
	unsigned int depth;
	XGetGeometry(dpy, wnd, &winDummy, &x, &y,
		&width, &height, &borderDummy, &depth);
	pRect->right = width + x;
	pRect->bottom = height + y;
	pRect->top = y;
	pRect->left = x;
#else
  pRect->left = pRect->right = pRect->top = pRect->bottom = 0;
#endif
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y) {
#ifdef USE_X11
	XResizeWindow(dpy, wnd, X, Y);
#endif
}
void CStdWindow::SetTitle(const char * Title) {
#ifdef USE_X11
	XTextProperty title_property;
	StdStrBuf tbuf(Title, true);
	char * tbufstr = tbuf.getMData();
	XStringListToTextProperty(&tbufstr, 1, &title_property);
	XSetWMName(dpy, wnd, &title_property);
#endif
}

void CStdWindow::FlashWindow() {
#ifdef USE_X11

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

	if(!HasFocus)
	{
		XWMHints * wm_hint = static_cast<XWMHints*>(Hints);
		wm_hint->flags |= XUrgencyHint;
		XSetWMHints(dpy, wnd, wm_hint);
	}
#endif
}

#ifdef USE_X11
void CStdWindow::HandleMessage(XEvent& event)
{
	if(event.type == FocusIn)
	{
		HasFocus = true;

		// Clear urgency flag
		XWMHints * wm_hint = static_cast<XWMHints*>(Hints);
		if(wm_hint->flags & XUrgencyHint)
		{
			wm_hint->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, wnd, wm_hint);
		}
	}
	else if(event.type == FocusOut /*|| event.type == UnmapNotify*/)
	{
		int detail = reinterpret_cast<XFocusChangeEvent*>(&event)->detail;

		// StdGtkWindow gets two FocusOut events, one of which comes
		// directly after a FocusIn event even when the window has
		// focus. For these FocusOut events, detail is set to
		// NotifyInferior which is why we are ignoring it here.
		if(detail != NotifyInferior)
		{
			HasFocus = false;
		}
	}
}
#endif
