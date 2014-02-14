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

#ifndef INC_MAPE_MATERIAL_MAP_H
#define INC_MAPE_MATERIAL_MAP_H

#include <glib-object.h>

#include "mape/group.h"
#include "mape/texture.h"

G_BEGIN_DECLS

#define MAPE_TYPE_MATERIAL_MAP                 (mape_material_map_get_type())
#define MAPE_MATERIAL_MAP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), MAPE_TYPE_MATERIAL_MAP, MapeMaterialMap))
#define MAPE_MATERIAL_MAP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), MAPE_TYPE_MATERIAL_MAP, MapeMaterialMapClass))
#define MAPE_IS_MATERIAL_MAP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAPE_TYPE_MATERIAL_MAP))
#define MAPE_IS_MATERIAL_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), MAPE_TYPE_MATERIAL_MAP))
#define MAPE_MATERIAL_MAP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), MAPE_TYPE_MATERIAL_MAP, MapeMaterialMapClass))

#define MAPE_TYPE_MATERIAL                     (mape_material_get_type())

typedef struct _MapeMaterial MapeMaterial;

typedef struct _MapeMaterialMap MapeMaterialMap;
typedef struct _MapeMaterialMapClass MapeMaterialMapClass;

/**
 * MapeMaterialMapError:
 * @MAPE_MATERIAL_MAP_ERROR_LOAD: An error occured when loading a material map.
 *
 * These errors are from the MAPE_MATERIAL_MAP_ERROR error domain. They can
 * occur when operating on material maps.
 */
typedef enum _MapeMaterialMapError {
  MAPE_MATERIAL_MAP_ERROR_LOAD
} MapeMaterialMapError;

/**
 * MapeMaterialMapClass:
 *
 * This structure does not contain any public fields.
 */
struct _MapeMaterialMapClass {
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * MapeMaterialMap:
 *
 * #MapeMaterialMap is an opaque data type. You should only access it via the
 * public API functions.
 */
struct _MapeMaterialMap {
  /*< private >*/
  GObject parent;
};

GType
mape_material_get_type(void) G_GNUC_CONST;

GType
mape_material_map_get_type(void) G_GNUC_CONST;

MapeMaterialMap*
mape_material_map_new(void);

gboolean
mape_material_map_load(MapeMaterialMap* map,
                       MapeGroup* from,
                       GError** error);

guint
mape_material_map_get_material_count(MapeMaterialMap* map);

void
mape_material_map_set_default_textures(MapeMaterialMap* matmap,
                                       MapeTextureMap* texmap);

const MapeMaterial*
mape_material_map_get_material(MapeMaterialMap* map,
                               guint index);

#if 0
MapeMaterial*
mape_material_map_get_material_by_name(MapeMaterialMap* map,
                                       const gchar* name);
#endif

const gchar*
mape_material_get_name(const MapeMaterial* material);

const gchar*
mape_material_get_texture_overlay(const MapeMaterial* material);

G_END_DECLS

#endif /* INC_MAPE_MATERIAL_MAP_H */

/* vim:set et sw=2 ts=2: */
