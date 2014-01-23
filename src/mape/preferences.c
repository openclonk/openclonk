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
#include <stdio.h>
#include "mape/configfile.h"
#include "mape/preferences.h"

static gint mape_preferences_read_int(MapeConfigFile* file,
                                      const gchar* key,
                                      gint default_value)
{
	MapeConfigFileEntry* entry;
	entry = mape_config_file_get_entry_by_key(file, key );

	if(entry == NULL) return default_value;
	return strtol(mape_config_file_entry_get_value(entry), NULL, 0);
}

static gint mape_preferences_read_ranged_int(MapeConfigFile* file,
                                             const gchar* key,
                                             gint lower_bound,
                                             gint upper_bound,
                                             gint default_value)
{
	gint value;
	value = mape_preferences_read_int(file, key, default_value);

	if(value < lower_bound) value = lower_bound;
	if(value > upper_bound) value = upper_bound;

	return value;
}

static gboolean mape_preferences_read_boolean(MapeConfigFile* file,
                                              const gchar* key,
                                              gboolean default_value)
{
	const gchar* value;
	MapeConfigFileEntry* entry;
	
	entry = mape_config_file_get_entry_by_key(file, key);
	if(entry == NULL) return default_value;
	
	value = mape_config_file_entry_get_value(entry);
	if(g_strcasecmp(value, "0") == 0 || g_strcasecmp(value, "off") == 0 ||
	   g_strcasecmp(value, "false") == 0) return FALSE;

	return TRUE;
}

static void mape_preferences_write_int(MapeConfigFile* file,
                                       const gchar* key,
                                       gint value)
{
	gchar buf[16];
	sprintf(buf, "%d", value);

	mape_config_file_set_entry(file, key, buf);
}

static void mape_preferences_write_boolean(MapeConfigFile* file,
                                           const gchar* key,
                                           gboolean value)
{
	const gchar* text = "true";
	if(value == FALSE) text = "false";

	mape_config_file_set_entry(file, key, text);
}

void mape_preferences_from_config(MapePreferences* preferences,
                                  MapeConfigFile* config)
{
	preferences->tab_width = mape_preferences_read_ranged_int(
		config, "tab_width", 1, 8, 2);
	preferences->tab_to_spaces = mape_preferences_read_boolean(
		config, "tab_to_spaces", TRUE);
	preferences->auto_indentation = mape_preferences_read_boolean(
		config, "auto_indentation", TRUE);
	preferences->text_wrapping = mape_preferences_read_boolean(
		config, "text_wrapping", TRUE);
	preferences->line_numbers = mape_preferences_read_boolean(
		config, "line_numbers", TRUE);
	preferences->highlight_line = mape_preferences_read_boolean(
		config, "highlight_line", TRUE);
	preferences->bracket_matching = mape_preferences_read_boolean(
		config, "bracket_matching", TRUE);
	preferences->fixed_seed = mape_preferences_read_boolean(
		config, "fixed_seed", FALSE);
	preferences->random_seed = mape_preferences_read_ranged_int(
		config, "random_seed", 0, (1u << 31u) - 1, rand() );
	preferences->map_width = mape_preferences_read_ranged_int(
		config, "map_width", 50, 500, 150);
	preferences->map_height = mape_preferences_read_ranged_int(
		config, "map_height", 50, 500, 150);
}

void mape_preferences_to_config(MapePreferences* preferences,
                                MapeConfigFile* config)
{
	mape_preferences_write_int(
		config, "tab_width", preferences->tab_width);
	mape_preferences_write_boolean(
		config, "tab_to_spaces", preferences->tab_to_spaces);
	mape_preferences_write_boolean(
		config, "auto_indentation", preferences->auto_indentation);
	mape_preferences_write_boolean(
		config, "text_wrapping", preferences->text_wrapping);
	mape_preferences_write_boolean(
		config, "line_numbers", preferences->line_numbers);
	mape_preferences_write_boolean(
		config, "highlight_line", preferences->highlight_line);
	mape_preferences_write_boolean(
		config, "bracket_matching", preferences->bracket_matching);
	mape_preferences_write_boolean(
		config, "fixed_seed", preferences->fixed_seed);
	mape_preferences_write_int(
		config, "random_seed", preferences->random_seed);
	mape_preferences_write_int(
		config, "map_width", preferences->map_width);
	mape_preferences_write_int(
		config, "map_height", preferences->map_height);
}
