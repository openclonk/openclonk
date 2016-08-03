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

#ifndef INC_C4ViewportWindow
#define INC_C4ViewportWindow

#include "platform/C4Window.h"

#ifdef WITH_QT_EDITOR
#define C4ViewportWindowStyle (WS_VISIBLE)
#else
#define C4ViewportWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)
#endif
enum { ViewportScrollSpeed=10 };

class C4ViewportWindow: public C4Window
{
public:
	C4Viewport * cvp;
	C4ViewportWindow(C4Viewport * cvp): cvp(cvp) { }
	void EditCursorMove(int X, int Y, uint32_t);
	using C4Window::Init;
	C4Window * Init(int32_t iPlayer);
	virtual void Close();
	virtual void PerformUpdate();

#ifdef WITH_QT_EDITOR
	class QWidget *viewport_widget;
#endif
};

#endif
