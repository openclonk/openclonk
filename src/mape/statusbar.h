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

#ifndef INC_MAPE_STATUSBAR_H
#define INC_MAPE_STATUSBAR_H

#include <gtk/gtk.h>
#include "mape/forward.h"

struct MapeStatusbar_ {
	GtkWidget* bar;
	
	guint context_compile;
};

MapeStatusbar* mape_statusbar_new();
void mape_statusbar_destroy(MapeStatusbar* view);

void mape_statusbar_set_compile(MapeStatusbar* bar,
                                const gchar* text);

#endif /* INC_MAPE_STATUSBAR_H */
