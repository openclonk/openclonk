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

#ifndef INC_MAPE_CONFIGFILE_H
#define INC_MAPE_CONFIGFILE_H

#include <glib.h>
#include "mape/forward.h"

struct MapeConfigFileEntry_ {
	gchar* key;
	gchar* value;
};

struct MapeConfigFile_ {
	gchar* file_path;

	MapeConfigFileEntry* entries;
	gsize entry_count;
};

MapeConfigFile* mape_config_file_new(const gchar* filename);
void mape_config_file_destroy(MapeConfigFile* file);

gboolean mape_config_file_serialise(MapeConfigFile* file,
                                    GError** error);

gsize mape_config_file_get_entry_count(MapeConfigFile* file);
MapeConfigFileEntry* mape_config_file_get_entry(MapeConfigFile* file,
                                                gsize index);
MapeConfigFileEntry* mape_config_file_get_entry_by_key(MapeConfigFile* file,
                                                       const gchar* key);

void mape_config_file_set_entry(MapeConfigFile* file,
                                const gchar* key,
                                const gchar* value);

const gchar* mape_config_file_entry_get_key(MapeConfigFileEntry* entry);
const gchar* mape_config_file_entry_get_value(MapeConfigFileEntry* entry);

#endif /* INC_MAPE_CONFIGFILE_H */
