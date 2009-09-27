/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009 Armin Burgmeier
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
