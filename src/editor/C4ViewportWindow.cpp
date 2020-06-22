/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "editor/C4ViewportWindow.h"

#include "game/C4Viewport.h"
#include "editor/C4Console.h"
#include "landscape/C4Landscape.h"
#include "player/C4PlayerList.h"


#ifdef WITH_QT_EDITOR
#include "editor/C4ConsoleQtViewport.h"
#endif

#ifdef WITH_QT_EDITOR
bool C4Viewport::ScrollBarsByViewPosition()
{
	if (PlayerLock) return false;
	scrollarea->ScrollBarsByViewPosition();
	return true;
}

bool C4Viewport::TogglePlayerLock()
{
	PlayerLock = !PlayerLock;
	scrollarea->setScrollBarVisibility(!PlayerLock);
	ScrollBarsByViewPosition();
	return true;
}

#elif defined(USE_WIN32_WINDOWS)

void UpdateWindowLayout(HWND hwnd)
{
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
	scroll.nMax = ::Landscape.GetHeight() * Zoom;
	scroll.nPage=ViewHgt;
	scroll.nPos=int(GetViewY() * Zoom);
	SetScrollInfo(pWindow->hWindow,SB_VERT,&scroll,true);
	// Horizontal
	scroll.fMask=SIF_ALL;
	scroll.nMin=0;
	scroll.nMax=::Landscape.GetWidth() * Zoom;
	scroll.nPage=ViewWdt;
	scroll.nPos = int(GetViewX() * Zoom);
	SetScrollInfo(pWindow->hWindow,SB_HORZ,&scroll,true);
	return true;
}

#endif

void C4ViewportWindow::PerformUpdate()
{
#ifdef WITH_QT_EDITOR
	if (viewport_widget)
		viewport_widget->update();
#else
	if (cvp)
	{
		cvp->UpdateOutputSize();
		cvp->Execute();
	}
#endif
}

C4Window * C4ViewportWindow::Init(int32_t Player)
{
	C4Window* result;
	const char * Title = Player == NO_OWNER ? LoadResStr("IDS_CNS_VIEWPORT") : ::Players.Get(Player)->GetName();
	C4Rect r(0,0,800,500);
	result = C4Window::Init(C4Window::W_Viewport, &Application, Title, &r);

	if (!result) return result;

	pSurface = new C4Surface(&Application, this);
#ifndef WITH_QT_EDITOR
	// Position and size
	RestorePosition(FormatString("Viewport%i", Player+1).getData(), Config.GetSubkeyPath("Console"));
#endif
	return result;
}

void C4ViewportWindow::Close()
{
	::Viewports.CloseViewport(cvp);
}
void C4ViewportWindow::EditCursorMove(int X, int Y, uint32_t state)
{
	Console.EditCursor.Move(cvp->WindowToGameX(X), cvp->WindowToGameY(Y), cvp->GetZoom(), state);
}
