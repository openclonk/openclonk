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

#ifndef INC_MAPE_HEADER_H
#define INC_MAPE_HEADER_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkuimanager.h>
#include <gtk/gtkactiongroup.h>
#include <gtk/gtkaction.h>
#include "forward.h"

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
