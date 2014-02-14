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

#include <stdlib.h>

#include "mape/preferences.h"
#include "mape/preferencesdialog.h"

static void mape_preferences_dialog_cb_fixed_seed_toggled(GtkWidget* widget,
                                                          gpointer user_data)
                                                          /*MapePreferencesDialog* dialog)*/
{
	MapePreferencesDialog* dialog;
	dialog = (MapePreferencesDialog*)user_data;
	
	gtk_widget_set_sensitive(
		GTK_WIDGET(dialog->hbox_random_seed),
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(dialog->cbx_fixed_seed)
		)
	);
}

MapePreferencesDialog* mape_preferences_dialog_new(GtkWindow* parent,
                                                   MapePreferences* prefs)
{
	MapePreferencesDialog* dialog;
	GtkBox* content_area;

	dialog = malloc(sizeof(MapePreferencesDialog) );
	
	dialog->lbl_tab_width = gtk_label_new("Tab width:");
	gtk_widget_show(dialog->lbl_tab_width);

	dialog->ent_tab_width = gtk_spin_button_new_with_range(1, 8, 1);
	gtk_widget_show(dialog->ent_tab_width);

	dialog->hbox_tab_width = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_tab_width),
		dialog->lbl_tab_width,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_tab_width),
		dialog->ent_tab_width,
		TRUE,
		TRUE,
		0
	);
	
	gtk_widget_show(dialog->hbox_tab_width);
	
	/* Map width */
	
	dialog->lbl_map_width = gtk_label_new("Map width:");
	gtk_widget_show(dialog->lbl_map_width);

	dialog->ent_map_width = gtk_spin_button_new_with_range(50, 500, 5);
	gtk_widget_show(dialog->ent_map_width);

	dialog->hbox_map_width = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_map_width),
		dialog->lbl_map_width,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_map_width),
		dialog->ent_map_width,
		TRUE,
		TRUE,
		0
	);
	
	gtk_widget_show(dialog->hbox_map_width);
	
	/* Map height */
	
	dialog->lbl_map_height = gtk_label_new("Map height:");
	gtk_widget_show(dialog->lbl_map_height);

	dialog->ent_map_height = gtk_spin_button_new_with_range(50, 500, 5);
	gtk_widget_show(dialog->ent_map_height);

	dialog->hbox_map_height = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_map_height),
		dialog->lbl_map_height,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_map_height),
		dialog->ent_map_height,
		TRUE,
		TRUE,
		0
	);
	
	gtk_widget_show(dialog->hbox_map_height);
	
	/* Checkboxes */
	
	dialog->cbx_tab_to_spaces = gtk_check_button_new_with_label(
		"Insert spaces instead of tabs"
	);
	gtk_widget_show(dialog->cbx_tab_to_spaces);
	
	dialog->cbx_auto_indentation = gtk_check_button_new_with_label(
		"Automatic indentation"
	);
	gtk_widget_show(dialog->cbx_auto_indentation);
	
	dialog->cbx_text_wrapping = gtk_check_button_new_with_label(
		"Text wrapping"
	);
	gtk_widget_show(dialog->cbx_text_wrapping);
	
	dialog->cbx_line_numbers = gtk_check_button_new_with_label(
		"Display line numbers"
	);
	gtk_widget_show(dialog->cbx_line_numbers);
	
	dialog->cbx_highlight_line = gtk_check_button_new_with_label(
		"Highlight current line"
	);
	gtk_widget_show(dialog->cbx_highlight_line);
	
	dialog->cbx_bracket_matching = gtk_check_button_new_with_label(
		"Highlight matching brackets"
	);
	gtk_widget_show(dialog->cbx_bracket_matching);
	
	dialog->cbx_fixed_seed = gtk_check_button_new_with_label(
		"Fixed random seed"
	);
	gtk_widget_show(dialog->cbx_fixed_seed);
	
	dialog->lbl_random_seed = gtk_label_new("Random seed:");
	gtk_widget_show(dialog->lbl_random_seed);

	dialog->ent_random_seed = gtk_spin_button_new_with_range(
		0,
		(1u << 31u) - 1,
		1
	);
	gtk_widget_show(dialog->ent_random_seed);

	dialog->hbox_random_seed = gtk_hbox_new(FALSE, 5);
	
	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_random_seed),
		dialog->lbl_random_seed,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		GTK_BOX(dialog->hbox_random_seed),
		dialog->ent_random_seed,
		TRUE,
		TRUE,
		0
	);
	gtk_widget_show(dialog->hbox_random_seed);
	
	g_signal_connect(
		G_OBJECT(dialog->cbx_fixed_seed),
		"toggled",
		G_CALLBACK(mape_preferences_dialog_cb_fixed_seed_toggled),
		dialog
	);

	/* Read values from preferences */
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(dialog->ent_tab_width),
		prefs->tab_width
	);
	
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(dialog->ent_map_width),
		prefs->map_width
	);

	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(dialog->ent_map_height),
		prefs->map_height
	);

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_tab_to_spaces),
		prefs->tab_to_spaces
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_auto_indentation),
		prefs->auto_indentation
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_text_wrapping),
		prefs->text_wrapping
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_line_numbers),
		prefs->line_numbers
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_highlight_line),
		prefs->highlight_line
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_bracket_matching),
		prefs->bracket_matching
	);
	
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_fixed_seed),
		prefs->fixed_seed
	);

	/* Pseudo-emit toggled signal to hide the random seed entry if
	 * the fixed seed CheckButton is not active. */
	mape_preferences_dialog_cb_fixed_seed_toggled(
		dialog->cbx_fixed_seed,
		dialog
	);
	
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(dialog->ent_random_seed),
		prefs->random_seed
	);

	dialog->dialog = gtk_dialog_new_with_buttons(
		"Preferences",
		parent,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK,
		GTK_RESPONSE_OK,
		NULL
	);

	content_area = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog->dialog)));
	gtk_box_pack_start(
		content_area,
		dialog->hbox_tab_width,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->hbox_map_width,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->hbox_map_height,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_tab_to_spaces,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_auto_indentation,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_text_wrapping,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_line_numbers,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_highlight_line,
		FALSE,
		TRUE,
		0
	);
	
	gtk_box_pack_start(
		content_area,
		dialog->cbx_bracket_matching,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		content_area,
		dialog->cbx_fixed_seed,
		FALSE,
		TRUE,
		0
	);

	gtk_box_pack_start(
		content_area,
		dialog->hbox_random_seed,
		FALSE,
		TRUE,
		0
	);
	
	gtk_container_set_border_width(
		GTK_CONTAINER(content_area),
		10
	);

	gtk_box_set_spacing(content_area, 5);

	gtk_window_set_transient_for(GTK_WINDOW(dialog->dialog), parent);
	gtk_window_set_resizable(GTK_WINDOW(dialog->dialog), FALSE);

	return dialog;
}

void mape_preferences_dialog_destroy(MapePreferencesDialog* dialog)
{
	gtk_widget_destroy(dialog->dialog);
	free(dialog);
}

MapePreferences mape_preferences_dialog_get(MapePreferencesDialog* dialog)
{
	MapePreferences prefs;
	
	prefs.tab_width = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(dialog->ent_tab_width)
	);

	prefs.map_width = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(dialog->ent_map_width)
	);

	prefs.map_height = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(dialog->ent_map_height)
	);
	
	prefs.tab_to_spaces = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_tab_to_spaces)
	);
	
	prefs.auto_indentation = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_auto_indentation)
	);
	
	prefs.text_wrapping = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_text_wrapping)
	);
	
	prefs.line_numbers = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_line_numbers)
	);
	
	prefs.highlight_line = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_highlight_line)
	);
	
	prefs.bracket_matching = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_bracket_matching)
	);
	
	prefs.fixed_seed = gtk_toggle_button_get_active(
		GTK_TOGGLE_BUTTON(dialog->cbx_fixed_seed)
	);
	
	prefs.random_seed = gtk_spin_button_get_value_as_int(
		GTK_SPIN_BUTTON(dialog->ent_random_seed)
	);

	return prefs;
}
