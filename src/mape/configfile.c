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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mape/configfile.h"

MapeConfigFile* mape_config_file_new(const gchar* filename)
{
	/* If filename does not exist, we return an empty config file. */
	MapeConfigFile* file;
	gchar* contents;
	gchar** lines;
	gchar** cur_line;
	gchar* sep;
	gsize length;

	file = malloc(sizeof(MapeConfigFile) );
	file->file_path = g_strdup(filename);
	file->entries = NULL;
	file->entry_count = 0;

	if(g_file_get_contents(filename, &contents, &length, NULL) == FALSE)
		return file;

	lines = g_strsplit(contents, "\n", 0);
	g_free(contents);

	for(cur_line = lines; *cur_line != NULL; ++ cur_line)
	{
		sep = strchr(*cur_line, '=');
		if(sep == NULL) continue;

		*sep = '\0';
		mape_config_file_set_entry(file, *cur_line, sep + 1);
		*sep = '=';
	}

	g_strfreev(lines);
	return file;
}

void mape_config_file_destroy(MapeConfigFile* file)
{
	gsize i;
	for(i = 0; i < file->entry_count; ++ i)
	{
		g_free(file->entries[i].key);
		g_free(file->entries[i].value);
	}

	g_free(file->entries);
	g_free(file->file_path);
	free(file);
}

gboolean mape_config_file_serialise(MapeConfigFile* file,
                                    GError** error)
{
	gchar* dir;
	gchar* content;
	gchar* temp;
	gsize i;

	int dir_result;
	gboolean cont_result;

	dir = g_dirname(file->file_path);
	dir_result = g_mkdir_with_parents(dir, 0755);

	g_free(dir);
	if(dir_result == -1)
	{
		g_set_error(
			error,
			g_quark_from_static_string("MAPE_CONFIG_FILE_ERROR"),
			errno,
			"%s",
			g_strerror(errno)
		);

		return FALSE;
	}

	content = g_strdup("");
	for(i = 0; i < file->entry_count; ++ i)
	{
		temp = g_strconcat(
			content,
			file->entries[i].key,
			"=",
			file->entries[i].value,
			"\n",
			NULL
		);

		g_free(content);
		content = temp;
	}

	cont_result = g_file_set_contents(file->file_path, content, -1, error);
	g_free(content);

	return(cont_result);
}

gsize mape_config_file_get_entry_count(MapeConfigFile* file)
{
	return file->entry_count;
}

MapeConfigFileEntry* mape_config_file_get_entry(MapeConfigFile* file,
                                                gsize index)
{
	g_assert(index < file->entry_count);
	return &file->entries[index];
}

MapeConfigFileEntry* mape_config_file_get_entry_by_key(MapeConfigFile* file,
                                                       const gchar* key)
{
	gsize i;
	for(i = 0; i < file->entry_count; ++ i)
		if(g_strcasecmp(file->entries[i].key, key) == 0)
			return &file->entries[i];

	return NULL;
}

void mape_config_file_set_entry(MapeConfigFile* file,
                                const gchar* key,
                                const gchar* value)
{
	MapeConfigFileEntry* entry;
	entry = mape_config_file_get_entry_by_key(file, key);
	
	if(entry != NULL)
	{
		g_free(entry->value);
		entry->value = g_strdup(value);
	}
	else
	{
		++ file->entry_count;
		file->entries = realloc(
			file->entries,
			sizeof(MapeConfigFileEntry) * file->entry_count
		);
		
		entry = &file->entries[file->entry_count - 1];
		entry->key = g_strdup(key);
		entry->value = g_strdup(value);
	}
}

const gchar* mape_config_file_entry_get_key(MapeConfigFileEntry* entry)
{
	return entry->key;
}

const gchar* mape_config_file_entry_get_value(MapeConfigFileEntry* entry)
{
	return entry->value;
}
