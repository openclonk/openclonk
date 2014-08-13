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
#include "mape/preview.h"
#include "mape/preferences.h"

MapePreView* mape_pre_view_new(MapeMatTexView* mat_tex,
                               GError** error)
{
	MapePreView* view;
	view = malloc(sizeof(MapePreView) );

	view->mat_tex = mat_tex;

	view->image = gtk_image_new();
	gtk_widget_show(view->image);

	view->event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(view->event_box), view->image);
	gtk_widget_add_events(view->event_box, GDK_BUTTON_PRESS_MASK);
	gtk_widget_show(view->event_box);
	
	view->frame = gtk_frame_new(NULL);/*"Landscape preview");*/
	gtk_frame_set_shadow_type(GTK_FRAME(view->frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(view->frame), view->event_box);
	gtk_widget_show(view->frame);
	
	/*gtk_widget_set_size_request(view->image, preferences->map_width, preferences->map_height);*/

	return view;
}

void mape_pre_view_destroy(MapePreView* view)
{
	free(view);
}

void mape_pre_view_update(MapePreView* view,
                          GdkPixbuf* pixbuf)
{
	/* TODO: Unref old pixbuf */
	/* TODO: ref new pixbuf? */
	gtk_image_set_from_pixbuf(GTK_IMAGE(view->image), pixbuf);

	/* Update size from image, in case Map.c specifies different map dimensions */
	if(pixbuf != NULL)
		gtk_widget_set_size_request(view->image, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
}

void mape_pre_view_apply_preferences(MapePreView* view,
                                      MapePreferences* preferences)
{
	gtk_widget_set_size_request(view->image, preferences->map_width, preferences->map_height);
}

