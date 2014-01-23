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

#include <string.h>

#include "mape/cpp-handles/group-handle.h"
#include "mape/cpp-handles/texture-handle.h"
#include "mape/texture.h"

/* Declare private API */
C4GroupHandle*
_mape_group_get_handle(MapeGroup* group);

C4TextureMapHandle*
_mape_texture_map_get_handle(MapeTextureMap* map);

typedef struct _MapeTextureMapPrivate MapeTextureMapPrivate;
struct _MapeTextureMapPrivate {
  C4TextureMapHandle* handle;
  GHashTable* texture_table;
  gboolean overload_materials;
  gboolean overload_textures;
};

enum {
  PROP_0,

  /* read only */
  PROP_N_TEXTURES,
  PROP_OVERLOAD_MATERIALS,
  PROP_OVERLOAD_TEXTURES
};

#define MAPE_TEXTURE_MAP_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MAPE_TYPE_TEXTURE_MAP, MapeTextureMapPrivate))

static GQuark mape_texture_map_error_quark;

G_DEFINE_TYPE(MapeTextureMap, mape_texture_map, G_TYPE_OBJECT)

/*
 * Private helper functions
 */
static guint32
mape_texture_map_get_average_color(GdkPixbuf* pixbuf)
{
  int width;
  int height;
  int bpp;
  const guchar* pixels;
  int x, y;
  int rowstride;
  unsigned int accum[4];
  double size;

  width = gdk_pixbuf_get_width(pixbuf);
  height = gdk_pixbuf_get_height(pixbuf);
  bpp = gdk_pixbuf_get_n_channels(pixbuf);
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  accum[0] = accum[1] = accum[2] = accum[3] = 0;

  for(y = 0; y < height; ++y)
  {
    for(x = 0; x < width; ++x)
    {
      accum[0] += (int)pixels[bpp*x + y*rowstride + 0];
      accum[1] += (int)pixels[bpp*x + y*rowstride + 1];
      accum[2] += (int)pixels[bpp*x + y*rowstride + 2];
      if(bpp >= 4)
        accum[3] += (int)pixels[bpp*x + y*rowstride + 3];
    }
  }

  size = width * height;
  accum[0] = (unsigned int)(accum[0] / size + 0.5);
  accum[1] = (unsigned int)(accum[1] / size + 0.5);
  accum[2] = (unsigned int)(accum[2] / size + 0.5);
  accum[3] = (unsigned int)(accum[3] / size + 0.5);
  return accum[0] | (accum[1] << 8) | (accum[2] << 16) | (accum[3] << 24);
}

/*
 * GObject overrides.
 */

static void
mape_texture_map_init(MapeTextureMap* texture_map)
{
  MapeTextureMapPrivate* priv;
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);

  priv->handle = c4_texture_map_handle_new();
  priv->texture_table = NULL;
  priv->overload_materials = FALSE;
  priv->overload_textures = FALSE;
}

static void
mape_texture_map_finalize(GObject* object)
{
  MapeTextureMap* texture_map;
  MapeTextureMapPrivate* priv;

  texture_map = MAPE_TEXTURE_MAP(object);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);

  if(priv->texture_table != NULL)
    g_hash_table_destroy(priv->texture_table);
  c4_texture_map_handle_free(priv->handle);

  G_OBJECT_CLASS(mape_texture_map_parent_class)->finalize(object);
}

static void
mape_texture_map_set_property(GObject* object,
                              guint prop_id,
                              const GValue* value,
                              GParamSpec* pspec)
{
  switch(prop_id)
  {
    /* we have only readonly properties */
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(value, prop_id, pspec);
    break;
  }
}

static void
mape_texture_map_get_property(GObject* object,
                              guint prop_id,
                              GValue* value,
                              GParamSpec* pspec)
{
  MapeTextureMap* texture_map;
  MapeTextureMapPrivate* priv;

  texture_map = MAPE_TEXTURE_MAP(object);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);

  switch(prop_id)
  {
  case PROP_N_TEXTURES:
    if(priv->texture_table != NULL)
      g_value_set_uint(value, g_hash_table_size(priv->texture_table));
    else
      g_value_set_uint(value, 0u);
    break;
  case PROP_OVERLOAD_MATERIALS:
    g_value_set_boolean(value, priv->overload_materials);
    break;
  case PROP_OVERLOAD_TEXTURES:
    g_value_set_boolean(value, priv->overload_textures);
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
mape_texture_map_class_init(MapeTextureMapClass *class)
{
  GObjectClass* object_class;

  object_class = G_OBJECT_CLASS(class);
  mape_texture_map_parent_class =
    G_OBJECT_CLASS(g_type_class_peek_parent(class));
  g_type_class_add_private(class, sizeof(MapeTextureMapPrivate));

  object_class->finalize = mape_texture_map_finalize;
  object_class->set_property = mape_texture_map_set_property;
  object_class->get_property = mape_texture_map_get_property;

  mape_texture_map_error_quark =
    g_quark_from_static_string("MAPE_TEXTURE_MAP_ERROR");

  g_object_class_install_property(
    object_class,
    PROP_N_TEXTURES,
    g_param_spec_uint(
      "n-textures",
      "Texture count",
      "The number of loaded textures",
      0,
      G_MAXUINT,
      0,
      G_PARAM_READABLE
    )
  );

  g_object_class_install_property(
    object_class,
    PROP_OVERLOAD_MATERIALS,
    g_param_spec_boolean(
      "overload-materials",
      "Overload Materials",
      "Whether to overload globally loaded materials",
      FALSE,
      G_PARAM_READABLE
    )
  );

  g_object_class_install_property(
    object_class,
    PROP_OVERLOAD_MATERIALS,
    g_param_spec_boolean(
      "overload-textures",
      "Overload Textures",
      "Whether to overload globally loaded textures",
      FALSE,
      G_PARAM_READABLE
    )
  );
}

/*
 * Public API.
 */

/**
 * mape_texture_map_new:
 *
 * Creates a new #MapeTextureMap. The map is initially empty. Use
 * mape_texture_map_load_map() to load mappings and
 * mape_texture_map_load_textures() from a Material.ocg group file.
 *
 * Return Value: A new #MapeTextureMap. Free with g_object_unref().
 **/
MapeTextureMap*
mape_texture_map_new(void)
{
  return MAPE_TEXTURE_MAP(g_object_new(MAPE_TYPE_TEXTURE_MAP, NULL));
}

/**
 * mape_texture_map_load_map:
 * @texture_map: A #MapeTextureMap.
 * @group: A #MapeGroup to load the material mappings from.
 * @error: Location to store error information, if any.
 *
 * Loads the mappings of material-texture combinations to palette indices
 * from the group's TexMap.txt. This should only be loaded once per group.
 * If an error occurs, @error is set and the function returns %FALSE.
 *
 * Returns: %TRUE if the mapping could be loaded or %FALSE otherwise.
 */
gboolean
mape_texture_map_load_map(MapeTextureMap* texture_map,
                          MapeGroup* group,
                          GError** error)
{
  MapeTextureMapPrivate* priv;

  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), FALSE);
  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);

  c4_texture_map_handle_load_map(
    priv->handle,
    _mape_group_get_handle(group),
    "TexMap.txt",
    &priv->overload_materials,
    &priv->overload_textures
  );

  g_object_notify(G_OBJECT(texture_map), "overload-materials");
  g_object_notify(G_OBJECT(texture_map), "overload-textures");

  return TRUE;
}

/**
 * mape_texture_map_load_textures:
 * @texture_map: A #MapeTextureMap.
 * @group: The group to load textures from.
 * @error: Location to store error information, if any.
 *
 * Loads all textures in @group. If this includes textures which are already
 * contained in @texture_map, then these textures will not be loaded. If an
 * error occurs, @error is set and the function returns %FALSE.
 *
 * Returns: %TRUE if the textures could be loaded or %FALSE otherwise.
 */
gboolean
mape_texture_map_load_textures(MapeTextureMap* texture_map,
                               MapeGroup* group,
                               GError** error)
{
  /* We don't use C4TextureMap::LoadTextures here because that would try
   * to load them as C4Surface. Instead, we have our own routine which loads
   * them into GdkPixbufs. */
  static const char* SUFFIXES[] = {
    ".png", ".jpg", ".jpeg", ".bmp",
    ".PNG", ".JPG", ".JPEG", ".BMP",
    NULL
  };

  MapeTextureMapPrivate* priv;

  gchar* name;
  const char* const* ext;
  gsize len;
  gchar* casefold_name;

  guchar* data;
  gsize datalen;
  GdkPixbufLoader* loader;
  GdkPixbuf* pixbuf;

  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), FALSE);
  g_return_val_if_fail(MAPE_IS_GROUP(group), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);

  if(priv->texture_table == NULL)
  {
    priv->texture_table = g_hash_table_new_full(
      g_str_hash,
      g_str_equal,
      g_free,
      g_object_unref
    );
  }

  mape_group_rewind(group);
  while( (name = mape_group_get_next_entry(group)) != NULL)
  {
    for(ext = SUFFIXES; *ext != NULL; ++ext)
      if(g_str_has_suffix(name, *ext))
        break;

    if(*ext != NULL)
    {
      /* Make texname out of filename */
      len = strlen(name)-strlen(*ext);
      /* Use this for hashtable lookup */
      casefold_name = g_utf8_casefold(name, len);

      /* If we have this texture loaded already then don't load it again --
       * this is how overloading works: We first load the primary group and
       * then the overloaded one. */
      if(g_hash_table_lookup(priv->texture_table, casefold_name) == NULL)
      {
        data = mape_group_load_entry(group, name, &datalen, error);
        if(data == NULL)
        {
          g_free(name);
          return FALSE;
        }

        loader = gdk_pixbuf_loader_new();
        gdk_pixbuf_loader_set_size(loader, 24, 24);
        if(!gdk_pixbuf_loader_write(loader, data, datalen, error))
        {
          g_free(name);
          g_free(data);
          gdk_pixbuf_loader_close(loader, NULL);
          g_object_unref(loader);
          return FALSE;
        }

        g_free(data);
        if(!gdk_pixbuf_loader_close(loader, error))
        {
          g_free(name);
          g_object_unref(loader);
          return FALSE;
        }

        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        g_object_ref(pixbuf);
        g_object_unref(loader);

        /* Add texture to texmap (without actual Surface, only texture color),
         * just so that the map generator knows the presence
         * of the texture. */
        name[len] = '\0';
        c4_texture_map_handle_add_texture(
          priv->handle,
          name,
          mape_texture_map_get_average_color(pixbuf)
        );
        g_hash_table_insert(priv->texture_table, casefold_name, pixbuf);
      }
      else
      {
        g_free(casefold_name);
      }
    }

    g_free(name);
  }

  return TRUE;
}

/**
 * mape_texture_map_get_overload_materials:
 * @texture_map: A #MapeTextureMap.
 *
 * Returns whether the OverloadMaterials flag is set in the mapping. This
 * flag is initialized by calling mape_texture_map_load_map().
 *
 * Returns: Whether the OverloadMaterials flag is set in the mapping.
 */
gboolean
mape_texture_map_get_overload_materials(MapeTextureMap* texture_map)
{
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), FALSE);
  return MAPE_TEXTURE_MAP_PRIVATE(texture_map)->overload_materials;
}

/**
 * mape_texture_map_get_overload_textures:
 * @texture_map: A #MapeTextureMap.
 *
 * Returns whether the OverloadTextures flag is set in the mapping. This
 * flag is initialized by calling mape_texture_map_load_map().
 *
 * Returns: Whether the OverloadTextures flag is set in the mapping.
 */
gboolean
mape_texture_map_get_overload_textures(MapeTextureMap* texture_map)
{
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), FALSE);
  return MAPE_TEXTURE_MAP_PRIVATE(texture_map)->overload_materials;
}

/**
 * mape_texture_map_get_texture_count:
 * @texture_map: A #MapeTextureMap.
 *
 * Returns the number of textures loaded. This will only be non-zero after
 * mape_texture_map_load_textures() was called.
 *
 * Returns: The number of textures loaded.
 */
guint
mape_texture_map_get_texture_count(MapeTextureMap* texture_map)
{
  MapeTextureMapPrivate* priv;
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), FALSE);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  return g_hash_table_size(priv->texture_table);
}

/**
 * mape_texture_map_get_texture_name:
 * @texture_map: A #MapeTextureMap.
 * @index: The index of the texture whose name to retrieve.
 *
 * Returns the name of the @index<-- -->th material in @texture_map. Allowed
 * values for @index range from 0 to mape_texture_map_get_texture_count() - 1.
 *
 * Returns: The texture name of the texture corresponding to @index.
 */
const gchar*
mape_texture_map_get_texture_name(MapeTextureMap* texture_map,
                                  guint index)
{
  MapeTextureMapPrivate* priv;

  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), NULL);
  g_return_val_if_fail(
    index < mape_texture_map_get_texture_count(texture_map), NULL
  );

  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  return c4_texture_map_handle_get_texture(priv->handle, index);
}

/**
 * mape_texture_map_lookup_texture:
 * @texture_map: A #MapeTextureMap.
 * @name: The name of the texture to lookup.
 *
 * Looks up the graphics for the texture with name @name. If there is no such
 * texture available in @texture_map then the function returns %FALSE.
 *
 * Returns: A #GdkPixbuf corresponding to the texture with the given name.
 * This is owned by the texture map and should not be unrefed or freed.
 */
GdkPixbuf*
mape_texture_map_lookup_texture(MapeTextureMap* texture_map,
                                const gchar* name)
{
  MapeTextureMapPrivate* priv;
  gchar* casefold_name;
  GdkPixbuf* pixbuf;

  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), NULL);
  g_return_val_if_fail(name != NULL, NULL);

  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  casefold_name = g_utf8_casefold(name, -1);
  pixbuf = GDK_PIXBUF(g_hash_table_lookup(priv->texture_table, casefold_name));
  g_free(casefold_name);

  return pixbuf;
}

/**
 * mape_texture_map_get_material_name_from_mapping:
 * @texture_map: A #MapeTextureMap.
 * @index: A material-texture combination index.
 *
 * Returns the name of the material which corresponds to the given
 * index in the material-texture mapping. The function returns %NULL if the
 * corresponding entry is empty. The mapping must be loaded with the function
 * mape_texture_map_load_map() before this function can be used.
 *
 * Returns: The name of the material which corresponds to the given index,
 * or %NULL.
 */
const gchar*
mape_texture_map_get_material_name_from_mapping(MapeTextureMap* texture_map,
                                                guint index)
{
  MapeTextureMapPrivate* priv;
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), NULL);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  return c4_texture_handle_get_entry_material_name(priv->handle, index);
}

/**
 * mape_texture_map_get_texture_name_from_mapping:
 * @texture_map: A #MapeTextureMap.
 * @index: A material-texture combination index.
 *
 * Returns the name of the texture which corresponds to the given
 * index in the material-texture mapping. The function returns %NULL if the
 * corresponding entry is empty. The mapping must be loaded with the function
 * mape_texture_map_load_map() before this function can be used.
 *
 * Returns: The name of the texture which corresponds to the given index,
 * or %NULL.
 */
const gchar*
mape_texture_map_get_texture_name_from_mapping(MapeTextureMap* texture_map,
                                               guint index)
{
  MapeTextureMapPrivate* priv;
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), NULL);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  return c4_texture_handle_get_entry_texture_name(priv->handle, index);
}

/**
 * mape_texture_map_get_average_texture_color:
 * @texture_map: A #MapeTextureMap.
 * @name: The name of the texture whose color to retrieve.
 *
 * Returns the average color of the texture with name @name. The color is
 * returned in 32-bit ABGR format.
 *
 * Returns: The average texture color of the texture with name @name as a
 * 32-bit integer.
 */
guint32
mape_texture_map_get_average_texture_color(MapeTextureMap* texture_map,
                                           const gchar* name)
{
  MapeTextureMapPrivate* priv;
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(texture_map), 0);
  priv = MAPE_TEXTURE_MAP_PRIVATE(texture_map);
  return c4_texture_handle_get_average_texture_color(priv->handle, name);
}

/* This function is for internal use only */
C4TextureMapHandle*
_mape_texture_map_get_handle(MapeTextureMap* map)
{
  g_return_val_if_fail(MAPE_IS_TEXTURE_MAP(map), NULL);
  return MAPE_TEXTURE_MAP_PRIVATE(map)->handle;
}

/* vim:set et sw=2 ts=2: */
