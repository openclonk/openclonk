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

#ifndef INC_MAPE_FILEICON_H
#define INC_MAPE_FILEICON_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkwidget.h>
#include "forward.h"

typedef enum MapeFileIconType_ {
	MAPE_FILE_ICON_DRIVE,
	MAPE_FILE_ICON_FOLDER,
	MAPE_FILE_ICON_C4GROUP,
	MAPE_FILE_ICON_C4SCENARIO,
	MAPE_FILE_ICON_C4OBJECT,
	MAPE_FILE_ICON_C4FOLDER,
	MAPE_FILE_ICON_C4MATERIAL,
	MAPE_FILE_ICON_C4TEXTURE,

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
