/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, 2011  Günther Brammer
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2006-2008, 2010  Armin Burgmeier
 * Copyright (c) 2010  Martin Plicht
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
#if GTK_CHECK_VERSION(2,21,8)
	if (event->keyval == GDK_KEY_Scroll_Lock)
#else
	if (event->keyval == GDK_Scroll_Lock)
#endif
	{
		static_cast<C4ViewportWindow*>(user_data)->cvp->TogglePlayerLock();
		return true;
	}

	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Console.EditCursor.KeyDown(key, event->state);
	return false;
}

static gboolean OnKeyReleaseStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Console.EditCursor.KeyUp(key, event->state);
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
#if GTK_CHECK_VERSION(3,4,0)
	gdouble dx, dy;
	if (gdk_event_get_scroll_deltas((GdkEvent*)event, &dx, &dy))
	{
		idy = short(round(dy));
	}
	else
#endif
	{
		if (event->direction == GDK_SCROLL_UP)
			idy = 32;
		if (event->direction == GDK_SCROLL_DOWN)
			idy = -32;
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

bool C4Window::FindInfo(int samples, void** info)
{
#ifdef USE_GL
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
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

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
{
#ifdef USE_GL
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
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
#if GTK_CHECK_VERSION(3,0,0)
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
#endif
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
#if GTK_CHECK_VERSION(3,0,0)
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
#endif
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
#if GTK_CHECK_VERSION(3,0,0)
		gtk_window_set_has_resize_grip(GTK_WINDOW(window), false);
#endif
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
#if GTK_CHECK_VERSION(3,4,0)
	gtk_widget_add_events(GTK_WIDGET(window), GDK_SMOOTH_SCROLL_MASK);
#endif

	GdkScreen * scr = gtk_widget_get_screen(GTK_WIDGET(render_widget));
	GdkVisual * vis = gdk_x11_screen_lookup_visual(scr, ((XVisualInfo*)Info)->visualid);
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);
#else
	GdkColormap * cmap = gdk_colormap_new(vis, true);
	gtk_widget_set_colormap(GTK_WIDGET(render_widget), cmap);
	g_object_unref(cmap);
#endif
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
#ifdef USE_GL
	int value;
	Display * const dpy = gdk_x11_display_get_xdisplay(gdk_display_get_default());
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

	// Un- and re-realizing the render_widget does not work, the window
	// remains hidden afterwards. So we re-create it from scratch.
	gtk_widget_destroy(GTK_WIDGET(render_widget));
	render_widget = gtk_drawing_area_new();
	gtk_widget_set_double_buffered (GTK_WIDGET(render_widget), false);
	g_object_set(G_OBJECT(render_widget), "can-focus", TRUE, NULL);
	
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_visual(GTK_WIDGET(render_widget),vis);
#else
	GdkColormap * cmap = gdk_colormap_new(vis, true);
	gtk_widget_set_colormap(GTK_WIDGET(render_widget), cmap);
	g_object_unref(cmap);
#endif

	delete static_cast<XVisualInfo*>(Info);
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

	// We must free it here since we do not call C4Window::Clear()
	if (Info)
	{
		delete static_cast<XVisualInfo*>(Info);
		Info = 0;
	}
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
