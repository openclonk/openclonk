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
#include "landscape/C4MapScript.h"
#include "landscape/C4MapCreatorS2.h"
#include "script/C4ScriptHost.h"
#include "object/C4DefList.h"
#include "object/C4Def.h"
#include "script/C4Aul.h"
#include "lib/StdMeshLoader.h"

#include "mape/cpp-handles/material-handle.h"
#include "mape/cpp-handles/texture-handle.h"
#include "mape/cpp-handles/log-handle.h"
#include "mape/cpp-handles/mapgen-handle.h"

#define HANDLE_TO_MATERIAL_MAP(handle) (reinterpret_cast<C4MaterialMap*>(handle))
#define HANDLE_TO_TEXTURE_MAP(handle) (reinterpret_cast<C4TextureMap*>(handle))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

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

class FakeSkeletonLoader: public StdMeshSkeletonLoader
{
public:
	virtual StdMeshSkeleton* GetSkeletonByDefinition(const char* definition) const { return nullptr; }
};

}

extern "C" {

struct _C4MapgenHandle {
	unsigned int width;
	unsigned int height;
	unsigned int rowstride;
	StdCopyStrBuf error_message;
	BYTE* data;
};

void c4_mapgen_handle_init_script_engine()
{
	InitCoreFunctionMap(&ScriptEngine);
	::MapScript.InitFunctionMap(&ScriptEngine);
}

void c4_mapgen_handle_deinit_script_engine()
{
	MapScript.Clear();
	GameScript.Clear();
	ScriptEngine.Clear();
}

void c4_mapgen_handle_set_map_library(C4GroupHandle* group_handle)
{
	::Definitions.Clear();

	C4Def* libmap = new C4Def;
	libmap->id = C4ID(std::string("Library_Map"));
	libmap->SetName(libmap->id.ToString());
	libmap->Category = C4D_StaticBack;
	FakeSkeletonLoader loader;
	if(!libmap->Load(*HANDLE_TO_GROUP(group_handle), loader, C4D_Load_Script, nullptr, nullptr))
	{
		fprintf(stderr, "Failed to load Library_Map script\n");
		delete libmap;
	}
	else
	{
		::Definitions.Add(libmap, false);
	}
}

C4MapgenHandle* c4_mapgen_handle_new_script(const char* filename, const char* source, C4MaterialMapHandle* material_map, C4TextureMapHandle* texture_map, unsigned int map_width, unsigned int map_height)
{
	// Re-initialize script engine. Otherwise, we get a warning when the user
	// changes the value of a constant, since it is defined already from the
	// previous map rendering.  Note that we do not need to re-load the map library.
	c4_mapgen_handle_deinit_script_engine();
	c4_mapgen_handle_init_script_engine();

	try
	{
		// TODO: Could also re-use an existing CSurface8,
		// saving unnecessary malloc/free between map renderings
		C4SLandscape landscape;
		landscape.Default();

		landscape.MapWdt.Set(map_width, 0, map_width, map_width);
		landscape.MapHgt.Set(map_height, 0, map_height, map_height);
		landscape.MapPlayerExtend = 0;

		c4_log_handle_clear();
		::MapScript.LoadData(filename, source, nullptr);
		// If InitializeMap() returns false, the map creator wants to
		// call a fallback in the scenario script. This crashes if no
		// scenario script is loaded, so simply load an empty script
		// here:
		::GameScript.LoadData("Script.c", "", nullptr);

		const char* parse_error = c4_log_handle_get_first_log_message();
		if(parse_error)
			throw std::runtime_error(parse_error);

		// Link script engine (resolve includes/appends, generate code)
		c4_log_handle_clear();
		ScriptEngine.Link(&::Definitions);
		if(c4_log_handle_get_n_log_messages() > 1)
			throw std::runtime_error(c4_log_handle_get_first_log_message());

		// Generate map, fail if return error occurs
		c4_log_handle_clear();
		std::unique_ptr<CSurface8> out_ptr_fg, out_ptr_bg;
		const bool result = ::MapScript.InitializeMap(
			&landscape,
			HANDLE_TO_TEXTURE_MAP(texture_map),
			HANDLE_TO_MATERIAL_MAP(material_map),
			1,
			&out_ptr_fg, &out_ptr_bg);

		// Don't show any map if there was a script runtime error
		const char* runtime_error = c4_log_handle_get_first_log_message();
		if(runtime_error)
			throw std::runtime_error(runtime_error);

		if(!result)
			throw std::runtime_error("No InitializeMap() function present in the script, or it returns false");

		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = out_ptr_fg->Wdt;
		handle->height = out_ptr_fg->Hgt;
		handle->rowstride = out_ptr_fg->Wdt;
		handle->error_message = nullptr;
		handle->data = out_ptr_fg->Bits;
		out_ptr_fg->ReleaseBuffer();

		return handle;
	}
	catch(const std::exception& ex)
	{
		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = 0;
		handle->height = 0;
		handle->error_message.Copy(ex.what());
		handle->data = nullptr;
		return handle;
	}
}

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

		C4MCMap* map = mapgen.GetMap(nullptr);
		if(!map) throw std::runtime_error("No map definition in source file");

		// Setup the script engine if there is an algo=script overlay in the
		// Landscape.txt file
		if(HasAlgoScript(mapgen.GetMap(nullptr)))
		{
			// Re-initialize script engine. Otherwise, we get a warning when the user
			// changes the value of a constant, since it is defined already from the
			// previous map rendering.  Note that we do not need to re-load the map library.
			c4_mapgen_handle_deinit_script_engine();
			c4_mapgen_handle_init_script_engine();

			if(script_path == nullptr)
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
			if(!File.FindNextEntry(basename, (char*)nullptr))
			{
				g_free(dirname);
				g_free(basename);
				StdStrBuf error_msg = FormatString("Failed to load '%s': No such file", script_path);
				throw std::runtime_error(error_msg.getData());
			}

			c4_log_handle_clear();
			GameScript.Load(File, basename, nullptr, nullptr);
			g_free(dirname);
			g_free(basename);

			const char* parse_error = c4_log_handle_get_first_log_message();
			if(parse_error)
				throw std::runtime_error(parse_error);

			// Link script engine (resolve includes/appends, generate code)
			c4_log_handle_clear();
			ScriptEngine.Link(&::Definitions);
			if(c4_log_handle_get_n_log_messages() > 1)
				throw std::runtime_error(c4_log_handle_get_first_log_message());
		}

		c4_log_handle_clear();
		int32_t out_width, out_height;
		BYTE* array = mapgen.RenderBuf(nullptr, out_width, out_height);

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
		handle->error_message = nullptr;
		handle->data = array;
		return handle;
	}
	catch(const C4MCParserErr& err)
	{
		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = 0;
		handle->height = 0;
		handle->error_message.Copy(err.Msg);
		handle->data = nullptr;
		return handle;
	}
	catch(const std::exception& ex)
	{
		C4MapgenHandle* handle = new C4MapgenHandle;
		handle->width = 0;
		handle->height = 0;
		handle->error_message.Copy(ex.what());
		handle->data = nullptr;
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
	assert(mapgen->data != nullptr);
	return mapgen->width;
}

unsigned int c4_mapgen_handle_get_height(C4MapgenHandle* mapgen)
{
	assert(mapgen->data != nullptr);
	return mapgen->height;
}

unsigned int c4_mapgen_handle_get_rowstride(C4MapgenHandle* mapgen)
{
	assert(mapgen->data != nullptr);
	return mapgen->rowstride;
}

const char* c4_mapgen_handle_get_error(C4MapgenHandle* mapgen)
{
	if(mapgen->data != nullptr)
		return nullptr;
	return mapgen->error_message.getData();
}

} // extern "C"
