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

#include "C4Include.h"
#include "c4group/C4Group.h"
#include "mape/cpp-handles/group-handle.h"

#define GROUP_TO_HANDLE(group) (reinterpret_cast<C4GroupHandle*>(group))
#define HANDLE_TO_GROUP(handle) (reinterpret_cast<C4Group*>(handle))

extern "C" {

C4GroupHandle* c4_group_handle_new(void)
{
  return GROUP_TO_HANDLE(new C4Group);
}

void c4_group_handle_free(C4GroupHandle* handle)
{
  delete HANDLE_TO_GROUP(handle);
}

const gchar* c4_group_handle_get_error(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->GetError();
}

gboolean c4_group_handle_open(C4GroupHandle* handle, const gchar* path, gboolean create)
{
  return HANDLE_TO_GROUP(handle)->Open(path, create);
}

gboolean c4_group_handle_open_as_child(C4GroupHandle* handle, C4GroupHandle* mother, const gchar* name, gboolean exclusive, gboolean create)
{
  return HANDLE_TO_GROUP(handle)->OpenAsChild(HANDLE_TO_GROUP(mother),
                                              name, exclusive, create);
}

const gchar* c4_group_handle_get_name(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->GetName();
}

gchar* c4_group_handle_get_full_name(C4GroupHandle* handle)
{
  StdStrBuf buf(HANDLE_TO_GROUP(handle)->GetFullName());
  gchar* res = static_cast<gchar*>(g_malloc(buf.getSize()*sizeof(gchar)));
  memcpy(res, buf.getData(), buf.getSize());
  return res;
}

void c4_group_handle_reset_search(C4GroupHandle* handle)
{
  HANDLE_TO_GROUP(handle)->ResetSearch();
}

gboolean c4_group_handle_find_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename)
{
  return HANDLE_TO_GROUP(handle)->FindNextEntry(wildcard, filename, size, start_at_filename);
}

gboolean c4_group_handle_access_next_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean start_at_filename)
{
  return HANDLE_TO_GROUP(handle)->AccessNextEntry(wildcard, size, filename, start_at_filename);
}

gboolean c4_group_handle_access_entry(C4GroupHandle* handle, const gchar* wildcard, gsize* size, gchar* filename, gboolean needs_to_be_a_group)
{
  return HANDLE_TO_GROUP(handle)->AccessEntry(wildcard, size, filename, needs_to_be_a_group);
}

gsize c4_group_handle_accessed_entry_size(C4GroupHandle* handle)
{
  return HANDLE_TO_GROUP(handle)->AccessedEntrySize();
}

gboolean c4_group_handle_read(C4GroupHandle* handle, gpointer buffer, gsize size)
{
  return HANDLE_TO_GROUP(handle)->Read(buffer, size);
}

gboolean c4_group_handle_is_folder(C4GroupHandle* handle)
{
	C4Group *group = HANDLE_TO_GROUP(handle);
	return group->IsOpen() && !group->IsPacked();
}

} /* extern "C" */
