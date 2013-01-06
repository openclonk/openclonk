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
                                   MapeMaterialMap* material_map,
                                   unsigned int matnum)
{
  const MapeMaterial* mat;

  if(matnum == 0)
  {
    dest[matnum * 4 + 1] = 100;
    dest[matnum * 4 + 2] = 100;
    dest[matnum * 4 + 3] = 255;
  }
  else
  {
    /* TODO: matnum is actually texmap entry, so find matnum from
     * it. Actually we don't need to know the material
     * actually, just get color from texture + render... */
    mat = mape_material_map_get_material(material_map, matnum - 1);

    dest[matnum * 4 + 1] = 0xff;
    dest[matnum * 4 + 2] = 0xff;
    dest[matnum * 4 + 3] = 0xff;
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
 * In case an error occurs, for example when the map generator source code is
 * not valid, @error is set and the function returns %NULL.
 *
 * Return Value: A #GdkPixbuf with the generated map, or %NULL. Free with
 * g_object_unref().
 **/
GdkPixbuf*
mape_mapgen_render(const gchar* filename,
                   const gchar* source,
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

  handle = c4_mapgen_handle_new(
    filename,
    source,
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

  for(x = 0; x < out_width; ++x)
  {
    for(y = 0; y < out_height; ++y)
    {
      unsigned int matnum = *in_p & 0x7f;
      if(matclrs[matnum * 4] == 0)
      {
        mape_mapgen_read_color(
          matclrs,
          material_map,
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
