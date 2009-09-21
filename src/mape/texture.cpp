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

#define MAPE_COMPILING_CPP

#include <exception>
#include <C4Texture.h>
#include "group.h"
#include "texture.h"

#define CPPTEXMAP(map) ( (C4TextureMap*)map->handle)
#define CPPGROUP(group) ((C4Group*)group->group_handle)

extern "C" {

MapeTextureMap* mape_texture_map_new(MapeGroup* base,
                                     MapeGroup* overload_from,
                                     GError** error)
{
	MapeTextureMap* map;
	map = (MapeTextureMap*)malloc(sizeof(MapeTextureMap) );

	try
	{
		// TODO: Allow indefinite overloading... does Clonk do this?
		map->handle = new C4TextureMap;
		if(overload_from != NULL)
		{
			map->count = CPPTEXMAP(map)->LoadTextures(
				*CPPGROUP(overload_from),
				CPPGROUP(base)
			);
		}
		else
		{
			map->count = CPPTEXMAP(map)->LoadTextures(
				*CPPGROUP(base),
				NULL
			);
		}
	}
	catch(std::exception& e)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_TEXTURE_ERROR"),
			MAPE_TEXTURE_ERROR_FAILED,
			"%s",
			e.what()
		);

		free(map);
		return NULL;
	}

	return map;
}

void mape_texture_map_destroy(MapeTextureMap* map)
{
	delete CPPTEXMAP(map);
	free(map);
}

unsigned int mape_texture_map_get_texture_count(MapeTextureMap* map)
{
	return map->count;//return CPPTEXMAP(map)->GetCount();
}

const char* mape_texture_map_get_texture(MapeTextureMap* map,
                                         unsigned int index)
{
	return CPPTEXMAP(map)->GetTexture(index);
}

} // extern "C"
