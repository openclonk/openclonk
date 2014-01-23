/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Common window for drawing and property tool dialogs in console mode */

#ifndef INC_C4DevmodeDlg
#define INC_C4DevmodeDlg

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtk.h>
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
