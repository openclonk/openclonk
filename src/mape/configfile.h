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
