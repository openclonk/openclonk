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
                   MapeMaterialMap* material_map,
                   MapeTextureMap* texture_map,
                   guint width,
                   guint height,
                   GError** error);

G_END_DECLS

#endif /* INC_MAPE_MAPGEN_H */

/* vim:set et sw=2 ts=2: */
