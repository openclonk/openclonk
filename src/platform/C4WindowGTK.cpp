/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2015, The OpenClonk Team and contributors
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

/* A wrapper class to OS dependent event and window interfaces, GTK+ version */

#include <C4Include.h>
#include <C4Window.h>

#include <C4App.h>
#include "C4Version.h"
#include <C4Config.h>

#include <C4DrawGL.h>
#include <C4Draw.h>
#include <StdFile.h>
#include <StdBuf.h>

#include <C4Rect.h>

#include <C4Console.h>
#include <C4ViewportWindow.h>
#include <C4Viewport.h>
#include "C4MouseControl.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#ifdef USE_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

#include "C4AppGTKImpl.h"

// Some helper functions for choosing a proper visual

#ifndef USE_CONSOLE

namespace {
static const std::map<int, int> base_attrib_map {
	{GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT},
	{GLX_X_RENDERABLE, True},
	{GLX_RED_SIZE, 4},
	{GLX_GREEN_SIZE, 4},
	{GLX_BLUE_SIZE, 4},
	{GLX_DEPTH_SIZE, 8}
};

// Turns an int->int map into an attribute list suitable for any GLX calls.
std::unique_ptr<int[]> MakeGLXAttribList(const std::map<int, int> &map)
{
	// We need two ints for every attribute, plus one as a sentinel
	auto list = std::make_unique<int[]>(map.size() * 2 + 1);
	int *cursor = list.get();
	for(const auto &attrib : map)
	{
		*cursor++ = attrib.first;
		*cursor++ = attrib.second;
	}
	*cursor = None;
	return list;
}

// This function picks an acceptable GLXFBConfig. To do this, we first
// request a list of framebuffer configs with no less than 4 bits per color;
// no less than 8 bits of depth buffer; if multisampling is not -1,
// with at least the requested number of samples; and with double buffering.
// If that returns no suitable configs, we retry with only a single buffer.
GLXFBConfig PickGLXFBConfig(Display* dpy, int multisampling)
{
	std::map<int, int> attrib_map = base_attrib_map;

	if (multisampling >= 0)
	{
		attrib_map[GLX_SAMPLE_BUFFERS] = multisampling > 0 ? 1 : 0;
		attrib_map[GLX_SAMPLES] = multisampling;
	}

	GLXFBConfig *configs = NULL;
	int config_count;
	// Find a double-buffered FB config
	attrib_map[GLX_DOUBLEBUFFER] = True;
	std::unique_ptr<int[]> attribs = MakeGLXAttribList(attrib_map);
	configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs.get(), &config_count);
	if (config_count == 0)
	{
		// If none exists, try to find a single-buffered one
		if (configs != NULL)
			XFree(configs);
		attrib_map[GLX_DOUBLEBUFFER] = False;
		attribs = MakeGLXAttribList(attrib_map);
		configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs.get(), &config_count);
	}

	GLXFBConfig config = NULL;
	if (config_count > 0)
	{
		config = configs[0];
	}

	XFree(configs);
	return config;
}
}

#endif // #ifndef USE_CONSOLE
static void OnDestroyStatic(GtkWidget* widget, gpointer data)
{
	C4Window* wnd = static_cast<C4Window*>(data);
	wnd->Clear();
}

static gboolean OnDelete(GtkWidget* widget, GdkEvent* event, gpointer data)
{
	C4Window* wnd = static_cast<C4Window*>(data);
	wnd->Close();
	return true;
}

static gboolean OnKeyPress(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
	C4Window* wnd = static_cast<C4Window*>(data);
	// keycode = scancode + 8
	if (event->hardware_keycode <= 8) return false;
	Game.DoKeyboardInput(event->hardware_keycode-8, KEYEV_Down, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	wnd->CharIn(event->string); // FIXME: Use GtkIMContext somehow
	return true;
}

static gboolean OnKeyRelease(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	// keycode = scancode + 8
	if (event->hardware_keycode <= 8) return false;
	Game.DoKeyboardInput(event->hardware_keycode-8, KEYEV_Up, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	return true;
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
	if (event->keyval == GDK_KEY_Scroll_Lock)
	{
		static_cast<C4ViewportWindow*>(user_data)->cvp->TogglePlayerLock();
		return true;
	}
	if (event->hardware_keycode <= 8) return false;
	Console.EditCursor.KeyDown(event->hardware_keycode - 8, event->state);
	return false;
}

static gboolean OnKeyReleaseStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	if (event->hardware_keycode <= 8) return false;
	Console.EditCursor.KeyUp(event->hardware_keycode - 8, event->state);
	return false;
}

static gboolean OnScrollVW(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
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
			Console.EditCursor.LeftButtonDown(event->state);
			break;
		case 3:
			Console.EditCursor.RightButtonDown(event->state);
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
			Console.EditCursor.LeftButtonUp(event->state);
			break;
		case 3:
			Console.EditCursor.RightButtonUp(event->state);
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
		Application.SetVideoMode(Application.GetConfigWidth(), Application.GetConfigHeight(), Config.Graphics.BitDepth, Config.Graphics.RefreshRate, Config.Graphics.Monitor, Application.FullScreenMode());
	fullscreen_needs_restore = false;
	return FALSE;
}

static gboolean OnFocusInFS(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	Application.Active = true;
	if (Application.FullScreenMode())
	{
		fullscreen_needs_restore = true;
		gdk_threads_add_idle(fullscreen_restore, NULL);
	}
	return false;
}
static gboolean OnFocusOutFS(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
	Application.Active = false;
	if (Application.FullScreenMode() && Application.GetConfigWidth() != -1)
	{
		Application.RestoreVideoMode();
		gtk_window_iconify(GTK_WINDOW(widget));
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

static gboolean OnScroll(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
{
	C4GUI::DialogWindow * window = static_cast<C4GUI::DialogWindow*>(user_data);
	C4GUI::Dialog *pDlg = ::pGUI->GetDialog(window);
	int idy;
	switch (event->direction)
	{
	case GDK_SCROLL_UP: idy = 32; break;
	case GDK_SCROLL_DOWN: idy = -32; break;
	default: return false;
	}

	// FIXME: make the GUI api less insane here
	if (pDlg)
		::pGUI->MouseInput(C4MC_Button_Wheel, event->x, event->y, event->state + (idy << 16), pDlg, NULL);
	else
		C4GUI::MouseMove(C4MC_Button_Wheel, event->x, event->y, event->state + (idy << 16), NULL);
	return true;
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

C4Window::C4Window ():
		Active(false), pSurface(0), wnd(0), renderwnd(0), Info(0), window(NULL)
{
}

C4Window::~C4Window ()
{
	Clear();
}

bool C4Window::FindFBConfig(int samples, GLXFBConfig *info)
{
#ifndef USE_CONSOLE
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	GLXFBConfig config = PickGLXFBConfig(dpy, samples);
	if (info)
	{
		*info = config;
	}
	return config != NULL;
#else
	// TODO: Do we need to handle this case?
#endif // #ifndef USE_CONSOLE

	return false;
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
{
#ifndef USE_CONSOLE
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	std::map<int, int> attribs = base_attrib_map;
	attribs[GLX_SAMPLE_BUFFERS_ARB] = 1;

	int config_count = 0;
	GLXFBConfig *configs = glXChooseFBConfig(dpy, DefaultScreen(dpy), MakeGLXAttribList(attribs).get(), &config_count);

	std::set<int> multisamples;
	for(int i = 0; i < config_count; ++i)
	{
		int v_samples;
		glXGetFBConfigAttrib(dpy, configs[i], GLX_SAMPLES, &v_samples);
		multisamples.insert(v_samples);
	}

	XFree(configs);
	samples.assign(multisamples.cbegin(), multisamples.cend());
#endif
}

bool C4Window::StorePosition(const char *, const char *, bool) { return true; }

bool C4Window::RestorePosition(const char *, const char *, bool)
{
	// The Windowmanager is responsible for window placement.
	return true;
}

void C4Window::FlashWindow()
{
	//FIXME - how is this reset? gtk_window_set_urgency_hint(window, true);
}

C4Window* C4Window::Init(WindowKind windowKind, C4AbstractApp * pApp, const char * Title, const C4Rect * size)
{
	Active = true;

	if(!FindFBConfig(Config.Graphics.MultiSampling, &Info))
	{
		// Disable multisampling if we don't find a visual which
		// supports the currently configured setting.
		if(!FindFBConfig(0, &Info)) return NULL;
		Config.Graphics.MultiSampling = 0;
	}

	assert(!window);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if (windowKind == W_Viewport)
	{
		C4ViewportWindow * vw = static_cast<C4ViewportWindow *>(this);

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
		g_signal_connect(G_OBJECT(render_widget), "draw", G_CALLBACK(OnExposeStatic), this);
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnKeyPressStatic), this);
		g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnKeyReleaseStatic), this);
		g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScrollVW), this);
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
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
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
		g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScroll), this);
		gtk_widget_add_events(GTK_WIDGET(window), GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
		gtk_widget_set_double_buffered (GTK_WIDGET(render_widget), false);

		GValue val = {0,{{0}}};
		g_value_init (&val, G_TYPE_BOOLEAN);
		g_value_set_boolean (&val, true);
		g_object_set_property (G_OBJECT (render_widget), "can-focus", &val);
		g_object_set_property (G_OBJECT (window), "can-focus", &val);
		g_value_unset (&val);
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
	}
	else if (windowKind == W_GuiWindow)
	{
		render_widget = window;
		g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPressGD), this);
		g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(OnButtonReleaseGD), this);
		g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(OnMotionNotifyGD), this);
		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(OnConfigureGD), this);
		g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScroll), this);

		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(Console.window));
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
	}
	else if (windowKind == W_Console)
	{
		render_widget = window;
	}
	assert(window);
	assert(render_widget);
	// Override gtk's default to match name/class of the XLib windows
	gtk_window_set_wmclass(GTK_WINDOW(window), C4ENGINENAME, C4ENGINENAME);
	gtk_window_set_default_size(GTK_WINDOW(window), size->Wdt, size->Hgt);

	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(OnDelete), this);
	handlerDestroy = g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(OnDestroyStatic), this);
	gtk_widget_add_events(GTK_WIDGET(window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK);

	// TODO: It would be nice to support GDK_SCROLL_SMOOTH_MASK and
	// smooth scrolling for scrolling in menus, however that should not
	// change the scroll wheel behaviour ingame for zooming or
	// inventory change. Note that when both GDK_SCROLL_MASK and
	// GDK_SMOOTH_SCROLL_MASK are enabled, both type of scroll events
	// are reported, so one needs to make sure to not double-process them.
	// It would be nice to have smooth scrolling also e.g. for zooming
	// ingame, but it probably requires the notion of smooth scrolling
	// other parts of the engine as well.

	GdkScreen * scr = gtk_widget_get_screen(GTK_WIDGET(render_widget));
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	XVisualInfo *vis_info = glXGetVisualFromFBConfig(dpy, Info);
	assert(vis_info);
	GdkVisual * vis = gdk_x11_screen_lookup_visual(scr, vis_info->visualid);
	XFree(vis_info);
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);
	gtk_widget_show_all(GTK_WIDGET(window));

//  XVisualInfo vitmpl; int blub;
//  vitmpl.visual = gdk_x11_visual_get_xvisual(gtk_widget_get_visual(window));
//  vitmpl.visualid = XVisualIDFromVisual(vitmpl.visual);
//  Info = XGetVisualInfo(dpy, VisualIDMask, &vitmpl, &blub);

//  printf("%p\n", gtk_widget_get_visual(render_widget));
//  Info = gdk_x11_visual_get_xvisual(gtk_widget_get_visual(render_widget));

	// Default icon has been set right after gtk_init(),
	// so we don't need to take care about setting the icon here.

	SetTitle(Title);

	GdkWindow* window_wnd = gtk_widget_get_window(GTK_WIDGET(window));

	// Wait until window is mapped to get the window's XID
	gtk_widget_show_now(GTK_WIDGET(window));
	wnd = GDK_WINDOW_XID(window_wnd);

	if (GTK_IS_LAYOUT(render_widget))
	{
		GdkWindow* bin_wnd = gtk_layout_get_bin_window(GTK_LAYOUT(render_widget));

		renderwnd = GDK_WINDOW_XID(bin_wnd);
	}
	else
	{
		GdkWindow* render_wnd = gtk_widget_get_window(GTK_WIDGET(render_widget));

		renderwnd = GDK_WINDOW_XID(render_wnd);
	}

	// Make sure the window is shown and ready to be rendered into,
	// this avoids an async X error.
	gdk_flush();

	if (windowKind == W_Fullscreen)
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(render_widget)), gdk_cursor_new(GDK_BLANK_CURSOR));
	return this;
}

bool C4Window::ReInit(C4AbstractApp* pApp)
{
	// Check whether multisampling settings was changed. If not then we
	// don't need to ReInit anything.
#ifndef USE_CONSOLE
	int value;
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
	glXGetFBConfigAttrib(dpy, Info, GLX_SAMPLES, &value);
	if(value == Config.Graphics.MultiSampling) return true;
#else
	return true;
#endif
	// Check whether we have a visual with the requested number of samples
	GLXFBConfig new_info;
	if(!FindFBConfig(Config.Graphics.MultiSampling, &new_info)) return false;

	GdkScreen * scr = gtk_widget_get_screen(GTK_WIDGET(render_widget));
	XVisualInfo *vis_info = glXGetVisualFromFBConfig(dpy, new_info);
	assert(vis_info);
	GdkVisual * vis = gdk_x11_screen_lookup_visual(scr, vis_info->visualid);
	XFree(vis_info);

	// Un- and re-realizing the render_widget does not work, the window
	// remains hidden afterwards. So we re-create it from scratch.
	gtk_widget_destroy(GTK_WIDGET(render_widget));
	render_widget = gtk_drawing_area_new();
	gtk_widget_set_double_buffered (GTK_WIDGET(render_widget), false);
	g_object_set(G_OBJECT(render_widget), "can-focus", TRUE, NULL);
	
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);

	Info = new_info;

	// Wait until window is mapped to get the window's XID
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(render_widget));
	gtk_widget_show_now(GTK_WIDGET(render_widget));

	if (GTK_IS_LAYOUT(render_widget))
	{
		GdkWindow* bin_wnd = gtk_layout_get_bin_window(GTK_LAYOUT(render_widget));
		renderwnd = GDK_WINDOW_XID(bin_wnd);
	}
	else
	{
		GdkWindow* render_wnd = gtk_widget_get_window(GTK_WIDGET(render_widget));
		renderwnd = GDK_WINDOW_XID(render_wnd);
	}

	gdk_flush();
	gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(render_widget)), gdk_cursor_new(GDK_BLANK_CURSOR));
	return true;
}

void C4Window::Clear()
{
	if (window != NULL)
	{
		g_signal_handler_disconnect(window, handlerDestroy);
		gtk_widget_destroy(GTK_WIDGET(window));
		handlerDestroy = 0;
	}

	// Avoid that the base class tries to free these
	wnd = renderwnd = 0;

	window = NULL;
	Active = false;

	Info = 0;
}

void C4Window::SetSize(unsigned int width, unsigned int height)
{
	gtk_window_resize(GTK_WINDOW(window), width, height);
}

bool C4Window::GetSize(C4Rect * r)
{
	r->x = 0; r->y = 0;
	gtk_window_get_size(GTK_WINDOW(window), &r->Wdt, &r->Hgt);
	return true;
}

void C4Window::SetTitle(char const * Title)
{
	gtk_window_set_title(GTK_WINDOW(window), Title);
}

void C4Window::RequestUpdate()
{
	// just invoke directly
	PerformUpdate();
}

bool OpenURL(const char *szURL)
{
	GError *error = 0;
	if (gtk_show_uri(NULL, szURL, GDK_CURRENT_TIME, &error))
		return true;
	if (error != NULL)
	{
		fprintf (stderr, "Unable to open URL: %s\n", error->message);
		g_error_free (error);
	}
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
