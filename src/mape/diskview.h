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
