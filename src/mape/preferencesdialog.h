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

#ifndef INC_MAPE_PREFERENCESDIALOG_H
#define INC_MAPE_PREFERENCESDIALOG_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include "forward.h"

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
