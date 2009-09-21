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

#ifndef INC_MAPE_CONFIGFILE_H
#define INC_MAPE_CONFIGFILE_H

#include <glib.h>
#include "forward.h"

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
