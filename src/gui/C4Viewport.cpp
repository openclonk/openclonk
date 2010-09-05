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
#include <C4Viewport.h>

#include <C4Console.h>
#include <C4UserMessages.h>
#include <C4Object.h>
#include <C4ObjectInfo.h>
#include <C4FullScreen.h>
#include <C4Application.h>
#include <C4ObjectCom.h>
#include <C4Stat.h>
#include <C4Gui.h>
#include <C4Network2Dialogs.h>
#include <C4GameDialogs.h>
#include <C4Player.h>
#include <C4ChatDlg.h>
#include <C4ObjectMenu.h>
#include <C4MouseControl.h>
#include <C4PXS.h>
#include <C4GameMessage.h>
#include <C4GraphicsResource.h>
#include <C4GraphicsSystem.h>
#include <C4Landscape.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4Network2.h>
#include <C4GamePadCon.h>

#include <StdGL.h>
#include <StdRegistry.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#ifdef WITH_DEVELOPER_MODE
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtktable.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkselection.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkhscrollbar.h>
#include <gtk/gtkvscrollbar.h>
#endif
#endif

namespace
{
	const int32_t ViewportScrollSpeed=10;
}

#ifdef _WIN32

#include <shellapi.h>

LRESULT APIENTRY ViewportWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Determine viewport
	C4Viewport *cvp;
	if (!(cvp=::GraphicsSystem.GetViewport(hwnd)))
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
		case WM_LBUTTONDOWN: ::GraphicsSystem.MouseMove(C4MC_Button_LeftDown,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONUP: ::GraphicsSystem.MouseMove(C4MC_Button_LeftUp,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDOWN: ::GraphicsSystem.MouseMove(C4MC_Button_RightDown,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONUP: ::GraphicsSystem.MouseMove(C4MC_Button_RightUp,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_LBUTTONDBLCLK: ::GraphicsSystem.MouseMove(C4MC_Button_LeftDouble,LOWORD(lParam),HIWORD(lParam),wParam, cvp);  break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_RBUTTONDBLCLK: ::GraphicsSystem.MouseMove(C4MC_Button_RightDouble,LOWORD(lParam),HIWORD(lParam),wParam, cvp); break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEMOVE:
			if ( Inside<int32_t>(LOWORD(lParam)-cvp->DrawX,0,cvp->ViewWdt-1)
			     && Inside<int32_t>(HIWORD(lParam)-cvp->DrawY,0,cvp->ViewHgt-1) )
				SetCursor(NULL);
			::GraphicsSystem.MouseMove(C4MC_Button_None,LOWORD(lParam),HIWORD(lParam),wParam, cvp);
			break;
			//----------------------------------------------------------------------------------------------------------------------------------
		case WM_MOUSEWHEEL:
			::GraphicsSystem.MouseMove(C4MC_Button_Wheel,LOWORD(lParam),HIWORD(lParam),wParam, cvp);
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

bool C4Viewport::DropFiles(HANDLE hDrop)
{
	if (!Console.Editing) { Console.Message(LoadResStr("IDS_CNS_NONETEDIT")); return false; }

	int32_t iFileNum = DragQueryFile((HDROP)hDrop,0xFFFFFFFF,NULL,0);
	POINT pntPoint;
	char szFilename[500+1];
	for (int32_t cnt=0; cnt<iFileNum; cnt++)
	{
		DragQueryFile((HDROP)hDrop,cnt,szFilename,500);
		DragQueryPoint((HDROP)hDrop,&pntPoint);
		Game.DropFile(szFilename,ViewX+float(pntPoint.x)/Zoom,ViewY+float(pntPoint.y)/Zoom);
	}
	DragFinish((HDROP)hDrop);
	return true;
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

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));
	adjustment->page_increment = pWindow->drawing_area->allocation.width;
	adjustment->page_size = pWindow->drawing_area->allocation.width;
	adjustment->value = ViewX;
	gtk_adjustment_changed(adjustment);

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	adjustment->page_increment = pWindow->drawing_area->allocation.height;
	adjustment->page_size = pWindow->drawing_area->allocation.height;
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
	if (event->keyval == GDK_Scroll_Lock)
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
			::GraphicsSystem.MouseMove(C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y, event->state + (short(1) << 16), window->cvp);
			break;
		case GDK_SCROLL_DOWN:
			::GraphicsSystem.MouseMove(C4MC_Button_Wheel, (int32_t)event->x, (int32_t)event->y, event->state + (short(-1) << 16), window->cvp);
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
				::GraphicsSystem.MouseMove(C4MC_Button_LeftDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			else if (event->type == GDK_2BUTTON_PRESS)
				::GraphicsSystem.MouseMove(C4MC_Button_LeftDouble, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 2:
			::GraphicsSystem.MouseMove(C4MC_Button_MiddleDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 3:
			if (event->type == GDK_BUTTON_PRESS)
				::GraphicsSystem.MouseMove(C4MC_Button_RightDown, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			else if (event->type == GDK_2BUTTON_PRESS)
				::GraphicsSystem.MouseMove(C4MC_Button_RightDouble, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
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
			::GraphicsSystem.MouseMove(C4MC_Button_LeftUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 2:
			::GraphicsSystem.MouseMove(C4MC_Button_MiddleUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
			break;
		case 3:
			::GraphicsSystem.MouseMove(C4MC_Button_RightUp, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
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
		::GraphicsSystem.MouseMove(C4MC_Button_None, (int32_t)event->x, (int32_t)event->y, event->state, window->cvp);
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
					::GraphicsSystem.MouseMove(C4MC_Button_LeftDouble,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_left_click = 0;
				}
				else
				{
					::GraphicsSystem.MouseMove(C4MC_Button_LeftDown,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_left_click = timeGetTime();
				}
				break;
			case Button2:
				::GraphicsSystem.MouseMove(C4MC_Button_MiddleDown,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button3:
				if (timeGetTime() - last_right_click < 400)
				{
					::GraphicsSystem.MouseMove(C4MC_Button_RightDouble,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_right_click = 0;
				}
				else
				{
					::GraphicsSystem.MouseMove(C4MC_Button_RightDown,
					                           e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
					last_right_click = timeGetTime();
				}
				break;
			case Button4:
				::GraphicsSystem.MouseMove(C4MC_Button_Wheel,
				                           e.xbutton.x, e.xbutton.y, e.xbutton.state + (short(1) << 16), cvp);
				break;
			case Button5:
				::GraphicsSystem.MouseMove(C4MC_Button_Wheel,
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
				::GraphicsSystem.MouseMove(C4MC_Button_LeftUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button2:
				::GraphicsSystem.MouseMove(C4MC_Button_MiddleUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
				break;
			case Button3:
				::GraphicsSystem.MouseMove(C4MC_Button_RightUp, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
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
			::GraphicsSystem.MouseMove(C4MC_Button_None, e.xbutton.x, e.xbutton.y, e.xbutton.state, cvp);
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
	::GraphicsSystem.CloseViewport(cvp);
}

bool C4Viewport::UpdateOutputSize()
{
	if (!pWindow) return false;
	// Output size
	RECT rect;
#ifdef WITH_DEVELOPER_MODE
	// Use only size of drawing area without scrollbars
	rect.left = pWindow->drawing_area->allocation.x;
	rect.top = pWindow->drawing_area->allocation.y;
	rect.right = rect.left + pWindow->drawing_area->allocation.width;
	rect.bottom = rect.top + pWindow->drawing_area->allocation.height;
#else
	if (!pWindow->GetSize(&rect)) return false;
#endif
	OutX=rect.left; OutY=rect.top;
	ViewWdt=rect.right-rect.left; ViewHgt=rect.bottom-rect.top;
	// Scroll bars
	ScrollBarsByViewPosition();
	// Reset menus
	ResetMenuPositions=true;
	// update internal GL size
	if (pWindow && pWindow->pSurface)
		pWindow->pSurface->UpdateSize(ViewWdt, ViewHgt);
	// Done
	return true;
}

C4Viewport::C4Viewport()
{
	Default();
}

C4Viewport::~C4Viewport()
{
	Clear();
}

void C4Viewport::Clear()
{
	if (pWindow) { delete pWindow->pSurface; pWindow->Clear(); delete pWindow; pWindow = NULL; }
	Player=NO_OWNER;
	ViewX=ViewY=0;
	ViewWdt=ViewHgt=0;
	OutX=OutY=ViewWdt=ViewHgt=0;
	DrawX=DrawY=0;
	Regions.Clear();
	ViewOffsX = ViewOffsY = 0;
}

void C4Viewport::DrawOverlay(C4TargetFacet &cgo, const ZoomData &GameZoom)
{
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		C4ST_STARTNEW(FoWStat, "C4Viewport::DrawOverlay: FoW")
		// Player fog of war
		DrawPlayerFogOfWar(cgo);
		C4ST_STOP(FoWStat)
		// Player info
		C4ST_STARTNEW(PInfoStat, "C4Viewport::DrawOverlay: Player Info")
		DrawPlayerInfo(cgo);
		C4ST_STOP(PInfoStat)
		C4ST_STARTNEW(MenuStat, "C4Viewport::DrawOverlay: Menu")
		DrawMenu(cgo);
		C4ST_STOP(MenuStat)
	}
	// Game messages
	C4ST_STARTNEW(MsgStat, "C4Viewport::DrawOverlay: Messages")
	::Messages.Draw(cgo, Player, Zoom);
	C4ST_STOP(MsgStat)

	// Control overlays (if not film/replay)
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay)
	{
		// Mouse control
		if (::MouseControl.IsViewport(this))
		{
			C4ST_STARTNEW(MouseStat, "C4Viewport::DrawOverlay: Mouse")
			::MouseControl.Draw(cgo, GameZoom);
			// Draw GUI-mouse in EM if active
			if (pWindow && ::pGUI) ::pGUI->RenderMouse(cgo);
			C4ST_STOP(MouseStat)
		}
	}
}

void C4Viewport::DrawMenu(C4TargetFacet &cgo)
{

	// Get player
	C4Player *pPlr = ::Players.Get(Player);

	// Player eliminated
	if (pPlr && pPlr->Eliminated)
	{
		Application.DDraw->TextOut(FormatString(LoadResStr(pPlr->Surrendered ? "IDS_PLR_SURRENDERED" :  "IDS_PLR_ELIMINATED"),pPlr->GetName()).getData(),
		                           ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.X+cgo.Wdt/2,cgo.Y+2*cgo.Hgt/3,0xfaFF0000,ACenter);
		return;
	}

	// for menus, cgo is using GUI-syntax: TargetX/Y marks the drawing offset; x/y/Wdt/Hgt marks the offset rect
	float iOldTx = cgo.TargetX; float iOldTy = cgo.TargetY;
	cgo.TargetX = float(cgo.X); cgo.TargetY = float(cgo.Y);
	cgo.X = 0; cgo.Y = 0;

	// Player cursor object menu
	if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu)
	{
		if (ResetMenuPositions) pPlr->Cursor->Menu->ResetLocation();
		// if mouse is dragging, make it transparent to easy construction site drag+drop
		bool fDragging=false;
		if (::MouseControl.IsDragging() && ::MouseControl.IsViewport(this))
		{
			fDragging = true;
			lpDDraw->ActivateBlitModulation(0x4fffffff);
		}
		// draw menu
		pPlr->Cursor->Menu->Draw(cgo);
		// reset modulation for dragging
		if (fDragging) lpDDraw->DeactivateBlitModulation();
	}
	// Player menu
	if (pPlr && pPlr->Menu.IsActive())
	{
		if (ResetMenuPositions) pPlr->Menu.ResetLocation();
		pPlr->Menu.Draw(cgo);
	}
	// Fullscreen menu
	if (FullScreen.pMenu && FullScreen.pMenu->IsActive())
	{
		if (ResetMenuPositions) FullScreen.pMenu->ResetLocation();
		FullScreen.pMenu->Draw(cgo);
	}

	// Flag done
	ResetMenuPositions=false;

	// restore cgo
	cgo.X = int32_t(cgo.TargetX); cgo.Y = int32_t(cgo.TargetY);
	cgo.TargetX = iOldTx; cgo.TargetY = iOldTy;
}

extern int32_t iLastControlSize,iPacketDelay,ScreenRate;
extern int32_t ControlQueueSize,ControlQueueDataSize;

void C4Viewport::Draw(C4TargetFacet &cgo0, bool fDrawOverlay)
{

#ifdef USE_CONSOLE
	// No drawing in console mode
	return;
#endif
	C4TargetFacet cgo; cgo.Set(cgo0);
	ZoomData GameZoom;
	GameZoom.X = cgo.X; GameZoom.Y = cgo.Y;
	GameZoom.Zoom = Zoom;

	if (fDrawOverlay)
	{
		// Draw landscape borders. Only if overlay, so complete map screenshots don't get messed up
		if (BorderLeft)  Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX,DrawY,BorderLeft,ViewHgt,-DrawX,-DrawY);
		if (BorderTop)   Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+BorderLeft,DrawY,ViewWdt-BorderLeft-BorderRight,BorderTop,-DrawX-BorderLeft,-DrawY);
		if (BorderRight) Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+ViewWdt-BorderRight,DrawY,BorderRight,ViewHgt,-DrawX-ViewWdt+BorderRight,-DrawY);
		if (BorderBottom)Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,DrawX+BorderLeft,DrawY+ViewHgt-BorderBottom,ViewWdt-BorderLeft-BorderRight,BorderBottom,-DrawX-BorderLeft,-DrawY-ViewHgt+BorderBottom);

		// Set clippers
		cgo.X += BorderLeft; cgo.Y += BorderTop; cgo.Wdt -= int(float(BorderLeft+BorderRight)/Zoom); cgo.Hgt -= int(float(BorderTop+BorderBottom)/Zoom);
		GameZoom.X = cgo.X; GameZoom.Y = cgo.Y;
		cgo.TargetX += BorderLeft/Zoom; cgo.TargetY += BorderTop/Zoom;
		// Apply Zoom
		lpDDraw->SetZoom(GameZoom);
		Application.DDraw->SetPrimaryClipper(cgo.X,cgo.Y,DrawX+ViewWdt-1-BorderRight,DrawY+ViewHgt-1-BorderBottom);
	}
	last_game_draw_cgo = cgo;

	// landscape mod by FoW
	C4Player *pPlr=::Players.Get(Player);
	if (pPlr && pPlr->fFogOfWar)
	{
		ClrModMap.Reset(Game.C4S.Landscape.FoWRes, Game.C4S.Landscape.FoWRes, ViewWdt, ViewHgt, int(cgo.TargetX*Zoom), int(cgo.TargetY*Zoom), 0, cgo.X-BorderLeft, cgo.Y-BorderTop, Game.FoWColor, cgo.Surface);
		pPlr->FoW2Map(ClrModMap, int(float(cgo.X)/Zoom-cgo.TargetX), int(float(cgo.Y)/Zoom-cgo.TargetY));
		lpDDraw->SetClrModMap(&ClrModMap);
		lpDDraw->SetClrModMapEnabled(true);
	}
	else
		lpDDraw->SetClrModMapEnabled(false);

	C4ST_STARTNEW(SkyStat, "C4Viewport::Draw: Sky")
	::Landscape.Sky.Draw(cgo);
	C4ST_STOP(SkyStat)
	::Objects.BackObjects.DrawAll(cgo, Player);

	// Draw Landscape
	C4ST_STARTNEW(LandStat, "C4Viewport::Draw: Landscape")
	::Landscape.Draw(cgo,Player);
	C4ST_STOP(LandStat)

	// draw PXS (unclipped!)
	C4ST_STARTNEW(PXSStat, "C4Viewport::Draw: PXS")
	::PXS.Draw(cgo);
	C4ST_STOP(PXSStat)

	// draw objects
	C4ST_STARTNEW(ObjStat, "C4Viewport::Draw: Objects")
	::Objects.Draw(cgo, Player);
	C4ST_STOP(ObjStat)

	// draw global particles
	C4ST_STARTNEW(PartStat, "C4Viewport::Draw: Particles")
	::Particles.GlobalParticles.Draw(cgo,NULL);
	C4ST_STOP(PartStat)

	// Draw PathFinder
	if (::GraphicsSystem.ShowPathfinder) Game.PathFinder.Draw(cgo);

	// Draw overlay
	if (!Game.C4S.Head.Film || !Game.C4S.Head.Replay) Game.DrawCursors(cgo, Player);

	// FogOfWar-mod off
	lpDDraw->SetClrModMapEnabled(false);

	if (fDrawOverlay)
	{
		// Determine zoom of overlay
		float fGUIZoom = C4GUI::GetZoom();
		// now restore complete cgo range for overlay drawing
		lpDDraw->SetZoom(DrawX,DrawY, fGUIZoom);
		Application.DDraw->SetPrimaryClipper(DrawX,DrawY,DrawX+(ViewWdt-1),DrawY+(ViewHgt-1));
		cgo.Set(cgo0);

		cgo.X = DrawX; cgo.Y = DrawY;
		cgo.Wdt = int(float(ViewWdt)/fGUIZoom); cgo.Hgt = int(float(ViewHgt)/fGUIZoom);
		cgo.TargetX = ViewX; cgo.TargetY = ViewY;

		last_gui_draw_cgo = cgo;

		// draw custom GUI objects
		::Objects.ForeObjects.DrawIfCategory(cgo, Player, C4D_Foreground, false);

		// Draw overlay
		C4ST_STARTNEW(OvrStat, "C4Viewport::Draw: Overlay")

		if (!Application.isFullScreen) Console.EditCursor.Draw(cgo, Zoom);

		DrawOverlay(cgo, GameZoom);

		// Netstats
		if (::GraphicsSystem.ShowNetstatus)
			::Network.DrawStatus(cgo);

		C4ST_STOP(OvrStat)

		// Remove zoom n clippers
		lpDDraw->SetZoom(0, 0, 1.0);
		Application.DDraw->NoPrimaryClipper();
	}

}

void C4Viewport::BlitOutput()
{
	if (pWindow)
	{
		RECT rtSrc,rtDst;
		rtSrc.left=DrawX; rtSrc.top=DrawY;  rtSrc.right=DrawX+ViewWdt; rtSrc.bottom=DrawY+ViewHgt;
		rtDst.left=OutX;  rtDst.top=OutY;   rtDst.right=OutX+ ViewWdt; rtDst.bottom=OutY+ ViewHgt;
		pWindow->pSurface->PageFlip(&rtSrc, &rtDst);
	}
}

void C4Viewport::Execute()
{
	// Update regions
	static int32_t RegionUpdate=0;
	SetRegions=NULL;
	RegionUpdate++;
	if (RegionUpdate>=5)
	{
		RegionUpdate=0;
		Regions.Clear();
		Regions.SetAdjust(-OutX,-OutY);
		SetRegions=&Regions;
	}
	// Adjust position
	AdjustPosition();
	// Current graphics output
	C4TargetFacet cgo;
	CStdWindow * w = pWindow;
	if (!w) w = &FullScreen;
	cgo.Set(w->pSurface,DrawX,DrawY,int32_t(ceilf(float(ViewWdt)/Zoom)),int32_t(ceilf(float(ViewHgt)/Zoom)),ViewX,ViewY);
	lpDDraw->PrepareRendering(w->pSurface);
	// Draw
	Draw(cgo, true);
	// Video record & status (developer mode, first player viewport)
	if (!Application.isFullScreen)
		if (Player==0 && (this==::GraphicsSystem.GetViewport((int32_t) 0)))
			::GraphicsSystem.Video.Execute();
	// Blit output
	BlitOutput();
}

void C4Viewport::ChangeZoom(float by_factor)
{
	ZoomTarget *= by_factor;
}

void C4Viewport::AdjustPosition()
{
	float ViewportScrollBorder = fIsNoOwnerViewport ? 0 : float(C4ViewportScrollBorder);
	C4Player *pPlr = ::Players.Get(Player);
	if (ZoomTarget < 0.000001f)
	{
		ZoomTarget = Max(float(ViewWdt)/GBackWdt, 1.0f);
		if (pPlr) ZoomTarget = Max(ViewWdt / (2.0f * C4FOW_Def_View_RangeX), ZoomTarget);
		Zoom = ZoomTarget; 
	}
	// View position
	if (PlayerLock && ValidPlr(Player))
	{
		float PrefViewX = ViewX + ViewWdt / (Zoom * 2) - ViewOffsX;
		float PrefViewY = ViewY + ViewHgt / (Zoom * 2) - ViewOffsY;
		// Change Zoom
		assert(Zoom>0);
		assert(ZoomTarget>0);

		if(Zoom != ZoomTarget)
		{
			float DeltaZoom = Zoom/ZoomTarget;
			if(DeltaZoom<1) DeltaZoom = 1/DeltaZoom;

			// Minimal Zoom change factor
			static const float Z0 = pow(C4GFX_ZoomStep, 1.0f/8.0f);

			// We change zoom based on (logarithmic) distance of current zoom
			// to target zoom. The greater the distance the more we adjust the
			// zoom in one frame. There is a minimal zoom change Z0 to make sure
			// we reach ZoomTarget in finite time.
			float ZoomAdjustFactor = Z0 * pow(DeltaZoom, 1.0f/8.0f);

			if (Zoom < ZoomTarget)
				Zoom = Min(Zoom * ZoomAdjustFactor, ZoomTarget);
			if (Zoom > ZoomTarget)
				Zoom = Max(Zoom / ZoomAdjustFactor, ZoomTarget);
		}

		float ScrollRange = Min(ViewWdt/(10*Zoom),ViewHgt/(10*Zoom));
		float ExtraBoundsX = 0, ExtraBoundsY = 0;
		if (pPlr->ViewMode == C4PVM_Scrolling)
		{
			ScrollRange=0;
			ExtraBoundsX=ExtraBoundsY=ViewportScrollBorder;
		}
		else
		{
			// if view is close to border, allow scrolling
			if (pPlr->ViewX < ViewportScrollBorder) ExtraBoundsX = Min<float>(ViewportScrollBorder - pPlr->ViewX, ViewportScrollBorder);
			else if (pPlr->ViewX >= GBackWdt - ViewportScrollBorder) ExtraBoundsX = Min<float>(float(pPlr->ViewX - GBackWdt), 0) + ViewportScrollBorder;
			if (pPlr->ViewY < ViewportScrollBorder) ExtraBoundsY = Min<float>(ViewportScrollBorder - pPlr->ViewY, ViewportScrollBorder);
			else if (pPlr->ViewY >= GBackHgt - ViewportScrollBorder) ExtraBoundsY = Min<float>(float(pPlr->ViewY - GBackHgt), 0) + ViewportScrollBorder;
		}
		ExtraBoundsX = Max(ExtraBoundsX, (ViewWdt/Zoom - GBackWdt) / 2+1);
		ExtraBoundsY = Max(ExtraBoundsY, (ViewHgt/Zoom - GBackHgt) / 2+1);
		// calc target view position
		float TargetViewX = pPlr->ViewX/* */;
		float TargetViewY = pPlr->ViewY/* */;
		// add mouse auto scroll
		if (pPlr->MouseControl && ::MouseControl.InitCentered && Config.Controls.MouseAScroll)
		{
			TargetViewX += (::MouseControl.VpX - ViewWdt / 2) / Zoom;
			TargetViewY += (::MouseControl.VpY - ViewHgt / 2) / Zoom;
		}
		// scroll range
		TargetViewX = BoundBy(PrefViewX, TargetViewX - ScrollRange, TargetViewX + ScrollRange);
		TargetViewY = BoundBy(PrefViewY, TargetViewY - ScrollRange, TargetViewY + ScrollRange);
		// bounds
		TargetViewX = BoundBy(TargetViewX, ViewWdt / (Zoom * 2) - ExtraBoundsX, GBackWdt - ViewWdt / (Zoom * 2) + ExtraBoundsX);
		TargetViewY = BoundBy(TargetViewY, ViewHgt / (Zoom * 2) - ExtraBoundsY, GBackHgt - ViewHgt / (Zoom * 2) + ExtraBoundsY);
		// smooth
		ViewX = PrefViewX + (TargetViewX - PrefViewX) / BoundBy<int32_t>(Config.General.ScrollSmooth, 1, 50);
		ViewY = PrefViewY + (TargetViewY - PrefViewY) / BoundBy<int32_t>(Config.General.ScrollSmooth, 1, 50);
		// apply offset
		ViewX -= ViewWdt / (Zoom * 2) - ViewOffsX;
		ViewY -= ViewHgt / (Zoom * 2) - ViewOffsY;
	}
	// NO_OWNER can't scroll
	if (fIsNoOwnerViewport) { ViewOffsX=0; ViewOffsY=0; }
	// clip at borders, update vars
	UpdateViewPosition();
#ifdef WITH_DEVELOPER_MODE
	//ScrollBarsByViewPosition();
#endif
}

void C4Viewport::CenterPosition()
{
	// center viewport position on map
	// set center position
	ViewX = (GBackWdt-ViewWdt/Zoom)/2;
	ViewY = (GBackHgt-ViewHgt/Zoom)/2;
	// clips and updates
	UpdateViewPosition();
}

void C4Viewport::UpdateViewPosition()
{
	// no-owner viewports should not scroll outside viewing area
	if (fIsNoOwnerViewport)
	{
		if (Application.isFullScreen && GBackWdt<ViewWdt / Zoom)
		{
			ViewX = (GBackWdt-ViewWdt / Zoom)/2;
		}
		else
		{
			ViewX = Min(ViewX, GBackWdt-ViewWdt / Zoom);
			ViewX = Max(ViewX, 0.0f);
		}
		if (Application.isFullScreen && GBackHgt<ViewHgt / Zoom)
		{
			ViewY = (GBackHgt-ViewHgt / Zoom)/2;
		}
		else
		{
			ViewY = Min(ViewY, GBackHgt-ViewHgt / Zoom);
			ViewY = Max(ViewY, 0.0f);
		}
	}
	// update borders
	BorderLeft = int32_t(Max(-ViewX * Zoom, 0.0f));
	BorderTop = int32_t(Max(-ViewY * Zoom, 0.0f));
	BorderRight = int32_t(Max(ViewWdt - GBackWdt * Zoom + ViewX * Zoom, 0.0f));
	BorderBottom = int32_t(Max(ViewHgt - GBackHgt * Zoom + ViewY * Zoom, 0.0f));
}

void C4Viewport::Default()
{
	pWindow=NULL;
	Player=0;
	ViewX=ViewY=0;
	ViewWdt=ViewHgt=0;
	BorderLeft=BorderTop=BorderRight=BorderBottom=0;
	OutX=OutY=ViewWdt=ViewHgt=0;
	DrawX=DrawY=0;
	Zoom = 1.0;
	ZoomTarget = 0.0;
	Next=NULL;
	PlayerLock=true;
	ResetMenuPositions=false;
	SetRegions=NULL;
	Regions.Default();
	ViewOffsX = ViewOffsY = 0;
	fIsNoOwnerViewport = false;
	last_game_draw_cgo.Default();
	last_gui_draw_cgo.Default();
}

void C4Viewport::DrawPlayerInfo(C4TargetFacet &cgo)
{
	C4Facet ccgo;
	if (!ValidPlr(Player)) return;
	
	// Controls
	DrawPlayerStartup(cgo);
}

bool C4Viewport::Init(int32_t iPlayer, bool fSetTempOnly)
{
	// Fullscreen viewport initialization
	// Set Player
	if (!ValidPlr(iPlayer)) iPlayer = NO_OWNER;
	Player=iPlayer;
	if (!fSetTempOnly) fIsNoOwnerViewport = (iPlayer == NO_OWNER);
	// Owned viewport: clear any flash message explaining observer menu
	if (ValidPlr(iPlayer)) ::GraphicsSystem.FlashMessage("");
	return true;
}

bool C4Viewport::Init(CStdWindow * pParent, CStdApp * pApp, int32_t iPlayer)
{
	// Console viewport initialization
	// Set Player
	if (!ValidPlr(iPlayer)) iPlayer = NO_OWNER;
	Player=iPlayer;
	fIsNoOwnerViewport = (Player == NO_OWNER);
	// Create window
	pWindow = new C4ViewportWindow(this);
	if (!pWindow->Init(pApp, (Player==NO_OWNER) ? LoadResStr("IDS_CNS_VIEWPORT") : ::Players.Get(Player)->GetName(), pParent, false))
		return false;
	pWindow->pSurface = new CSurface(pApp, pWindow);
	// Position and size
	pWindow->RestorePosition(FormatString("Viewport%i", Player+1).getData(), Config.GetSubkeyPath("Console"));
	//UpdateWindow(hWnd);
	// Updates
	UpdateOutputSize();
	// Disable player lock on unowned viewports
	if (!ValidPlr(Player)) TogglePlayerLock();
	// Draw
	Execute();
	// Success
	return true;
}

extern int32_t DrawMessageOffset;

void C4Viewport::DrawPlayerStartup(C4TargetFacet &cgo)
{
	C4Player *pPlr;
	if (!(pPlr = ::Players.Get(Player))) return;
	if (!pPlr->LocalControl || !pPlr->ShowStartup) return;
	int32_t iNameHgtOff=0;

	// Control
	if (pPlr->MouseControl)
		GfxR->fctMouse.Draw(cgo.Surface,
		                    cgo.X+(cgo.Wdt-GfxR->fctKeyboard.Wdt)/2+55,
		                    cgo.Y+cgo.Hgt * 2/3 - 10 + DrawMessageOffset,
		                    0,0);
	if (pPlr && pPlr->ControlSet)
	{
		C4Facet controlset_facet = pPlr->ControlSet->GetPicture();
		if (controlset_facet.Wdt) controlset_facet.Draw(cgo.Surface,
			    cgo.X+(cgo.Wdt-controlset_facet.Wdt)/2,
			    cgo.Y+cgo.Hgt * 2/3 + DrawMessageOffset,
			    0,0);
		iNameHgtOff=GfxR->fctKeyboard.Hgt;
	}

	// Name
	Application.DDraw->TextOut(pPlr->GetName(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
	                           cgo.X+cgo.Wdt/2,cgo.Y+cgo.Hgt*2/3+iNameHgtOff + DrawMessageOffset,
	                           pPlr->ColorDw | 0xff000000, ACenter);
}

void C4Viewport::SetOutputSize(int32_t iDrawX, int32_t iDrawY, int32_t iOutX, int32_t iOutY, int32_t iOutWdt, int32_t iOutHgt)
{
	// update view position: Remain centered at previous position
	ViewX += (ViewWdt-iOutWdt)/2;
	ViewY += (ViewHgt-iOutHgt)/2;
	// update output parameters
	DrawX=iDrawX; DrawY=iDrawY;
	OutX=iOutX; OutY=iOutY;
	ViewWdt=iOutWdt; ViewHgt=iOutHgt;
	UpdateViewPosition();
	// Reset menus
	ResetMenuPositions=true;
	// player uses mouse control? then clip the cursor
	C4Player *pPlr;
	if ((pPlr=::Players.Get(Player)))
		if (pPlr->MouseControl)
		{
			::MouseControl.UpdateClip();
			// and inform GUI
			if (::pGUI)
				//::pGUI->SetBounds(C4Rect(iOutX, iOutY, iOutWdt, iOutHgt));
				::pGUI->SetPreferredDlgRect(C4Rect(iOutX, iOutY, iOutWdt, iOutHgt));
		}
}

void C4Viewport::ClearPointers(C4Object *pObj)
{
	Regions.ClearPointers(pObj);
}

void C4Viewport::DrawPlayerFogOfWar(C4TargetFacet &cgo)
{
	/*
	C4Player *pPlr=::Players.Get(Player);
	if (!pPlr || !pPlr->FogOfWar) return;
	pPlr->FogOfWar->Draw(cgo);*/ // now done by modulation
}

void C4Viewport::NextPlayer()
{
	C4Player *pPlr; int32_t iPlr;
	if (!(pPlr = ::Players.Get(Player)))
	{
		if (!(pPlr = ::Players.First)) return;
	}
	else if (!(pPlr = pPlr->Next))
		if (Game.C4S.Head.Film && Game.C4S.Head.Replay)
			pPlr = ::Players.First; // cycle to first in film mode only; in network obs mode allow NO_OWNER-view
	if (pPlr) iPlr = pPlr->Number; else iPlr = NO_OWNER;
	if (iPlr != Player) Init(iPlr, true);
}

bool C4Viewport::IsViewportMenu(class C4Menu *pMenu)
{
	// check all associated menus
	// Get player
	C4Player *pPlr = ::Players.Get(Player);
	// Player eliminated: No menu
	if (pPlr && pPlr->Eliminated) return false;
	// Player cursor object menu
	if (pPlr && pPlr->Cursor && pPlr->Cursor->Menu == pMenu) return true;
	// Player menu
	if (pPlr && pPlr->Menu.IsActive() && &(pPlr->Menu) == pMenu) return true;
	// Fullscreen menu (if active, only one viewport can exist)
	if (FullScreen.pMenu && FullScreen.pMenu->IsActive() && FullScreen.pMenu == pMenu) return true;
	// no match
	return false;
}
