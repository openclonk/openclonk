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
C4MaterialMapHandle*
_mape_material_map_get_handle(MapeMaterialMap* map);

C4TextureMapHandle*
_mape_texture_map_get_handle(MapeTextureMap* map);

static GQuark mape_mapgen_error_quark()
{
  return g_quark_from_static_string("MAPE_MAPGEN_ERROR");
}

static void mape_mapgen_read_color(guint8* dest,
                                   MapeTextureMap* texture_map,
                                   unsigned int matnum)
{
  const gchar* texture_name;
  const gchar* first_tex_separator;
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

    if(!texture_name)
    {
      /* Texture not found, make the pixel black */
      dest[matnum * 4 + 1] = 0;
      dest[matnum * 4 + 2] = 0;
      dest[matnum * 4 + 3] = 0;
    }
    else
    {
      /* When the texture is animated, the texture name consists of more than
       * one texture, separated with a '-' character. In this case, we simply
       * use the first one for display. */
      own_texture_name = NULL;
      first_tex_separator = strchr(texture_name, '-');
      if(first_tex_separator != NULL)
      {
        own_texture_name = g_strndup(
          texture_name,
          first_tex_separator - texture_name
        );

        texture_name = own_texture_name;
      }

      color = mape_texture_map_get_average_texture_color(
        texture_map,
        texture_name
      );
      g_free(own_texture_name);

      dest[matnum * 4 + 1] = (color      ) & 0xff;
      dest[matnum * 4 + 2] = (color >>  8) & 0xff;
      dest[matnum * 4 + 3] = (color >> 16) & 0xff;
    }
  }
}

/*
 * Public API.
 */

/**
 * mape_mapgen_render:
 *
 * @filename: The filename of the file that is being parsed. This is only used
 * for display purposes.
 * @source: The map generator source code for the map to generate.
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
  guint8 matclrs[128 * 4];
  unsigned int x, y;
  unsigned int matnum;

  handle = c4_mapgen_handle_new(
    filename,
    source,
    script_path,
    _mape_material_map_get_handle(material_map),
    _mape_texture_map_get_handle(texture_map),
    width,
    height
  );

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
      matnum = *in_p & 0x7f;
      if(matclrs[matnum * 4] == 0)
      {
        mape_mapgen_read_color(
          matclrs,
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
