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

// TODO: Move this to an extra file:

#include <C4Application.h>
#include <C4Console.h>
#include <C4FullScreen.h>
#include <C4Game.h>
#include <C4Network2.h>

C4Application Application;
C4Console     Console;
C4FullScreen  FullScreen;
C4Game        Game;
C4Network2    Network;


#define MAPE_COMPILING_CPP

#include <exception>
#include <C4Material.h>

#include "cpp-handles/group-handle.h"

#include "group.h"
#include "material.h"

#define CPPMAT(mat) ( (C4Material*)mat)
#define CPPMATMAP(map) ( (C4MaterialMap*)map->handle)

extern "C" C4GroupHandle* _mape_group_get_handle(MapeGroup*);
#define CPPGROUP(group) (reinterpret_cast<C4Group*>(_mape_group_get_handle(group)))

extern "C" {

MapeMaterialMap* mape_material_map_new(MapeGroup* base,
                                       MapeGroup* overload_from,
                                       GError** error)
{
	MapeMaterialMap* map;
	map = (MapeMaterialMap*)malloc(sizeof(MapeMaterialMap) );

	try
	{
		// TODO: Make sure overloading works as expected...
		map->handle = new C4MaterialMap;
		CPPMATMAP(map)->Load(*CPPGROUP(base));
		if(overload_from != NULL)
			CPPMATMAP(map)->Load(*CPPGROUP(overload_from));
	}
	catch(const std::exception& e)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_MATERIAL_ERROR"),
			MAPE_MATERIAL_ERROR_FAILED,
			"%s",
			e.what()
		);

		free(map);
		return NULL;
	}

	return map;
}

void mape_material_map_destroy(MapeMaterialMap* map)
{
	delete CPPMATMAP(map);
	free(map);
}

unsigned int mape_material_map_get_material_count(MapeMaterialMap* map)
{
	return CPPMATMAP(map)->Num;
}

MapeMaterial* mape_material_map_get_material(MapeMaterialMap* map,
                                             unsigned int index)
{
	return (MapeMaterial*)&CPPMATMAP(map)->Map[index];
}

MapeMaterial* mape_material_map_get_material_by_string(MapeMaterialMap* map,
                                                       const char* mat_name)
{
	unsigned int i;
	for(i = 0; i < CPPMATMAP(map)->Num; ++ i)
		if(stricmp(CPPMATMAP(map)->Map[i].Name, mat_name) == 0)
			return (MapeMaterial*)&CPPMATMAP(map)->Map[i];

	return NULL;
}

const char* mape_material_get_name(MapeMaterial* material)
{
	return CPPMAT(material)->Name;
}

GdkColor mape_material_get_color(MapeMaterial* material)
{
	// TODO: Color field in C4Material is gone. Read from texture?
	GdkColor color = {
		0,
		/*CPPMAT(material)->Color[0]*/0xff * 0xffff / 0xff,
		/*CPPMAT(material)->Color[1]*/0xff * 0xffff / 0xff,
		/*CPPMAT(material)->Color[2]*/0xff * 0xffff / 0xff
	};

	return color;
}

} // extern "C"
