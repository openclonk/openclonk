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
