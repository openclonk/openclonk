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
