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

#include "mape/fileicon.h"
#include "mape-icons.h" // Generated file

static MapeFileIcon* mape_file_icon_new(GtkWidget* widget,
                                        MapeFileIconType type)
{
	MapeFileIcon* icon;
	GdkPixbuf* pixbuf;
	gint width, height;
	GdkPixbuf* scaled_pixbuf;

	switch(type)
	{
	case MAPE_FILE_ICON_DRIVE:
		pixbuf = gtk_widget_render_icon(
			widget,
			GTK_STOCK_HARDDISK,
			GTK_ICON_SIZE_BUTTON,
			NULL
		);
		break;
	case MAPE_FILE_ICON_FOLDER:
		pixbuf = gtk_widget_render_icon(
			widget,
			GTK_STOCK_DIRECTORY,
			GTK_ICON_SIZE_BUTTON,
			NULL
		);
		break;
	case MAPE_FILE_ICON_C4OBJECT:
		pixbuf = gdk_pixbuf_new_from_inline(
			-1,
			mape_icon_ocd,
			FALSE,
			NULL
		);
		break;
	case MAPE_FILE_ICON_C4FOLDER:
		pixbuf = gdk_pixbuf_new_from_inline(
			-1,
			mape_icon_ocf,
			FALSE,
			NULL
		);
		break;
	case MAPE_FILE_ICON_C4GROUP:
		pixbuf = gdk_pixbuf_new_from_inline(
			-1,
			mape_icon_ocg,
			FALSE,
			NULL
		);
		break;
	case MAPE_FILE_ICON_C4SCENARIO:
		pixbuf = gdk_pixbuf_new_from_inline(
			-1,
			mape_icon_ocs,
			FALSE,
			NULL
		);
		break;
	case MAPE_FILE_ICON_C4MATERIAL:
		pixbuf = gdk_pixbuf_new_from_inline(
			-1,
			mape_icon_ocm,
			FALSE,
			NULL
		);
		break;
	default:
		g_assert_not_reached();
		break;
	}

	if(pixbuf == NULL)
		return NULL;

	gtk_icon_size_lookup(GTK_ICON_SIZE_BUTTON, &width, &height);

	/* Scale pixbuf to size of GTK_ICON_SIZE_BUTTON */
	if(gdk_pixbuf_get_width(pixbuf) != width ||
	   gdk_pixbuf_get_height(pixbuf) != height)
	{
		scaled_pixbuf = gdk_pixbuf_scale_simple(
			pixbuf,
			width,
			height,
			GDK_INTERP_HYPER
		);

		g_object_unref(pixbuf);
		pixbuf = scaled_pixbuf;

		if(pixbuf == NULL)
			return NULL;
	}

	icon = malloc(sizeof(MapeFileIcon) );
	icon->type = type;
	icon->pixbuf = pixbuf;

	return icon;
}

static void mape_file_icon_destroy(MapeFileIcon* icon)
{
	g_object_unref(G_OBJECT(icon->pixbuf) );
	free(icon);
}

MapeFileIconSet* mape_file_icon_set_new(GtkWidget* widget)
{
	MapeFileIconSet* set;
	unsigned int i;

	set = malloc(sizeof(MapeFileIconSet) );

	for(i = 0; i < MAPE_FILE_ICON_COUNT; ++ i)
		set->icons[i] = mape_file_icon_new(widget, (MapeFileIconType)i);

	return set;
}

void mape_file_icon_set_destroy(MapeFileIconSet* set)
{
	unsigned int i;
	for(i = 0; i < MAPE_FILE_ICON_COUNT; ++ i)
		mape_file_icon_destroy(set->icons[i]);
}

MapeFileIcon* mape_file_icon_set_lookup(MapeFileIconSet* set,
                                        MapeFileIconType type)
{
	g_assert(type < MAPE_FILE_ICON_COUNT);

	return set->icons[type];
}

GdkPixbuf* mape_file_icon_get(MapeFileIcon* icon)
{
	return icon->pixbuf;
}
