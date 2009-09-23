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

#ifndef INC_MAPE_TEXTURE_H
#define INC_MAPE_TEXTURE_H

#include <glib/gerror.h>
#include "forward.h"

/* Simple C-based interface to C4TextureMap */

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

typedef enum MapeTextureError_ {
	MAPE_TEXTURE_ERROR_FAILED
} MapeTextureError;

struct MapeTextureMap_ {
	void* handle;
	unsigned int count;
};

MapeTextureMap* mape_texture_map_new(MapeGroup* base,
                                     MapeGroup* overload_from,
                                     GError** error);
void mape_texture_map_destroy(MapeTextureMap* map);

unsigned int mape_texture_map_get_texture_count(MapeTextureMap* map);
const char* mape_texture_map_get_texture(MapeTextureMap* map,
                                         unsigned int index);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_TEXTURE_H */
