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

#ifndef INC_MAPE_C4_MAPGEN_HANDLE_H
#define INC_MAPE_C4_MAPGEN_HANDLE_H

#include <glib.h>

#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"

G_BEGIN_DECLS

typedef struct _C4MapgenHandle C4MapgenHandle;

void c4_mapgen_handle_init_script_engine();
void c4_mapgen_handle_deinit_script_engine();
void c4_mapgen_handle_set_map_library(C4GroupHandle* group_handle);

C4MapgenHandle* c4_mapgen_handle_new_script(const char* filename, const char* source, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height);
C4MapgenHandle* c4_mapgen_handle_new(const char* filename, const char* source, const char* script_path, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height);
void c4_mapgen_handle_free(C4MapgenHandle* mapgen);

const unsigned char* c4_mapgen_handle_get_map(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_width(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_height(C4MapgenHandle* mapgen);
unsigned int c4_mapgen_handle_get_rowstride(C4MapgenHandle* mapgen);
const char* c4_mapgen_handle_get_error(C4MapgenHandle* mapgen);

G_END_DECLS

#endif /* INC_MAPE_C4_MAPGEN_HANDLE_H */
