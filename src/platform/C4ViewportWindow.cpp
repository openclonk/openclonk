/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2007-2008  Matthes Bender
 * Copyright (c) 2001-2002, 2005-2008  Sven Eberhardt
 * Copyright (c) 2003-2005, 2007-2008  Peter Wortmann
 * Copyright (c) 2005-2009  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
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
#include <C4UserMessages.h>
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

namespace
{
	const int32_t ViewportScrollSpeed=10;
}

#ifdef _WIN32

#include <shellapi.h>
#include "resource.h"

LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Determine viewport
	C4Viewport *cvp;
	if (!(cvp=::Viewports.GetViewport(hwnd)))
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

	// Process message
	switch (uMsg)
	{
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYDOWN:
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		switch (wParam)
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case VK_SCROLL:
			// key bound to this specific viewport. Don't want to pass this through C4Game...
			cvp->TogglePlayerLock();
			break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		default:
			if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), NULL)) return 0;
			break;
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		}
		break;
		//---------------------------------------------------------------------------------------------------------------------------
	case WM_KEYUP:
		if (Game.DoKeyboardInput(wParam, KEYEV_Up, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), false, NULL)) return 0;
		break;
		//------------------------------------------------------------------------------------------------------------
	case WM_SYSKEYDOWN:
		if (wParam == 18) break;
		if (Game.DoKeyboardInput(wParam, KEYEV_Down, !!(lParam & 0x20000000), Application.IsControlDown(), Application.IsShiftDown(), !!(lParam & 0x40000000), NULL)) return 0;
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_DESTROY:
		StoreWindowPosition(hwnd, FormatString("Viewport%i",cvp->Player+1).getData(), Config.GetSubkeyPath("Console"));
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_CLOSE:
		cvp->pWindow->Close();
		break;

		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_DROPFILES:
		cvp->DropFiles((HANDLE) wParam);
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_USER_DROPDEF:
		Game.DropDef(C4ID(lParam),cvp->ViewX+float(LOWORD(wParam))/cvp->Zoom,cvp->ViewY+float(HIWORD(wParam)/cvp->Zoom));
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_SIZE:
		cvp->UpdateOutputSize();
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_PAINT:
		::GraphicsSystem.Execute();
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION: cvp->ViewX=HIWORD(wParam); break;
		case SB_LINELEFT: cvp->ViewX-=ViewportScrollSpeed; break;
		case SB_LINERIGHT: cvp->ViewX+=ViewportScrollSpeed; break;
		case SB_PAGELEFT: cvp->ViewX-=cvp->ViewWdt; break;
		case SB_PAGERIGHT: cvp->ViewX+=cvp->ViewWdt; break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case SB_THUMBTRACK: case SB_THUMBPOSITION: cvp->ViewY=HIWORD(wParam); break;
		case SB_LINEUP: cvp->ViewY-=ViewportScrollSpeed; break;
		case SB_LINEDOWN: cvp->ViewY+=ViewportScrollSpeed; break;
		case SB_PAGEUP: cvp->ViewY-=cvp->ViewWdt; break;
		case SB_PAGEDOWN: cvp->ViewY+=cvp->ViewWdt; break;
		}
		cvp->Execute();
		cvp->ScrollBarsByViewPosition();
		return 0;
		//----------------------------------------------------------------------------------------------------------------------------------
	case WM_ACTIVATE:
		// Keep editing dialogs on top of the current viewport, but don't make them
		// float on other windows (i.e., no HWND_TOPMOST).
		// Also, don't use SetParent, since that activates the window, which in turn
		// posts a new WM_ACTIVATE to us, and so on, ultimately leading to a hang.
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			if (Console.PropertyDlg.hDialog)
				SetWindowLongPtr(Console.PropertyDlg.hDialog, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(Console.hWindow));
			if (Console.ToolsDlg.hDialog)
				SetWindowLongPtr(Console.PropertyDlg.hDialog, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(Console.hWindow));
		}
		else
		{
			// FALLTHROUGH
		case WM_MOUSEACTIVATE:
			// WM_MOUSEACTIVATE is emitted when the user hovers over a window and pushes a mouse button.
			// Setting the window owner here avoids z-order flickering.
			if (Console.PropertyDlg.hDialog)
				SetWindowLongPtr(Console.PropertyDlg.hDialog, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwnd));
			if (Console.ToolsDlg.hDialog)
				SetWindowLongPtr(Console.ToolsDlg.hDialog, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwnd));
		}
		break;
		//----------------------------------------------------------------------------------------------------------------------------------
	}

	// Viewport mouse control
	if (::MouseControl.IsViewport(cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
	{
		switch (uMsg)
		{
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_LeftDown,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONUP: C4GUI::MouseMove(C4MC_Button_LeftUp,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDOWN: C4GUI::MouseMove(C4MC_Button_RightDown,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONUP: C4GUI::MouseMove(C4MC_Button_RightUp,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_LeftDouble,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDBLCLK: C4GUI::MouseMove(C4MC_Button_RightDouble,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEMOVE:
			if ( Inside<int32_t>(LOWORD(lParam)-cvp->DrawX,0,cvp->ViewWdt-1)
			     && Inside<int32_t>(HIWORD(lParam)-cvp->DrawY,0,cvp->ViewHgt-1) )
				SetCursor(NULL);
			C4GUI::MouseMove(C4MC_Button_None,LOWORD(lParam),HIWORD(lParam),wParam, cvp);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEWHEEL:
			C4GUI::MouseMove(C4MC_Button_Wheel,LOWORD(lParam),HIWORD(lParam),wParam, cvp);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------

		}
	}
	// Console edit cursor control
	else
	{
		switch (uMsg)
		{
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDOWN:
			// movement update needed before, so target is always up-to-date
			Console.EditCursor.Move(cvp->ViewX+cvp->Zoom*LOWORD(lParam),cvp->ViewY+cvp->Zoom*HIWORD(lParam),wParam);
			Console.EditCursor.LeftButtonDown(!!(wParam & MK_CONTROL)); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONUP: Console.EditCursor.LeftButtonUp(); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDOWN: Console.EditCursor.RightButtonDown(!!(wParam & MK_CONTROL)); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONUP: Console.EditCursor.RightButtonUp(); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEMOVE: Console.EditCursor.Move(cvp->ViewX+cvp->Zoom*LOWORD(lParam),cvp->ViewY+cvp->Zoom*HIWORD(lParam),wParam); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool C4ViewportWindow::RegisterViewportClass(HINSTANCE hInst)
{
	static bool fViewportClassRegistered = false;
	if (fViewportClassRegistered) return true;
	// register landscape viewport class
	WNDCLASSEX WndClass;
	WndClass.cbSize=sizeof(WNDCLASSEX);
	WndClass.style         = CS_DBLCLKS | CS_BYTEALIGNCLIENT;
	WndClass.lpfnWndProc   = ViewportWinProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = 0;
	WndClass.hInstance     = hInst;
	WndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = C4ViewportClassName;
	WndClass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE (IDI_01_C4S) );
	WndClass.hIconSm       = LoadIcon (hInst, MAKEINTRESOURCE (IDI_01_C4S) );
	if (!RegisterClassEx(&WndClass)) return false;
	// register GUI dialog class
	return fViewportClassRegistered = C4GUI::Dialog::RegisterWindowClass(hInst);
}

CStdWindow * C4ViewportWindow::Init(CStdApp * pApp, const char * Title, CStdWindow * pParent, bool)
{
	Active = true;
	// Create window
	hWindow = CreateWindowEx (
	            WS_EX_ACCEPTFILES,
	            C4ViewportClassName, Title, C4ViewportWindowStyle,
	            CW_USEDEFAULT,CW_USEDEFAULT,400,250,
	            pParent->hWindow,NULL,pApp->GetInstance(),NULL);
	return hWindow ? this : 0;
}

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
	adjustment->lower = 0;
	adjustment->upper = GBackWdt;
	adjustment->step_increment = ViewportScrollSpeed;

	g_signal_connect(
	  G_OBJECT(adjustment),
	  "value-changed",
	  G_CALLBACK(OnHScrollStatic),
	  this
	);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(v_scrollbar));
	adjustment->lower = 0;
	adjustment->upper = GBackHgt;
	adjustment->step_increment = ViewportScrollSpeed;

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
	g_signal_connect(G_OBJECT(drawing_area), "expose-event", G_CALLBACK(OnExposeStatic), this);

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
	adjustment->page_increment = allocation.width;
	adjustment->page_size = allocation.width;
	adjustment->value = ViewX;
	gtk_adjustment_changed(adjustment);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	adjustment->page_increment = allocation.height;
	adjustment->page_size = allocation.height;
	adjustment->value = ViewY;
	gtk_adjustment_changed(adjustment);

	return true;
}

bool C4Viewport::ViewPositionByScrollBars()
{
	if (PlayerLock) return false;

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));
	ViewX = static_cast<int32_t>(adjustment->value);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	ViewY = static_cast<int32_t>(adjustment->value);

	return true;
}

void C4ViewportWindow::OnDragDataReceivedStatic(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data)
{
	C4ViewportWindow* window = static_cast<C4ViewportWindow*>(user_data);

	gchar** uris = gtk_selection_data_get_uris(data);
	if (!uris) return;

	for (gchar** uri = uris; *uri != NULL; ++ uri)
	{
		gchar* file = g_filename_from_uri(*uri, NULL, NULL);
		if (!file) continue;

		Game.DropFile(file, window->cvp->ViewX+x, window->cvp->ViewY+y);
		g_free(file);
	}

	g_strfreev(uris);
}

gboolean C4ViewportWindow::OnExposeStatic(GtkWidget* widget, GdkEventExpose* event, gpointer user_data)
{
	C4Viewport* cvp = static_cast<C4ViewportWindow*>(user_data)->cvp;

	// TODO: Redraw only event->area
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
		Console.EditCursor.Move(window->cvp->ViewX + event->x/window->cvp->Zoom, window->cvp->ViewY + event->y/window->cvp->Zoom, event->state);
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

#else // WITH_DEVELOPER_MODE
bool C4Viewport::TogglePlayerLock() { return false; }
bool C4Viewport::ScrollBarsByViewPosition() { return false; }
#if defined(USE_X11)
void C4ViewportWindow::HandleMessage (XEvent & e)
{
	switch (e.type)
	{
	case KeyPress:
	{
		// Do not take into account the state of the various modifiers and locks
		// we don't need that for keyboard control
		DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
		//case VK_SCROLL: cvp->TogglePlayerLock();
		Game.DoKeyboardInput(key, KEYEV_Down, Application.IsAltDown(), Application.IsControlDown(), Application.IsShiftDown(), false, NULL);
		break;
	}
	case KeyRelease:
	{
		DWORD key = XKeycodeToKeysym(e.xany.display, e.xkey.keycode, 0);
		Game.DoKeyboardInput(key, KEYEV_Up, e.xkey.state & Mod1Mask, e.xkey.state & ControlMask, e.xkey.state & ShiftMask, false, NULL);
		break;
	}
	case ButtonPress:
	{
		static int last_left_click, last_right_click;
		if (::MouseControl.IsViewport(cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
		{
			switch (e.xbutton.button)
			{
			case Button1:
				if (timeGetTime() - last_left_click < 400)
				{
					C4GUI::MouseMove(C4MC_Button_LeftDouble,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_left_click = 0;
				}
				else
				{
					C4GUI::MouseMove(C4MC_Button_LeftDown,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_left_click = timeGetTime();
				}
				break;
			case Button2:
				C4GUI::MouseMove(C4MC_Button_MiddleDown,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button3:
				if (timeGetTime() - last_right_click < 400)
				{
					C4GUI::MouseMove(C4MC_Button_RightDouble,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_right_click = 0;
				}
				else
				{
					C4GUI::MouseMove(C4MC_Button_RightDown,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_right_click = timeGetTime();
				}
				break;
			case Button4:
				C4GUI::MouseMove(C4MC_Button_Wheel,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(1) << 16), cvp);
				break;
			case Button5:
				C4GUI::MouseMove(C4MC_Button_Wheel,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(-1) << 16), cvp);
				break;
			default:
				break;
			}
		}
		else
		{
			switch (e.xbutton.button)
			{
			case Button1:
				Console.EditCursor.LeftButtonDown(e.xbutton.state & MK_CONTROL);
				break;
			case Button3:
				Console.EditCursor.RightButtonDown(e.xbutton.state & MK_CONTROL);
				break;
			}
		}
	}
	break;
	case ButtonRelease:
		if (::MouseControl.IsViewport(cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
		{
			switch (e.xbutton.button)
			{
			case Button1:
				C4GUI::MouseMove(C4MC_Button_LeftUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button2:
				C4GUI::MouseMove(C4MC_Button_MiddleUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button3:
				C4GUI::MouseMove(C4MC_Button_RightUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			default:
				break;
			}
		}
		else
		{
			switch (e.xbutton.button)
			{
			case Button1:
				Console.EditCursor.LeftButtonUp();
				break;
			case Button3:
				Console.EditCursor.RightButtonUp();
				break;
			}
		}
		break;
	case MotionNotify:
		if (::MouseControl.IsViewport(cvp) && (Console.EditCursor.GetMode()==C4CNS_ModePlay))
		{
			C4GUI::MouseMove(C4MC_Button_None, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
		}
		else
		{
			Console.EditCursor.Move(cvp->ViewX + e.xbutton.x, cvp->ViewY + e.xbutton.y, e.xbutton.state);
		}
		break;
	case ConfigureNotify:
		cvp->UpdateOutputSize();
		break;
	}
}
#endif // USE_X11
#endif // WITH_DEVELOPER_MODE/_WIN32

void C4ViewportWindow::Close()
{
	::Viewports.CloseViewport(cvp);
}
