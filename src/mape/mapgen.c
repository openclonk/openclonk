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

#include "mape/cpp-handles/mapgen-handle.h"
#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"
#include "mape/mapgen.h"

/* Declare private API */
C4GroupHandle*
_mape_group_get_handle(MapeGroup* group);


C4MaterialMapHandle*
_mape_material_map_get_handle(MapeMaterialMap* map);

C4TextureMapHandle*
_mape_texture_map_get_handle(MapeTextureMap* map);

static GQuark mape_mapgen_error_quark()
{
  return g_quark_from_static_string("MAPE_MAPGEN_ERROR");
}

static void mape_mapgen_read_color(guint8* dest,
                                   MapeMaterialMap* material_map,
                                   MapeTextureMap* texture_map,
                                   unsigned int matnum)
{
  const gchar* texture_name;
  const gchar* material_name;
  const gchar* first_tex_separator;
  const MapeMaterial* material;
  gchar* own_texture_name;
  guint32 color;

  if(matnum == 0)
  {
    /* Sky */
    dest[matnum * 4 + 1] = 100;
    dest[matnum * 4 + 2] = 100;
    dest[matnum * 4 + 3] = 255;
  }
  else
  {
    texture_name = mape_texture_map_get_texture_name_from_mapping(
      texture_map,
      matnum
    );

    own_texture_name = NULL;
    if(texture_name != NULL)
    {
      /* When the texture is animated, the texture name consists of more than
       * one texture, separated with a '-' character. In this case, we simply
       * use the first one for display. */
      first_tex_separator = strchr(texture_name, '-');
      if(first_tex_separator != NULL)
      {
        own_texture_name = g_strndup(
          texture_name,
          first_tex_separator - texture_name
        );

        texture_name = own_texture_name;
      }

      /* Make sure the texture exists */
      if(!mape_texture_map_lookup_texture(texture_map, texture_name))
      {
        material_name = mape_texture_map_get_material_name_from_mapping(
          texture_map,
          matnum
        );

        material = mape_material_map_get_material_by_name(
          material_map,
          material_name
        );

        /* It can happen that the material does not exist; this happens when
         * a material-texture specification with texture set and invalid
         * material occurs, such as "E-rough". In this case we display sky,
         * since this is what happens when the texture specification is
         * omitted (in which case no entry in the texmap is created, and
         * matnum=0). */
        if(!material)
        {
          dest[matnum * 4 + 1] = 100;
          dest[matnum * 4 + 2] = 100;
          dest[matnum * 4 + 3] = 255;
          texture_name = NULL;
        }
        else
        {
          texture_name = mape_material_get_texture_overlay(material);
        }
      }
    }

    if(texture_name != NULL)
    {
      color = mape_texture_map_get_average_texture_color(
        texture_map,
        texture_name
      );

      dest[matnum * 4 + 1] = (color      ) & 0xff;
      dest[matnum * 4 + 2] = (color >>  8) & 0xff;
      dest[matnum * 4 + 3] = (color >> 16) & 0xff;
    }

    g_free(own_texture_name);
  }
}

/*
 * Public API.
 */

/**
 * mape_mapgen_init:
 * @error: Location to store error information, if any.
 *
 * Initializes the map generator.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
mape_mapgen_init(GError** error)
{
  c4_mapgen_handle_init_script_engine();
  return TRUE;
}

/**
 * mape_mapgen_deinit():
 *
 * Deinitializes the map generator.
 */
void
mape_mapgen_deinit()
{
  c4_mapgen_handle_deinit_script_engine();
}

/**
 * mape_mapgen_set_root_group:
 * @group: The root group.
 *
 * Sets the root group for the map generator. This group is used to lookup the
 * Library_Map definition.
 */
void
mape_mapgen_set_root_group(MapeGroup* group)
{
  MapeGroup* objects;
  MapeGroup* libraries;
  MapeGroup* map;
  GError* error;

  error = NULL;
  if(!error)
    objects = mape_group_open_child(group, "Objects.ocd", &error);
  if(!error)
    libraries = mape_group_open_child(objects, "Libraries.ocd", &error);
  if(!error)
    map = mape_group_open_child(libraries, "Map.ocd", &error);

  /* TODO: Error reporting? */
  if(error == NULL)
    c4_mapgen_handle_set_map_library(_mape_group_get_handle(map));

  if(error != NULL)
  {
    fprintf(stderr, "Failed to load Objects.ocd/Libraries.ocd/Map.ocd/Script.c: %s\n", error->message);
    g_error_free(error);
  }
}

/**
 * mape_mapgen_render:
 *
 * @filename: The filename of the file that is being parsed. This is only used
 * for display purposes.
 * @source: The map generator source code for the map to generate.
 * @type: Specifies how the text in @source should be interpreted. Must not be
 * %MAPE_MAPGEN_NONE.
 * @script_path: Path to the script source for algo=script overlays, or %NULL.
 * @material_map: The material map containing the materials to be used during
 * map generation.
 * @texture_map: The texture map containing the textures to be used during map
 * generation.
 * @width: The width of the map to generate.
 * @height: The height of the map to generate.
 * @error: Location to store error information, if any, or %NULL.
 *
 * Renders the map described by @source with the C4MapCreatorS2 into a pixbuf.
 * The pixel color depends on the texture at the corresponding position and is
 * determined by the average color of that texture.
 *
 * If the source contains one or more algo=script overlays and @script_path is
 * %NULL, an error is generated. Otherwise, the file at @script_path is opened
 * and used to look up the relevant script functions.
 *
 * In case an error occurs, for example when the map generator source code is
 * not valid, @error is set and the function returns %NULL.
 *
 * Return Value: A #GdkPixbuf with the generated map, or %NULL. Free with
 * g_object_unref().
 **/
GdkPixbuf*
mape_mapgen_render(const gchar* filename,
                   const gchar* source,
                   MapeMapgenType type,
                   const gchar* script_path,
                   MapeMaterialMap* material_map,
                   MapeTextureMap* texture_map,
                   guint width,
                   guint height,
                   GError** error)
{
  C4MapgenHandle* handle;
  const char* error_message;
  unsigned int out_width;
  unsigned int out_height;
  GdkPixbuf* pixbuf;
  guint8* out_p;
  const unsigned char* in_p;
  guint out_rowstride;
  unsigned int in_rowstride;
  guint datawidth;
  guint8 matclrs[256 * 4];
  unsigned int x, y;
  unsigned int matnum;

  switch(type)
  {
  case MAPE_MAPGEN_LANDSCAPE_TXT:
    handle = c4_mapgen_handle_new(
      filename,
      source,
      script_path,
      _mape_material_map_get_handle(material_map),
      _mape_texture_map_get_handle(texture_map),
      width,
      height
    );

    break;
  case MAPE_MAPGEN_MAP_C:
    handle = c4_mapgen_handle_new_script(
      filename,
      source,
      _mape_material_map_get_handle(material_map),
      _mape_texture_map_get_handle(texture_map),
      width,
      height
    );

    break;
  default:
    handle = NULL;
    g_assert_not_reached();
    break;
  }

  error_message = c4_mapgen_handle_get_error(handle);
  if(error_message)
  {
    g_set_error(
      error,
      mape_mapgen_error_quark(),
      MAPE_MAPGEN_ERROR_COMPILE,
      "%s",
      error_message
    );

    c4_mapgen_handle_free(handle);
    return NULL;
  }

  out_width = c4_mapgen_handle_get_width(handle);
  out_height = c4_mapgen_handle_get_height(handle);

  pixbuf = gdk_pixbuf_new(
    GDK_COLORSPACE_RGB,
    FALSE,
    8,
    out_width,
    out_height
  );

  if(pixbuf == NULL)
  {
    g_set_error(
      error,
      mape_mapgen_error_quark(),
      MAPE_MAPGEN_ERROR_MEMORY,
      "Insufficient memory is available"
    );

    c4_mapgen_handle_free(handle);
    return NULL;
  }

  out_p = gdk_pixbuf_get_pixels(pixbuf);
  in_p = c4_mapgen_handle_get_map(handle);
  out_rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  in_rowstride = c4_mapgen_handle_get_rowstride(handle);
  datawidth = gdk_pixbuf_get_width(pixbuf) * 3;
  memset(matclrs, 0, sizeof(matclrs) );

  for(y = 0; y < out_height; ++y)
  {
    for(x = 0; x < out_width; ++x)
    {
      matnum = *in_p;

      if(matclrs[matnum * 4] == 0)
      {
        mape_mapgen_read_color(
          matclrs,
          material_map,
          texture_map,
          matnum
        );

        /* Color has been loaded */
        matclrs[matnum * 4] = 1;
      }

      out_p[0] = matclrs[matnum * 4 + 1];
      out_p[1] = matclrs[matnum * 4 + 2];
      out_p[2] = matclrs[matnum * 4 + 3];
      ++in_p;
      out_p += 3;
    }

    in_p += in_rowstride - out_width;
    out_p += out_rowstride - datawidth;
  }

  c4_mapgen_handle_free(handle);
  return pixbuf;
}

/* vim:set et sw=2 ts=2: */
