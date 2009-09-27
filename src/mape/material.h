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

#ifndef INC_MAPE_MATERIAL_H
#define INC_MAPE_MATERIAL_H

#include <gtk/gtk.h>

#include "mape/forward.h"
#include "mape/group.h"

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
