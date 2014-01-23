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

#ifndef INC_MAPE_DISKVIEW_H
#define INC_MAPE_DISKVIEW_H

#include <gtk/gtk.h>

#include "mape/forward.h"
#include "mape/group.h"

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
