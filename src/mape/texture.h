/* mape - C4 Landscape.txt editor
 * Copyright (C) 2005 Armin Burgmeier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
