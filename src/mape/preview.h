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

#ifndef INC_MAPE_PREVIEW_H
#define INC_MAPE_PREVIEW_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "mape/forward.h"

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
