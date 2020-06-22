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

#include "C4Include.h"
#include "landscape/C4Material.h"
#include "landscape/C4Texture.h"
#include "mape/cpp-handles/material-handle.h"

#define MATERIAL_MAP_TO_HANDLE(material_map) (reinterpret_cast<C4MaterialMapHandle*>(material_map))
#define HANDLE_TO_MATERIAL_MAP(handle) (reinterpret_cast<C4MaterialMap*>(handle))

#define MATERIAL_TO_HANDLE(material) (reinterpret_cast<C4MaterialHandle*>(material))
#define HANDLE_TO_MATERIAL(handle) (reinterpret_cast<C4Material*>(handle))

#define HANDLE_TO_TEXTURE_MAP(handle) (reinterpret_cast<C4TextureMap*>(handle))

#define GROUP_TO_HANDLE(group) (reinterpret_cast<C4GroupHandle*>(group))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

extern "C" {

C4MaterialMapHandle* c4_material_map_handle_new(void)
{
  // Simply return a pointer to the global material map. This is a bit stupid,
  // but when looking up a material-texture combination, C4TextureMap uses the
  // global material map for the material lookup when the default texture is
  // requested. This should be changed to get rid of the global variable, but yeah...
  C4MaterialMap* map = &::MaterialMap;
  map->Clear();
  return MATERIAL_MAP_TO_HANDLE(map); //new C4MaterialMap);
}

void c4_material_map_handle_free(C4MaterialMapHandle* material_map)
{
  //delete HANDLE_TO_MATERIAL_MAP(material_map);
}

guint c4_material_map_handle_load(C4MaterialMapHandle* material_map, C4GroupHandle* group)
{
  return HANDLE_TO_MATERIAL_MAP(material_map)->Load(*HANDLE_TO_GROUP(group));
}

void c4_material_map_crossmap_materials(C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map)
{
  // Don't call the official crossmap function since it tries to load some surfaces.
  // All we need is the default texture.
  for(int i = 0; i < HANDLE_TO_MATERIAL_MAP(material_map)->Num; ++i)
  {
    C4Material* mat = &HANDLE_TO_MATERIAL_MAP(material_map)->Map[i];
    const char* overlay = mat->sTextureOverlay.getData();
    if(!overlay || *overlay == '\0')
    {
      int first_tex_map_entry = HANDLE_TO_TEXTURE_MAP(texture_map)->GetIndex(mat->Name, nullptr, false);
      overlay = HANDLE_TO_TEXTURE_MAP(texture_map)->GetEntry(first_tex_map_entry)->GetTextureName();
    }

    if(overlay)
      mat->DefaultMatTex = HANDLE_TO_TEXTURE_MAP(texture_map)->GetIndex(mat->Name, overlay, true);
  }
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
