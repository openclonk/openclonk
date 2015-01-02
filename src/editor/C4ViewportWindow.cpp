/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* A viewport to each player */

#include <C4Include.h>
#include <C4ViewportWindow.h>

#include <C4Viewport.h>
#include <C4Console.h>
#include <C4Landscape.h>
#include <C4PlayerList.h>

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
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
	SetViewY(float(scroll.nPos));
	// Horizontal
	scroll.fMask=SIF_POS;
	GetScrollInfo(pWindow->hWindow,SB_HORZ,&scroll);
	SetViewX(float(scroll.nPos));
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
	scroll.nMax = GBackHgt * Zoom;
	scroll.nPage=ViewHgt;
	scroll.nPos=int(GetViewY() * Zoom);
	SetScrollInfo(pWindow->hWindow,SB_VERT,&scroll,true);
	// Horizontal
	scroll.fMask=SIF_ALL;
	scroll.nMin=0;
	scroll.nMax=GBackWdt * Zoom;
	scroll.nPage=ViewWdt;
	scroll.nPos = int(GetViewX() * Zoom);
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
	                         GetViewX(), // value
	                         0, // lower
	                         GBackWdt, // upper
	                         ViewportScrollSpeed, // step_increment
	                         allocation.width / Zoom, // page_increment
	                         allocation.width / Zoom // page_size
	                         );

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	gtk_adjustment_configure(adjustment,
	                         GetViewY(), // value
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
	SetViewX(gtk_adjustment_get_value(adjustment));

	adjustment = gtk_range_get_adjustment(GTK_RANGE(pWindow->v_scrollbar));
	SetViewY(gtk_adjustment_get_value(adjustment));

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
	Console.EditCursor.Move(cvp->GetViewX() + X / cvp->Zoom, cvp->GetViewY() + Y / cvp->Zoom, state);
}
