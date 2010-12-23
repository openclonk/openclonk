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

#include <StdWindow.h>

class C4Viewport;

#ifdef WITH_DEVELOPER_MODE
#include <StdGtkWindow.h>
typedef CStdGtkWindow C4ViewportBase;
#else
typedef CStdWindow C4ViewportBase;
#endif

class C4ViewportWindow: public C4ViewportBase
{
public:
	C4Viewport * cvp;
	C4ViewportWindow(C4Viewport * cvp): cvp(cvp) { }
#ifdef _WIN32
	virtual CStdWindow * Init(CStdWindow::WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool);
	static bool RegisterViewportClass(HINSTANCE hInst);
#elif defined(WITH_DEVELOPER_MODE)
	virtual GtkWidget* InitGUI();

	static gboolean OnKeyPressStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data);
	static gboolean OnKeyReleaseStatic(GtkWidget* widget, GdkEventKey* event, gpointer user_data);
	static gboolean OnScrollStatic(GtkWidget* widget, GdkEventScroll* event, gpointer user_data);
	static gboolean OnButtonPressStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
	static gboolean OnButtonReleaseStatic(GtkWidget* widget, GdkEventButton* event, gpointer user_data);
	static gboolean OnMotionNotifyStatic(GtkWidget* widget, GdkEventMotion* event, gpointer user_data);
	static gboolean OnConfigureStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data);
	static void OnRealizeStatic(GtkWidget* widget, gpointer user_data);
	static gboolean OnExposeStatic(GtkWidget* widget, GdkEventExpose* event, gpointer user_data);
	static void OnDragDataReceivedStatic(GtkWidget* widget, GdkDragContext* context, gint x, gint y, GtkSelectionData* data, guint info, guint time, gpointer user_data);

	static gboolean OnConfigureDareaStatic(GtkWidget* widget, GdkEventConfigure* event, gpointer user_data);

	static void OnVScrollStatic(GtkAdjustment* adjustment, gpointer user_data);
	static void OnHScrollStatic(GtkAdjustment* adjustment, gpointer user_data);

	GtkWidget* h_scrollbar;
	GtkWidget* v_scrollbar;
	GtkWidget* drawing_area;
#elif defined(USE_X11) && !defined(WITH_DEVELOPER_MODE)
	virtual void HandleMessage (XEvent &);
#endif
	void EditCursorMove(int X, int Y, uint16_t);
	virtual void Close();
	virtual void PerformUpdate();
};

#define C4ViewportClassName "C4Viewport"
#define C4ViewportWindowStyle (WS_VISIBLE | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX)

#endif
