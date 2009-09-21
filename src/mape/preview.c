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
#include <gtk/gtkimage.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkeventbox.h>
#include "preview.h"
#include "preferences.h"

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
}

void mape_pre_view_apply_preferences(MapePreView* view,
                                      MapePreferences* preferences)
{
	gtk_widget_set_size_request(view->image, preferences->map_width, preferences->map_height);
}

