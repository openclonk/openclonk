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

#ifndef INC_MAPE_C4_MATERIAL_HANDLE_H
#define INC_MAPE_C4_MATERIAL_HANDLE_H

#include <glib.h>

#include "mape/cpp-handles/group-handle.h"
#include "mape/cpp-handles/texture-handle.h"

G_BEGIN_DECLS

typedef struct _C4MaterialHandle C4MaterialHandle;
typedef struct _C4MaterialMapHandle C4MaterialMapHandle;

C4MaterialMapHandle* c4_material_map_handle_new(void);
void c4_material_map_handle_free(C4MaterialMapHandle* material_map);

guint c4_material_map_handle_load(C4MaterialMapHandle* material_map, C4GroupHandle* group);
void c4_material_map_crossmap_materials(C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map);

guint c4_material_map_handle_get_num(C4MaterialMapHandle* material_map);
C4MaterialHandle* c4_material_map_handle_get_material(C4MaterialMapHandle* material_map, guint index);

const gchar* c4_material_handle_get_name(C4MaterialHandle* material);
const gchar* c4_material_handle_get_texture_overlay(C4MaterialHandle* material);

G_END_DECLS

#endif /* INC_MAPE_C4_MATERIAL_HANDLE_H */
