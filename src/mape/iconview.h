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
