/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2006, 2008  Armin Burgmeier
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
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <C4Config.h>
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

#include "StdXPrivate.h"

// Some helper functions for choosing a proper visual

#ifdef USE_GL
// Returns which XVisual attribute for two given attributes is greater.
static int CompareVisualAttribute(Display* dpy, XVisualInfo* first, XVisualInfo* second, int attrib)
{
	int first_value, second_value;
	glXGetConfig(dpy, first, attrib, &first_value);
	glXGetConfig(dpy, second, attrib, &second_value);
	if(first_value != second_value) return first_value > second_value ? 1 : -1;
	return 0;
}

// Given two X visuals, check which one is superior, according to
// the following rule: Double buffering is preferred over single
// buffering, then highest color buffer is preferred. If both are equal
// then the buffers are considered equal (return value 0).
static int CompareVisual(Display* dpy, XVisualInfo* first, XVisualInfo* second)
{
	int result = CompareVisualAttribute(dpy, first, second, GLX_DOUBLEBUFFER);
	if(result != 0) return result;

	result = CompareVisualAttribute(dpy, first, second, GLX_BUFFER_SIZE);
	return result;
}

// Compare otherwise equivalent visuals. If the function above
// considered two visuals to be equivalent then this function can
// be used to decide which one to use. We prefer visuals with high depth
// beffer size and low accumulation and stencil buffer sizes since the latter
// two are not used in Clonk.
static int CompareEquivalentVisual(Display* dpy, XVisualInfo* first, XVisualInfo* second)
{
	int result = CompareVisualAttribute(dpy, first, second, GLX_DEPTH_SIZE);
	if(result != 0) return result;

	result = CompareVisualAttribute(dpy, first, second, GLX_STENCIL_SIZE);
	if(result != 0) return -result;

	result = CompareVisualAttribute(dpy, first, second, GLX_ACCUM_RED_SIZE);
	if(result != 0) return -result;
	
	result = CompareVisualAttribute(dpy, first, second, GLX_ACCUM_GREEN_SIZE);
	if(result != 0) return -result;

	result = CompareVisualAttribute(dpy, first, second, GLX_ACCUM_BLUE_SIZE);
	if(result != 0) return -result;

	result = CompareVisualAttribute(dpy, first, second, GLX_ACCUM_ALPHA_SIZE);
	return -result;
}

// This function generates a list of acceptable visuals. The most
// superiour visual as defined by CompareVisual is chosen. If there
// are two or more visuals which compare equal with CompareVisual then
// we add all of them to the output list as long as their multi
// sampling properties differ. If they do not differ then we use
// CompareEquivalentVisual to decide which one to put into the output
// list.
static std::vector<XVisualInfo> EnumerateVisuals(Display* dpy)
{
	XVisualInfo templateInfo;
	templateInfo.screen = DefaultScreen(dpy);
	long vinfo_mask = VisualScreenMask;
	int nitems;
	XVisualInfo* infos = XGetVisualInfo(dpy, vinfo_mask, &templateInfo, &nitems);

	std::vector<XVisualInfo> selected_infos;
	for(int i = 0; i < nitems; ++i)
	{
		// Require minimum depth and color buffer
		if(infos[i].depth < 8 || infos[i].bits_per_rgb < 4) continue;

		// Require it to be an RGBA visual
		int value;
		glXGetConfig(dpy, &infos[i], GLX_RGBA, &value);
		if(!value) continue;

		// Require GL rendering to be supported (probably always true...)
		glXGetConfig(dpy, &infos[i], GLX_USE_GL, &value);
		if(!value) continue;

		// Multisampling with only 1 sample gives the same result as
		// no multisampling at all, so simply ignore these visuals.
		int second_value;
		glXGetConfig(dpy, &infos[i], GLX_SAMPLE_BUFFERS_ARB, &value);
		glXGetConfig(dpy, &infos[i], GLX_SAMPLES_ARB, &second_value);
		if(value == 1 && second_value == 1) continue;

		// This visual is acceptable in principle. Use it if
		// we don't have any other.
		if(selected_infos.empty())
		{
			selected_infos.push_back(infos[i]);
		}
		// Otherwise, check which one is superior. Note that all selected
		// visuals have same buffering and RGBA sizes.
		else
		{
			unsigned int j;
			switch(CompareVisual(dpy, &infos[i], &selected_infos[0]))
			{
			case 1:
				// The new visual is superior.
				selected_infos.clear();
				selected_infos.push_back(infos[i]);
				break;
			case -1:
				// The old visual is superior.
				break;
			case 0:
				// The visuals are equal. OK, so check whether there is an otherwise equivalent
				// visual (read: same multisampling properties) but with different depth, stencil or
				// auxiliary buffer sizes. If so, replace it, otherwise add the new one.
				for(j = 0; j < selected_infos.size(); ++j)
				{
					if(CompareVisualAttribute(dpy, &infos[i], &selected_infos[j], GLX_SAMPLE_BUFFERS_ARB) != 0) continue;
					if(CompareVisualAttribute(dpy, &infos[i], &selected_infos[j], GLX_SAMPLES_ARB) != 0) continue;

					// The new visual has the same multi sampling properties then the current one.
					// Use CompareEquivalentVisual() to decide
					switch(CompareEquivalentVisual(dpy, &infos[i], &selected_infos[j]))
					{
					case 1:
						// The current info is more suitable
						selected_infos[j] = infos[i];
						break;
					case -1:
						// The existing info is more suitable;
						break;
					case 0:
						// No decision. Keep the existing one, but we could as well take
						// the new one since we don't know what the difference between the two is.
						break;
					}

					// Break the for loop. There is only one visual
					// with the same multi sampling properties.
					break;
				}

				// If we did not find a visual with the same multisampling in the for loop
				// then add this visual to the result list
				if(j == selected_infos.size())
					selected_infos.push_back(infos[i]);

				break;
			}
		}
	}

	XFree(infos);
	return selected_infos;
}
#endif // USE_GL

static Window CreateRenderWindow(Display* dpy, Window parent, XVisualInfo* info)
{
	XWindowAttributes parent_attr;
	if(!XGetWindowAttributes(dpy, parent, &parent_attr)) return NULL;

	XSetWindowAttributes attr;
	attr.border_pixel = 0;
	attr.background_pixel = 0;
	attr.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), info->visual, AllocNone);
	unsigned long attrmask = CWBackPixel | CWBorderPixel | CWColormap;

	return XCreateWindow(dpy, parent, 0, 0, parent_attr.width, parent_attr.height, 0,
	                     info->depth, InputOutput, info->visual, attrmask, &attr);
}

/* CStdWindow */

CStdWindow::CStdWindow ():
		Active(false), pSurface(0), wnd(0), renderwnd(0), dpy(0), Hints(0), HasFocus(false), Info(0)
{
}
CStdWindow::~CStdWindow ()
{
	Clear();
}
CStdWindow * CStdWindow::Init(CStdApp * pApp)
{
	return Init(CStdWindow::W_Viewport, pApp, C4ENGINENAME);
}

CStdWindow * CStdWindow::Init(CStdWindow::WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor)
{
	Active = true;
	dpy = pApp->dpy;

	// Try to get a multisampling visual 
	if(!FindInfo(Config.Graphics.MultiSampling, &Info))
	{
		// Disable multisampling if we don't find a visual which
		// supports the currently configured setting.
		if(!FindInfo(0, &Info)) return NULL;
		Config.Graphics.MultiSampling = 0;
	}

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

	unsigned long attrmask = CWBackPixel | CWBorderPixel | CWEventMask;
	Pixmap bitmap = 0;
	if (HideCursor)
	{
		// Hide the mouse cursor
		XColor cursor_color;
		// We do not care what color the invisible cursor has
		memset(&cursor_color, 0, sizeof(cursor_color));
		bitmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), "\000", 1, 1);
		if (bitmap)
		{
			attr.cursor = XCreatePixmapCursor(dpy, bitmap, bitmap, &cursor_color, &cursor_color, 0, 0);
			if (attr.cursor)
				attrmask |= CWCursor;
			else
				Log("Error creating cursor.");
		}
		else
		{
			Log("Error creating bitmap for cursor.");
			attr.cursor = 0;
		}
	}
	else
	{
		attr.cursor = 0;
	}

	wnd = XCreateWindow(dpy, DefaultRootWindow(dpy),
		                 0, 0, Config.Graphics.ResX, Config.Graphics.ResY, 0, CopyFromParent, InputOutput, CopyFromParent,
		                 attrmask, &attr);

	if (attr.cursor)
		XFreeCursor(dpy, attr.cursor);
	if (bitmap)
		XFreePixmap(dpy, bitmap);
	if (!wnd)
	{
		Log("Error creating window.");
		return NULL;
	}

	if (pApp->Priv->xim && !pApp->Priv->xic)
	{
		pApp->Priv->xic = XCreateIC(pApp->Priv->xim,
			                         XNClientWindow, wnd,
			                         XNFocusWindow, wnd,
			                         XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			                         XNResourceName, C4ENGINENAME,
			                         XNResourceClass, C4ENGINENAME,
			                         NULL);
		if(pApp->Priv->xic)
		{
			long ic_event_mask;
			if (XGetICValues(pApp->Priv->xic, XNFilterEvents, &ic_event_mask, NULL) == NULL)
				attr.event_mask |= ic_event_mask;
			XSelectInput(dpy, wnd, attr.event_mask);
			XSetICFocus(pApp->Priv->xic);
		}
		else
		{
			Log("Failed to create input context.");
			XCloseIM(pApp->Priv->xim);
			pApp->Priv->xim=0;
		}
	}

#if 0
	// Set _NET_WM_STATE
	if(n_net_wm_state_atoms)
	{
		XChangeProperty(dpy, wnd_info.wnd, XInternAtom(dpy, "_NET_WM_STATE", False), XInternAtom(dpy, "ATOM", False), 32, PropModeReplace, reinterpret_cast<unsigned char*>(net_wm_state_atoms), n_net_wm_state_atoms);
	} 
#endif

	// We want notification of closerequests and be killed if we hang
	Atom WMProtocols[2];
	const char * WMProtocolnames[] = { "WM_DELETE_WINDOW", "_NET_WM_PING" };
	XInternAtoms(dpy, const_cast<char **>(WMProtocolnames), 2, false, WMProtocols);
	XSetWMProtocols(dpy, wnd, WMProtocols, 2);
	// Let the window manager know our pid so it can kill us
	Atom PID = XInternAtom(dpy, "_NET_WM_PID", false);
	int32_t pid = getpid();
	if (PID != None) XChangeProperty(dpy, wnd, PID, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(&pid), 1);
	// State and Icon
	XWMHints* wm_hints = XAllocWMHints();
	wm_hints->input = 1;
	wm_hints->flags = StateHint | InputHint | IconPixmapHint | IconMaskHint;
	wm_hints->initial_state = NormalState;
	// Trust XpmCreatePixmapFromData to not modify the xpm...
	XpmCreatePixmapFromData (dpy, wnd, const_cast<char **>(c4x_xpm), &wm_hints->icon_pixmap, &wm_hints->icon_mask, 0);
	// Window class
	XClassHint * class_hint = XAllocClassHint();
	class_hint->res_name = const_cast<char *>(C4ENGINENAME);
	class_hint->res_class = const_cast<char *>(C4ENGINENAME);
	Xutf8SetWMProperties(dpy, wnd, const_cast<char*>(Title), const_cast<char*>(Title), pApp->Priv->argv, pApp->Priv->argc, 0, wm_hints, class_hint);
	XFree(class_hint);
	Hints = wm_hints;
	// Set "parent". Clonk does not use "real" parent windows, but multiple toplevel windows.
	if (pParent) XSetTransientForHint(dpy, wnd, pParent->wnd);

	// Update XWindow<->StdWindow map
	CStdAppPrivate::SetWindow(wnd, this);

	// Create the subwindow which we render into
	renderwnd = CreateRenderWindow(dpy, wnd, static_cast<XVisualInfo*>(Info));
	if(!renderwnd)
	{
		XDestroyWindow(dpy, wnd); wnd = NULL;
		Log("Error creating render window.");
		return NULL;
	}

	// Clean up
	// The pixmap has to stay as long as the window exists, so it does not hurt to never free it.
	//XFreePixmap(dpy,xwmh->icon_pixmap);
	//XFreePixmap(dpy,xwmh->icon_mask);

	// Show window
	XMapWindow (dpy, renderwnd);
	XMapWindow (dpy, wnd);

	return this;
}

bool CStdWindow::ReInit(CStdApp* pApp)
{
	// Can only re-init if we have been initialized already
	if(!wnd) return false;

	// Check whether multisampling settings was changed. If not then we
	// don't need to ReInit anything.
#ifdef USE_GL
	int value;
	glXGetConfig(dpy, static_cast<XVisualInfo*>(Info), GLX_SAMPLES_ARB, &value);
	if(value == Config.Graphics.MultiSampling) return true;
#else
	return true;
#endif

	// Check whether we have a visual with the requested number of samples
	void* new_info;
	if(!FindInfo(Config.Graphics.MultiSampling, &new_info)) return false;

	Window new_window = CreateRenderWindow(dpy, wnd, static_cast<XVisualInfo*>(new_info));
	if(!new_window)
	{
		delete static_cast<XVisualInfo*>(new_info);
		return false;
	}

	delete static_cast<XVisualInfo*>(Info);
	Info = new_info;

	// Replace existing render window with new one
	XUnmapWindow(dpy, renderwnd);
	XMapWindow(dpy, new_window);
	XDestroyWindow(dpy, renderwnd);
	renderwnd = new_window;
	return true;
}

void CStdWindow::Clear()
{
	// Destroy window
	if (wnd)
	{
		CStdAppPrivate::SetWindow(wnd, 0);
		XUnmapWindow(dpy, wnd);
		XDestroyWindow(dpy, renderwnd);
		XDestroyWindow(dpy, wnd);
		if (Hints) XFree(Hints);
		Hints = NULL;

		// Might be necessary when the last window is closed
		XFlush(dpy);
	}

	wnd = renderwnd = 0;
	if (Info) delete static_cast<XVisualInfo*>(Info);
	Info = NULL;
}

bool CStdWindow::FindInfo(unsigned int samples, void** info)
{
#ifdef USE_GL
	std::vector<XVisualInfo> infos = EnumerateVisuals(dpy);
	for(unsigned int i = 0; i < infos.size(); ++i)
	{
		int v_buffers, v_samples;
		glXGetConfig(dpy, &infos[i], GLX_SAMPLE_BUFFERS_ARB, &v_buffers);
		glXGetConfig(dpy, &infos[i], GLX_SAMPLES_ARB, &v_samples);

		if((samples == 0 && v_buffers == 0) ||
		   (samples > 0 && v_buffers == 1 && v_samples == samples))
		{
			*info = new XVisualInfo(infos[i]);
			return true;
		}
	}
#else
	// TODO: Do we need to handle this case?
#endif // USE_GL

	return false;
}

void CStdWindow::EnumerateMultiSamples(std::vector<int>& samples) const
{
#ifdef USE_GL
	std::vector<XVisualInfo> infos = EnumerateVisuals(dpy);
	for(unsigned int i = 0; i < infos.size(); ++i)
	{
		int v_buffers, v_samples;
		glXGetConfig(dpy, &infos[i], GLX_SAMPLE_BUFFERS_ARB, &v_buffers);
		glXGetConfig(dpy, &infos[i], GLX_SAMPLES_ARB, &v_samples);

		if(v_buffers == 1) samples.push_back(v_samples);
	}
#endif
}

bool CStdWindow::StorePosition(const char *, const char *, bool) { return true; }

bool CStdWindow::RestorePosition(const char *, const char *, bool)
{
	// The Windowmanager is responsible for window placement.
	return true;
}

bool CStdWindow::GetSize(RECT * pRect)
{
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
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y)
{
	XResizeWindow(dpy, wnd, X, Y);
}
void CStdWindow::SetTitle(const char * Title)
{
	XTextProperty title_property;
	StdStrBuf tbuf(Title, true);
	char * tbufstr = tbuf.getMData();
	XStringListToTextProperty(&tbufstr, 1, &title_property);
	XSetWMName(dpy, wnd, &title_property);
}

void CStdWindow::FlashWindow()
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

void CStdWindow::HandleMessage(XEvent& event)
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
