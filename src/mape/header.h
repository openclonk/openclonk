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

#ifndef INC_MAPE_HEADER_H
#define INC_MAPE_HEADER_H

#include <gtk/gtk.h>
#include "mape/forward.h"

struct MapeHeader_ {
	GtkUIManager* ui_manager;
	GtkAccelGroup* accel_group;

	GtkWidget* menubar;
	GtkWidget* toolbar;

	GtkActionGroup* group_file;
	GtkAction* file_new;
	GtkAction* file_open;
	GtkAction* file_save;
	GtkAction* file_save_as;
	GtkAction* file_quit;
	
	GtkActionGroup* group_edit;
	GtkAction* edit_undo;
	GtkAction* edit_redo;
	GtkAction* edit_preferences;

	GtkActionGroup* group_help;
	GtkAction* help_about;
};

MapeHeader* mape_header_new(void);
void mape_header_destroy(MapeHeader* header);

#endif /* INC_MAPE_HEADER_H */
