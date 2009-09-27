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

#ifndef INC_MAPE_EDITVIEW_H
#define INC_MAPE_EDITVIEW_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksourcelanguagemanager.h>
#include <gtksourceview/gtksourcestyleschememanager.h>
#include "mape/forward.h"

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
