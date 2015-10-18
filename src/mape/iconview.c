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
#include "mape/iconview.h"

MapeIconView* mape_icon_view_new(GError** error)
{
	MapeIconView* view;
	view = malloc(sizeof(MapeIconView) );

	view->list_store = GTK_TREE_MODEL(
		gtk_list_store_new(
			MAPE_ICON_VIEW_COLUMN_COUNT,
			GDK_TYPE_PIXBUF,
			G_TYPE_STRING
		)
	);

	view->view = gtk_icon_view_new_with_model(view->list_store);
	g_object_unref(view->list_store);

	gtk_icon_view_set_item_orientation(
		GTK_ICON_VIEW(view->view),
		GTK_ORIENTATION_HORIZONTAL
	);

	view->renderer_icon = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(
		GTK_CELL_LAYOUT(view->view),
		view->renderer_icon,
		FALSE
	);

	gtk_cell_layout_set_attributes(
		GTK_CELL_LAYOUT(view->view),
		view->renderer_icon,
		"pixbuf", MAPE_ICON_VIEW_COLUMN_ICON,
		NULL
	);

	view->renderer_name = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(
		GTK_CELL_LAYOUT(view->view),
		view->renderer_name,
		TRUE
	);

	gtk_cell_layout_set_attributes(
		GTK_CELL_LAYOUT(view->view),
		view->renderer_name,
		"text", MAPE_ICON_VIEW_COLUMN_NAME,
		NULL
	);
	
	gtk_icon_view_set_selection_mode(
		GTK_ICON_VIEW(view->view),
		GTK_SELECTION_NONE
	);

	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(view->list_store),
		MAPE_ICON_VIEW_COLUMN_NAME,
		GTK_SORT_ASCENDING
	);

	gtk_widget_show(view->view);

	view->window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(
		GTK_CONTAINER(view->window),
		view->view
	);

	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC
	);

	gtk_scrolled_window_set_shadow_type(
		GTK_SCROLLED_WINDOW(view->window),
		GTK_SHADOW_IN
	);

	gtk_widget_show(view->window);
	return view;
}

void mape_icon_view_destroy(MapeIconView* view)
{
	/* TODO: unref cell renderers? */
	free(view);
}

void mape_icon_view_clear(MapeIconView* view)
{
	gtk_list_store_clear(GTK_LIST_STORE(view->list_store) );
}

void mape_icon_view_add(MapeIconView* view,
                        GdkPixbuf* icon,
                        const char* name)
{
	GtkTreeIter iter;

	gtk_list_store_append(
		GTK_LIST_STORE(view->list_store),
		&iter
	);

	gtk_list_store_set(
		GTK_LIST_STORE(view->list_store),
		&iter,
		MAPE_ICON_VIEW_COLUMN_ICON,
		icon,
		MAPE_ICON_VIEW_COLUMN_NAME,
		name,
		-1
	);
}

