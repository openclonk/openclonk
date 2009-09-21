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

#ifndef INC_MAPE_GROUP_H
#define INC_MAPE_GROUP_H

#include <glib/gerror.h>
#include "forward.h"

/* Simple C-based interface to C4Group */

#ifdef MAPE_COMPILING_CPP
extern "C" {
#endif

typedef enum MapeGroupError_ {
	MAPE_GROUP_ERROR_OPEN,
	MAPE_GROUP_ERROR_FAILED
} MapeGroupError;
	
struct MapeGroup_ {
	void* group_handle;
#ifdef G_OS_WIN32
	unsigned int drive_idtf;
#endif
};

MapeGroup* mape_group_new(const char* path,
                          GError** error);
MapeGroup* mape_group_new_from_parent(MapeGroup* parent,
                                      const char* entry,
                                      GError** error);
void mape_group_destroy(MapeGroup* group);

const char* mape_group_get_name(MapeGroup* group);
const char* mape_group_get_full_name(MapeGroup* group);

gboolean mape_group_has_entry(MapeGroup* group,
                              const char* entry);

void mape_group_rewind(MapeGroup* group);
char* mape_group_get_next_entry(MapeGroup* group);

gboolean mape_group_is_folder(MapeGroup* group);
gboolean mape_group_is_child_folder(MapeGroup* group,
                                    const char* child);

#ifdef MAPE_COMPILING_CPP
} /* extern "C" */
#endif

#endif /* INC_MAPE_GORUP_H */
