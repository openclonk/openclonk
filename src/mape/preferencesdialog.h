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

#ifndef INC_MAPE_PREFERENCESDIALOG_H
#define INC_MAPE_PREFERENCESDIALOG_H

#include <gtk/gtk.h>
#include "mape/forward.h"

struct MapePreferencesDialog_ {
	GtkWidget* dialog;

	GtkWidget* hbox_tab_width;
	GtkWidget* lbl_tab_width;
	GtkWidget* ent_tab_width;

	GtkWidget* cbx_tab_to_spaces;
	GtkWidget* cbx_auto_indentation;
	GtkWidget* cbx_text_wrapping;
	GtkWidget* cbx_line_numbers;
	GtkWidget* cbx_highlight_line;
	GtkWidget* cbx_bracket_matching;
	GtkWidget* cbx_fixed_seed;

	GtkWidget* hbox_random_seed;
	GtkWidget* lbl_random_seed;
	GtkWidget* ent_random_seed;
	
	GtkWidget* hbox_map_width;
	GtkWidget* lbl_map_width;
	GtkWidget* ent_map_width;
	GtkWidget* hbox_map_height;
	GtkWidget* lbl_map_height;
	GtkWidget* ent_map_height;
	
};

MapePreferencesDialog* mape_preferences_dialog_new(GtkWindow* parent,
                                                   MapePreferences* prefs);
void mape_preferences_dialog_destroy(MapePreferencesDialog* dialog);

MapePreferences mape_preferences_dialog_get(MapePreferencesDialog* dialog);

#endif /* INC_MAPE_PREFERENCESDIALOG_H */
