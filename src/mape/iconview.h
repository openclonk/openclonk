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

#ifndef INC_MAPE_ICONVIEW_H
#define INC_MAPE_ICONVIEW_H

#include <gtk/gtk.h>
#include "mape/forward.h"

typedef enum MapeIconViewColumns_ {
	MAPE_ICON_VIEW_COLUMN_ICON,
	MAPE_ICON_VIEW_COLUMN_NAME,

	MAPE_ICON_VIEW_COLUMN_COUNT
} MapeIconViewColumns;

struct MapeIconView_ {
	GtkWidget* window;
	GtkWidget* view;

	GtkTreeModel* list_store;

	GtkCellRenderer* renderer_icon;
	GtkCellRenderer* renderer_name;
};

MapeIconView* mape_icon_view_new(GError** error);
void mape_icon_view_destroy(MapeIconView* view);

void mape_icon_view_clear(MapeIconView* view);
void mape_icon_view_add(MapeIconView* view,
                        GdkPixbuf* icon,
                        const char* name);

#endif /* INC_MAPE_ICONVIEW_H */
