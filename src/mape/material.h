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

#ifndef INC_MAPE_MATERIAL_H
#define INC_MAPE_MATERIAL_H

#include <glib/gerror.h>
#include <gdk/gdkcolor.h>
#include "forward.h"

/* Simple C-based interface to C4MaterialMap */

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

typedef enum MapeMaterialError_ {
	MAPE_MATERIAL_ERROR_FAILED
} MapeMaterialError;

typedef void MapeMaterial;

struct MapeMaterialMap_ {
	void* handle;
};

MapeMaterialMap* mape_material_map_new(MapeGroup* base,
                                       MapeGroup* overload_from,
                                       GError** error);
void mape_material_map_destroy(MapeMaterialMap* map);

unsigned int mape_material_map_get_material_count(MapeMaterialMap* map);
MapeMaterial* mape_material_map_get_material(MapeMaterialMap* map,
                                             unsigned int index);
MapeMaterial* mape_material_map_get_material_by_string(MapeMaterialMap* map,
                                                       const char* mat_name);

const char* mape_material_get_name(MapeMaterial* material);
GdkColor mape_material_get_color(MapeMaterial* material);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_MATERIAL_H */
