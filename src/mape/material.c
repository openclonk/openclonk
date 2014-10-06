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

/**
 * SECTION:mape-material-map
 * @title: MapeMaterialMap
 * @short_description: C4MaterialMap interface
 * @include: mape/material.h
 * @stability: Unstable
 *
 * #MapeMaterialMap is a simple GObject-based interface to C4MaterialMap.
 * It supports loading a material map from a Material.ocg group file. It can
 * load multiple files, with newer entries overloading previous ones in case
 * of name clashes to support material overloading.
 **/

#include "mape/cpp-handles/group-handle.h"
#include "mape/cpp-handles/material-handle.h"
#include "mape/material.h"

/* Declare private API */
C4GroupHandle*
_mape_group_get_handle(MapeGroup* group);

C4MaterialMapHandle*
_mape_material_map_get_handle(MapeMaterialMap* map);

C4TextureMapHandle*
_mape_texture_map_get_handle(MapeTextureMap* map);

typedef struct _MapeMaterialMapPrivate MapeMaterialMapPrivate;
struct _MapeMaterialMapPrivate {
  C4MaterialMapHandle* handle;
};

enum {
  PROP_0,

  /* read only */
  PROP_N_MATERIALS
};

#define MAPE_MATERIAL_MAP_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MAPE_TYPE_MATERIAL_MAP, MapeMaterialMapPrivate))

static GQuark mape_material_map_error_quark;

G_DEFINE_TYPE(MapeMaterialMap, mape_material_map, G_TYPE_OBJECT)

/*
 * MapeMaterial
 */

static MapeMaterial*
mape_material_new(MapeMaterialMap* map,
                  guint mat_index)
{
  MapeMaterialMapPrivate* priv;
  priv = MAPE_MATERIAL_MAP_PRIVATE(map);

  return (MapeMaterial*)c4_material_map_handle_get_material(priv->handle,
                                                            mat_index);
}

static MapeMaterial*
mape_material_copy(const MapeMaterial* material)
{
  return (MapeMaterial*)material;
}

static void
mape_material_free(MapeMaterial* material)
{
}

/*
 * GObject overrides.
 */

static void
mape_material_map_init(MapeMaterialMap* material_map)
{
  MapeMaterialMapPrivate* priv;
  priv = MAPE_MATERIAL_MAP_PRIVATE(material_map);

  priv->handle = c4_material_map_handle_new();
}

static void
mape_material_map_finalize(GObject* object)
{
  MapeMaterialMap* material_map;
  MapeMaterialMapPrivate* priv;

  material_map = MAPE_MATERIAL_MAP(object);
  priv = MAPE_MATERIAL_MAP_PRIVATE(material_map);

  c4_material_map_handle_free(priv->handle);
  G_OBJECT_CLASS(mape_material_map_parent_class)->finalize(object);
}

static void
mape_material_map_set_property(GObject* object,
                               guint prop_id,
                               const GValue* value,
                               GParamSpec* pspec)
{
  MapeMaterialMap* material_map;
  MapeMaterialMapPrivate* priv;

  material_map = MAPE_MATERIAL_MAP(object);
  priv = MAPE_MATERIAL_MAP_PRIVATE(material_map);

  switch(prop_id)
  {
    /* we have only readonly properties */
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(value, prop_id, pspec);
    break;
  }
}

static void
mape_material_map_get_property(GObject* object,
                               guint prop_id,
                               GValue* value,
                               GParamSpec* pspec)
{
  MapeMaterialMap* material_map;
  MapeMaterialMapPrivate* priv;

  material_map = MAPE_MATERIAL_MAP(object);
  priv = MAPE_MATERIAL_MAP_PRIVATE(material_map);

  switch(prop_id)
  {
  case PROP_N_MATERIALS:
    g_value_set_uint(value, c4_material_map_handle_get_num(priv->handle));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/*
 * Gype registration.
 */

static void
mape_material_map_class_init(MapeMaterialMapClass *class)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS(class);
  mape_material_map_parent_class =
    G_OBJECT_CLASS(g_type_class_peek_parent(class));
  g_type_class_add_private(class, sizeof(MapeMaterialMapPrivate));

  object_class->finalize = mape_material_map_finalize;
  object_class->set_property = mape_material_map_set_property;
  object_class->get_property = mape_material_map_get_property;

  mape_material_map_error_quark =
    g_quark_from_static_string("MAPE_MATERIAL_MAP_ERROR");

  g_object_class_install_property(
    object_class,
    PROP_N_MATERIALS,
    g_param_spec_uint(
      "n-materials",
      "Material count",
      "The number of loaded materials",
      0,
      G_MAXUINT,
      0,
      G_PARAM_READABLE
    )
  );
}

GType
mape_material_get_type(void)
{
  static GType material_type = 0;

  if(material_type == 0)
  {
    material_type = g_boxed_type_register_static(
      "MapeMaterial",
      (GBoxedCopyFunc)mape_material_copy,
      (GBoxedFreeFunc)mape_material_free);
  }

  return material_type;
}

/*
 * Public API.
 */

/**
 * mape_material_map_new:
 *
 * Creates a new #MapeMaterialMap. The map is initially empty. Use
 * mape_material_map_load() to load materials from one or more Material.ocg
 * group files.
 *
 * Return Value: A new #MapeMaterialMap. Free with g_object_unref().
 **/
MapeMaterialMap*
mape_material_map_new(void)
{
  return MAPE_MATERIAL_MAP(g_object_new(MAPE_TYPE_MATERIAL_MAP, NULL));
}

/**
 * mape_material_map_load:
 * @map: A #MapeMaterialMap.
 * @from: An open #MapeGroup to load materials from.
 * @error: Location to store error information, if any.
 *
 * Loads all the material files (*.c4m) from the group @from. If this includes
 * materials with the same name as materials already contained in @map, then
 * the new materials will not be loaded. If an error occurs while loading the
 * material map the function returns %FALSE and @error is set.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
gboolean
mape_material_map_load(MapeMaterialMap* map,
                       MapeGroup* from,
                       GError** error)
{
  MapeMaterialMapPrivate* priv;
  guint new_count;

  g_return_val_if_fail(MAPE_IS_MATERIAL_MAP(map), FALSE);
  g_return_val_if_fail(MAPE_IS_GROUP(from), FALSE);
  g_return_val_if_fail(mape_group_is_open(from), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

  priv = MAPE_MATERIAL_MAP_PRIVATE(map);

  new_count = c4_material_map_handle_load(
    priv->handle, _mape_group_get_handle(from));

  if(new_count > 0)
    g_object_notify(G_OBJECT(map), "n-materials");

  return TRUE;
}

/**
 * mape_material_map_get_material_count:
 * @map: A #MapeMaterialMap.
 *
 * Returns the number of materials contained in @map.
 *
 * Return Value: The number of materials in @map.
 **/
guint
mape_material_map_get_material_count(MapeMaterialMap* map)
{
  g_return_val_if_fail(MAPE_IS_MATERIAL_MAP(map), 0);
  return c4_material_map_handle_get_num(
    MAPE_MATERIAL_MAP_PRIVATE(map)->handle);
}

/**
 * mape_material_map_set_default_textures:
 * @mapmap: A #MapeMaterialMap.
 * @texmap: A #MapeTextureMap to load textures from.
 *
 * Sets the default textures for the materials in @matmap by looking up the
 * texture overlay specified in the material file in @texmap.
 **/
void
mape_material_map_set_default_textures(MapeMaterialMap* matmap,
                                       MapeTextureMap* texmap)
{
  c4_material_map_crossmap_materials(
    MAPE_MATERIAL_MAP_PRIVATE(matmap)->handle,
    _mape_texture_map_get_handle(texmap)
  );
}

/**
 * mape_material_map_get_material:
 * @map: A #MapeMaterialMap.
 * @index: A material index.
 *
 * Returns the entry with the given index in the map.
 *
 * Returns: A #MapeMaterial representing the indexed material. It is owned
 * by #map and must not be used anymore after the map is finalized.
 **/
const MapeMaterial*
mape_material_map_get_material(MapeMaterialMap* map,
                               guint index)
{
  MapeMaterialMapPrivate* priv;

  g_return_val_if_fail(MAPE_IS_MATERIAL_MAP(map), NULL);

  priv = MAPE_MATERIAL_MAP_PRIVATE(map);
  g_return_val_if_fail(index < c4_material_map_handle_get_num(priv->handle),
                       NULL);

  return mape_material_new(map, index);
}

/*
 * mape_material_map_get_material_by_name:
 * @map: A #MapeMaterialMap.
 * @name: The name of the material to retrieve.
 *
 * Returns the material in the map which has the given name, if any. If there
 * is no such material the function returns %NULL.
 *
 * Returns: A new #MapeMaterial to be freed with mape_material_free() when
 * no longer needed, or %NULL.
 */
MapeMaterial*
mape_material_map_get_material_by_name(MapeMaterialMap* map,
                                       const gchar* name)
{
  MapeMaterialMapPrivate* priv;
  const gchar* cur_name;
  guint i;

  g_return_val_if_fail(MAPE_IS_MATERIAL_MAP(map), NULL);
  g_return_val_if_fail(name != NULL, NULL);

  priv = MAPE_MATERIAL_MAP_PRIVATE(map);

  for(i = 0; i < c4_material_map_handle_get_num(priv->handle); ++i)
  {
    cur_name = c4_material_handle_get_name(
      c4_material_map_handle_get_material(priv->handle, i)
    );

    if(g_ascii_strcasecmp(cur_name, name) == 0)
      return mape_material_new(map, i);
  }

  return NULL;
}

/**
 * mape_material_get_name:
 * @material: A #MapeMaterial.
 *
 * Returns the material's name.
 *
 * Return Value: The name of the material. The string is owned by the
 * #MapeMaterial and must not be freed by the user.
 */
const gchar*
mape_material_get_name(const MapeMaterial* material)
{
  g_return_val_if_fail(material != NULL, NULL);
  return c4_material_handle_get_name((C4MaterialHandle*)material);
}

/**
 * mape_material_get_texture_overlay:
 * @material: A #MapeMaterial.
 *
 * Returns the material's texture overlay as a string. This can be used to
 * make a texture lookup in a corresponding #MapeTextureMap.
 *
 * Return Value: The texture overlay of the material. The string is owned by
 * the #MapeMaterial and must not be freed by the user.
 */
const gchar*
mape_material_get_texture_overlay(const MapeMaterial* material)
{
  g_return_val_if_fail(material != NULL, NULL);
  return c4_material_handle_get_texture_overlay((C4MaterialHandle*)material);
}

/* This function is for internal use only */
C4MaterialMapHandle*
_mape_material_map_get_handle(MapeMaterialMap* map)
{
  g_return_val_if_fail(MAPE_IS_MATERIAL_MAP(map), NULL);
  return MAPE_MATERIAL_MAP_PRIVATE(map)->handle;
}

/* vim:set et sw=2 ts=2: */
