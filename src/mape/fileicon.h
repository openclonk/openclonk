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
