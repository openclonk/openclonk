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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "mape/material.h"
#include "mape/texture.h"

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

typedef enum MapeMapgenError_ {
	MAPE_MAPGEN_ERROR_MEMORY,
	MAPE_MAPGEN_ERROR_COMPILE,
	MAPE_MAPGEN_ERROR_MISSING_MAP,

	MAPE_MAPGEN_ERROR_FAILED
} MapeMapgenError;

GdkPixbuf* mape_mapgen_generate(const gchar* source,
                                MapeMaterialMap* material_map,
                                MapeTextureMap* texture_map,
                                GError** error,
								unsigned int map_with,
								unsigned int map_height);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_MAPGEN_H */
