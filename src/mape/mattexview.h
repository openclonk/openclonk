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

#ifndef INC_MAPE_MATTEXVIEW_H
#define INC_MAPE_MATTEXVIEW_H

#include <gtk/gtkwidget.h>
#include "forward.h"

struct MapeMatTexView_ {
	GtkWidget* notebook;

	MapeIconView* view_mat;
	MapeIconView* view_tex;

	MapeMaterialMap* mat_map;
	MapeTextureMap* tex_map;

	MapeFileIconSet* icon_set;
};

MapeMatTexView* mape_mat_tex_view_new(MapeFileIconSet* icon_set,
                                      GError** error);
void mape_mat_tex_view_destroy(MapeMatTexView* view);

gboolean mape_mat_tex_view_reload(MapeMatTexView* view,
                                  MapeGroup* base_group,
                                  MapeGroup* overload_from,
                                  GError** error);

#endif /* INC_MAPE_MATTEXVIEW_H */
