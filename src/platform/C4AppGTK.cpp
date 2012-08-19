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
#include <C4App.h>

#include <C4Window.h>
#include <C4DrawGL.h>
#include <C4Draw.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <glib.h>
#include <gtk/gtk.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xrandr.h>
#include <gdk/gdkx.h>
#endif

#include "c4x.xpm"

#include "C4AppGTKImpl.h"

C4AbstractApp::C4AbstractApp(): Active(false), fQuitMsgReceived(false),
		// main thread
#ifdef HAVE_PTHREAD
		MainThread (pthread_self()),
#endif
		Priv(new C4X11AppImpl(this)), fDspModeSet(false)
{
	Add(&Priv->GLibProc);
}

C4AbstractApp::~C4AbstractApp()
{
	Remove(&Priv->GLibProc);
	delete Priv;
}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");
	gtk_init(&argc, &argv);

	GdkPixbuf* icon = gdk_pixbuf_new_from_xpm_data(c4x_xpm);
	gtk_window_set_default_icon(icon);
	g_object_unref(icon);
	// Try to figure out the location of the executable
	Priv->argc=argc; Priv->argv=argv;

	int xf86vmode_event_base, xf86vmode_error_base;
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (!XF86VidModeQueryExtension(dpy, &xf86vmode_event_base, &xf86vmode_error_base)
	    || !XF86VidModeQueryVersion(dpy, &Priv->xf86vmode_major_version, &Priv->xf86vmode_minor_version))
	{
		Priv->xf86vmode_major_version = -1;
		Priv->xf86vmode_minor_version = 0;
	}
	int xrandr_error_base;
	if (!XRRQueryExtension(dpy, &Priv->xrandr_event, &xrandr_error_base)
	    || !XRRQueryVersion(dpy, &Priv->xrandr_major_version, &Priv->xrandr_minor_version))
	{
		Priv->xrandr_major_version = -1;
		Priv->xrandr_minor_version = 0;
	}
	XRRSelectInput(dpy, DefaultRootWindow(dpy), RRScreenChangeNotifyMask);
	if (Priv->xrandr_major_version < 0 && Priv->xf86vmode_major_version < 0)
		Log("Xrandr and xf86vmode extensions are missing. Resolution switching will not work.");
	else if (Priv->xrandr_major_version >= 0)
		LogF("  Using XRandR version %d.%d", Priv->xrandr_major_version, Priv->xrandr_minor_version);
	else
		LogF("  Using XF86VidMode version %d.%d", Priv->xf86vmode_major_version, Priv->xf86vmode_minor_version);

#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_install (">", readline_callback);
	readline_callback_use_this_app = this;

	Priv->stdin_channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(Priv->stdin_channel, G_IO_IN, &OnStdInInputStatic, this);
#endif

	// Custom initialization
	return DoInit (argc, argv);
}

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

void C4AbstractApp::Clear()
{
	gtk_clipboard_store_all();
#if USE_CONSOLE && HAVE_LIBREADLINE
	rl_callback_handler_remove();
#endif
}

void C4AbstractApp::Quit()
{
	fQuitMsgReceived = true;
}

bool C4AbstractApp::FlushMessages()
{
	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;

	Priv->GLibProc.IteratePendingEvents();
	return true;
}

bool C4AbstractApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (Priv->tasked_out)
		return false;
	bool modefound = false;
	if (fDspModeSet)
	{
		Priv->SwitchToDesktop(this, pWindow);
		fDspModeSet = false;
	}
	if (!fFullScreen)
	{
		if (iXRes != -1)
			pWindow->SetSize(iXRes, iYRes);
		return true;
	}
	if (Priv->xf86vmode_targetmode.hdisplay == iXRes && Priv->xf86vmode_targetmode.vdisplay == iYRes)
		modefound = true;
	// randr spec says to always get fresh info, so don't cache.
	if (Priv->xrandr_major_version >= 0)
	{
		modefound = true;
		Priv->wdt = iXRes; Priv->hgt = iYRes;
	}
	if (Priv->xf86vmode_major_version >= 0 && !modefound)
	{
		// save desktop-resolution before switching modes
		// XF86VidMode has a really weird API.
		XF86VidModeGetModeLine(dpy, DefaultScreen(dpy), (int*)&Priv->xf86vmode_oldmode.dotclock,
		                       (XF86VidModeModeLine*)(((char *)&Priv->xf86vmode_oldmode) + sizeof(Priv->xf86vmode_oldmode.dotclock)));
		if (iXRes == -1 && iYRes == -1)
		{
			Priv->xf86vmode_targetmode = Priv->xf86vmode_oldmode;
			modefound = true;
		}
	}
	if (Priv->xf86vmode_major_version >= 0 && !modefound)
	{
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
	fDspModeSet = Priv->SwitchToFullscreen(this, pWindow);
	return fDspModeSet;
}

void C4AbstractApp::RestoreVideoMode()
{
	if (fDspModeSet)
	{
		Priv->SwitchToDesktop(this, pWindow);
		fDspModeSet = false;
	}
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	if (Priv->xf86vmode_major_version < 0) return false;
	bool r = false;
	int mode_num;
	XF86VidModeModeInfo **modes;
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
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

bool C4X11AppImpl::SwitchToFullscreen(C4AbstractApp * pApp, C4Window * pWindow)
{
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (xf86vmode_major_version >= 0 && xrandr_major_version < 0 &&
	    memcmp(&xf86vmode_targetmode, &xf86vmode_oldmode, sizeof(XF86VidModeModeInfo)))
	{
		XF86VidModeModeInfo & mode = xf86vmode_targetmode;
		XResizeWindow(dpy, pWindow->wnd, mode.hdisplay, mode.vdisplay);
		XSizeHints hints;
		hints.flags = PMinSize | PMaxSize;
		hints.min_width = mode.hdisplay;
		hints.min_height = mode.vdisplay;
		hints.max_width = mode.hdisplay;
		hints.max_height = mode.vdisplay;
		XSetWMNormalHints(dpy, pWindow->wnd, &hints);
		XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), &mode);
		// Move the viewport on the virtual screen
		Window bla; int wnd_x = 0; int wnd_y = 0;
		XTranslateCoordinates(dpy, pWindow->wnd, DefaultRootWindow(dpy), 0, 0, &wnd_x, &wnd_y, &bla);
		XF86VidModeSetViewPort(dpy, DefaultScreen(dpy), wnd_x, wnd_y);
		GdkWindow * wnd = gtk_widget_get_window(GTK_WIDGET(pWindow->window));
		gdk_pointer_grab(wnd, true, GdkEventMask(0), wnd, NULL, gdk_x11_display_get_user_time(gdk_display_get_default()));
		return true;
	}
	if (xrandr_major_version >= 0 && !(wdt == -1 && hgt == -1))
	{
		XRRScreenConfiguration * conf = XRRGetScreenInfo (dpy, pWindow->wnd);
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
				XRRSetScreenConfig (dpy, conf, pWindow->wnd, i, xrandr_rot, CurrentTime);
				break;
			}
		}
		XRRFreeScreenConfigInfo(conf);
	}
	gtk_window_fullscreen(GTK_WINDOW(pWindow->window));
	return true;
}

void C4X11AppImpl::SwitchToDesktop(C4AbstractApp * pApp, C4Window * pWindow)
{
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (xf86vmode_major_version >= 0 && xrandr_major_version < 0 &&
	    memcmp(&xf86vmode_targetmode, &xf86vmode_oldmode, sizeof(XF86VidModeModeInfo)))
	{
		XF86VidModeModeInfo & mode = xf86vmode_oldmode;
		XF86VidModeSwitchToMode(dpy, DefaultScreen(dpy), &mode);
		XF86VidModeSetViewPort(dpy, DefaultScreen(dpy), 0, 0);
		XSizeHints hints;
		hints.flags = 0;
		XSetWMNormalHints(dpy, pWindow->wnd, &hints);
		gdk_pointer_ungrab(gdk_x11_display_get_user_time(gdk_display_get_default()));
		return;
	}
	gtk_window_unfullscreen(GTK_WINDOW(pWindow->window));
	// Restore resolution
	if (xrandr_major_version >= 0 && !(wdt == -1 && hgt == -1))
	{
		XRRScreenConfiguration * conf = XRRGetScreenInfo (dpy, pWindow->wnd);
#ifdef _DEBUG
		LogF("XRRSetScreenConfig %d (back)", xrandr_oldmode);
#endif
		XRRSetScreenConfig (dpy, conf, pWindow->wnd, xrandr_oldmode, xrandr_rot, CurrentTime);
		XRRFreeScreenConfigInfo(conf);
	}
}

bool C4AbstractApp::ApplyGammaRamp(_D3DGAMMARAMP& ramp, bool fForce)
{
	if (!Active && !fForce) return false;
	if (Priv->xf86vmode_major_version < 2) return false;
	if (Priv->gammasize != 256) return false;
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	return XF86VidModeSetGammaRamp(dpy, DefaultScreen(dpy), 256,
	                               ramp.red, ramp.green, ramp.blue);
}

bool C4AbstractApp::SaveDefaultGammaRamp(_D3DGAMMARAMP& ramp)
{
	if (Priv->xf86vmode_major_version < 2) return false;
	// Get the Display
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	XF86VidModeGetGammaRampSize(dpy, DefaultScreen(dpy), &Priv->gammasize);
	if (Priv->gammasize != 256)
	{
		LogF("  Size of GammaRamp is %d, not 256", Priv->gammasize);
	}
	else
	{
		// store default gamma
		if (!XF86VidModeGetGammaRamp(dpy, DefaultScreen(dpy), 256,
		                             ramp.red, ramp.green, ramp.blue))
		{
			Log("  Error getting default gamma ramp; using standard");
			return false;
		}
	}
	return true;
}

// Copy the text to the clipboard or the primary selection
bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	gtk_clipboard_set_text(gtk_clipboard_get(fClipboard ? GDK_SELECTION_CLIPBOARD : GDK_SELECTION_PRIMARY),
	                       text.getData(), text.getLength());
	return true;
}

// Paste the text from the clipboard or the primary selection
StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	char * r = gtk_clipboard_wait_for_text(gtk_clipboard_get(fClipboard ? GDK_SELECTION_CLIPBOARD : GDK_SELECTION_PRIMARY));
//	gtk_clipboard_request_text(gtk_clipboard_get(fClipboard ? GDK_SELECTION_CLIPBOARD : GDK_SELECTION_PRIMARY),
//	                           GtkClipboardTextReceivedFunc callback, gpointer user_data);
	StdStrBuf rbuf;
	rbuf.Copy(r);
	g_free(r);
	return rbuf;
}

// Is there something in the clipboard?
bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return gtk_clipboard_wait_is_text_available(gtk_clipboard_get(fClipboard ? GDK_SELECTION_CLIPBOARD : GDK_SELECTION_PRIMARY));
}

#if 0
void C4AbstractApp::OnXInput()
{
	while (XEventsQueued(dpy, QueuedAfterReading))
	{
		HandleXMessage();
	}
	// At least the _NET_WM_PING reply needs to be flushed,
	// and having received events is a good heuristic for
	// having issued X11 commands, even if most events
	// are mouse moves that don't generate X11 commands.
	XFlush(dpy);
}
#endif
void C4AbstractApp::MessageDialog(const char * message)
{
	GtkWidget * dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "OpenClonk Error");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}
