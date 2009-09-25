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

static const char* SUFFIXES[] = {
	".png", ".jpg", ".jpeg", ".bmp", NULL
};

/* Load all textures from a group. If there is a texture in the group that
 * has the same name as an already loaded one, then replace the loaded
 * one (overloading). */
static gboolean mape_texture_map_load_group(MapeTextureMap* map,
                                            MapeGroup* group,
                                            GError** error)
{
	gchar* name;
	const char* const* ext;
	gchar* extfile;
	gsize len;
	gchar* casefold_name;

	guchar* data;
	gsize datalen;
	GdkPixbufLoader* loader;
	GdkPixbuf* pixbuf;

	mape_group_rewind(group);
	while( (name = mape_group_get_next_entry(group)) != NULL)
	{
		for(ext = SUFFIXES; *ext != NULL; ++ext)
			if(g_str_has_suffix(name, *ext))
				break;

		if(*ext != NULL)
		{
			data = mape_group_load_entry(group, &datalen, error);
			if(data == NULL)
			{
				g_free(name);
				return FALSE;
			}

			loader = gdk_pixbuf_loader_new();
			gdk_pixbuf_loader_set_size(loader, 24, 24);
			if(!gdk_pixbuf_loader_write(loader, data, datalen, error))
			{
				g_free(name);
				gdk_pixbuf_loader_close(loader, NULL);
				g_object_unref(loader);
				return FALSE;
			}

			if(!gdk_pixbuf_loader_close(loader, error))
			{
				g_free(name);
				g_object_unref(loader);
				return FALSE;
			}

			pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
			g_object_ref(pixbuf);
			g_object_unref(loader);

			/* Make texname out of filename */
			len = strlen(name)-strlen(*ext);
			name[len] = '\0';
			/* Add texture to texmap (without actual Surface),
			 * just so that the map generator knows the presence
			 * of the texture. */
			CPPTEXMAP(map)->AddTexture(name, NULL);
			/* Use this for hashtable lookup */
			casefold_name = g_utf8_casefold(name, len);

			g_hash_table_insert(map->textures, casefold_name, pixbuf);
		}

		g_free(name);
	}

	return TRUE;
}

MapeTextureMap* mape_texture_map_new(MapeGroup* base,
                                     MapeGroup* overload_from,
                                     GError** error)
{
	MapeTextureMap* map;
	map = (MapeTextureMap*)g_slice_new(MapeTextureMap);
	map->handle = NULL;
	map->textures = g_hash_table_new_full(
		g_str_hash,
		g_str_equal,
		g_free,
		g_object_unref
	);

	try
	{
		// TODO: Allow indefinite overloading... does Clonk do this?
		// TODO: Do we need to LoadMap?
		map->handle = new C4TextureMap;
		// Load TexMap.txt
		CPPTEXMAP(map)->LoadMap(overload_from ? *CPPGROUP(overload_from) : *CPPGROUP(base), "TexMap.txt", NULL, NULL);
		// Load textures
		if(!mape_texture_map_load_group(map, base, error))
		{
			mape_texture_map_destroy(map);
			return NULL;
		}

		if(overload_from != NULL &&
		   !mape_texture_map_load_group(map, overload_from, error))
		{
			mape_texture_map_destroy(map);
			return NULL;
		}
	}
	catch(const std::exception& e)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_TEXTURE_ERROR"),
			MAPE_TEXTURE_ERROR_FAILED,
			"%s",
			e.what()
		);

		mape_texture_map_destroy(map);
		return NULL;
	}

	return map;
}

void mape_texture_map_destroy(MapeTextureMap* map)
{
	g_hash_table_unref(map->textures);
	delete CPPTEXMAP(map);
	g_slice_free(MapeTextureMap, map);
}

guint mape_texture_map_get_texture_count(MapeTextureMap* map)
{
	return g_hash_table_size(map->textures);
}

const char* mape_texture_map_get_texture(MapeTextureMap* map,
                                         unsigned int index)
{
	// Bah, this is O(n). Better remove this function, and introduce
	// mape_texture_map_foreach_texture().
	return CPPTEXMAP(map)->GetTexture(index);
}

GdkPixbuf* mape_texture_map_lookup_texture(MapeTextureMap* map,
                                           const gchar* name)
{
	return GDK_PIXBUF(g_hash_table_lookup(map->textures, name));
}

} // extern "C"
