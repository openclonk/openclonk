/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Armin Burgmeier
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

/* Common window for drawing and property tool dialogs in console mode */

#ifndef INC_C4DevmodeDlg
#define INC_C4DevmodeDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#endif // WITH_DEVELOPER_MODE

// TODO: Threadsafety?
class C4DevmodeDlg
{
	// Make sure all developer tools are held in the same window
#ifdef WITH_DEVELOPER_MODE
private:
	static GtkWidget* window;
	static GtkWidget* notebook;

	static int x, y;

	static void OnDestroy(GtkWidget* widget, gpointer user_data);

public:
	static GtkWidget* GetWindow() { return window; }
	static void AddPage(GtkWidget* widget, GtkWindow* parent, const char* title);
	static void RemovePage(GtkWidget* widget);
	static void SwitchPage(GtkWidget* widget);

	static void SetTitle(GtkWidget* widget, const char* title);
#endif // WITH_DEVELOPER_MODE
};

#endif //INC_C4DevmodeDlg
