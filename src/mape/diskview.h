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

#ifndef INC_MAPE_DISKVIEW_H
#define INC_MAPE_DISKVIEW_H

#include <gtk/gtkwidget.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include "forward.h"

typedef enum MapeDiskViewColumns_ {
	MAPE_DISK_VIEW_COLUMN_ICON,
	MAPE_DISK_VIEW_COLUMN_FILE,
	MAPE_DISK_VIEW_COLUMN_GROUP,

	MAPE_DISK_VIEW_COLUMN_COUNT
} MapeDiskViewColumns;

typedef enum MapeDiskViewError_ {
	MAPE_DISK_VIEW_ERROR_NOENT,

	MAPE_DISK_VIEW_ERROR_FAILED
} MapeDiskViewError;

struct MapeDiskView_ {
	GtkWidget* window;
	GtkWidget* view;

	GtkTreeModel* tree_store;

	GtkCellRenderer* renderer_icon;
	GtkCellRenderer* renderer_file;

	GtkTreeViewColumn* col_file;

	MapeMatTexView* mat_tex;
	MapeEditView* edit_view;

	MapeFileIconSet* icon_set;
	MapeGroup* group_top;
	MapeConfigFile* config;
};

MapeDiskView* mape_disk_view_new(MapeFileIconSet* icon_set,
                                 MapeMatTexView* mat_tex,
                                 MapeEditView* edit_view,
                                 MapeConfigFile* config,
                                 GError** error);
void mape_disk_view_destroy(MapeDiskView* disk_view);

gboolean mape_disk_view_extend_to_path(MapeDiskView* disk_view,
                                       const gchar* filepath,
                                       GError** error);

#endif /* INC_MAPE_DISKVIEW_H */
