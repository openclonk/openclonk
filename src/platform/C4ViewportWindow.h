/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
 * Copyright (c) 2005-2006, 2008, 2010  Günther Brammer
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

#ifndef INC_C4ViewportWindow
#define INC_C4ViewportWindow

#include <C4Window.h>
#include <C4Viewport.h>

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
#endif
#define C4ViewportWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)
enum { ViewportScrollSpeed=10 };

class C4ViewportWindow: public C4Window
{
public:
	C4Viewport * cvp;
	C4ViewportWindow(C4Viewport * cvp): cvp(cvp) { }
#if defined(WITH_DEVELOPER_MODE)
	GtkWidget* h_scrollbar;
	GtkWidget* v_scrollbar;
#endif
	void EditCursorMove(int X, int Y, uint32_t);
	using C4Window::Init;
	C4Window * Init(int32_t iPlayer);
	virtual void Close();
	virtual void PerformUpdate();
};

#endif
