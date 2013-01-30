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

#ifndef INC_MAPE_C4_MAPGEN_HANDLE_H
#define INC_MAPE_C4_MAPGEN_HANDLE_H

#include <glib.h>

#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"

G_BEGIN_DECLS

typedef struct _C4MapgenHandle C4MapgenHandle;

C4MapgenHandle* c4_mapgen_handle_new(const char* filename, const char* source, const char* script_path, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height);
void c4_mapgen_handle_free(C4MapgenHandle* mapgen);

const unsigned char* c4_mapgen_handle_get_map(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_width(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_height(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_rowstride(C4MapgenHandle* mapgen);
const char* c4_mapgen_handle_get_error(C4MapgenHandle* mapgen);

G_END_DECLS

#endif /* INC_MAPE_C4_MAPGEN_HANDLE_H */
