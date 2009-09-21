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

#ifndef INC_MAPE_PREVIEW_H
#define INC_MAPE_PREVIEW_H

#include <gtk/gtkwidget.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "forward.h"

typedef enum MapePreViewError_ {
	MAPE_PRE_VIEW_ERROR_MISSING_MAPS,
	MAPE_PRE_VIEW_ERROR_FAILED
} MapePreViewError;

struct MapePreView_ {
	GtkWidget* frame;
	GtkWidget* event_box;
	GtkWidget* image;
	/*GdkPixbuf* pixbuf;*/

	MapeMatTexView* mat_tex;
};

MapePreView* mape_pre_view_new(MapeMatTexView* mat_tex,
                               GError** error);
void mape_pre_view_destroy(MapePreView* view);

void mape_pre_view_update(MapePreView* view,
                          GdkPixbuf* pixbuf);

void mape_pre_view_apply_preferences(MapePreView* view,
                                      MapePreferences* preferences);

#endif /* INC_MAPE_PREVIEW_H */
