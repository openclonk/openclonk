/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2008, 2010  Armin Burgmeier
 * Copyright (c) 2007, 2011  Günther Brammer
 * Copyright (c) 2010  Martin Plicht
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
#include <C4WindowGTK.h>

#include <C4App.h>
#include "C4Version.h"
#include "C4Config.h"
#include <C4Console.h>
#include <C4ViewportWindow.h>
#include "C4MouseControl.h"
#include <X11/Xlib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <GL/glx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>


/* C4GtkWindow */

C4GtkWindow::C4GtkWindow():
		C4Window(), window(NULL)
{
}

C4GtkWindow::~C4GtkWindow()
{
	Clear();
}

static gboolean OnKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
	C4Window* wnd = static_cast<C4Window*>(data);
	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Game.DoKeyboardInput(key, KEYEV_Down, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	wnd->CharIn(event->string); // FIXME: Use GtkIMContext somehow
	return true;
}

static gboolean OnKeyRelease(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Game.DoKeyboardInput(key, KEYEV_Up, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	return true;
}

static gboolean OnButtonPress(GtkWidget *widget, GdkEventButton * event, C4AbstractApp * pApp)
{
	pApp->KeyMask = event->state;
	return false;
}
static void OnDragDataReceivedStatic(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data)
{
	if (!Console.Editing) { Console.Message(LoadResStr("IDS_CNS_NONETEDIT")); return; }
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	gchar** uris = gtk_selection_data_get_uris(data);
	if (!uris) return;

	for (gchar** uri = uris; *uri != NULL; ++ uri)
	{
		gchar* file = g_filename_from_uri(*uri, NULL, NULL);
		if (!file) continue;

		window->cvp->DropFile(file, x, y);
		g_free(file);
	}

	g_strfreev(uris);
}

static gboolean OnExposeStatic(GtkWidget* widget, void *, gpointer user_data)
{
	C4Viewport* cvp = static_cast<C4ViewportWindow*>(user_data)->cvp;
	cvp->Execute();
	return true;
}

static void OnRealizeStatic(GtkWidget* widget, gpointer user_data)
{
	// Initial PlayerLock
	if (static_cast<C4ViewportWindow*>(user_data)->cvp->GetPlayerLock())
	{
		gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->h_scrollbar);
		gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->v_scrollbar);
	}
}

static gboolean OnKeyPressStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
#if GTK_CHECK_VERSION(2,90,7)
	if (event->keyval == GDK_KEY_Scroll_Lock)
#else
	if (event->keyval == GDK_Scroll_Lock)
#endif
	{
		static_cast<C4ViewportWindow*>(user_data)->cvp->TogglePlayerLock();
		return true;
	}
	return false;
}

static gboolean OnScrollStatic(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	if (::MouseControl.IsViewport(window->cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		switch (event->direction)
		{
		case GDK_SCROLL_UP:
			C4GUI::MouseMove(C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y, event->state + (short(1) << 16), window->cvp);
			break;
		case GDK_SCROLL_DOWN:
			C4GUI::MouseMove(C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y, event->state + (short(-1) << 16), window->cvp);
			break;
		default:
			return false;
		}
	}
	return true;
}

static gboolean OnButtonPressStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	if (::MouseControl.IsViewport(window->cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		switch (event->button)
		{
		case 1:
			if (event->type == GDK_BUTTON_PRESS)
				C4GUI::MouseMove(C4MC_Button_LeftDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			else if (event->type == GDK_2BUTTON_PRESS)
				C4GUI::MouseMove(C4MC_Button_LeftDouble, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 2:
			C4GUI::MouseMove(C4MC_Button_MiddleDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 3:
			if (event->type == GDK_BUTTON_PRESS)
				C4GUI::MouseMove(C4MC_Button_RightDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			else if (event->type == GDK_2BUTTON_PRESS)
				C4GUI::MouseMove(C4MC_Button_RightDouble, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		}
	}
	else
	{
		switch (event->button)
		{
		case 1:
			Console.EditCursor.LeftButtonDown(event->state & MK_CONTROL);
			break;
		case 3:
			Console.EditCursor.RightButtonDown(event->state & MK_CONTROL);
			break;
		}
	}

	return true;
}

static gboolean OnButtonReleaseStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	if (::MouseControl.IsViewport(window->cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		switch (event->button)
		{
		case 1:
			C4GUI::MouseMove(C4MC_Button_LeftUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 2:
			C4GUI::MouseMove(C4MC_Button_MiddleUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 3:
			C4GUI::MouseMove(C4MC_Button_RightUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		}
	}
	else
	{
		switch (event->button)
		{
		case 1:
			Console.EditCursor.LeftButtonUp();
			break;
		case 3:
			Console.EditCursor.RightButtonUp();
			break;
		}
	}

	return true;
}

static gboolean OnMotionNotifyStatic(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	if (::MouseControl.IsViewport(window->cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		C4GUI::MouseMove(C4MC_Button_None, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
	}
	else
	{
		window->EditCursorMove(event->x, event->y, event->state);
	}

	return true;
}

static gboolean OnConfigureStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
	C4Viewport* cvp = window->cvp;

	//cvp->UpdateOutputSize();
	cvp->ScrollBarsByViewPosition();

	return false;
}

static gboolean OnConfigureDareaStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
	C4Viewport* cvp = window->cvp;

	cvp->UpdateOutputSize();

	return false;
}

static void OnVScrollStatic(GtkAdjustment* adjustment, gpointer user_data)
{
	static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

static void OnHScrollStatic(GtkAdjustment* adjustment, gpointer user_data)
{
	static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

static GtkTargetEntry drag_drop_entries[] =
{
	{ const_cast<gchar*>("text/uri-list"), 0, 0 }
};

static gboolean OnConfigureNotify(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	Application.OnResolutionChanged(event->configure.width, event->configure.height);
	return false;
}

static bool fullscreen_needs_restore = false;
static gboolean fullscreen_restore(gpointer data)
{
	if (fullscreen_needs_restore)
		Application.SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, !Config.Graphics.Windowed);
	fullscreen_needs_restore = false;
	return FALSE;
}

static gboolean OnFocusInFS(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	Application.Active = true;
	fullscreen_needs_restore = true;
	gdk_threads_add_idle(fullscreen_restore, NULL);
	return false;
}
static gboolean OnFocusOutFS(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	Application.Active = false;
	if (!Config.Graphics.Windowed)
	{
		Application.RestoreVideoMode();
		fullscreen_needs_restore = false;
	}
	return false;
}

static gboolean OnButtonPressFS(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	switch (event->button)
	{
	case 1:
		if (event->type == GDK_BUTTON_PRESS)
			C4GUI::MouseMove(C4MC_Button_LeftDown, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
		else if (event->type == GDK_2BUTTON_PRESS)
			C4GUI::MouseMove(C4MC_Button_LeftDouble, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
		break;
	case 2:
		C4GUI::MouseMove(C4MC_Button_MiddleDown, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
		break;
	case 3:
		if (event->type == GDK_BUTTON_PRESS)
			C4GUI::MouseMove(C4MC_Button_RightDown, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
		else if (event->type == GDK_2BUTTON_PRESS)
			C4GUI::MouseMove(C4MC_Button_RightDouble, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
		break;
	default:
		return false;
	}
	return true;
}

gboolean OnButtonRelease(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	int b;
	switch (event->button)
	{
	case 1: b = C4MC_Button_LeftUp; break;
	case 2: b = C4MC_Button_MiddleUp; break;
	case 3: b = C4MC_Button_RightUp; break;
	default: return false;
	}
	C4GUI::MouseMove(b, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
	return true;
}

static gboolean OnMotionNotify(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
	C4GUI::MouseMove(C4MC_Button_None, (int32_t)event->x, (int32_t)event->y, event->state, NULL);
	return true;
}

static gboolean OnScrollGD(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(window);
	if (!pDlg) return false;
	switch (event->direction)
	{
	case GDK_SCROLL_UP:
		::pGUI->MouseInput(C4MC_Button_Wheel, event->x, event->y, event->state + (short(32) << 16), pDlg, NULL);
		return true;
	case GDK_SCROLL_DOWN:
		::pGUI->MouseInput(C4MC_Button_Wheel, event->x, event->y, event->state + (short(-32) << 16), pDlg, NULL);
		return true;
	default:
		return false;
	}
}

static gboolean OnButtonPressGD(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(window);

	switch (event->button)
	{
	case 1:
		if (event->type == GDK_2BUTTON_PRESS)
		{
			::pGUI->MouseInput(C4MC_Button_LeftDouble, event->x, event->y, event->state, pDlg, NULL);
		}
		else if (event->type == GDK_BUTTON_PRESS)
		{
			::pGUI->MouseInput(C4MC_Button_LeftDown,event->x, event->y, event->state, pDlg, NULL);
		}
		break;
	case 2:
		if (event->type == GDK_BUTTON_PRESS)
			::pGUI->MouseInput(C4MC_Button_MiddleDown, event->x, event->y, event->state, pDlg, NULL);
		break;
	case 3:
		if (event->type == GDK_2BUTTON_PRESS)
		{
			::pGUI->MouseInput(C4MC_Button_RightDouble, event->x, event->y, event->state, pDlg, NULL);
		}
		else if (event->type == GDK_BUTTON_PRESS)
		{
			::pGUI->MouseInput(C4MC_Button_RightDown, event->x, event->y, event->state, pDlg, NULL);
		}
		break;
	}

	return true;
}

static gboolean OnButtonReleaseGD(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(window);

	switch (event->button)
	{
	case 1:
		::pGUI->MouseInput(C4MC_Button_LeftUp, event->x, event->y, event->state, pDlg, NULL);
		break;
	case 2:
		::pGUI->MouseInput(C4MC_Button_MiddleUp, event->x, event->y, event->state, pDlg, NULL);
		break;
	case 3:
		::pGUI->MouseInput(C4MC_Button_RightUp, event->x, event->y, event->state, pDlg, NULL);
		break;
	}
	return true;
}

static gboolean OnMotionNotifyGD(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(window);

	::pGUI->MouseInput(C4MC_Button_None, event->x, event->y, event->state, pDlg, NULL);

	return true;
}

static gboolean OnConfigureGD(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);

	window->pSurface->UpdateSize(event->width, event->height);

	return false;
}

C4Window* C4GtkWindow::Init(WindowKind windowKind, C4AbstractApp * pApp, const char * Title, C4Window * pParent, bool HideCursor)
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
	if (windowKind == W_Viewport)
	{
		C4ViewportWindow * vw = static_cast<C4ViewportWindow *>(this);
		gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

		// Cannot just use ScrolledWindow because this would just move
		// the GdkWindow of the DrawingArea.
		GtkWidget* table;

		render_widget = gtk_drawing_area_new();
		vw->h_scrollbar = gtk_hscrollbar_new(NULL);
		vw->v_scrollbar = gtk_vscrollbar_new(NULL);
		table = gtk_table_new(2, 2, false);

		GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(vw->h_scrollbar));

		g_signal_connect(
		  G_OBJECT(adjustment),
		  "value-changed",
		  G_CALLBACK(OnHScrollStatic),
		  this
		);

		adjustment = gtk_range_get_adjustment(GTK_RANGE(vw->v_scrollbar));

		g_signal_connect(
		  G_OBJECT(adjustment),
		  "value-changed",
		  G_CALLBACK(OnVScrollStatic),
		  this
		);

		gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(render_widget), 0, 1, 0, 1, static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), 0, 0);
		gtk_table_attach(GTK_TABLE(table), vw->v_scrollbar, 1, 2, 0, 1, GTK_SHRINK, static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND), 0, 0);
		gtk_table_attach(GTK_TABLE(table), vw->h_scrollbar, 0, 1, 1, 2, static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 0, 0);

		gtk_container_add(GTK_CONTAINER(window), table);

		gtk_widget_add_events(GTK_WIDGET(window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK | GDK_POINTER_MOTION_MASK);

		gtk_drag_dest_set(GTK_WIDGET(render_widget), GTK_DEST_DEFAULT_ALL, drag_drop_entries, 1, GDK_ACTION_COPY);
		g_signal_connect(G_OBJECT(render_widget), "drag-data-received", G_CALLBACK(OnDragDataReceivedStatic), this);
	#if GTK_CHECK_VERSION(3,0,0)
		g_signal_connect(G_OBJECT(render_widget), "draw", G_CALLBACK(OnExposeStatic), this);
	#else
		g_signal_connect(G_OBJECT(render_widget), "expose-event", G_CALLBACK(OnExposeStatic), this);
	#endif
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnKeyPressStatic), this);
		g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScrollStatic), this);
		g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPressStatic), this);
		g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(OnButtonReleaseStatic), this);
		g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(OnMotionNotifyStatic), this);
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnKeyPress), this);
		g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnKeyRelease), this);
		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(OnConfigureStatic), this);
		g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(OnRealizeStatic), this);

		g_signal_connect_after(G_OBJECT(render_widget), "configure-event", G_CALLBACK(OnConfigureDareaStatic), this);

		// do not draw the default background
		gtk_widget_set_double_buffered (GTK_WIDGET(render_widget), false);

		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(Console.window));
	}
	else if (windowKind == W_Fullscreen)
	{
		render_widget = gtk_drawing_area_new();
		gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(render_widget));

		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(OnConfigureNotify), this);
		g_signal_connect(G_OBJECT(window), "focus-in-event", G_CALLBACK(OnFocusInFS), this);
		g_signal_connect(G_OBJECT(window), "focus-out-event", G_CALLBACK(OnFocusOutFS), this);
		g_signal_connect(G_OBJECT(window), "unmap-event", G_CALLBACK(OnFocusOutFS), this);
		g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPressFS), this);
		g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(OnButtonRelease), this);
		g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(OnMotionNotify), this);
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnKeyPress), this);
		g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnKeyRelease), this);
		gtk_widget_add_events(GTK_WIDGET(window), GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
		gtk_widget_set_double_buffered (GTK_WIDGET(render_widget), false);

		GValue val = G_VALUE_INIT;
		g_value_init (&val, G_TYPE_BOOLEAN);
		g_value_set_boolean (&val, true);
		g_object_set_property (G_OBJECT (render_widget), "can-focus", &val);
		g_object_set_property (G_OBJECT (window), "can-focus", &val);
		g_value_unset (&val);
	}
	else if (windowKind == W_GuiWindow)
	{
		render_widget = window;
		g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPressGD), this);
		g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(OnButtonReleaseGD), this);
		g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(OnMotionNotifyGD), this);
		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(OnConfigureGD), this);
		g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScrollGD), this);

		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(Console.window));
	}
	assert(window);
	// Override gtk's default to match name/class of the XLib windows
	gtk_window_set_wmclass(GTK_WINDOW(window), C4ENGINENAME, C4ENGINENAME);

	handlerDestroy = g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroyStatic), this);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPress), pApp);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnUpdateKeyMask), pApp);
	g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnUpdateKeyMask), pApp);

	if(!render_widget)
		render_widget = InitGUI();

	GdkScreen * scr = gtk_widget_get_screen(GTK_WIDGET(render_widget));
	GdkVisual * vis = gdk_x11_screen_lookup_visual(scr, ((XVisualInfo*)Info)->visualid);
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);
#else
	GdkColormap * cmap = gdk_colormap_new(vis, true);
	gtk_widget_set_colormap(GTK_WIDGET(render_widget), cmap);
	g_object_unref(cmap);
#endif
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
	wnd = GDK_WINDOW_XID(window_wnd);
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

		renderwnd = GDK_WINDOW_XID(bin_wnd);
	}
	else
	{
#if GTK_CHECK_VERSION(2,14,0)
		GdkWindow* render_wnd = gtk_widget_get_window(GTK_WIDGET(render_widget));
#else
		GdkWindow* render_wnd = GTK_WIDGET(render_widget)->window;
#endif

		renderwnd = GDK_WINDOW_XID(render_wnd);
	}

	// Make sure the window is shown and ready to be rendered into,
	// this avoids an async X error.
	gdk_flush();

	if (windowKind == W_Fullscreen)
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(render_widget)), gdk_cursor_new(GDK_BLANK_CURSOR));
	return this;
}

bool C4GtkWindow::ReInit(C4AbstractApp* pApp)
{
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

	GdkScreen * scr = gtk_widget_get_screen(GTK_WIDGET(render_widget));
	GdkVisual * vis = gdk_x11_screen_lookup_visual(scr, static_cast<XVisualInfo*>(new_info)->visualid);
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);
#else
	GdkColormap * cmap = gdk_colormap_new(vis, true);
	gtk_widget_set_colormap(GTK_WIDGET(render_widget), cmap);
	g_object_unref(cmap);
#endif
	// create a new X11 window
	gtk_widget_unrealize(GTK_WIDGET(render_widget));
	gtk_widget_realize(GTK_WIDGET(render_widget));

	delete static_cast<XVisualInfo*>(Info);
	Info = new_info;

	return true;
}

void C4GtkWindow::Clear()
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

	// We must free it here since we do not call C4Window::Clear()
	if (Info)
	{
		delete static_cast<XVisualInfo*>(Info);
		Info = 0;
	}
}

void C4GtkWindow::OnDestroyStatic(GtkWidget* widget, gpointer data)
{
	C4GtkWindow* wnd = static_cast<C4GtkWindow*>(data);

	g_signal_handler_disconnect(wnd->window, wnd->handlerDestroy);
	//gtk_widget_destroy(wnd->window);
	wnd->handlerDestroy = 0;
	wnd->window = NULL;
	wnd->Active = false;
	wnd->wnd = wnd->renderwnd = 0;

	wnd->Close();
}

GdkFilterReturn C4GtkWindow::OnFilter(GdkXEvent* xevent, GdkEvent* event, gpointer user_data)
{
	// Handle raw X message, then let GTK+ process it
	static_cast<C4GtkWindow*>(user_data)->HandleMessage(*reinterpret_cast<XEvent*>(xevent));
	return GDK_FILTER_CONTINUE;
}

gboolean C4GtkWindow::OnUpdateKeyMask(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	// Update mask so that Application.IsShiftDown,
	// Application.IsControlDown etc. work.
	unsigned int mask = 0;
	if (event->state & GDK_SHIFT_MASK) mask |= MK_SHIFT;
	if (event->state & GDK_CONTROL_MASK) mask |= MK_CONTROL;
	if (event->state & GDK_MOD1_MASK) mask |= (1 << 3);

	// For keypress/relases, event->state contains the state _before_
	// the event, but we need to store the current state.
#if !GTK_CHECK_VERSION(2,21,8)
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

	static_cast<C4AbstractApp*>(user_data)->KeyMask = mask;
	return false;
}

GtkWidget* C4GtkWindow::InitGUI()
{
	return window;
}

void C4Window::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

bool OpenURL(const char *szURL)
{
	GError *error = 0;
#if GTK_CHECK_VERSION(2,14,0)
	if (gtk_show_uri(NULL, szURL, GDK_CURRENT_TIME, &error))
		return true;
	if (error != NULL)
	{
		fprintf (stderr, "Unable to open URL: %s\n", error->message);
		g_error_free (error);
	}
#endif
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
		error = 0;
		if (g_spawn_async (g_get_home_dir(), const_cast<char**>(argv[i]), 0, G_SPAWN_SEARCH_PATH, 0, 0, 0, &error))
			return true;
		else
		{
			fprintf(stderr, "%s\n", error->message);
			g_error_free (error);
		}
	}
	return false;
}
