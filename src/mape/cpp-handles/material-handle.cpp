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

#include "C4Include.h"
#include "C4Material.h"
#include "mape/cpp-handles/material-handle.h"

#define MATERIAL_MAP_TO_HANDLE(material_map) (reinterpret_cast<C4MaterialMapHandle*>(material_map))
#define HANDLE_TO_MATERIAL_MAP(handle) (reinterpret_cast<C4MaterialMap*>(handle))

#define MATERIAL_TO_HANDLE(material) (reinterpret_cast<C4MaterialHandle*>(material))
#define HANDLE_TO_MATERIAL(handle) (reinterpret_cast<C4Material*>(handle))

#define GROUP_TO_HANDLE(group) (reinterpret_cast<C4GroupHandle*>(group))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

extern "C" {

C4MaterialMapHandle* c4_material_map_handle_new(void)
{
  return MATERIAL_MAP_TO_HANDLE(new C4MaterialMap);
}

void c4_material_map_handle_free(C4MaterialMapHandle* material_map)
{
  delete HANDLE_TO_MATERIAL_MAP(material_map);
}

guint c4_material_map_handle_load(C4MaterialMapHandle* material_map, C4GroupHandle* group)
{
  return HANDLE_TO_MATERIAL_MAP(material_map)->Load(*HANDLE_TO_GROUP(group));
}

guint c4_material_map_handle_get_num(C4MaterialMapHandle* material_map)
{
  return HANDLE_TO_MATERIAL_MAP(material_map)->Num;
}

C4MaterialHandle* c4_material_map_handle_get_material(C4MaterialMapHandle* material_map, guint index)
{
  g_assert(index < (guint)(HANDLE_TO_MATERIAL_MAP(material_map)->Num));
  return MATERIAL_TO_HANDLE(&HANDLE_TO_MATERIAL_MAP(material_map)->Map[index]);
}

const gchar* c4_material_handle_get_name(C4MaterialHandle* material)
{
  return HANDLE_TO_MATERIAL(material)->Name;
}
const gchar* c4_material_handle_get_texture_overlay(C4MaterialHandle* material)
{
  return HANDLE_TO_MATERIAL(material)->sTextureOverlay.getData();
}

} /* extern "C" */
