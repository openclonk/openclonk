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
#include "group.h"
#include "material.h"

#define CPPMAT(mat) ( (C4Material*)mat)
#define CPPMATMAP(map) ( (C4MaterialMap*)map->handle)
#define CPPGROUP(group) ((C4Group*)group->group_handle)

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
	catch(std::exception& e)
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
