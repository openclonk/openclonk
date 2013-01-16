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
