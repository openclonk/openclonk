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

#include <stdlib.h>
#include <gtk/gtknotebook.h>
#include "material.h"
#include "texture.h"
#include "fileicon.h"
#include "iconview.h"
#include "mattexview.h"

MapeMatTexView* mape_mat_tex_view_new(MapeFileIconSet* icon_set,
                                      GError** error)
{
	MapeMatTexView* view;
	view = malloc(sizeof(MapeMatTexView) );

	view->mat_map = NULL;
	view->tex_map = NULL;
	view->icon_set = icon_set;

	view->view_mat = mape_icon_view_new(error);
	if(view->view_mat == NULL)
	{
		free(view);
		return NULL;
	}

	view->view_tex = mape_icon_view_new(error);
	if(view->view_tex == NULL)
	{
		mape_icon_view_destroy(view->view_mat);
		free(view);
		return NULL;
	}

	view->notebook = gtk_notebook_new();

	gtk_notebook_append_page(
		GTK_NOTEBOOK(view->notebook),
		view->view_mat->window,
		NULL
	);

	gtk_notebook_set_tab_label_text(
		GTK_NOTEBOOK(view->notebook),
		view->view_mat->window,
		"Materials"
	);

	gtk_notebook_append_page(
		GTK_NOTEBOOK(view->notebook),
		view->view_tex->window,
		NULL
	);

	gtk_notebook_set_tab_label_text(
		GTK_NOTEBOOK(view->notebook),
		view->view_tex->window,
		"Textures"
	);

	gtk_widget_set_size_request(view->notebook, -1, 150);
	gtk_widget_show(view->notebook);
	return view;
}

void mape_mat_tex_view_destroy(MapeMatTexView* view)
{
	if(view->mat_map != NULL)
		mape_material_map_destroy(view->mat_map);
	if(view->tex_map != NULL)
		mape_texture_map_destroy(view->tex_map);

	mape_icon_view_destroy(view->view_mat);
	mape_icon_view_destroy(view->view_tex);

	free(view);
}

gboolean mape_mat_tex_view_reload(MapeMatTexView* view,
                                  MapeGroup* base_group,
                                  MapeGroup* overload_from,
                                  GError** error)
{
	MapeMaterialMap* new_mat_map;
	MapeTextureMap* new_tex_map;
	MapeFileIcon* icon;
	MapeMaterial* mat;
	unsigned int i;

	new_mat_map = mape_material_map_new(base_group, overload_from, error);
	if(new_mat_map == NULL) return FALSE;

	new_tex_map = mape_texture_map_new(base_group, overload_from, error);
	if(new_tex_map == NULL)
	{
		mape_material_map_destroy(new_mat_map);
		return FALSE;
	}

	if(view->mat_map != NULL) mape_material_map_destroy(view->mat_map);
	if(view->tex_map != NULL) mape_texture_map_destroy(view->tex_map);
	view->mat_map = new_mat_map; view->tex_map = new_tex_map;

	mape_icon_view_clear(view->view_mat);
	for(i = 0; i < mape_material_map_get_material_count(new_mat_map); ++ i)
	{
		mat = mape_material_map_get_material(new_mat_map, i);

		icon = mape_file_icon_set_lookup(
			view->icon_set,
			MAPE_FILE_ICON_C4MATERIAL
		);

		mape_icon_view_add(
			view->view_mat,
			mape_file_icon_get(icon),
			mape_material_get_name(mat)
		);
	}

	mape_icon_view_clear(view->view_tex);
	for(i = 0; i < mape_texture_map_get_texture_count(new_tex_map); ++ i)
	{
		icon = mape_file_icon_set_lookup(
			view->icon_set,
			MAPE_FILE_ICON_C4TEXTURE
		);

		mape_icon_view_add(
			view->view_tex,
			mape_file_icon_get(icon),
			mape_texture_map_get_texture(new_tex_map, i)
		);
	}

	return TRUE;
}

