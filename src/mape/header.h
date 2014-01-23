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
