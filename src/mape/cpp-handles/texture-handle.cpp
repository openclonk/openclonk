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
#include "C4Texture.h"
#include "mape/cpp-handles/texture-handle.h"

#define TEXTURE_MAP_TO_HANDLE(texture_map) (reinterpret_cast<C4TextureMapHandle*>(texture_map))
#define HANDLE_TO_TEXTURE_MAP(handle) (reinterpret_cast<C4TextureMap*>(handle))

#define GROUP_TO_HANDLE(group) (reinterpret_cast<C4GroupHandle*>(group))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

extern "C" {

C4TextureMapHandle* c4_texture_map_handle_new(void)
{
  return TEXTURE_MAP_TO_HANDLE(new C4TextureMap);
}

void c4_texture_map_handle_free(C4TextureMapHandle* texture_map)
{
  delete HANDLE_TO_TEXTURE_MAP(texture_map);
}


guint c4_texture_map_handle_load_map(C4TextureMapHandle* texture_map, C4GroupHandle* group, const char* entry_name, gboolean* overload_materials, gboolean* overload_textures)
{
  bool fOverloadMaterials;
  bool fOverloadTextures;
  guint32 retval = HANDLE_TO_TEXTURE_MAP(texture_map)->LoadMap(*HANDLE_TO_GROUP(group), entry_name, &fOverloadMaterials, &fOverloadTextures);
  if(overload_materials) *overload_materials = fOverloadMaterials;
  if(overload_textures) *overload_textures = fOverloadTextures;
  return retval;
}

gboolean c4_texture_map_handle_add_texture(C4TextureMapHandle* texture_map, const char* texture, guint32 avg_color)
{
  gboolean result = HANDLE_TO_TEXTURE_MAP(texture_map)->AddTexture(texture, NULL);
  if(!result) return FALSE;
  HANDLE_TO_TEXTURE_MAP(texture_map)->GetTexture(texture)->SetAverageColor(avg_color);
  return TRUE;
}

const char* c4_texture_map_handle_get_texture(C4TextureMapHandle* texture_map, guint index)
{
  return HANDLE_TO_TEXTURE_MAP(texture_map)->GetTexture(index);
}

guint32 mape_texture_handle_get_average_texture_color(C4TextureMapHandle* texture_map, const char* name)
{
  return HANDLE_TO_TEXTURE_MAP(texture_map)->GetTexture(name)->GetAverageColor();
}

const char* mape_texture_handle_get_entry_material_name(C4TextureMapHandle* texture_map, guint index)
{
  const C4TexMapEntry* entry = HANDLE_TO_TEXTURE_MAP(texture_map)->GetEntry(index);
  if(!entry) return NULL;
  return entry->GetMaterialName();
}

const char* mape_texture_handle_get_entry_texture_name(C4TextureMapHandle* texture_map, guint index)
{
  const C4TexMapEntry* entry = HANDLE_TO_TEXTURE_MAP(texture_map)->GetEntry(index);
  if(!entry) return NULL;
  return entry->GetTextureName();
}

} /* extern "C" */
