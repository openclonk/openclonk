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

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <gdk/gdkx.h>
#endif

class C4GLibProc: public StdSchedulerProc
{
public:
	C4GLibProc(GMainContext *context): context(context), query_time(C4TimeMilliseconds::NegativeInfinity) { fds.resize(1); g_main_context_ref(context); }
	~C4GLibProc()
	{
		g_main_context_unref(context);
	}

	GMainContext *context;
#ifdef STDSCHEDULER_USE_EVENTS
	std::vector<GPollFD> fds;
#else
	std::vector<pollfd> fds;
#endif
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
#ifdef STDSCHEDULER_USE_EVENTS
	virtual HANDLE GetEvent()
	{
		return reinterpret_cast<HANDLE>(fds[0].fd);
	}
#else
	virtual void GetFDs(std::vector<struct pollfd> & rfds)
	{
		if (query_time.IsInfinite()) query(C4TimeMilliseconds::Now());
		rfds.insert(rfds.end(), fds.begin(), fds.end());
	}
#endif
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
			xrandr_major_version(-1), xrandr_minor_version(-1),
			xrandr_oldmode(-1),
			xrandr_rot(0),
			xrandr_event(-1),
			argc(0), argv(0)
	{
	}

	int xrandr_major_version, xrandr_minor_version;
	int xrandr_oldmode;
	unsigned short xrandr_rot;
	int xrandr_event;

	int argc; char ** argv;
};

C4AbstractApp::C4AbstractApp(): Active(false), fQuitMsgReceived(false),
		// main thread
#ifdef _WIN32
		hInstance(NULL),
		idMainThread(::GetCurrentThreadId()),
#elif defined(HAVE_PTHREAD)
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

	GdkPixbuf* icon = gdk_pixbuf_new_from_resource("/org/openclonk/engine/oc.ico", NULL);
	gtk_window_set_default_icon(icon);
	g_object_unref(icon);
	// Try to figure out the location of the executable
	Priv->argc=argc; Priv->argv=argv;

#ifdef GDK_WINDOWING_X11
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	int xrandr_error_base;
	if (!XRRQueryExtension(dpy, &Priv->xrandr_event, &xrandr_error_base)
	    || !XRRQueryVersion(dpy, &Priv->xrandr_major_version, &Priv->xrandr_minor_version))
	{
		Priv->xrandr_major_version = -1;
		Priv->xrandr_minor_version = 0;
	}
	if (Priv->xrandr_major_version >= 0)
	{
		XRRSelectInput(dpy, DefaultRootWindow(dpy), RRScreenChangeNotifyMask);
	}
	else
		Log("The Xrandr extension is missing. Resolution switching will not work.");
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

bool C4AbstractApp::SetVideoMode(int iXRes, int iYRes, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	if (!fFullScreen)
	{
		RestoreVideoMode();
		if (iXRes != -1)
			pWindow->SetSize(iXRes, iYRes);
		return true;
	}

#ifdef GDK_WINDOWING_X11
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (Priv->xrandr_major_version >= 0 && !(iXRes == -1 && iYRes == -1))
	{
		// randr spec says to always get fresh info, so don't cache.
		XRRScreenConfiguration * conf = XRRGetScreenInfo (dpy, pWindow->renderwnd);
		if (Priv->xrandr_oldmode == -1)
			Priv->xrandr_oldmode = XRRConfigCurrentConfiguration (conf, &Priv->xrandr_rot);
		int n;
		XRRScreenSize * sizes = XRRConfigSizes(conf, &n);
		for (int i = 0; i < n; ++i)
		{
			if (int(sizes[i].width) == iXRes && int(sizes[i].height) == iYRes)
			{
#ifdef _DEBUG
				LogF("XRRSetScreenConfig %d", i);
#endif
				fDspModeSet = XRRSetScreenConfig(dpy, conf, pWindow->renderwnd, i, Priv->xrandr_rot, CurrentTime) == RRSetConfigSuccess;
				break;
			}
		}
		XRRFreeScreenConfigInfo(conf);
	}
#endif
	gtk_window_fullscreen(GTK_WINDOW(pWindow->window));
	return fDspModeSet || (iXRes == -1 && iYRes == -1);
}

void C4AbstractApp::RestoreVideoMode()
{
#ifdef GDK_WINDOWING_X11
	// Restore resolution
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	if (fDspModeSet && Priv->xrandr_major_version >= 0 && Priv->xrandr_oldmode != -1)
	{
		XRRScreenConfiguration * conf = XRRGetScreenInfo (dpy, pWindow->renderwnd);
#ifdef _DEBUG
		LogF("XRRSetScreenConfig %d (back)", Priv->xrandr_oldmode);
#endif
		XRRSetScreenConfig (dpy, conf, pWindow->renderwnd, Priv->xrandr_oldmode, Priv->xrandr_rot, CurrentTime);
		Priv->xrandr_oldmode = -1;
		XRRFreeScreenConfigInfo(conf);
		fDspModeSet = false;
	}
#endif
	// pWindow may be unset when C4AbstractApp gets destroyed during the
	// initialization code, before a window has been created
	if (pWindow)
		gtk_window_unfullscreen(GTK_WINDOW(pWindow->window));
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
#ifdef GDK_WINDOWING_X11
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	int n;
	XRRScreenSize * sizes = XRRSizes(dpy, XDefaultScreen(dpy), &n);
	if (iIndex < n && iIndex >= 0)
	{
		*piXRes = sizes[iIndex].width;
		*piYRes = sizes[iIndex].height;
		*piBitDepth = 32;
		return true;
	}
	return false;
#endif
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

void C4AbstractApp::MessageDialog(const char * message)
{
	GtkWidget * dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), "OpenClonk Error");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}
