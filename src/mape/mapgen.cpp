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

#include <Standard.h>
#include <C4Scenario.h>
#include <C4Texture.h>
#include <C4Material.h>
#include <C4MapCreatorS2.h>

#include "mapgen.h"

#define CPPTEXMAP(map) ((C4TextureMap*)map->handle)
#define CPPMATMAP(map) ((C4MaterialMap*)map->handle)

extern "C"
{

static void mape_mapgen_read_color(guint8* dest,
                                   MapeMaterialMap* material_map,
                                   unsigned int matnum)
{
	MapeMaterial* mat;
	GdkColor color;
	
	if(matnum == 0)
	{
		dest[matnum * 4 + 1] = 100;
		dest[matnum * 4 + 2] = 100;
		dest[matnum * 4 + 3] = 255;
	}
	else
	{
		mat = mape_material_map_get_material(material_map, matnum - 1);
		color = mape_material_get_color(mat);
	
		dest[matnum * 4 + 1] = color.red * 0xff / 0xffff;
		dest[matnum * 4 + 2] = color.green * 0xff / 0xffff;
		dest[matnum * 4 + 3] = color.blue * 0xff / 0xffff;
	}
}

GdkPixbuf* mape_mapgen_generate(const gchar* source,
                                MapeMaterialMap* material_map,
                                MapeTextureMap* texture_map,
                                GError** error,
                                unsigned int map_width,
                                unsigned int map_height)
{
	int32_t width, height;
	int32_t x, y;
	guint8* pix;
	BYTE* arry;
	BYTE* index;
	int aligned_width;
	unsigned int rowstride;
	unsigned int datawidth;
	GdkPixbuf* pixbuf;

	aligned_width = DWordAligned(map_width);

	arry = NULL;
	try
	{
		C4SLandscape landscape;
		landscape.Default();

		landscape.MapWdt = map_width;
		landscape.MapHgt = map_height;
		landscape.MapPlayerExtend = 0;

		C4MapCreatorS2 mapgen(
			&landscape,
			CPPTEXMAP(texture_map),
			CPPMATMAP(material_map),
			1
		);

		C4MCParser parser(&mapgen);

		// TODO: Pass correct filename
		parser.ParseMemFile(source, "Landscape.txt");

		arry = mapgen.RenderBuf(NULL, width, height);
		if(arry == NULL)
		{
			g_set_error(
				error,
				g_quark_from_static_string("MAPE_MAPGEN_ERROR"),
				MAPE_MAPGEN_ERROR_MISSING_MAP,
				"No map definition in source file"
			);

			return NULL;
		}

		pixbuf = gdk_pixbuf_new(
			GDK_COLORSPACE_RGB,
			FALSE,
			8,
			map_width,
			map_height
		);

		if(pixbuf == NULL)
		{
			g_set_error(
				error,
				g_quark_from_static_string("MAPE_MAPGEN_ERROR"),
				MAPE_MAPGEN_ERROR_MEMORY,
				"Insufficient memory is available"
			);

			delete[] arry;
			return NULL;
		}

		pix = gdk_pixbuf_get_pixels(pixbuf);
		rowstride = gdk_pixbuf_get_rowstride(pixbuf);
		datawidth = gdk_pixbuf_get_width(pixbuf) * 3;
		index = arry;
		
		guint8 matclrs[128 * 4];
		memset(matclrs, 0, sizeof(matclrs) );

		for(x = 0; x < map_height; ++ x)
		{
			for(y = 0; y < map_width; ++ y)
			{
				BYTE matnum = *index & 0x7f;
				if(matclrs[matnum * 4] == 0)
				{
					mape_mapgen_read_color(
						matclrs,
						material_map,
						matnum
					);
	
					/* Color has been loaded */
					matclrs[matnum * 4] = 1;
				}
	
				pix[0] = matclrs[matnum * 4 + 1];
				pix[1] = matclrs[matnum * 4 + 2];
				pix[2] = matclrs[matnum * 4 + 3];
				++ index;
				pix += 3;
			}

			index += aligned_width - map_width;
			pix += rowstride - datawidth;
		}

		delete[] arry;
		return pixbuf;
	}
	catch(const C4MCParserErr& err)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_MAPGEN_ERROR"),
			MAPE_MAPGEN_ERROR_COMPILE,
			"%s",
			err.Msg
		);

		delete[] arry;
		return NULL;
	}
}

} // extern "C"
