/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006, 2008-2009  Armin Burgmeier
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
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

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XKBlib.h>

#include <string>
#include <sstream>
#include <map>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

/* CStdApp */

#ifdef WITH_GLIB
# include <glib.h>
#endif

#ifdef WITH_DEVELOPER_MODE
# include "c4x.xpm"
# include <gtk/gtk.h>
#endif

#include "StdXPrivate.h"

namespace
{
	unsigned int KeyMaskFromKeyEvent(Display* dpy, XKeyEvent* xkey)
	{
		unsigned int mask = xkey->state;
		KeySym sym = XKeycodeToKeysym(dpy, xkey->keycode, 1);
		// We need to correct the keymask since the event.xkey.state
		// is the state _before_ the event, but we want to store the
		// current state.
		if (sym == XK_Control_L || sym == XK_Control_R) mask ^= MK_CONTROL;
		if (sym == XK_Shift_L || sym == XK_Shift_L) mask ^= MK_SHIFT;
		if (sym == XK_Alt_L || sym == XK_Alt_R) mask ^= (1 << 3);
		return mask;
	}
}

CStdAppPrivate::WindowListT CStdAppPrivate::WindowList;

CStdApp::CStdApp(): Active(false), fQuitMsgReceived(false), dpy(0), Location(""),
		// main thread
#ifdef HAVE_PTHREAD
		MainThread (pthread_self()),
#endif
		DoNotDelay(false), Priv(new CStdAppPrivate(this)), fDspModeSet(false)
{
	Add(&Priv->X11Proc);
#ifdef WITH_GLIB
	Add(&Priv->GLibProc);
#endif
}

CStdApp::~CStdApp()
{
	Remove(&Priv->X11Proc);
#ifdef WITH_GLIB
	Remove(&Priv->GLibProc);
#endif
	delete Priv;
}

bool CStdApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");
	// FIXME: This should only be done in developer mode.
#ifdef WITH_DEVELOPER_MODE
	gtk_init(&argc, &argv);

	GdkPixbuf* icon = gdk_pixbuf_new_from_xpm_data(c4x_xpm);
	gtk_window_set_default_icon(icon);
	g_object_unref(icon);
#endif
	// Try to figure out the location of the executable
	Priv->argc=argc; Priv->argv=argv;
	static char dir[PATH_MAX];
	SCopy(argv[0], dir);
	if (dir[0] != '/')
	{
		SInsert(dir, "/");
		SInsert(dir, GetWorkingDirectory());
		Location = dir;
	}
	else
	{
		Location = dir;
	}

	if (!(dpy = XOpenDisplay (0)))
	{
		Log("Error opening display.");
		return false;
	}

	int xf86vmode_event_base, xf86vmode_error_base;
	if (!XF86VidModeQueryExtension(dpy, &xf86vmode_event_base, &xf86vmode_error_base)
	    || !XF86VidModeQueryVersion(dpy, &xf86vmode_major_version, &xf86vmode_minor_version))
	{
		xf86vmode_major_version = -1;
		xf86vmode_minor_version = 0;
	}
	int xrandr_error_base;
	if (!XRRQueryExtension(dpy, &Priv->xrandr_event, &xrandr_error_base)
	    || !XRRQueryVersion(dpy, &xrandr_major_version, &xrandr_minor_version))
	{
		xrandr_major_version = -1;
		xrandr_minor_version = 0;
	}
	XRRSelectInput(dpy, DefaultRootWindow(dpy), RRScreenChangeNotifyMask);
	if (xrandr_major_version < 0 && xf86vmode_major_version < 0)
		Log("Xrandr and xf86vmode extensions are missing. Resolution switching will not work.");
	else if (xrandr_major_version >= 0)
		LogF("  Using XRandR version %d.%d", xrandr_major_version, xrandr_minor_version);
	else
		LogF("  Using XF86VidMode version %d.%d", xf86vmode_major_version, xf86vmode_minor_version);
	// So a repeated keypress-event is not preceded with a keyrelease.
	XkbSetDetectableAutoRepeat(dpy, True, &Priv->detectable_autorepeat_supported);

	XSetLocaleModifiers("");
	Priv->xim = XOpenIM(dpy, 0, 0, 0);
	if (!Priv->xim) Log("Failed to open input method.");

	// Get the Atoms for the Clipboard
	Atom PrefetchAtoms[8];
	const char * PrefetchAtomnames[] = { "CLIPBOARD", "WM_CHANGE_STATE", "WM_DELETE_WINDOW",
	                                     "_NET_WM_STATE", "_NET_WM_STATE_FULLSCREEN", "_NET_WM_PING", "_NET_WM_PID",
	                                     "_NET_WM_STATE_DEMANDS_ATTENTION"
	                                   };
	XInternAtoms(dpy, const_cast<char **>(PrefetchAtomnames), 8, true, PrefetchAtoms);

#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install (">", readline_callback);
	readline_callback_use_this_app = this;

#ifdef WITH_GLIB
	Priv->stdin_channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(Priv->stdin_channel, G_IO_IN, &OnStdInInputStatic, this);
#endif
#endif

	// Custom initialization
	return DoInit (argc, argv);
}

#ifdef WITH_GLIB

static void
gtk_clipboard_store_all (void)
{
  GtkClipboard *clipboard;
  GSList *displays, *list;
  
  displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

  list = displays;
  while (list)
    {
      GdkDisplay *display = static_cast<GdkDisplay *>(list->data);

      clipboard = gtk_clipboard_get_for_display (display, GDK_SELECTION_CLIPBOARD);

      if (clipboard)
	gtk_clipboard_store (clipboard);
      
      list = list->next;
    }
  g_slist_free (displays);
  
}
#endif

void CStdApp::Clear()
{
#ifdef WITH_GLIB
	gtk_clipboard_store_all();
#endif
	XCloseDisplay(dpy);
	dpy = 0;
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
}

void CStdApp::Quit()
{
	fQuitMsgReceived = true;
}

bool CStdApp::FlushMessages()
{

	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;

#ifdef WITH_GLIB
	Priv->GLibProc.IteratePendingEvents();
#endif

	return Priv->X11Proc.Execute(0);
}

void CStdApp::HandleXMessage()
{
	XEvent event;
	XNextEvent(dpy, &event);
	bool filtered = XFilterEvent(&event, event.xany.window);
	switch (event.type)
	{
	case EnterNotify:
		KeyMask = event.xcrossing.state;
		break;
	case KeyPress:
		// Needed for input methods
		if (!filtered)
		{
			char c[10] = "";
			if (Priv->xic)
			{
				Status lsret;
				Xutf8LookupString(Priv->xic, &event.xkey, c, 10, 0, &lsret);
				if (lsret == XLookupKeySym) fprintf(stderr, "FIXME: XmbLookupString returned XLookupKeySym\n");
				if (lsret == XBufferOverflow) fprintf(stderr, "FIXME: XmbLookupString returned XBufferOverflow\n");
			}
			else
			{
				static XComposeStatus state;
				XLookupString(&event.xkey, c, 10, 0, &state);
			}
			if (c[0])
			{
				CStdWindow * pWindow = Priv->GetWindow(event.xany.window);
				if (pWindow)
				{
					pWindow->CharIn(c);
				}
			}
			// Fallthrough
		}
	case KeyRelease:
		KeyMask = KeyMaskFromKeyEvent(dpy, &event.xkey);
		Priv->LastEventTime = event.xkey.time;
		break;
	case ButtonPress:
		// We can take this directly since there are no key presses
		// involved. TODO: We probably need to correct button state
		// here though.
		KeyMask = event.xbutton.state;
		Priv->LastEventTime = event.xbutton.time;
		break;
	case SelectionRequest:
	{
		// We should compare the timestamp with the timespan when we owned the selection
		// But slow network connections are not supported anyway, so do not bother
		CStdAppPrivate::ClipboardData & d = (event.xselectionrequest.selection == XA_PRIMARY) ?
		                                    Priv->PrimarySelection : Priv->ClipboardSelection;
		XEvent responseevent;
		XSelectionEvent & re = responseevent.xselection;
		re.type = SelectionNotify;
		re.display = dpy;
		re.selection = event.xselectionrequest.selection;
		re.target = event.xselectionrequest.target;
		re.time = event.xselectionrequest.time;
		re.requestor = event.xselectionrequest.requestor;
		// Note: we're implementing the spec only partially here
		if (d.Text.getData())
		{
			re.property = event.xselectionrequest.property;
			XChangeProperty(dpy, re.requestor, re.property, re.target, 8, PropModeReplace,
			                (const unsigned char *) d.Text.getData(), d.Text.getLength());
		}
		else
		{
			re.property = None;
		}
		XSendEvent(dpy, re.requestor, false, NoEventMask, &responseevent);
		break;
	}
	case SelectionClear:
	{
		CStdAppPrivate::ClipboardData & d = (event.xselectionrequest.selection == XA_PRIMARY) ?
		                                    Priv->PrimarySelection : Priv->ClipboardSelection;
		d.Text.Clear();
		break;
	}
	case ClientMessage:
		if (!strcmp(XGetAtomName(dpy, event.xclient.message_type), "WM_PROTOCOLS"))
		{
			if (!strcmp(XGetAtomName(dpy, event.xclient.data.l[0]), "WM_DELETE_WINDOW"))
			{
				CStdWindow * pWindow = Priv->GetWindow(event.xclient.window);
				if (pWindow) pWindow->Close();
			}
			else if (!strcmp(XGetAtomName(dpy, event.xclient.data.l[0]), "_NET_WM_PING"))
			{
				// We're still alive
				event.xclient.window = DefaultRootWindow(dpy);
				XSendEvent(dpy, DefaultRootWindow(dpy), false,
				           SubstructureNotifyMask | SubstructureRedirectMask, &event);
			}
		}
		break;
	case MappingNotify:
		XRefreshKeyboardMapping(&event.xmapping);
		break;
	case DestroyNotify:
	{
		CStdWindow * pWindow = Priv->GetWindow(event.xany.window);
		if (pWindow)
		{
			pWindow->wnd = 0;
			pWindow->Clear();
		}
		Priv->SetWindow(event.xany.window, 0);
		break;
	}
	case FocusIn:
		if (Priv->xic) XSetICFocus(Priv->xic);
		if (Priv->pending_desktop)
			Priv->pending_desktop = false;
		if (pWindow && event.xany.window == pWindow->wnd && Priv->tasked_out)
		{
			fDspModeSet = Priv->SwitchToFullscreen(this, pWindow->wnd);
			Priv->tasked_out = false;
		}
		break;
	case FocusOut:
		if (Priv->xic) XUnsetICFocus(Priv->xic);
		// fallthrough
	case UnmapNotify:
		if (pWindow && event.xany.window == pWindow->wnd && fDspModeSet)
		{
			Priv->pending_desktop = true;
		}
		break;
	case ConfigureNotify:
		if (pWindow && event.xany.window == pWindow->wnd)
		{
			XResizeWindow(dpy, pWindow->renderwnd, event.xconfigure.width, event.xconfigure.height);
			OnResolutionChanged(event.xconfigure.width, event.xconfigure.height);
		}
		break;
	default:
		if (event.type == Priv->xrandr_event)
			XRRUpdateConfiguration(&event);
		break;
	}
	CStdWindow * pWindow = Priv->GetWindow(event.xany.window);
	if (pWindow)
		pWindow->HandleMessage(event);
}

bool CStdApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	if (Priv->tasked_out)
		return false;
	bool modefound = false;
	if (fDspModeSet)
	{
		Priv->SwitchToDesktop(this, pWindow->wnd);
		fDspModeSet = false;
	}
	if (!fFullScreen)
	{
		XResizeWindow(dpy, pWindow->wnd, iXRes, iYRes);
		return true;
	}
	if (Priv->xf86vmode_targetmode.hdisplay == iXRes && Priv->xf86vmode_targetmode.vdisplay == iYRes)
		modefound = true;
	// randr spec says to always get fresh info, so don't cache.
	if (xrandr_major_version >= 0)
	{
		modefound = true;
		Priv->wdt = iXRes; Priv->hgt = iYRes;
	}
	if (xf86vmode_major_version >= 0 && !modefound)
	{
		// save desktop-resolution before switching modes
		// XF86VidMode has a really weird API.
		XF86VidModeGetModeLine(dpy, DefaultScreen(dpy), (int*)&Priv->xf86vmode_oldmode.dotclock,
		                       (XF86VidModeModeLine*)(((char *)&Priv->xf86vmode_oldmode) + sizeof(Priv->xf86vmode_oldmode.dotclock)));
		//Priv->oldmode = *modes[0];
		// Change resolution
		int mode_num;
		XF86VidModeModeInfo **modes;
		XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy), &mode_num, &modes);
		// look for mode with requested resolution
		for (int i = 0; i < mode_num; i++)
		{
			if ((modes[i]->hdisplay == iXRes) && (modes[i]->vdisplay == iYRes))
			{
				if (!modefound) Priv->xf86vmode_targetmode = *modes[i];
				modefound = true;
			}
		}
		XFree(modes);
	}
	if (!modefound) return false;
	fDspModeSet = Priv->SwitchToFullscreen(this, pWindow->wnd);
	return fDspModeSet;
}

void CStdApp::RestoreVideoMode()
{
	if (fDspModeSet)
	{
		Priv->SwitchToDesktop(this, pWindow->wnd);
		fDspModeSet = false;
		// Minimize
		if (pWindow->wnd)
		{
			XEvent e;
			e.xclient.type = ClientMessage;
			e.xclient.window = pWindow->wnd;
			e.xclient.message_type = XInternAtom(dpy, "WM_CHANGE_STATE", true);
			e.xclient.format = 32;
			e.xclient.data.l[0] = IconicState;
			XSendEvent(dpy, DefaultRootWindow(dpy), false, SubstructureRedirectMask | SubstructureNotifyMask, &e);
		}
	}
}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	if (xf86vmode_major_version < 0) return false;
	bool r = false;
	int mode_num;
	XF86VidModeModeInfo **modes;
	XF86VidModeGetAllModeLines(dpy, DefaultScreen(dpy), &mode_num, &modes);
	if (iIndex < mode_num)
	{
		*piXRes = modes[iIndex]->hdisplay;
		*piYRes = modes[iIndex]->vdisplay;
		*piBitDepth = 32;
		r = true;
	}
	XFree(modes);
	return r;
}

void CStdAppPrivate::SetEWMHFullscreen (CStdApp * pApp, bool fFullScreen, Window wnd)
{
	static Atom atoms[2];
	static const char * names[] = { "_NET_WM_STATE", "_NET_WM_STATE_FULLSCREEN" };
	if (!atoms[0]) XInternAtoms(pApp->dpy, const_cast<char **>(names), 2, false, atoms);
	XEvent e;
	e.xclient.type = ClientMessage;
	e.xclient.window = wnd;
	e.xclient.message_type = atoms[0];
	e.xclient.format = 32;
	if (fFullScreen)
	{
		e.xclient.data.l[0] = 1; //_NET_WM_STATE_ADD
	}
	else
	{
		e.xclient.data.l[0] = 0; //_NET_WM_STATE_REMOVE
	}
	e.xclient.data.l[1] = atoms[1];
	e.xclient.data.l[2] = 0; //second property to alter
	e.xclient.data.l[3] = 1; //source indication
	e.xclient.data.l[4] = 0;
	XSendEvent(pApp->dpy, DefaultRootWindow(pApp->dpy), false, SubstructureNotifyMask | SubstructureRedirectMask, &e);
}

bool CStdAppPrivate::SwitchToFullscreen(CStdApp * pApp, Window wnd)
{
	if (pApp->xrandr_major_version >= 0)
	{
		XRRScreenConfiguration * conf = XRRGetScreenInfo (pApp->dpy, wnd);
		xrandr_oldmode = XRRConfigCurrentConfiguration (conf, &xrandr_rot);
		int n;
		XRRScreenSize * sizes = XRRConfigSizes(conf, &n);
		for (int i = 0; i < n; ++i)
		{
			if (sizes[i].width == wdt && sizes[i].height == hgt)
			{
#ifdef _DEBUG
				LogF("XRRSetScreenConfig %d", i);
#endif
				XRRSetScreenConfig (pApp->dpy, conf, wnd, i, xrandr_rot, CurrentTime);
				break;
			}
		}
		XRRFreeScreenConfigInfo(conf);
		SetEWMHFullscreen(pApp, true, wnd);
	}
	else if (pApp->xf86vmode_major_version >= 0)
	{
		XF86VidModeModeInfo & mode = xf86vmode_targetmode;
		XResizeWindow(pApp->dpy, wnd, mode.hdisplay, mode.vdisplay);
		XSizeHints hints;
		hints.flags = PMinSize | PMaxSize;
		hints.min_width = mode.hdisplay;
		hints.min_height = mode.vdisplay;
		hints.max_width = mode.hdisplay;
		hints.max_height = mode.vdisplay;
		XSetWMNormalHints(pApp->dpy, wnd, &hints);
		// Changing not necessary
		if (!memcmp(&xf86vmode_targetmode, &xf86vmode_oldmode, sizeof(XF86VidModeModeInfo)))
		{
			// Set the window to fullscreen mode to get rid of window manager decorations
			SetEWMHFullscreen(pApp, true, wnd);
		}
		else
		{
			XF86VidModeSwitchToMode(pApp->dpy, DefaultScreen(pApp->dpy), &mode);
			// Move the viewport on the virtual screen
			Window bla; int wnd_x = 0; int wnd_y = 0;
			XTranslateCoordinates(pApp->dpy, wnd, DefaultRootWindow(pApp->dpy), 0, 0, &wnd_x, &wnd_y, &bla);
			XF86VidModeSetViewPort(pApp->dpy, DefaultScreen(pApp->dpy), wnd_x, wnd_y);
		}
	}
	XGrabPointer(pApp->dpy, wnd, true, 0, GrabModeAsync, GrabModeAsync, wnd, None, LastEventTime);
	return true;
}

void CStdAppPrivate::SwitchToDesktop(CStdApp * pApp, Window wnd)
{
	XUngrabPointer(pApp->dpy, LastEventTime);
	// Restore resolution
	if (pApp->xrandr_major_version >= 0)
	{
		XRRScreenConfiguration * conf = XRRGetScreenInfo (pApp->dpy, wnd);
#ifdef _DEBUG
		LogF("XRRSetScreenConfig %d (back)", xrandr_oldmode);
#endif
		XRRSetScreenConfig (pApp->dpy, conf, wnd, xrandr_oldmode, xrandr_rot, CurrentTime);
		XRRFreeScreenConfigInfo(conf);
	}
	else if (pApp->xf86vmode_major_version >= 0)
	{
		XF86VidModeModeInfo & mode = xf86vmode_oldmode;
		XF86VidModeSwitchToMode(pApp->dpy, DefaultScreen(pApp->dpy), &mode);
		XF86VidModeSetViewPort(pApp->dpy, DefaultScreen(pApp->dpy), 0, 0);
	}
	XSizeHints hints;
	hints.flags = 0;
	XSetWMNormalHints(pApp->dpy, wnd, &hints);
	SetEWMHFullscreen(pApp, false, wnd);
}

// Copy the text to the clipboard or the primary selection
bool CStdApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	CStdAppPrivate::ClipboardData & d = fClipboard ? Priv->ClipboardSelection : Priv->PrimarySelection;
	XSetSelectionOwner(dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY, pWindow->wnd, Priv->LastEventTime);
	Window owner = XGetSelectionOwner(dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY);
	if (owner != pWindow->wnd) return false;
	d.Text.Copy(text);
	d.AcquirationTime = Priv->LastEventTime;
	return true;
}

// Paste the text from the clipboard or the primary selection
StdStrBuf CStdApp::Paste(bool fClipboard)
{
	Window owner = XGetSelectionOwner (dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY);
	if (owner == None) return StdStrBuf(0);
	// Retrieve the selection into the XA_STRING property of our main window
	XConvertSelection (dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY, XA_STRING, XA_STRING,
	                   pWindow->wnd, Priv->LastEventTime);
	// Give the owner some time to respond
	Priv->X11Proc.ExecuteUntil(50);
	// Get the length of the data, so we can request it all at once
	Atom type;
	int format;
	unsigned long len, bytes_left;
	unsigned char *data;
	XGetWindowProperty (dpy, pWindow->wnd,
	                    XA_STRING,  // property
	                    0, 0,     // offset - len
	                    0,        // do not delete it now
	                    AnyPropertyType, // flag
	                    &type,      // return type
	                    &format,    // return format
	                    &len, &bytes_left, //that
	                    &data);
	//printf ("type:%i len:%li format:%d byte_left:%ld\n", (int)type, len, format, bytes_left);
	// nothing to read?
	if (bytes_left == 0) return StdStrBuf(0);
	int result = XGetWindowProperty (dpy, pWindow->wnd,
	                                 XA_STRING, 0, bytes_left,
	                                 1, // delete it now
	                                 AnyPropertyType,
	                                 &type, &format, &len, &bytes_left, &data);
	if (result != Success) return StdStrBuf(0);
	StdStrBuf res (reinterpret_cast<char *>(data), true);
	XFree (data);
	return res;
}

// Is there something in the clipboard?
bool CStdApp::IsClipboardFull(bool fClipboard)
{
	return None != XGetSelectionOwner (dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY);
}

// Give up Selection ownership
void CStdApp::ClearClipboard(bool fClipboard)
{
	CStdAppPrivate::ClipboardData & d = fClipboard ? Priv->ClipboardSelection : Priv->PrimarySelection;
	if (!d.Text.getData()) return;
	XSetSelectionOwner(dpy, fClipboard ? XInternAtom(dpy,"CLIPBOARD",false) : XA_PRIMARY,
	                   None, d.AcquirationTime);
	d.Text.Clear();
}

CStdWindow * CStdAppPrivate::GetWindow(unsigned long wnd)
{
	WindowListT::iterator i = WindowList.find(wnd);
	if (i != WindowList.end()) return i->second;
	return 0;
}
void CStdAppPrivate::SetWindow(unsigned long wnd, CStdWindow * pWindow)
{
	if (!pWindow)
	{
		WindowList.erase(wnd);
	}
	else
	{
		WindowList[wnd] = pWindow;
	}
}

void CStdApp::OnXInput()
{
	while (XEventsQueued(dpy, QueuedAfterReading))
	{
		HandleXMessage();
	}
	if (Priv->pending_desktop)
	{
		RestoreVideoMode();
		fDspModeSet = false;
		Priv->tasked_out = true;
		Priv->pending_desktop = false;
	}
	// At least the _NET_WM_PING reply needs to be flushed,
	// and having received events is a good heuristic for
	// having issued X11 commands, even if most events
	// are mouse moves that don't generate X11 commands.
	XFlush(dpy);
}

void CStdApp::MessageDialog(const char * message)
{
#ifdef WITH_DEVELOPER_MODE
	GtkWidget * dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "OpenClonk Error");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
#endif
}
#endif /* USE_X11 */
