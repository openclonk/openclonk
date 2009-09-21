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
#include <gtk/gtkstatusbar.h>
#include "statusbar.h"

MapeStatusbar* mape_statusbar_new(void)
{
	MapeStatusbar* bar;
	bar = malloc(sizeof(MapeStatusbar) );
	
	bar->bar = gtk_statusbar_new();
	bar->context_compile = gtk_statusbar_get_context_id(
		GTK_STATUSBAR(bar->bar),
		"Compiler report"
	);

	gtk_statusbar_push(
		GTK_STATUSBAR(bar->bar),
		bar->context_compile,
		"Initialized"
	);

	gtk_widget_show(bar->bar);

	return bar;
}

void mape_statusbar_destroy(MapeStatusbar* bar)
{
	free(bar);
}

void mape_statusbar_set_compile(MapeStatusbar* bar,
                                const gchar* text)
{
	gtk_statusbar_pop(GTK_STATUSBAR(bar->bar), bar->context_compile);
	gtk_statusbar_push(GTK_STATUSBAR(bar->bar), bar->context_compile, text);
}
