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

#ifndef INC_MAPE_MATTEXVIEW_H
#define INC_MAPE_MATTEXVIEW_H

#include <gtk/gtk.h>

#include "mape/forward.h"
#include "mape/group.h"
#include "mape/material.h"
#include "mape/texture.h"

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
                                  MapeTextureMap* new_tex_map,
                                  MapeGroup* base_group,
                                  gboolean overload_materials,
                                  gboolean overload_textures,
                                  MapeGroup* overload_from,
                                  GError** error);

#endif /* INC_MAPE_MATTEXVIEW_H */
