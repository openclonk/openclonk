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

#ifndef INC_MAPE_ICONVIEW_H
#define INC_MAPE_ICONVIEW_H

#include <gtk/gtkwidget.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtkcellrenderer.h>
#include "forward.h"

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
