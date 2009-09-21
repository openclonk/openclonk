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

#ifndef INC_MAPE_MAPGEN_H
#define INC_MAPE_MAPGEN_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "material.h"
#include "texture.h"

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

typedef enum MapeMapgenError_ {
	MAPE_MAPGEN_ERROR_MEMORY,
	MAPE_MAPGEN_ERROR_COMPILE,
	MAPE_MAPGEN_ERROR_MISSING_MAP,

	MAPE_MAPGEN_ERROR_FAILED
} MapeMapgenError;

GdkPixbuf* mape_mapgen_generate(const gchar* source,
                                MapeMaterialMap* material_map,
                                MapeTextureMap* texture_map,
                                GError** error,
								unsigned int map_with,
								unsigned int map_height);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_MAPGEN_H */
