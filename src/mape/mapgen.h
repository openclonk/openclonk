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

#ifndef INC_MAPE_MAPGEN_H
#define INC_MAPE_MAPGEN_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mape/material.h"
#include "mape/texture.h"

G_BEGIN_DECLS

/**
 * MapeMapgenError:
 * @MAPE_MAPGEN_ERROR_COMPILE: An error occured while compiling the
 * Landscape.txt source code.
 * @MAPE_GROUP_ERROR_MEMORY: Insufficient memory was available to render the
 * map.
 *
 * These errors are from the MAPE_MAPGEN_ERROR error domain. They can occur
 * when rendering a map from a Landscape.txt file.
 */
typedef enum _MapeMapgenError {
  MAPE_MAPGEN_ERROR_COMPILE,
  MAPE_MAPGEN_ERROR_MEMORY
} MapeMapgenError;

GdkPixbuf*
mape_mapgen_render(const gchar* filename,
                   const gchar* source,
                   const gchar* script_path,
                   MapeMaterialMap* material_map,
                   MapeTextureMap* texture_map,
                   guint width,
                   guint height,
                   GError** error);

G_END_DECLS

#endif /* INC_MAPE_MAPGEN_H */

/* vim:set et sw=2 ts=2: */
