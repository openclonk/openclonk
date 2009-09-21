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

#ifndef INC_MAPE_EDITVIEW_H
#define INC_MAPE_EDITVIEW_H

#include <gtk/gtkwidget.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include "forward.h"

typedef enum MapeEditViewError_ {
	MAPE_EDIT_VIEW_ERROR_MISSING_MAPS,
	MAPE_EDIT_VIEW_ERROR_UNKNOWN_ENCODING,
	MAPE_EDIT_VIEW_ERROR_FAILED
} MapeEditViewError;

struct MapeEditView_ {
	GtkWidget* window;
	GtkWidget* view;
	gchar* file_path;
	const gchar* encoding;

	GtkSourceLanguageManager* lang_manager;
	GtkSourceStyleSchemeManager* style_manager;
	PangoFontDescription* font_desc;

	MapePreView* pre_view;
	MapeStatusbar* statusbar;
	
	gboolean fixed_seed;
	unsigned int random_seed;
	
	unsigned int map_width;
	unsigned int map_height;
	
	GThread* render_thread;
	gboolean rerender;
};

MapeEditView* mape_edit_view_new(MapePreView* pre_view,
                                 MapeStatusbar* statusbar,
                                 GError** error);
void mape_edit_view_destroy(MapeEditView* view);

void mape_edit_view_clear(MapeEditView* view);
gboolean mape_edit_view_open(MapeEditView* view,
                             const gchar* filename,
                             GError** error);
gboolean mape_edit_view_save(MapeEditView* view,
                             const gchar* filename,
                             GError** error);
                             
gboolean mape_edit_view_get_modified(MapeEditView* view);
void mape_edit_view_undo(MapeEditView* edit_view);
void mape_edit_view_redo(MapeEditView* edit_view);

void mape_edit_view_apply_preferences(MapeEditView* edit_view,
                                      MapePreferences* preferences);
void mape_edit_view_reload(MapeEditView* edit_view);


#endif /* INC_MAPE_EDITVIEW_H */
