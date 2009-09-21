/* mape - C4 Landscape.txt editor
 * Copyright (C) 2005 Armin Burgmeier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef INC_MAPE_WINDOW_H
#define INC_MAPE_WINDOW_H

#include <gtk/gtkwidget.h>
#include "forward.h"
#include "preferences.h"

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
