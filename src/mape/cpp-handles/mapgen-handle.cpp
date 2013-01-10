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

#include "C4Include.h"
//#include <C4Include.h>
//#include <Standard.h>
//#include <C4Scenario.h>
//#include <C4Texture.h>
//#include <C4Material.h>
#include <C4MapCreatorS2.h>

#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"
#include "mape/cpp-handles/mapgen-handle.h"

#define HANDLE_TO_MATERIAL_MAP(handle) (reinterpret_cast<C4MaterialMap*>(handle))
#define HANDLE_TO_TEXTURE_MAP(handle) (reinterpret_cast<C4TextureMap*>(handle))

namespace
{

bool HasAlgoScript(C4MCNode* node)
{
  if(node->Type() == MCN_Overlay && static_cast<C4MCOverlay*>(node)->Algorithm == static_cast<C4MCOverlay*>(node)->GetAlgo("script"))
    return true;

  if(node->Child0) return HasAlgoScript(node->Child0);
  if(node->Next) return HasAlgoScript(node->Next);
  return false;
}

}

extern "C" {

struct _C4MapgenHandle {
	unsigned int width;
	unsigned int height;
	unsigned int rowstride;
	StdCopyStrBuf error_message;
	BYTE* data;
};

C4MapgenHandle* c4_mapgen_handle_new(const char* filename, const char* source, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height)
{
	try
	{
		C4SLandscape landscape;
		landscape.Default();

		landscape.MapWdt.Set(map_width, 0, map_width, map_width);
		landscape.MapHgt.Set(map_height, 0, map_height, map_height);
		landscape.MapPlayerExtend = 0;

		C4MapCreatorS2 mapgen(
			&landscape,
			HANDLE_TO_TEXTURE_MAP(texture_map),
			HANDLE_TO_MATERIAL_MAP(material_map),
			1
		);

		C4MCParser parser(&mapgen);
		parser.ParseMemFile(source, filename);

		C4MCMap* map = mapgen.GetMap(NULL);
		if(!map) throw C4MCParserErr(&parser, "No map definition in source file");

		if(HasAlgoScript(mapgen.GetMap(NULL)))
			throw C4MCParserErr(&parser, "algo=script is not yet supported!");

		int32_t out_width, out_height;
		BYTE* array = mapgen.RenderBuf(NULL, out_width, out_height);

		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = map_width;
		handle->height = map_height;
		handle->rowstride = out_width;
		handle->error_message = NULL;
		handle->data = array;
		return handle;
	}
	catch(const C4MCParserErr& err)
	{
		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = 0;
		handle->height = 0;
		handle->error_message.Copy(err.Msg);
		handle->data = NULL;
		return handle;
	}
}

void c4_mapgen_handle_free(C4MapgenHandle* mapgen)
{
	delete[] mapgen->data;
	delete mapgen;
}

const unsigned char* c4_mapgen_handle_get_map(C4MapgenHandle* mapgen)
{
	return reinterpret_cast<unsigned char*>(mapgen->data);
}

unsigned int c4_mapgen_handle_get_width(C4MapgenHandle* mapgen)
{
	assert(mapgen->data != NULL);
	return mapgen->width;
}

unsigned int c4_mapgen_handle_get_height(C4MapgenHandle* mapgen)
{
	assert(mapgen->data != NULL);
	return mapgen->height;
}

unsigned int c4_mapgen_handle_get_rowstride(C4MapgenHandle* mapgen)
{
	assert(mapgen->data != NULL);
	return mapgen->rowstride;
}

const char* c4_mapgen_handle_get_error(C4MapgenHandle* mapgen)
{
	if(mapgen->data != NULL)
		return NULL;
	return mapgen->error_message.getData();
}

} // extern "C"
