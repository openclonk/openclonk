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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "mape/material.h"
#include "mape/texture.h"
#include "mape/fileicon.h"
#include "mape/iconview.h"
#include "mape/mattexview.h"

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
		g_object_unref(view->mat_map);
	if(view->tex_map != NULL)
		g_object_unref(view->tex_map);

	mape_icon_view_destroy(view->view_mat);
	mape_icon_view_destroy(view->view_tex);

	free(view);
}

gboolean mape_mat_tex_view_reload(MapeMatTexView* view,
                                  MapeTextureMap* new_tex_map,
                                  MapeGroup* base_group,
                                  gboolean overload_materials,
                                  gboolean overload_textures,
                                  MapeGroup* overload_from,
                                  GError** error)
{
	MapeMaterialMap* new_mat_map;
	MapeFileIcon* icon;
	const MapeMaterial* mat;
	unsigned int i;

	new_mat_map = mape_material_map_new();
	if(!mape_material_map_load(new_mat_map, base_group, error))
	{
		g_object_unref(new_mat_map);
		return FALSE;
	}

	if(overload_materials && overload_from != NULL)
	{
		if(!mape_material_map_load(new_mat_map, overload_from, error))
		{
			g_object_unref(new_mat_map);
			return FALSE;
		}
	}

	if(!mape_texture_map_load_textures(new_tex_map, base_group, error))
	{
		g_object_unref(new_mat_map);
		return FALSE;
	}

	if(overload_textures && overload_from != NULL)
	{
		if(!mape_texture_map_load_textures(new_tex_map, overload_from, error))
		{
			g_object_unref(new_mat_map);
			return FALSE;
		}
	}

	/* Load the texture map again, because now that we have materials
	 * and textures loaded, we can actually assign the indices in the
	 * texture map. */
	if (!mape_texture_map_load_map(new_tex_map, base_group, error))
	{
		g_object_unref(new_mat_map);
		return FALSE;
	}

	mape_material_map_set_default_textures(new_mat_map, new_tex_map);

	if(view->mat_map != NULL) g_object_unref(view->mat_map);
	if(view->tex_map != NULL) g_object_unref(view->tex_map);
	view->mat_map = new_mat_map; view->tex_map = new_tex_map;
	g_object_ref(new_tex_map);

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
		mape_icon_view_add(
			view->view_tex,
			mape_texture_map_lookup_texture(
				new_tex_map,
				mape_texture_map_get_texture_name(new_tex_map, i)
			),
			mape_texture_map_get_texture_name(new_tex_map, i)
		);
	}

	return TRUE;
}

