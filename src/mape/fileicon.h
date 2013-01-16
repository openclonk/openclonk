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

#ifndef INC_MAPE_FILEICON_H
#define INC_MAPE_FILEICON_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "mape/forward.h"

typedef enum MapeFileIconType_ {
	MAPE_FILE_ICON_DRIVE,
	MAPE_FILE_ICON_FOLDER,
	MAPE_FILE_ICON_C4GROUP,
	MAPE_FILE_ICON_C4SCENARIO,
	MAPE_FILE_ICON_C4OBJECT,
	MAPE_FILE_ICON_C4FOLDER,
	MAPE_FILE_ICON_C4MATERIAL,

	MAPE_FILE_ICON_COUNT
} MapeFileIconType;

struct MapeFileIcon_ {
	MapeFileIconType type;
	GdkPixbuf* pixbuf;
};

struct MapeFileIconSet_ {
	MapeFileIcon* icons[MAPE_FILE_ICON_COUNT];
};

MapeFileIconSet* mape_file_icon_set_new(GtkWidget* widget);
void mape_file_icon_set_destroy(MapeFileIconSet* set);

MapeFileIcon* mape_file_icon_set_lookup(MapeFileIconSet* set,
                                        MapeFileIconType type);

GdkPixbuf* mape_file_icon_get(MapeFileIcon* icon);

#endif /* INC_MAPE_FILEICON_H */
