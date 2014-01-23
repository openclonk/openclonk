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

#include "C4Include.h"
#include <C4MapCreatorS2.h>
#include <C4ScriptHost.h>
#include <C4DefList.h>
#include <C4Aul.h>

#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"
#include "mape/cpp-handles/log-handle.h"
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

C4MapgenHandle* c4_mapgen_handle_new(const char* filename, const char* source, const char* script_path, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height)
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
		if(!map) throw std::runtime_error("No map definition in source file");

		// Setup the script engine if there is an algo=script overlay in the
		// Landscape.txt file
		if(HasAlgoScript(mapgen.GetMap(NULL)))
		{
			if(script_path == NULL)
				throw std::runtime_error("For algo=script overlays to work, save the file first at the location of the Script.c file");

			gchar* dirname = g_path_get_dirname(script_path);
			gchar* basename = g_path_get_basename(script_path);

			C4Group File;
			if(!File.Open(dirname))
			{
				StdStrBuf error_msg = FormatString("Failed to open directory '%s': %s", dirname, File.GetError());
				g_free(dirname);
				g_free(basename);
				throw std::runtime_error(error_msg.getData());
			}

			// get scripts
			File.ResetSearch();
			if(!File.FindNextEntry(basename, (char*)NULL))
			{
				g_free(dirname);
				g_free(basename);
				StdStrBuf error_msg = FormatString("Failed to load '%s': No such file", script_path);
				throw std::runtime_error(error_msg.getData());
			}

			// load core functions into script engine
			InitCoreFunctionMap(&ScriptEngine);

			c4_log_handle_clear();
			GameScript.Load(File, basename, NULL, NULL);
			g_free(dirname);
			g_free(basename);

			const char* parse_error = c4_log_handle_get_first_log_message();
			if(parse_error)
				throw std::runtime_error(parse_error);

			// Link script engine (resolve includes/appends, generate code)
			c4_log_handle_clear();
			ScriptEngine.Link(&::Definitions);
			if(ScriptEngine.warnCnt > 0 || ScriptEngine.errCnt > 0)
				throw std::runtime_error(c4_log_handle_get_first_log_message());
			// Set name list for globals
			ScriptEngine.GlobalNamed.SetNameList(&ScriptEngine.GlobalNamedNames);
		}

		c4_log_handle_clear();
		int32_t out_width, out_height;
		BYTE* array = mapgen.RenderBuf(NULL, out_width, out_height);
		GameScript.Clear();
		ScriptEngine.Clear();

		// Don't show any map if there was a script runtime error
		const char* runtime_error = c4_log_handle_get_first_log_message();
		if(runtime_error)
		{
			delete[] array;
			throw std::runtime_error(runtime_error);
		}

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
	catch(const std::exception& ex)
	{
		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = 0;
		handle->height = 0;
		handle->error_message.Copy(ex.what());
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
