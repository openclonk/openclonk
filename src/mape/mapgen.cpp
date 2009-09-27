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

#include <Standard.h>
#include <C4Scenario.h>
#include <C4Texture.h>
#include <C4Material.h>
#include <C4MapCreatorS2.h>

#include "mape/mapgen.h"

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
