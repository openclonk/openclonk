/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2008, 2010  Armin Burgmeier
 * Copyright (c) 2010  Mortimer
 * Copyright (c) 2006-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* A wrapper class to OS dependent event and window interfaces, GTK+ version */

#include <C4Include.h>
#include <StdGtkWindow.h>

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "C4Version.h"

/* CStdGtkWindow */

CStdGtkWindow::CStdGtkWindow():
		CStdWindow(), window(NULL)
{
}

CStdGtkWindow::~CStdGtkWindow()
{
	Clear();
}

CStdWindow* CStdGtkWindow::Init(WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor)
{
	Active = true;
	dpy = pApp->dpy;

	if(!FindInfo(Config.Graphics.MultiSampling, &Info))
	{
		// Disable multisampling if we don't find a visual which
		// supports the currently configured setting.
		if(!FindInfo(0, &Info)) return NULL;
		Config.Graphics.MultiSampling = 0;
	}

	assert(!window);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	// Override gtk's default to match name/class of the XLib windows
	gtk_window_set_wmclass(GTK_WINDOW(window), C4ENGINENAME, C4ENGINENAME);

	handlerDestroy = g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroyStatic), this);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnUpdateKeyMask), pApp);
	g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnUpdateKeyMask), pApp);

	GtkWidget* render_widget = InitGUI();

	gtk_widget_set_colormap(render_widget, gdk_colormap_new(gdkx_visual_get(((XVisualInfo*)Info)->visualid), true));

	gtk_widget_show_all(window);

//  XVisualInfo vitmpl; int blub;
//  vitmpl.visual = gdk_x11_visual_get_xvisual(gtk_widget_get_visual(window));
//  vitmpl.visualid = XVisualIDFromVisual(vitmpl.visual);
//  Info = XGetVisualInfo(dpy, VisualIDMask, &vitmpl, &blub);

//  printf("%p\n", gtk_widget_get_visual(render_widget));
//  Info = gdk_x11_visual_get_xvisual(gtk_widget_get_visual(render_widget));

	// Default icon has been set right after gtk_init(),
	// so we don't need to take care about setting the icon here.

	gtk_window_set_title(GTK_WINDOW(window), Title);

#if GTK_CHECK_VERSION(2,14,0)
	GdkWindow* window_wnd = gtk_widget_get_window(window);
#else
	GdkWindow* window_wnd = window->window;
#endif

	// Wait until window is mapped to get the window's XID
	gtk_widget_show_now(window);
	wnd = GDK_WINDOW_XWINDOW(window_wnd);
	gdk_window_add_filter(window_wnd, OnFilter, this);

	XWMHints * wm_hint = XGetWMHints(dpy, wnd);
	if (!wm_hint) wm_hint = XAllocWMHints();
	Hints = wm_hint;

	if (GTK_IS_LAYOUT(render_widget))
	{
#if GTK_CHECK_VERSION(2,14,0)
		GdkWindow* bin_wnd = gtk_layout_get_bin_window(GTK_LAYOUT(render_widget));
#else
		GdkWindow* bin_wnd = GTK_LAYOUT(render_widget)->bin_window;
#endif

		renderwnd = GDK_WINDOW_XWINDOW(bin_wnd);
	}
	else
	{
#if GTK_CHECK_VERSION(2,14,0)
		GdkWindow* render_wnd = gtk_widget_get_window(render_widget);
#else
		GdkWindow* render_wnd = render_widget->window;
#endif

		renderwnd = GDK_WINDOW_XWINDOW(render_wnd);
	}

	if (pParent) XSetTransientForHint(dpy, wnd, pParent->wnd);

	if (HideCursor)
	{
		// TODO!
//    GdkCursor* cursor = gdk_cursor_new_from_pixmap(NULL, NULL, NULL, NULL, 0, 0);
		gdk_window_set_cursor(window_wnd, NULL);
	}

	// Make sure the window is shown and ready to be rendered into,
	// this avoids an async X error.
	gdk_flush();

	return this;
}

bool CStdGtkWindow::ReInit(CStdApp* pApp)
{
	// TODO: Recreate the window with a newly chosen visual
	// Probably we don't need this, since there is no way to change
	// MultiSampling when no window is open.
	return false;
}

void CStdGtkWindow::Clear()
{
	if (window != NULL)
	{
		g_signal_handler_disconnect(window, handlerDestroy);
		gtk_widget_destroy(window);
		handlerDestroy = 0;
	}

	// Avoid that the base class tries to free these
	wnd = renderwnd = 0;

	window = NULL;
	Active = false;

	// We must free it here since we do not call CStdWindow::Clear()
	if (Info)
	{
		delete static_cast<XVisualInfo*>(Info);
		Info = 0;
	}
}

void CStdGtkWindow::OnDestroyStatic(GtkWidget* widget, gpointer data)
{
	CStdGtkWindow* wnd = static_cast<CStdGtkWindow*>(data);

	g_signal_handler_disconnect(wnd->window, wnd->handlerDestroy);
	//gtk_widget_destroy(wnd->window);
	wnd->handlerDestroy = 0;
	wnd->window = NULL;
	wnd->Active = false;
	wnd->wnd = wnd->renderwnd = 0;

	wnd->Close();
}

GdkFilterReturn CStdGtkWindow::OnFilter(GdkXEvent* xevent, GdkEvent* event, gpointer user_data)
{
	// Handle raw X message, then let GTK+ process it
	static_cast<CStdGtkWindow*>(user_data)->HandleMessage(*reinterpret_cast<XEvent*>(xevent));
	return GDK_FILTER_CONTINUE;
}

gboolean CStdGtkWindow::OnUpdateKeyMask(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	// Update mask so that Application.IsShiftDown,
	// Application.IsControlDown etc. work.
	unsigned int mask = 0;
	if (event->state & GDK_SHIFT_MASK) mask |= MK_SHIFT;
	if (event->state & GDK_CONTROL_MASK) mask |= MK_CONTROL;
	if (event->state & GDK_MOD1_MASK) mask |= (1 << 3);

	// For keypress/relases, event->state contains the state _before_
	// the event, but we need to store the current state.
#if !GTK_CHECK_VERSION(2,90,7)
# define GDK_KEY_Shift_L GDK_Shift_L
# define GDK_KEY_Shift_R GDK_Shift_R
# define GDK_KEY_Control_L GDK_Control_L
# define GDK_KEY_Control_R GDK_Control_R
# define GDK_KEY_Alt_L GDK_Alt_L
# define GDK_KEY_Alt_R GDK_Alt_R
#endif

	if (event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R) mask ^= MK_SHIFT;
	if (event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R) mask ^= MK_CONTROL;
	if (event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R) mask ^= (1 << 3);

	static_cast<CStdApp*>(user_data)->KeyMask = mask;
	return false;
}

GtkWidget* CStdGtkWindow::InitGUI()
{
	return window;
}

void CStdWindow::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

bool OpenURL(const char *szURL)
{
	const char * argv[][3] =
	{
		{ "xdg-open", szURL, 0 },
		{ "sensible-browser", szURL, 0 },
		{ "firefox", szURL, 0 },
		{ "mozilla", szURL, 0 },
		{ "konqueror", szURL, 0 },
		{ "epiphany", szURL, 0 },
		{ 0, 0, 0 }
	};
	for (int i = 0; argv[i][0]; ++i)
	{
		GError * error = 0;
		if (g_spawn_async (g_get_home_dir(), const_cast<char**>(argv[i]), 0, G_SPAWN_SEARCH_PATH, 0, 0, 0, &error))
			return true;
		else fprintf(stderr, "%s\n", error->message);
	}
	return false;
}