/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#ifndef INC_MAPE_WINDOW_H
#define INC_MAPE_WINDOW_H

#include <gtk/gtk.h>
#include "mape/forward.h"
#include "mape/preferences.h"

struct MapeWindow_ {
	MapeFileIconSet* icon_set;
	gchar* last_path;

	GtkWidget* window;

	GtkWidget* topbox;
	GtkWidget* mid_paned;
	GtkWidget* bottom_hbox;

	GtkWidget* menubar;
	GtkWidget* toolbar;

	MapeHeader* header;
	MapeStatusbar* statusbar;

	MapeDiskView* disk_view;
	MapeMatTexView* mat_tex_view;
	MapeEditView* edit_view;
	MapePreView* pre_view;
	
	MapeConfigFile* config;
	MapePreferences preferences;
};

MapeWindow* mape_window_new(int argc,
                            char* argv[],
			    GError** error);

void mape_window_destroy(MapeWindow* wnd);

#endif /* INC_MAPE_WINDOW_H */
