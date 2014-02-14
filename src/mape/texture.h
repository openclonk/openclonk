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

#ifndef INC_MAPE_TEXTURE_MAP_H
#define INC_MAPE_TEXTURE_MAP_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mape/group.h"

G_BEGIN_DECLS

#define MAPE_TYPE_TEXTURE_MAP                 (mape_texture_map_get_type())
#define MAPE_TEXTURE_MAP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), MAPE_TYPE_TEXTURE_MAP, MapeTextureMap))
#define MAPE_TEXTURE_MAP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), MAPE_TYPE_TEXTURE_MAP, MapeTextureMapClass))
#define MAPE_IS_TEXTURE_MAP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), MAPE_TYPE_TEXTURE_MAP))
#define MAPE_IS_TEXTURE_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), MAPE_TYPE_TEXTURE_MAP))
#define MAPE_TEXTURE_MAP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), MAPE_TYPE_TEXTURE_MAP, MapeTextureMapClass))

typedef struct _MapeTextureMap MapeTextureMap;
typedef struct _MapeTextureMapClass MapeTextureMapClass;

/**
 * MapeTextureMapError:
 * @MAPE_TEXTURE_MAP_ERROR_LOAD: An error occured when loading the texture map.
 *
 * These errors are from the MAPE_TEXTURE_MAP_ERROR error domain. They can
 * occur when operating on texture maps.
 */
typedef enum _MapeTextureMapError {
  MAPE_TEXTURE_MAP_ERROR_LOAD
} MapeTextureMapError;

/**
 * MapeTextureMapClass:
 *
 * This structure does not contain any public fields.
 */
struct _MapeTextureMapClass {
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * MapeTextureMap:
 *
 * #MapeTextureMap is an opaque data type. You should only access it via the
 * public API functions.
 */
struct _MapeTextureMap {
  /*< private >*/
  GObject parent;
};

GType
mape_texture_map_get_type(void) G_GNUC_CONST;

MapeTextureMap*
mape_texture_map_new(void);

gboolean
mape_texture_map_load_map(MapeTextureMap* texture_map,
                          MapeGroup* group,
                          GError** error);

gboolean
mape_texture_map_load_textures(MapeTextureMap* texture_map,
                               MapeGroup* group,
                               GError** error);

gboolean
mape_texture_map_get_overload_materials(MapeTextureMap* texture_map);

gboolean
mape_texture_map_get_overload_textures(MapeTextureMap* texture_map);

guint
mape_texture_map_get_texture_count(MapeTextureMap* texture_map);

const gchar*
mape_texture_map_get_texture_name(MapeTextureMap* texture_map,
                                  guint index);

GdkPixbuf*
mape_texture_map_lookup_texture(MapeTextureMap* texture_map,
                                const gchar* name);

const gchar*
mape_texture_map_get_material_name_from_mapping(MapeTextureMap* texture_map,
                                                guint index);

const gchar*
mape_texture_map_get_texture_name_from_mapping(MapeTextureMap* texture_map,
                                               guint index);

guint32
mape_texture_map_get_average_texture_color(MapeTextureMap* texture_map,
                                           const gchar* name);

G_END_DECLS

#endif /* INC_MAPE_TEXTURE_MAP_H */

/* vim:set et sw=2 ts=2: */
