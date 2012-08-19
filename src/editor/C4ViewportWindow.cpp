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

#ifdef USE_WIN32_WINDOWS

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
	gtk_widget_get_allocation(GTK_WIDGET(pWindow->render_widget), &allocation);

	GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->h_scrollbar));

	gtk_adjustment_configure(adjustment,
	                         ViewX, // value
	                         0, // lower
	                         GBackWdt, // upper
	                         ViewportScrollSpeed, // step_increment
	                         allocation.width / Zoom, // page_increment
	                         allocation.width / Zoom // page_size
	                         );

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	gtk_adjustment_configure(adjustment,
	                         ViewY, // value
	                         0, // lower
	                         GBackHgt, // upper
	                         ViewportScrollSpeed, // step_increment
	                         allocation.height / Zoom, // page_increment
	                         allocation.height / Zoom // page_size
	                         );
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
	C4Rect r(0,0,800,500);
	result = C4Window::Init(C4Window::W_Viewport, &Application, Title, &r);

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
void C4ViewportWindow::EditCursorMove(int X, int Y, uint32_t state)
{
	Console.EditCursor.Move(cvp->ViewX + X / cvp->Zoom, cvp->ViewY + Y / cvp->Zoom, state);
}
