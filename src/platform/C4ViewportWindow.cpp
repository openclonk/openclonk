/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2007-2008  Matthes Bender
 * Copyright (c) 2001-2002, 2005-2008  Sven Eberhardt
 * Copyright (c) 2003-2005, 2007-2008  Peter Wortmann
 * Copyright (c) 2005-2011  Günther Brammer
 * Copyright (c) 2006, 2010  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Martin Plicht
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

/* A viewport to each player */

#include <C4Include.h>
#include <C4ViewportWindow.h>

#include <C4Viewport.h>
#include <C4Console.h>
#include <C4MouseControl.h>
#include <C4GraphicsSystem.h>
#include <C4Landscape.h>
#include <C4PlayerList.h>
#include <StdRegistry.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#ifdef WITH_DEVELOPER_MODE
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#endif
#endif

#ifdef _WIN32

void UpdateWindowLayout(HWND hwnd)
{
	bool fMinimized = !!IsIconic(hwnd);
	bool fMaximized = !!IsZoomed(hwnd);
	RECT rect;
	GetWindowRect(hwnd,&rect);
	MoveWindow(hwnd,rect.left,rect.top,rect.right-rect.left-1,rect.bottom-rect.top,true);
	MoveWindow(hwnd,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,true);
}

bool C4Viewport::TogglePlayerLock()
{
	// Disable player lock
	if (PlayerLock)
	{
		PlayerLock=false;
		SetWindowLong(pWindow->hWindow,GWL_STYLE,C4ViewportWindowStyle | WS_HSCROLL | WS_VSCROLL);
		UpdateWindowLayout(pWindow->hWindow);
		ScrollBarsByViewPosition();
	}
	// Enable player lock
	else if (ValidPlr(Player))
	{
		PlayerLock=true;
		SetWindowLong(pWindow->hWindow,GWL_STYLE,C4ViewportWindowStyle);
		UpdateWindowLayout(pWindow->hWindow);
	}
	return true;
}

bool C4Viewport::ViewPositionByScrollBars()
{
	if (PlayerLock) return false;
	SCROLLINFO scroll;
	scroll.cbSize=sizeof(SCROLLINFO);
	// Vertical
	scroll.fMask=SIF_POS;
	GetScrollInfo(pWindow->hWindow,SB_VERT,&scroll);
	ViewY=float(scroll.nPos);
	// Horizontal
	scroll.fMask=SIF_POS;
	GetScrollInfo(pWindow->hWindow,SB_HORZ,&scroll);
	ViewX=float(scroll.nPos);
	return true;
}

bool C4Viewport::ScrollBarsByViewPosition()
{
	if (PlayerLock) return false;
	SCROLLINFO scroll;
	scroll.cbSize=sizeof(SCROLLINFO);
	// Vertical
	scroll.fMask=SIF_ALL;
	scroll.nMin=0;
	scroll.nMax=GBackHgt;
	scroll.nPage=ViewHgt;
	scroll.nPos=int(ViewY);
	SetScrollInfo(pWindow->hWindow,SB_VERT,&scroll,true);
	// Horizontal
	scroll.fMask=SIF_ALL;
	scroll.nMin=0;
	scroll.nMax=GBackWdt;
	scroll.nPage=ViewWdt;
	scroll.nPos=int(ViewX);
	SetScrollInfo(pWindow->hWindow,SB_HORZ,&scroll,true);
	return true;
}

#elif defined(WITH_DEVELOPER_MODE)
static GtkTargetEntry drag_drop_entries[] =
{
	{ const_cast<gchar*>("text/uri-list"), 0, 0 }
};

// GTK+ Viewport window implementation
GtkWidget* C4ViewportWindow::InitGUI()
{
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

	// Cannot just use ScrolledWindow because this would just move
	// the GdkWindow of the DrawingArea.
	GtkWidget* table;

	drawing_area = gtk_drawing_area_new();
	h_scrollbar = gtk_hscrollbar_new(NULL);
	v_scrollbar = gtk_vscrollbar_new(NULL);
	table = gtk_table_new(2, 2, false);

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(h_scrollbar));

	g_signal_connect(
	  G_OBJECT(adjustment),
	  "value-changed",
	  G_CALLBACK(OnHScrollStatic),
	  this
	);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(v_scrollbar));

	g_signal_connect(
	  G_OBJECT(adjustment),
	  "value-changed",
	  G_CALLBACK(OnVScrollStatic),
	  this
	);

	gtk_table_attach(GTK_TABLE(table), drawing_area, 0, 1, 0, 1, static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_table_attach(GTK_TABLE(table), v_scrollbar, 1, 2, 0, 1, GTK_SHRINK, static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND), 0, 0);
	gtk_table_attach(GTK_TABLE(table), h_scrollbar, 0, 1, 1, 2, static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 0, 0);

	gtk_container_add(GTK_CONTAINER(window), table);

	gtk_widget_add_events(window, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_STRUCTURE_MASK | GDK_POINTER_MOTION_MASK);

	gtk_drag_dest_set(drawing_area, GTK_DEST_DEFAULT_ALL, drag_drop_entries, 1, GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(drawing_area), "drag-data-received", G_CALLBACK(OnDragDataReceivedStatic), this);
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(OnExposeStatic), this);
#else
	g_signal_connect(G_OBJECT(drawing_area), "expose-event", G_CALLBACK(OnExposeStatic), this);
#endif
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(OnKeyPressStatic), this);
	g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(OnKeyReleaseStatic), this);
	g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(OnScrollStatic), this);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(OnButtonPressStatic), this);
	g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(OnButtonReleaseStatic), this);
	g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(OnMotionNotifyStatic), this);
	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(OnConfigureStatic), this);
	g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(OnRealizeStatic), this);

	g_signal_connect_after(G_OBJECT(drawing_area), "configure-event", G_CALLBACK(OnConfigureDareaStatic), this);

	// do not draw the default background
	gtk_widget_set_double_buffered (drawing_area, false);

	return drawing_area;
}

bool C4Viewport::TogglePlayerLock()
{
	if (PlayerLock)
	{
		PlayerLock = false;
		gtk_widget_show(pWindow->h_scrollbar);
		gtk_widget_show(pWindow->v_scrollbar);
		ScrollBarsByViewPosition();
	}
	else
	{
		PlayerLock = true;
		gtk_widget_hide(pWindow->h_scrollbar);
		gtk_widget_hide(pWindow->v_scrollbar);
	}

	return true;
}

bool C4Viewport::ScrollBarsByViewPosition()
{
	if (PlayerLock) return false;
	
	GtkAllocation allocation;
#if GTK_CHECK_VERSION(2,18,0)
	gtk_widget_get_allocation(pWindow->drawing_area, &allocation);
#else
	allocation = pWindow->drawing_area->allocation;
#endif

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));

#if GTK_CHECK_VERSION(2,14,0)
	gtk_adjustment_configure(adjustment,
	                         ViewX, // value
	                         0, // lower
	                         GBackWdt, // upper
	                         ViewportScrollSpeed, // step_increment
	                         allocation.width / Zoom, // page_increment
	                         allocation.width / Zoom // page_size
	                         );
#else
	adjustment->value = ViewX;
	adjustment->lower = 0;
	adjustment->upper = GBackWdt;
	adjustment->step_increment = ViewportScrollSpeed;
	adjustment->page_increment = allocation.width;
	adjustment->page_size = allocation.width;
	gtk_adjustment_changed(adjustment);
#endif

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
#if GTK_CHECK_VERSION(2,14,0)
	gtk_adjustment_configure(adjustment,
	                         ViewY, // value
	                         0, // lower
	                         GBackHgt, // upper
	                         ViewportScrollSpeed, // step_increment
	                         allocation.height / Zoom, // page_increment
	                         allocation.height / Zoom // page_size
	                         );
#else	
	adjustment->lower = 0;
	adjustment->upper = GBackHgt;
	adjustment->step_increment = ViewportScrollSpeed;
	adjustment->page_increment = allocation.height;
	adjustment->page_size = allocation.height;
	adjustment->value = ViewY;
	gtk_adjustment_changed(adjustment);
#endif
	return true;
}

bool C4Viewport::ViewPositionByScrollBars()
{
	if (PlayerLock) return false;

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));
	ViewX = static_cast<int32_t>(gtk_adjustment_get_value(adjustment));

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	ViewY = static_cast<int32_t>(gtk_adjustment_get_value(adjustment));

	return true;
}

void C4ViewportWindow::OnDragDataReceivedStatic(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data)
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

gboolean C4ViewportWindow::OnExposeStatic(GtkWidget* widget, void *, gpointer user_data)
{
	C4Viewport* cvp = static_cast<C4ViewportWindow*>(user_data)->cvp;
	cvp->Execute();
	return true;
}

void C4ViewportWindow::OnRealizeStatic(GtkWidget* widget, gpointer user_data)
{
	// Initial PlayerLock
	if (static_cast<C4ViewportWindow*>(user_data)->cvp->PlayerLock == true)
	{
		gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->h_scrollbar);
		gtk_widget_hide(static_cast<C4ViewportWindow*>(user_data)->v_scrollbar);
	}
}

gboolean C4ViewportWindow::OnKeyPressStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
#if GTK_CHECK_VERSION(2,90,7)
	if (event->keyval == GDK_KEY_Scroll_Lock)
#else
	if (event->keyval == GDK_Scroll_Lock)
#endif
		static_cast<C4ViewportWindow*>(user_data)->cvp->TogglePlayerLock();

	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Game.DoKeyboardInput(key, KEYEV_Down, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	return true;
}

gboolean C4ViewportWindow::OnKeyReleaseStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
	DWORD key = XKeycodeToKeysym(GDK_WINDOW_XDISPLAY(event->window), event->hardware_keycode, 0);
	Game.DoKeyboardInput(key, KEYEV_Up, !!(event->state & GDK_MOD1_MASK), !!(event->state & GDK_CONTROL_MASK), !!(event->state & GDK_SHIFT_MASK), false, NULL);
	return true;
}

gboolean C4ViewportWindow::OnScrollStatic(GtkWidget* widget, GdkEventScroll* event, gpointer user_data)
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
			break;
		}
	}

	return true;
}

gboolean C4ViewportWindow::OnButtonPressStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
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

gboolean C4ViewportWindow::OnButtonReleaseStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data)
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

gboolean C4ViewportWindow::OnMotionNotifyStatic(GtkWidget* widget, GdkEventMotion* event, gpointer user_data)
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

gboolean C4ViewportWindow::OnConfigureStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
	C4Viewport* cvp = window->cvp;

	//cvp->UpdateOutputSize();
	cvp->ScrollBarsByViewPosition();

	return false;
}

gboolean C4ViewportWindow::OnConfigureDareaStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);
	C4Viewport* cvp = window->cvp;

	cvp->UpdateOutputSize();

	return false;
}

void C4ViewportWindow::OnVScrollStatic(GtkAdjustment* adjustment, gpointer user_data)
{
	static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

void C4ViewportWindow::OnHScrollStatic(GtkAdjustment* adjustment, gpointer user_data)
{
	static_cast<C4ViewportWindow*>(user_data)->cvp->ViewPositionByScrollBars();
}

#endif // WITH_DEVELOPER_MODE

void C4ViewportWindow::PerformUpdate()
{
	if (cvp)
	{
		cvp->UpdateOutputSize();
		cvp->Execute();
	}
}

C4Window * C4ViewportWindow::Init(int32_t Player)
{
	C4Window* result;
	const char * Title = Player == NO_OWNER ? LoadResStr("IDS_CNS_VIEWPORT") : ::Players.Get(Player)->GetName();
	result = C4ViewportBase::Init(C4Window::W_Viewport, &Application, Title, &Console, false);
	if (!result) return result;

	pSurface = new C4Surface(&Application, this);
	// Position and size
	RestorePosition(FormatString("Viewport%i", Player+1).getData(), Config.GetSubkeyPath("Console"));
	return result;
}

void C4ViewportWindow::Close()
{
	::Viewports.CloseViewport(cvp);
}
void C4ViewportWindow::EditCursorMove(int X, int Y, uint16_t state)
{
	Console.EditCursor.Move(cvp->ViewX + X / cvp->Zoom, cvp->ViewY + Y / cvp->Zoom, state);
}
