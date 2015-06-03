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

#ifndef INC_MAPE_C4_GROUP_HANDLE_H
#define INC_MAPE_C4_GROUP_HANDLE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _C4GroupHandle C4GroupHandle;

C4GroupHandle* c4_group_handle_new(void);
void c4_group_handle_free(C4GroupHandle* handle);

const gchar* c4_group_handle_get_error(C4GroupHandle* handle);

gboolean c4_group_handle_open(C4GroupHandle* handle, const gchar* path, gboolean create);
gboolean c4_group_handle_open_as_child(C4GroupHandle* handle, C4GroupHandle* mother, const gchar* name, gboolean exclusive, gboolean create);

const gchar* c4_group_handle_get_name(C4GroupHandle* handle);
gchar* c4_group_handle_get_full_name(C4GroupHandle* handle);

void c4_group_handle_reset_search(C4GroupHandle* handle);

gboolean c4_group_handle_find_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename);
gboolean c4_group_handle_access_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename);
gboolean c4_group_handle_access_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean needs_to_be_a_group);
gsize c4_group_handle_accessed_entry_size(C4GroupHandle* handle);
gboolean c4_group_handle_read(C4GroupHandle* handle, gpointer buffer, gsize size);
gboolean c4_group_handle_is_folder(C4GroupHandle* handle);

G_END_DECLS

#endif /* INC_MAPE_C4_GROUP_HANDLE_H */
