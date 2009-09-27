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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "mape/statusbar.h"

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
